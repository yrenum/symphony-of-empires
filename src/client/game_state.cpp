#include "game_state.hpp"

#include <GL/glew.h>

#include <cstdint>
#include <cstdio>
#include <string>
#ifdef _MSC_VER
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_opengl.h>
#include <SDL_ttf.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_ttf.h>
#include <sys/wait.h>
#endif
#ifdef _MSC_VER
/* required before GL/gl.h */
#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif
#endif
/* msvsc does not know about glext, mingw does so we just use this ifdef */
#ifndef _MSC_VER
#include <GL/glext.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include "../company.hpp"
#include "../good.hpp"
#include "../industry.hpp"
#include "../io_impl.hpp"
#include "../path.hpp"
#include "../product.hpp"
#include "../serializer.hpp"
#include "../world.hpp"
#include "camera.hpp"
#include "client_network.hpp"
#include "interface/descision.hpp"
#include "interface/industry_view_nation.hpp"
#include "interface/pop_view_nation.hpp"
#include "interface/products_view_world.hpp"
#include "interface/province_view.hpp"
#include "interface/select_nation.hpp"
#include "interface/top_window.hpp"
#include "interface/treaty_window.hpp"
#include "interface/ui_reform.hpp"
#include "interface/build_unit_window.hpp"
#include "map.hpp"
#include "render/material.hpp"
#include "render/model.hpp"
#include "render/texture.hpp"
#include "ui.hpp"

void GameState::play_nation() {
    // TODO add this to action
    current_mode = MAP_MODE_NORMAL;

    const Province* capital = curr_nation->capital;
    if (capital != nullptr) {
        cam.position.x = capital->max_x;
        cam.position.y = capital->max_y;
        cam.position.z = -100.f;
    }

    // Make topwindow
    top_win = new TopWindow(*this);

    // Select the nation
    // TODO put in action
    g_client->packet_mutex.lock();
    Packet packet = Packet();
    Archive ar = Archive();
    ActionType action = ActionType::SELECT_NATION;
    ::serialize(ar, &action);
    ::serialize(ar, &curr_nation);
    packet.data(ar.get_buffer(), ar.size());
    g_client->packet_queue.push_back(packet);
    g_client->packet_mutex.unlock();
    print_info("Selected nation %s", curr_nation->ref_name.c_str());
}

const UnifiedRender::Texture& GameState::get_nation_flag(Nation& nation) {
    return *nation_flags[g_world->get_id(&nation)];
}

void handle_event(Input& input, GameState& gs, std::atomic<bool>& run) {
    SDL_Event event;
    int click_on_ui;
    std::pair<int, int>& mouse_pos = input.mouse_pos;
    std::pair<float, float>& select_pos = input.select_pos;
    Boat* selected_boat = input.selected_boat;
    Unit* selected_unit = input.selected_unit;
    Building* selected_building = input.selected_building;

    UI::Context* ui_ctx = gs.ui_ctx;

    int& width = gs.width;
    int& height = gs.height;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
                SDL_GetMouseState(&mouse_pos.first, &mouse_pos.second);
                ui_ctx->check_drag(mouse_pos.first, mouse_pos.second);
                if (event.button.button == SDL_BUTTON_MIDDLE) {
                    input.middle_mouse_down = true;
                    input.last_camera_drag_pos = gs.cam.get_map_pos(mouse_pos);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                SDL_GetMouseState(&mouse_pos.first, &mouse_pos.second);
                if (event.button.button == SDL_BUTTON_MIDDLE) {
                    input.middle_mouse_down = false;
                    break;
                }

                click_on_ui = ui_ctx->check_click(mouse_pos.first, mouse_pos.second);
                if (click_on_ui == 0) {
                    if (input.select_pos.first < 0 ||
                        input.select_pos.first >= gs.world->width ||
                        input.select_pos.second < 0 ||
                        input.select_pos.second >= gs.world->height) {
                        continue;
                    }

                    const Tile& tile = gs.world->get_tile(input.select_pos.first, input.select_pos.second);

                    switch (gs.current_mode) {
                        case MAP_MODE_COUNTRY_SELECT:
                            // TODO add call to functions from here
                            gs.select_nation->change_nation(tile.owner_id);
                            break;
                        case MAP_MODE_NORMAL:
                            // See untis that have been clicked on
                            if (event.button.button == SDL_BUTTON_LEFT) {
                                selected_boat = nullptr;
                                selected_unit = nullptr;
                                selected_building = nullptr;

                                // Check if we selected a boat
                                for (const auto& boat : gs.world->boats) {
                                    const float size = 2.f;
                                    if ((int)select_pos.first > (int)boat->x - size &&
                                        (int)select_pos.first < (int)boat->x + size &&
                                        (int)select_pos.second > (int)boat->y - size &&
                                        (int)select_pos.second < (int)boat->y + size) {
                                        selected_boat = boat;
                                        break;
                                    }
                                }
                                if (selected_boat != nullptr)
                                    break;

                                // Check if we selected an unit
                                for (const auto& unit : gs.world->units) {
                                    const float size = 2.f;
                                    if ((int)select_pos.first > (int)unit->x - size &&
                                        (int)select_pos.first < (int)unit->x + size &&
                                        (int)select_pos.second > (int)unit->y - size &&
                                        (int)select_pos.second < (int)unit->y + size) {
                                        selected_unit = unit;
                                        break;
                                    }
                                }
                                if (selected_unit != nullptr)
                                    break;

                                // Check if we selected an building
                                for (const auto& building : gs.world->buildings) {
                                    const float size = 2.f;
                                    if ((int)select_pos.first > (int)building->x - size &&
                                        (int)select_pos.first < (int)building->x + size &&
                                        (int)select_pos.second > (int)building->y - size &&
                                        (int)select_pos.second < (int)building->y + size) {
                                        selected_building = building;
                                        break;
                                    }
                                }
                                if (selected_building != nullptr)
                                    break;

                                // Server will reject building build request if it's not in our territory (and it's not being built on water either)
                                if (gs.world->get_tile(select_pos.first, select_pos.second).owner_id != gs.world->get_id(gs.curr_nation) &&
                                    gs.world->get_tile(select_pos.first, select_pos.second).owner_id != (Nation::Id)-1)
                                    break;

                                // Tell the server about an action for building an building
                                g_client->packet_mutex.lock();
                                Packet packet = Packet();
                                Archive ar = Archive();
                                ActionType action = ActionType::BUILDING_ADD;
                                ::serialize(ar, &action);
                                Building building = Building();

                                if (gs.world->get_tile(select_pos.first + 0, select_pos.second + 0).elevation <= gs.world->sea_level) {
                                    // Seaport if on bordering water
                                    building.type = gs.world->building_types[2];
                                } else {
                                    // Barracks if on land
                                    building.type = gs.world->building_types[0];
                                }

                                building.x = select_pos.first;
                                building.y = select_pos.second;
                                building.working_unit_type = nullptr;
                                building.working_boat_type = nullptr;
                                building.req_goods = building.type->req_goods;
                                // TODO FIX
                                building.owner = gs.world->nations[gs.select_nation->curr_selected_nation];
                                ::serialize(ar, &building);  // BuildingObj
                                packet.data(ar.get_buffer(), ar.size());
                                g_client->packet_queue.push_back(packet);
                                g_client->packet_mutex.unlock();
                                break;
                            } else if (event.button.button == SDL_BUTTON_RIGHT) {
                                if (selected_unit != nullptr) {
                                    selected_unit->tx = select_pos.first;
                                    selected_unit->ty = select_pos.second;

                                    g_client->packet_mutex.lock();
                                    Packet packet = Packet();
                                    Archive ar = Archive();
                                    ActionType action = ActionType::UNIT_CHANGE_TARGET;
                                    ::serialize(ar, &action);
                                    ::serialize(ar, &selected_unit);
                                    ::serialize(ar, &selected_unit->tx);
                                    ::serialize(ar, &selected_unit->ty);
                                    packet.data(ar.get_buffer(), ar.size());
                                    g_client->packet_queue.push_back(packet);
                                    g_client->packet_mutex.unlock();
                                    break;
                                }
                                if (selected_boat != nullptr) {
                                    selected_boat->tx = select_pos.first;
                                    selected_boat->ty = select_pos.second;

                                    g_client->packet_mutex.lock();
                                    Packet packet = Packet();
                                    Archive ar = Archive();
                                    ActionType action = ActionType::BOAT_CHANGE_TARGET;
                                    ::serialize(ar, &action);
                                    ::serialize(ar, &selected_boat);
                                    ::serialize(ar, &selected_boat->tx);
                                    ::serialize(ar, &selected_boat->ty);
                                    packet.data(ar.get_buffer(), ar.size());
                                    g_client->packet_queue.push_back(packet);
                                    g_client->packet_mutex.unlock();
                                    break;
                                }
                                if (selected_building != nullptr) {
                                    new BuildUnitWindow(gs, selected_building, gs.top_win->top_win);
                                    break;
                                }
                            }

                            if (tile.province_id != (Province::Id)-1) {
                                gs.province_view = new ProvinceView(gs, gs.top_win->top_win, tile);
                            }
                            break;
                    }
                }
                break;
            case SDL_MOUSEMOTION:
                SDL_GetMouseState(&mouse_pos.first, &mouse_pos.second);
                ui_ctx->check_hover(mouse_pos.first, mouse_pos.second);
                if (input.middle_mouse_down) {  // Drag the map with middlemouse
                    std::pair<float, float> map_pos = gs.cam.get_map_pos(mouse_pos);
                    gs.cam.position.x += input.last_camera_drag_pos.first - map_pos.first;
                    gs.cam.position.y += input.last_camera_drag_pos.second - map_pos.second;
                }
                input.select_pos = gs.cam.get_map_pos(input.mouse_pos);
                input.select_pos.first = (int)input.select_pos.first;
                input.select_pos.second = (int)input.select_pos.second;
                break;
            case SDL_MOUSEWHEEL:
                SDL_GetMouseState(&mouse_pos.first, &mouse_pos.second);
                ui_ctx->check_hover(mouse_pos.first, mouse_pos.second);
                click_on_ui = ui_ctx->check_wheel(mouse_pos.first, mouse_pos.second, event.wheel.y * 6);
                if (!click_on_ui) {
                    gs.cam.velocity.z += event.wheel.y * 2.0f;
                }
                break;
            case SDL_TEXTINPUT:
                ui_ctx->check_text_input((const char*)&event.text.text);
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        gs.cam.velocity.y -= std::min(4.f, std::max(0.5f, 0.02f * -gs.cam.position.z));
                        break;
                    case SDLK_DOWN:
                        gs.cam.velocity.y += std::min(4.f, std::max(0.5f, 0.02f * -gs.cam.position.z));
                        break;
                    case SDLK_LEFT:
                        gs.cam.velocity.x -= std::min(4.f, std::max(0.5f, 0.02f * -gs.cam.position.z));
                        break;
                    case SDLK_RIGHT:
                        gs.cam.velocity.x += std::min(4.f, std::max(0.5f, 0.02f * -gs.cam.position.z));
                        break;
                    case SDLK_t:
                        new TreatyWindow(gs, gs.top_win->top_win);
                        break;
                    case SDLK_p:
                        gs.products_view_world->show();
                        break;
                }
                break;
            case SDL_QUIT:
                run = false;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    std::pair<float, float> old_size = std::make_pair(width, height);
                    SDL_GetWindowSize(SDL_GetWindowFromID(event.window.windowID), &width, &height);
                    gs.cam.set_screen(width, height);

                    // Resize/recenter UI according to screen change
                    for (auto& widget : ui_ctx->widgets) {
                        widget->x *= width / old_size.first;
                        widget->y *= height / old_size.second;
                    }
                }
                break;
            default:
                break;
        }
    }
    ui_ctx->clear_dead();
}

void client_update() {
    // // We are going to update widgets which require real-time feeding
    // // this function **should** be called per tick
    // std::lock_guard<std::mutex> l1(render_lock);

    // if (current_mode == MAP_MODE_COUNTRY_SELECT)
    //     return;

    // std::lock_guard<std::recursive_mutex> l2(g_world->time_mutex);
    // const Nation& player_nation = *g_world->nations[curr_selected_nation];
    // if (!(g_world->time % 48)) {
    //     double gdp = 0.f;
    //     for (const auto& province : player_nation.owned_provinces) {
    //         for (const auto& product : g_world->products) {
    //             gdp += product->price * province->stockpile[g_world->get_id(product)];
    //         }
    //     }
    //     gdp_chart->data.push_back(gdp);
    //     if (gdp_chart->data.size() >= 30)
    //         gdp_chart->data.pop_front();

    //     size_t total_pop = 0, n_pops = 0;
    //     double living_std = 0.f;
    //     for (const auto& province : player_nation.owned_provinces) {
    //         for (const auto& pop : province->pops) {
    //             total_pop += pop.size;
    //             living_std += pop.life_needs_met;
    //             n_pops++;
    //         }
    //     }

    //     pop_chart->data.push_back(total_pop);
    //     if (pop_chart->data.size() >= 30)
    //         pop_chart->data.pop_front();

    //     hdi_chart->data.push_back(living_std / n_pops);
    //     if (hdi_chart->data.size() >= 30)
    //         hdi_chart->data.pop_front();
    // }

    // size_t total_pop = 0;
    // double militancy = 0.f, consciousness = 0.f;
    // for (const auto& province : player_nation.owned_provinces) {
    //     for (const auto& pop : province->pops) {
    //         total_pop += pop.size;
    //         militancy += pop.militancy;
    //         consciousness += pop.consciousness;
    //     }
    // }
    // militancy /= total_pop;
    // consciousness /= total_pop;

    // sprintf(tmpbuf, " %10.3f", militancy);
    // militancy_lab->text(tmpbuf);
    // sprintf(tmpbuf, " %10.3f", consciousness);
    // big_brain_lab->text(tmpbuf);
    // sprintf(tmpbuf, " %10.3f", player_nation.prestige);
    // prestige_lab->text(tmpbuf);
    // sprintf(tmpbuf, " %10.3f", player_nation.economy_score);
    // economy_lab->text(tmpbuf);
    // sprintf(tmpbuf, " %10.3f", player_nation.budget);
    // money_lab->text(tmpbuf);
    // sprintf(tmpbuf, " %14zu", (size_t)total_pop);
    // population_lab->text(tmpbuf);

    // map->update(*g_world);
}

void render(GameState& gs, Input& input, SDL_Window* window) {
    int& width = gs.width;
    int& height = gs.height;
    Map* map = gs.map;
    Camera& cam = gs.cam;

    std::pair<float, float>& select_pos = input.select_pos;
    Boat* selected_boat = input.selected_boat;
    Unit* selected_unit = input.selected_unit;
    Building* selected_building = input.selected_building;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(cam.get_projection()));
    glViewport(0, 0, width, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(cam.get_view()));

    map->draw(cam, width, height);

    gs.world->world_mutex.lock();
    if (selected_boat != nullptr) {
        glBegin(GL_LINE_STRIP);
        glColor3f(1.f, 0.f, 0.f);
        glVertex2f(selected_boat->x, selected_boat->y);
        glVertex2f(selected_boat->x + 1.f, selected_boat->y);
        glVertex2f(selected_boat->x + 1.f, selected_boat->y + 1.f);
        glVertex2f(selected_boat->x, selected_boat->y + 1.f);
        glEnd();
    }

    if (selected_unit != nullptr) {
        glBegin(GL_LINE_STRIP);
        glColor3f(1.f, 0.f, 0.f);
        glVertex2f(selected_unit->x, selected_unit->y);
        glVertex2f(selected_unit->x + 1.f, selected_unit->y);
        glVertex2f(selected_unit->x + 1.f, selected_unit->y + 1.f);
        glVertex2f(selected_unit->x, selected_unit->y + 1.f);
        glEnd();
    }

    if (selected_building != nullptr) {
        glBegin(GL_LINE_STRIP);
        glColor3f(1.f, 0.f, 0.f);
        glVertex2f(selected_building->x, selected_building->y);
        glVertex2f(selected_building->x + 1.f, selected_building->y);
        glVertex2f(selected_building->x + 1.f, selected_building->y + 1.f);
        glVertex2f(selected_building->x, selected_building->y + 1.f);
        glEnd();
    }
    gs.world->world_mutex.unlock();

    glBindTexture(GL_TEXTURE_2D, 0);
    glBegin(GL_QUADS);
    glColor3f(1.f, 1.f, 1.f);
    glVertex2f(select_pos.first, select_pos.second);
    glVertex2f(select_pos.first + 1.f, select_pos.second);
    glVertex2f(select_pos.first + 1.f, select_pos.second + 1.f);
    glVertex2f(select_pos.first, select_pos.second + 1.f);
    glEnd();

    glPopMatrix();

    gs.ui_ctx->render_all(width, height);
    glLoadIdentity();
    glRasterPos2f(-3.0f, -2.0f);
    SDL_GL_SwapWindow(window);

    cam.update();
}

void handle_popups(std::vector<Event>& displayed_events,
                   std::vector<Treaty*>& displayed_treaties,
                   GameState& gs) {
    // Put popups
    // Event + Descision popups
    for (auto& msg : gs.curr_nation->inbox) {
        auto iter = std::find_if(displayed_events.begin(), displayed_events.end(), [&msg](const auto& e) {
            return e.ref_name == msg.ref_name;
        });
        if (iter != displayed_events.end()) {
            continue;
        }

        new DescisionWindow(gs, msg);

        displayed_events.push_back(msg);
    }

    // Treaties popups
    extern std::string treaty_to_text(const Treaty& treaty);
    for (auto treaty : gs.world->treaties) {
        // Only show treaties we haven't decided on yet and that we have participation on
        // TODO FIX
        // if (std::find_if(treaty->approval_status.begin(), treaty->approval_status.end(), [](const auto& e) {
        //         return (gs.curr_nation == e.first) && (e.second == TREATY_APPROVAL_UNDECIDED);
        //     }) == treaty->approval_status.end())
        //     continue;

        // Must not be already shown
        if (std::find_if(displayed_treaties.begin(), displayed_treaties.end(), [&treaty](const auto& e) {
                return treaty == e;
            }) != displayed_treaties.end())
            continue;

        new TreatyPopup(gs, treaty);

        // Buttons for descisions
        displayed_treaties.push_back(treaty);
    }
}

void init_client(GameState& gs) {
    Map* map = gs.map;
    int& width = gs.width;
    int& height = gs.height;
    for (const auto& nation : gs.world->nations) {
        std::string path;
        FILE* fp;

        // try socialist
        path = Path::get("ui/flags/" + nation->ref_name + "_socialist.png");
        fp = fopen(path.c_str(), "rb");
        if (fp == NULL) {
            // try monarchy
            path = Path::get("ui/flags/" + nation->ref_name + "_monarchy.png");
            fp = fopen(path.c_str(), "rb");
            if (fp == NULL) {
                // try democracy
                path = Path::get("ui/flags/" + nation->ref_name + "_democracy.png");
                fp = fopen(path.c_str(), "rb");
                if (fp == NULL) {
                    // fail
                }
            }
        }

        map->nation_flags.push_back(&g_texture_manager->load_texture(path));
    }

    for (const auto& building_type : gs.world->building_types) {
        std::string path = Path::get("3d/building_types/" + building_type->ref_name + ".obj");
        map->outpost_type_icons.push_back(&g_model_manager->load_complex(path));
    }
    for (const auto& boat_type : gs.world->boat_types) {
        std::string path = Path::get("3d/boat_types/" + boat_type->ref_name + ".obj");
        map->boat_type_icons.push_back(&g_model_manager->load_complex(path));
    }
    for (const auto& unit_type : gs.world->unit_types) {
        std::string path = Path::get("3d/unit_types/" + unit_type->ref_name + ".obj");
        map->unit_type_icons.push_back(&g_model_manager->load_complex(path));
    }

    glClearColor(0.1f, 0.1f, 0.1f, 0.1f);

    gs.select_nation = new SelectNation(gs);
}

void GameState::add_command(Command* command) {
    pending_commands.push(command);
}

void main_loop(GameState& gs, Client* client, SDL_Window* window) {
    gs.products_view_world = new ProductsViewWorld(gs);
    gs.pop_view_nation = new PopViewNation(gs);
    gs.ui_reform = new UIReform(gs);

    std::atomic<bool> run;
    run = true;
    Nation* curr_nation = nullptr;

    Map* map = gs.map;

    gs.world->client_update = &client_update;

    gs.input = Input{};
    Input& input = gs.input;
    uint64_t last_time = 0;

    init_client(gs);

    std::mutex render_lock;
    std::vector<Event> displayed_events;
    std::vector<Treaty*> displayed_treaties;
    while (run) {
        if (last_time != gs.world->time) {
            last_time = gs.world->time;
            client_update();
        }

        std::unique_lock<std::mutex> lock(render_lock);

        handle_event(input, gs, run);
        if (gs.current_mode == MAP_MODE_NORMAL) {
            handle_popups(displayed_events, displayed_treaties, gs);
        }

        while (!gs.pending_commands.empty()) {
            Command* cmd = gs.pending_commands.front();
            cmd->run_command(*gs.world, client);
            gs.pending_commands.pop();
        }
        render(gs, input, window);
    }
}

extern UnifiedRender::TextureManager* g_texture_manager;
extern UnifiedRender::MaterialManager* g_material_manager;
extern UnifiedRender::ModelManager* g_model_manager;

char* tmpbuf;
World::World(void) {
    g_world = this;
};
World::~World(){};

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
void start_client(int argc, char** argv) {
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();

    // globals
    SDL_Window* window;
    int width = 1280, height = 800;
    GameState gs{Camera(width, height)};

    window = SDL_CreateWindow("Symphony of Empires", 0, 0, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);  // Enable OpenGL VSYNC

    const GLubyte* version = glGetString(GL_VERSION);
    printf("OpenGL Version: %s\n", version);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    g_texture_manager = new UnifiedRender::TextureManager();
    g_material_manager = new UnifiedRender::MaterialManager();
    g_model_manager = new UnifiedRender::ModelManager();

    gs.ui_ctx = new UI::Context();

    tmpbuf = new char[512];

    GLenum err = glewInit();
    if (err != GLEW_OK)
        throw std::runtime_error("Failed to init GLEW");

    std::string server_addr;
    if (argc > 1) {
        server_addr = argv[1];
    } else {
        server_addr = "127.0.0.1";
        print_info("No IP specified, assuming default %s", server_addr.c_str());
    }
    print_info("Connecting to server with IP %s", server_addr.c_str());

    gs.world = new World();
    Client* client = new Client(server_addr, 1836);
    client->wait_for_snapshot();

    gs.width = width;
    gs.height = height;
    gs.map = new Map(*gs.world);
    main_loop(gs, client, window);

    delete client;

    TTF_Quit();
    SDL_Quit();

    delete[] tmpbuf;
    return;
}