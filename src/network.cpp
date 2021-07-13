#include <mutex>
#ifdef unix
#	include <sys/socket.h>
#	include <sys/ioctl.h>
#	include <netdb.h>
#	include <arpa/inet.h>
#	ifndef INVALID_SOCKET
#		define INVALID_SOCKET -1
#	endif
#endif
#include <sys/types.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include "network.hpp"
#include "print.hpp"

#include <poll.h>

void SocketStream::write(const void* data, size_t size) {
	for(size_t i = 0; i < size; ) {
		int r = ::send(fd, (const char*)data, std::min<size_t>(max_read_size, size - i), 0);
		if(r == -1) {
#ifdef windows
			print_error("WSA Code: %u", WSAGetLastError());
#endif
			throw std::runtime_error("Socket write error for data in packet");
		}
		i += r;
	}
}

void SocketStream::read(const void* data, size_t size) {
	for(size_t i = 0; i < size; ) {
		int r = ::recv(fd, (char*)data, std::min<size_t>(max_write_size, size - i), MSG_WAITALL);
		if(r == -1) {
#ifdef windows
			print_error("WSA Code: %u", WSAGetLastError());
#endif
			throw std::runtime_error("Socket read error for data in packet");
		}
		i += r;
	}
}

#include <signal.h>
Server* g_server = nullptr;
Server::Server(const unsigned port, const unsigned max_conn) {
	g_server = this;
	
#ifdef windows
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
#endif

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(fd == INVALID_SOCKET) {
#ifdef windows
		WSACleanup();
#endif
		throw std::runtime_error("Cannot create server socket");
	}

	if(bind(fd, (sockaddr *)&addr, sizeof(addr)) != 0) {
#if defined windows
		print_error("WSA code: %u", WSAGetLastError());
		WSACleanup();
#endif
		throw std::runtime_error("Cannot bind server");
	}

	if(listen(fd, max_conn) != 0) {
#if defined windows
		print_error("WSA code: %u", WSAGetLastError());
		WSACleanup();
#endif
		throw std::runtime_error("Cannot listen in specified number of concurrent connections");
	}

	print_info("Deploying %u threads for clients", max_conn);
	threads.reserve(max_conn);
	for(size_t i = 0; i < max_conn; i++) {
		threads.push_back(std::thread(&Server::net_loop, this, i));
	}
	packet_queues.resize(max_conn);
	packet_mutexes = new std::mutex[max_conn];

	// We need to ignore pipe signals since any client disconnecting **will** crash the server
	signal(SIGPIPE, SIG_IGN);
	
	run = true;
	print_info("Server created sucessfully and listening to %u", port);
	print_info("Server ready, now people can join!");
}

Server::~Server() {
	run = true;
	for(auto& thread: threads) {
		thread.join();
	}

#ifdef unix
	close(fd);
#elif defined windows
	closesocket(fd);
	WSACleanup();
#endif
}

#include "actions.hpp"
#include "world.hpp"
#include "io_impl.hpp"
extern World* g_world;

#include <chrono>
#include <thread>

/** This will broadcast the given packet to all clients currently on the server
 */
void Server::broadcast(Packet& packet) {
	for(size_t i = 0; i < threads.size(); i++) {
		packet_mutexes[i].lock();
		packet_queues[i].push_back(packet);
		packet_mutexes[i].unlock();
	}
}

#include <sys/ioctl.h>
/** This is the handling thread-function for handling a connection to a single client
 * Sending packets will only be received by the other end, when trying to broadcast please
 * put the packets on the send queue, they will be sent accordingly
 */
void Server::net_loop(int id) {
	while(run) {
		int conn_fd = 0;
		try {
			sockaddr_in client;
			socklen_t len = sizeof(client);

			conn_fd = accept(fd, (sockaddr *)&client, &len);
			if(conn_fd == INVALID_SOCKET) {
				throw std::runtime_error("Cannot accept client connection");
			}

			print_info("New client connection established");

			Nation* selected_nation = nullptr;
			Packet packet = Packet(conn_fd);
			
			// Send the whole snapshot of the world
			Archive ar = Archive();
			::serialize(ar, g_world);
			packet.send(ar.get_buffer(), ar.size());
			
			enum ActionType action = ACTION_PING;
			packet.send(&action);
			print_info("Sent action %zu", (size_t)action);

			int has_pending;
			while(run) {
				// Read the messages if there is any pending bytes on the tx
				has_pending = 0;
				ioctl(conn_fd, FIONREAD, &has_pending);
				if(has_pending) {
					packet.recv();
					
					ar.set_buffer(packet.data(), packet.size());
					ar.rewind();
					::deserialize(ar, &action);

					if(selected_nation == nullptr &&
					(action != ACTION_PONG && action != ACTION_CHAT_MESSAGE && action != ACTION_SELECT_NATION))
						throw std::runtime_error("Unallowed operation without selected nation");
					
					switch(action) {
					case ACTION_PONG:
						action = ACTION_PING;
						packet.send(&action);
						print_info("Received pong, responding with ping!");
						break;
					case ACTION_NATION_ENACT_POLICY:
						{
							std::lock_guard<std::recursive_mutex> lock1(g_world->nations_mutex);

							Policies policies;
							::deserialize(ar, &policies);

							// TODO: Do parliament checks and stuff

							selected_nation->current_policy = policies;
						}
						break;
					case ACTION_UNIT_CHANGE_TARGET:
						{
							std::lock_guard<std::recursive_mutex> lock1(g_world->units_mutex);

							Unit* unit;
							::deserialize(ar, &unit);
							if(unit == nullptr)
								throw std::runtime_error("Unknown unit");

							// Must control unit
							if(selected_nation != unit->owner)
								throw std::runtime_error("Nation does not control unit");
							
							::deserialize(ar, &unit->tx);
							::deserialize(ar, &unit->ty);

							if(unit->tx >= g_world->width || unit->ty >= g_world->height)
								throw std::runtime_error("Coordinates out of range for unit");
							
							print_info("Unit changes targets to %zu.%zu", (size_t)unit->tx, (size_t)unit->ty);
						}
						break;
					case ACTION_OUTPOST_START_BUILDING_UNIT:
						{
							std::lock_guard<std::recursive_mutex> lock1(g_world->outposts_mutex);
							std::lock_guard<std::recursive_mutex> lock2(g_world->unit_types_mutex);

							Outpost* outpost = new Outpost();
							::deserialize(ar, &outpost);
							if(outpost == nullptr)
								throw std::runtime_error("Unknown outpost");
							UnitType* unit_type = new UnitType();
							::deserialize(ar, &unit_type);
							if(unit_type == nullptr)
								throw std::runtime_error("Unknown unit type");
							
							// Must control outpost
							if(outpost->owner != selected_nation)
								throw std::runtime_error("Nation does not control outpost");
							
							// TODO: Check nation can build this unit

							// Tell the outpost to build this specific unit type
							outpost->working_unit_type = unit_type;
							outpost->req_goods_for_unit = unit_type->req_goods;
							print_info("New order for building on outpost; build unit %s", unit_type->name.c_str());
						}
						break;
					case ACTION_OUTPOST_START_BUILDING_BOAT:
						{
							std::lock_guard<std::recursive_mutex> lock1(g_world->outposts_mutex);
							std::lock_guard<std::recursive_mutex> lock2(g_world->boat_types_mutex);

							Outpost* outpost = new Outpost();
							::deserialize(ar, &outpost);
							if(outpost == nullptr)
								throw std::runtime_error("Unknown outpost");
							BoatType* boat_type = new BoatType();
							::deserialize(ar, &boat_type);
							if(boat_type == nullptr)
								throw std::runtime_error("Unknown boat type");
							
							// Must control outpost
							if(outpost->owner != selected_nation)
								throw std::runtime_error("Nation does not control outpost");

							// Tell the outpost to build this specific unit type
							outpost->working_boat_type = boat_type;
							outpost->req_goods_for_unit = boat_type->req_goods;
							print_info("New order for building on outpost; build boat %s", boat_type->name.c_str());
						}
						break;
					case ACTION_OUTPOST_ADD:
						{
							std::lock_guard<std::recursive_mutex> lock(g_world->outposts_mutex);

							Outpost* outpost = new Outpost();
							::deserialize(ar, outpost);
							if(outpost->type == nullptr)
								throw std::runtime_error("Unknown outpost type");

							// Modify the serialized outpost
							ar.ptr -= ::serialized_size(outpost);
							outpost->owner = selected_nation;

							// Check that it's not out of bounds
							if(outpost->x >= g_world->width || outpost->y >= g_world->height)
								throw std::runtime_error("Outpost out of range");
							
							// Outposts can only be built on owned land
							if(g_world->nations[g_world->get_tile(outpost->x, outpost->y).owner_id] != selected_nation)
								throw std::runtime_error("Outpost cannot be built on foreign land");

							outpost->working_unit_type = nullptr;
							outpost->working_boat_type = nullptr;
							outpost->req_goods_for_unit = std::vector<std::pair<Good*, size_t>>();
							outpost->req_goods = std::vector<std::pair<Good*, size_t>>();
							::serialize(ar, outpost);

							g_world->outposts.push_back(outpost);
							print_info("New outpost of %s", outpost->owner->name.c_str());
						}

						// Rebroadcast
						broadcast(packet);
						break;
					/* Action of colonizing a province, this action is then re-broadcasted to all clients
					 * it's separate from ACTION_PROVINCE_UPDATE by mere convenience for clients(?)
					 */
					case ACTION_PROVINCE_COLONIZE:
						{
							std::lock_guard<std::recursive_mutex> lock1(g_world->nations_mutex);
							std::lock_guard<std::recursive_mutex> lock2(g_world->provinces_mutex);

							Province* province;
							::deserialize(ar, &province);

							if(province == nullptr)
								throw std::runtime_error("Unknown province");

							// Must not be already owned
							if(province->owner != nullptr)
								throw std::runtime_error("Province already has an owner");

							province->owner = selected_nation;
						}
						
						// Rebroadcast
						broadcast(packet);
						break;
					// Gaming chat
					case ACTION_CHAT_MESSAGE:
						{
							std::string msg;
							::deserialize(ar, &msg);
							print_info("Message: %s\n", msg.c_str());
						}
						
						// Rebroadcast
						broadcast(packet);
						break;
					// Events & Descisions
					case ACTION_NATION_TAKE_DESCISION:
						{
							std::lock_guard<std::recursive_mutex> lock1(g_world->events_mutex);

							// Find event by reference name
							std::string event_ref_name;
							::deserialize(ar, &event_ref_name);
							auto event = std::find_if(g_world->events.begin(), g_world->events.end(),
							[&event_ref_name](const Event* e) {
								return e->ref_name == event_ref_name;
							});
							if(event == g_world->events.end()) {
								throw std::runtime_error("Event not found");
							}
							
							// Find descision by reference name
							std::string descision_ref_name;
							::deserialize(ar, &descision_ref_name);
							auto descision = std::find_if((*event)->descisions.begin(), (*event)->descisions.end(),
							[&descision_ref_name](const Descision& e) {
								return e.ref_name == descision_ref_name;
							});
							if(descision == (*event)->descisions.end()) {
								throw std::runtime_error("Descision not found");
							}

							(*event)->take_descision(selected_nation, &(*descision));
							print_info("Event %s + descision %s taken by %s",
								event_ref_name.c_str(),
								descision_ref_name.c_str(),
								selected_nation->ref_name.c_str()
							);
						}
						break;
					// The client selects a nation
					case ACTION_SELECT_NATION:
						{
							std::lock_guard<std::recursive_mutex> lock(g_world->nations_mutex);

							Nation* nation;
							::deserialize(ar, &nation);
							if(nation == nullptr)
								throw std::runtime_error("Unknown nation");
							selected_nation = nation;
						}
						print_info("Nation %s selected by client %zu", selected_nation->name.c_str(), (size_t)id);
						break;
					// Nation and province addition and removals are not allowed to be done by clients
					default:
						break;
					}
				}
				
				// After reading everything we will send our queue appropriately
				packet_mutexes[id].lock();
				while(!packet_queues[id].empty()) {
					Packet elem = packet_queues[id].front();
					elem.stream = SocketStream(conn_fd);
					packet_queues[id].pop_front();
					elem.send();
				}
				packet_mutexes[id].unlock();
			}
		} catch(std::runtime_error& e) {
			print_error("Except: %s", e.what());
			print_info("Client disconnected");
		}
		
#ifdef windows
		shutdown(conn_fd, SD_BOTH);
#elif defined unix
		shutdown(conn_fd, SHUT_RDWR);
#endif

		// Unlock mutexes so we don't end up with weird situations... like deadlocks
		packet_mutexes[id].unlock();
		print_info("Client connection closed");
	}
}

Client* g_client = nullptr;
Client::Client(std::string host, const unsigned port) {
	g_client = this;

#ifdef windows
	WSADATA data;
	if(WSAStartup(MAKEWORD(2, 2), &data) != 0) {
		print_error("WSA code: %u", WSAGetLastError());
		throw std::runtime_error("Cannot start WSA");
	}
#endif
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(host.c_str());
	addr.sin_port = htons(port);
	
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(fd == INVALID_SOCKET) {
#ifdef windows
		print_error("WSA Code: %u", WSAGetLastError());
		WSACleanup();
#endif
		throw std::runtime_error("Cannot create client socket");
	}
		
	if(connect(fd, (sockaddr*)&addr, sizeof(addr)) != 0) {
#ifdef unix
		close(fd);
#elif defined windows
		print_error("WSA Code: %u", WSAGetLastError());
		closesocket(fd);
#endif
		throw std::runtime_error("Cannot connect to server");
	}
	
	// Launch the receive and send thread
	net_thread = std::thread(&Client::net_loop, this);
	has_snapshot = false;
}

/** The server assumes all clients are able to handle all events regardless of anything
 * if the client runs out of memory it needs to disconnect and then reconnect in order
 * to establish a new connection; since the server won't hand out snapshots - wait...
 * if you need snapshots for any reason (like desyncs) you can request with ACTION_SNAPSHOT
 */
void Client::net_loop(void) {
	Packet packet = Packet(fd);
	
	// Receive the first snapshot of the world 
	packet.recv();
	
	Archive ar = Archive();
	ar.set_buffer(packet.data(), packet.size());
	::deserialize(ar, g_world);
	
	has_snapshot = true;
	
	try {
		enum ActionType action;
		int has_pending;
		while(1) {
			// Read the messages if there is any pending bytes on the tx
			has_pending = 0;
			ioctl(fd, FIONREAD, &has_pending);
			if(has_pending) {
				// Obtain the action from the server
				packet.recv();
				ar.set_buffer(packet.data(), packet.size());
				ar.rewind();
				
				::deserialize(ar, &action);
				
				// Ping from server, we should answer with a pong!
				switch(action) {
				case ACTION_PONG:
					packet.send(&action);
					print_info("Received ping, responding with pong!");
					break;
				// Update/Remove/Add Actions
				//
				// These actions all follow the same format they give a specialized ID for the index
				// where the operated object is or should be; this allows for extreme-level fuckery
				// like ref-name changes in the middle of a game in the case of updates.
				//
				// After the ID the object in question is given in a serialized form, in which the
				// deserializer will deserialize onto the final object; after this the operation
				// desired is done.
				case ACTION_PROVINCE_UPDATE:
					{
						std::lock_guard<std::recursive_mutex> lock(g_world->provinces_mutex);

						Province* province;
						::deserialize(ar, &province);
						if(province == nullptr)
							throw std::runtime_error("Unknown province");
						
						::deserialize(ar, province);
					}
					break;
				case ACTION_NATION_UPDATE:
					{
						std::lock_guard<std::recursive_mutex> lock(g_world->nations_mutex);

						Nation* nation;
						::deserialize(ar, &nation);
						if(nation == nullptr)
							throw std::runtime_error("Unknown nation");
						
						::deserialize(ar, nation);
					}
					break;
				case ACTION_NATION_ENACT_POLICY:
					{
						std::lock_guard<std::recursive_mutex> lock(g_world->nations_mutex);

						Nation* nation;
						::deserialize(ar, &nation);
						if(nation == nullptr)
							throw std::runtime_error("Unknown nation");
						
						Policies policy;
						::deserialize(ar, &policy);
						nation->current_policy = policy;
					}
					break;
				case ACTION_UNIT_UPDATE:
					{
						std::lock_guard<std::recursive_mutex> lock(g_world->units_mutex);

						Unit* unit;
						::deserialize(ar, &unit);
						
						if(unit == nullptr)
							throw std::runtime_error("Unknown unit");
						
						::deserialize(ar, unit);
					}
					break;
				case ACTION_UNIT_ADD:
					{
						std::lock_guard<std::recursive_mutex> lock(g_world->units_mutex);

						Unit* unit = new Unit();
						::deserialize(ar, unit);
						g_world->units.push_back(unit);
						print_info("New unit of %s", unit->owner->name.c_str());
					}
					break;
				case ACTION_OUTPOST_ADD:
					{
						std::lock_guard<std::recursive_mutex> lock(g_world->outposts_mutex);

						Outpost* outpost = new Outpost();
						::deserialize(ar, outpost);
						g_world->outposts.push_back(outpost);
						print_info("New outpost of %s", outpost->owner->name.c_str());
					}
					break;
				case ACTION_WORLD_TICK:
					::deserialize(ar, &g_world->time);
					break;
				default:
					break;
				}
			}
			
			// Client will also flush it's queue to the server
			packet_mutex.lock();
			while(!packet_queue.empty()) {
				print_info("Network: sending packet");
				Packet elem = packet_queue.front();
				packet_queue.pop_front();

				elem.stream = SocketStream(fd);
				elem.send();
			}
			packet_mutex.unlock();
		}
	} catch(std::runtime_error& e) {
		print_error("Except: %s", e.what());
	}
}

/** Waits to receive the server initial world snapshot
 */
void Client::wait_for_snapshot(void) {
	while(!has_snapshot) {
		// Just wait...
	}
}

Client::~Client() {
	net_thread.join();
	close(fd);
}
