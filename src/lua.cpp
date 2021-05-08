#include <lua5.4/lua.h>
#ifdef HAS_WINDOWS
#include <lua.hpp>
#else
#include <lua5.4/lua.hpp>
#endif //HAS_WINDOWS
#include <string.h>
#include <stdlib.h>

#ifdef WINDOWS
#include <stdlib.h>
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)
#else
#include <byteswap.h>
#endif
#include "lua.hpp"
#include "world.hpp"
#include "nation.hpp"
#include "economy.hpp"

extern World * g_world;

int World_LuaAddGood(lua_State * L) {
	Good * good = new Good();

	good->ref_name = lua_tostring(L, 1);
	good->name = lua_tostring(L, 2);
	g_world->goods.push_back(*good);

	printf("good: %s (%s)\n", good->name.c_str(), good->ref_name.c_str());
	return 0;
}

int World_LuaAddIndustryType(lua_State * L) {
	IndustryType * industry = new IndustryType;

	industry->ref_name = lua_tostring(L, 1);
	industry->name = lua_tostring(L, 2);
	g_world->industry_types.push_back(*industry);

	printf("industry_type: %s (%s)\n", industry->name.c_str(), industry->ref_name.c_str());
	return 0;
}

int World_LuaAddInputToIndustryType(lua_State * L) {
	std::string ref_name;
	
	IndustryType * industry = NULL;
	ref_name = lua_tostring(L, 1);
	size_t n_industry_types = g_world->industry_types.size();
	for(size_t i = 0; i < n_industry_types; i++) {
		if(ref_name == g_world->industry_types[i].ref_name) continue;
		industry = &g_world->industry_types[i];
		break;
	} if(industry == NULL) {
		fprintf(stderr, "industry not found %s\n", ref_name.c_str());
		return 0;
	}

	size_t good_id = (size_t)-1;
	ref_name = lua_tostring(L, 2);
	size_t n_goods = g_world->goods.size();
	for(size_t i = 0; i < n_goods; i++) {
		if(ref_name == g_world->goods[i].ref_name) continue;
		good_id = i;
		break;
	} if(good_id == (size_t)-1) {
		fprintf(stderr, "good not found %s\n", ref_name.c_str());
		return 0;
	}
	industry->inputs.push_back(good_id);
	return 0;
}

int World_LuaAddOutputToIndustryType(lua_State * L) {
	std::string ref_name;

	IndustryType * industry = NULL;
	ref_name = lua_tostring(L, 1);
	size_t n_industry_types = g_world->industry_types.size();
	for(size_t i = 0; i < n_industry_types; i++) {
		if(ref_name != g_world->industry_types[i].ref_name) continue;
		industry = &g_world->industry_types[i];
		break;
	}
	if(industry == NULL) {
		fprintf(stderr, "industry not found %s\n", ref_name.c_str());
		return 0;
	}

	size_t good_id = (size_t)-1;
	ref_name = lua_tostring(L, 2);
	size_t n_goods = g_world->goods.size();
	for(size_t i = 0; i < n_goods; i++) {
		if(ref_name != g_world->goods[i].ref_name) continue;
		good_id = i;
		break;
	} if(good_id == (size_t)-1) {
		fprintf(stderr, "good not found %s\n", ref_name.c_str());
		return 0;
	}
	industry->outputs.push_back(good_id);
	return 0;
}

int World_LuaAddNation(lua_State * L) {
	Nation * nation = new Nation();

	nation->ref_name = lua_tostring(L, 1);

	nation->color = lua_tonumber(L, 2);
	nation->color = bswap_32(nation->color);
	nation->color >>= 8;
	nation->color |= 0xff000000;

	const char * default_flag = lua_tostring(L, 3);
	nation->default_flag.from_file(default_flag);

	nation->name = lua_tostring(L, 4);
	g_world->nations.push_back(*nation);

	printf("nation: %s (%s)\n", nation->name.c_str(), nation->ref_name.c_str());
	return 0;
}

int World_LuaAddProvince(lua_State * L) {
	Province * province = new Province();

	province->ref_name = lua_tostring(L, 1);
	province->color = lua_tonumber(L, 2);
	province->color = bswap_32(province->color);
	province->color >>= 8;
	province->color |= 0xff000000;

	province->name = lua_tostring(L, 3);
	province->population = 1000;
	province->budget = 500.f;

	g_world->provinces.push_back(*province);

	// TODO: this is NOT good
	Industry industry;
	if(g_world->companies.size()) {
		industry.owner_id = rand() % g_world->companies.size();
	} else {
		industry.owner_id = 0;
	}
	industry.type_id = rand() % g_world->industry_types.size();
	g_world->provinces.back().add_industry(g_world, &industry);
	return 0;
}

int World_LuaAddCompany(lua_State * L) {
	Company * company = new Company();

	company->name = lua_tostring(L, 1);
	company->money = lua_tonumber(L, 2);
	company->is_transport = lua_toboolean(L, 3);
	company->is_retailer = lua_toboolean(L, 4);
	company->is_industry = lua_toboolean(L, 5);

	company->operating_provinces.clear();
	company->relations.clear();

	// Company has relation of 0 with everyone else
	for(size_t i = 0; i < g_world->companies.size(); i++) {
		company->relations.push_back(90.f); // We are neutral to other companies
		g_world->companies[i].relations.push_back(90.f); // Everyone is cautious to us
	}
	company->relations.push_back(100.f); // Relations with self is 100

	// Add onto vector
	g_world->companies.push_back(*company);
	return 0;
}

int World_LuaAddOperationalProvinceToCompany(lua_State * L) {
	size_t idx = lua_tonumber(L, 1);
	std::string ref_name = lua_tostring(L, 2);
	for(size_t i = 0; i < g_world->provinces.size(); i++) {
		if(g_world->provinces[i].ref_name != ref_name) continue;
		g_world->companies[idx].operating_provinces.push_back(i);
		break;
	}
	return 0;
}

#include <libintl.h>
#include <locale.h>
int World_LuaGettext(lua_State * L) {
	std::string msgid = lua_tostring(L, 1);
	std::string end_msg = gettext(msgid.c_str());
	lua_pushstring(L, end_msg.c_str());
	return 1;
}
