// Symphony of Empires
// Copyright (C) 2021, Symphony of Empires contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ----------------------------------------------------------------------------
// Name:
//      client/map.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <memory>

#define NOMINMAX 1

#include <glm/gtc/matrix_transform.hpp>
// Required before GL/gl.h
#include <GL/glew.h>
#include <GL/gl.h>

#include "eng3d/state.hpp"
#include "eng3d/ui/tooltip.hpp"
#include "eng3d/font_sdf.hpp"
#include "eng3d/path.hpp"
#include "eng3d/texture.hpp"
#include "eng3d/primitive.hpp"
#include "eng3d/shader.hpp"
#include "eng3d/framebuffer.hpp"
#include "eng3d/model.hpp"
#include "eng3d/serializer.hpp"
#include "eng3d/locale.hpp"
#include "eng3d/log.hpp"
#include "eng3d/orbit_camera.hpp"
#include "eng3d/flat_camera.hpp"
#include "eng3d/camera.hpp"

#include "client/map.hpp"
#include "client/map_render.hpp"
#include "client/game_state.hpp"
#include "client/interface/province_view.hpp"
#include "client/interface/unit_widget.hpp"
#include "client/interface/battle_widget.hpp"
#include "client/interface/lobby.hpp"
#include "world.hpp"
#include "province.hpp"
#include "client/client_network.hpp"
#include "io_impl.hpp"
#include "action.hpp"
#include "client/rivers.hpp"
#include "client/borders.hpp"

void get_blob_bounds(std::set<Province*>* visited_provinces, const Nation& nation, const Province& province, glm::vec2* min_x, glm::vec2* min_y, glm::vec2* max_x, glm::vec2* max_y) {
    // Iterate over all neighbours
    for(const auto& neighbour : province.neighbours) {
        // Do not visit again
        if(visited_provinces->find(neighbour) != visited_provinces->end()) {
            continue;
        }

#if 1
        // Province must not be morbidly big
        if(std::fabs(neighbour->max_x - neighbour->min_x) >= g_world->width / 2.f || std::fabs(neighbour->max_y - neighbour->min_y) >= g_world->height / 2.f) {
            continue;
        }
#endif

        // Must own it
        if(neighbour->owner != &nation) {
            continue;
        }

        if(neighbour->min_x < min_x->x) {
            min_x->x = neighbour->min_x;
            min_x->y = neighbour->min_y;
        } if(neighbour->min_y < min_y->y) {
            min_y->x = neighbour->min_x;
            min_y->y = neighbour->min_y;
        }

        if(neighbour->max_x > max_x->x) {
            max_x->x = neighbour->max_x;
            max_x->y = neighbour->max_y;
        } if(neighbour->max_y > max_y->y) {
            max_y->x = neighbour->max_x;
            max_y->y = neighbour->max_y;
        }

        visited_provinces->insert(neighbour);
        get_blob_bounds(visited_provinces, nation, *neighbour, min_x, min_y, max_x, max_y);
    }
}

Map::Map(const World& _world, UI::Group* _map_ui_layer, int screen_width, int screen_height)
    : world(_world),
    map_ui_layer(_map_ui_layer),
    skybox(0.f, 0.f, 0.f, 255.f * 10.f, 40, false)
{
    Eng3D::State& s = Eng3D::State::get_instance();
    camera = new Eng3D::FlatCamera(glm::vec2(screen_width, screen_height), glm::vec2(world.width, world.height));

    // rivers = new Rivers();
    // borders = new Borders();
    map_render = new MapRender(world);

    // Shader used for drawing the models using custom model render
    obj_shader = std::unique_ptr<Eng3D::OpenGL::Program>(new Eng3D::OpenGL::Program());
    {
        obj_shader->attach_shader(*s.builtin_shaders["vs_3d"].get());
        obj_shader->attach_shader(*s.builtin_shaders["fs_3d"].get());
        obj_shader->link();
    }

    Eng3D::TextureOptions mipmap_options{};
    mipmap_options.wrap_s = GL_REPEAT;
    mipmap_options.wrap_t = GL_REPEAT;
    mipmap_options.min_filter = GL_LINEAR_MIPMAP_LINEAR;
    mipmap_options.mag_filter = GL_LINEAR;

    line_tex = s.tex_man->load(Path::get("gfx/line_target.png"), mipmap_options);
    skybox_tex = s.tex_man->load(Path::get("gfx/space.png"), mipmap_options);

    // Set the mapmode
    set_map_mode(political_map_mode, empty_province_tooltip);

    Eng3D::Log::debug("game", "Preloading-important stuff");

    map_font = new Eng3D::FontSDF("fonts/cinzel_sdf/cinzel");

    // Query the initial nation flags
    for(const auto& nation : world.nations) {
        std::string path = Path::get("gfx/flags/" + nation.ref_name + "_" + (nation.ideology == nullptr ? "none" : nation.ideology->ref_name.get_string()) + ".png");
        auto flag_texture = s.tex_man->load(path, mipmap_options);
        flag_texture->gen_mipmaps();
        nation_flags.push_back(flag_texture);
    }

    for(const auto& building_type : world.building_types) {
        std::string path;
        path = Path::get("models/building_types/" + building_type.ref_name + ".obj");
        building_type_models.push_back(&s.model_man->load(path));
        path = Path::get("gfx/buildingtype/" + building_type.ref_name + ".png");
        building_type_icons.push_back(s.tex_man->load(path));
    }

    for(const auto& unit_type : world.unit_types) {
        std::string path;
        path = Path::get("models/unit_types/" + unit_type.ref_name + ".obj");
        unit_type_models.push_back(&s.model_man->load(path));
        path = Path::get("gfx/unittype/" + unit_type.ref_name + ".png");
        unit_type_icons.push_back(s.tex_man->load(path));
    }

    create_labels();
}

Map::~Map() {

}

void Map::create_labels() {
    // Provinces
    for(auto& label : province_labels)
        delete label;
    province_labels.clear();
    for(const auto& province : world.provinces) {
        glm::vec2 min_point(province.min_x, province.min_y);
        glm::vec2 max_point(province.max_x, province.max_y);
        glm::vec2 mid_point = 0.5f * (min_point + max_point);

        glm::vec2 x_step = glm::vec2(std::min(max_point.x - mid_point.x, 250.f), 0);

        glm::vec3 center = glm::vec3(mid_point, 0.f);
        glm::vec3 left = glm::vec3(mid_point - x_step, 0.f);
        glm::vec3 right = glm::vec3(mid_point + x_step, 0.f);

        float width = glm::length(left - right);
        width *= 0.25f;

        glm::vec3 right_dir = glm::vec3(mid_point.x + 1.f, mid_point.y, 0.);
        right_dir = right_dir - center;
        glm::vec3 top_dir = glm::vec3(mid_point.x, mid_point.y - 1.f, 0.);
        top_dir = top_dir - center;

        center.z -= 0.1f;
        auto* label = map_font->gen_text(Eng3D::Locale::translate(province.name.get_string()), top_dir, right_dir, width, center);
        province_labels.push_back(label);
    }

    // Nations
    for(auto& label : nation_labels)
        delete label;
    nation_labels.clear();
    for(const auto& nation : world.nations) {
        if(!nation.exists())
            continue;

        glm::vec2 min_point_x(world.width - 1.f, world.height - 1.f), min_point_y(world.width - 1.f, world.height - 1.f);
        glm::vec2 max_point_x(0.f, 0.f), max_point_y(0.f, 0.f);

        std::set<Province*> visited_provinces;
        if(nation.capital != nullptr) {
            max_point_x.x = nation.capital->max_x;
            max_point_x.y = nation.capital->max_y;
            max_point_y.x = nation.capital->max_x;
            max_point_y.y = nation.capital->max_y;
            min_point_x.x = nation.capital->min_x;
            min_point_x.y = nation.capital->min_y;
            min_point_y.x = nation.capital->min_x;
            min_point_y.y = nation.capital->min_y;
            get_blob_bounds(&visited_provinces, nation, *nation.capital, &min_point_x, &min_point_y, &max_point_x, &max_point_y);
        } else {
            get_blob_bounds(&visited_provinces, nation, **(nation.controlled_provinces.begin()), &min_point_x, &min_point_y, &max_point_x, &max_point_y);
        }

#if 0
        // Stop super-big labels
        if(glm::abs(min_point_x.x - max_point_x.x) >= world.width / 2.f || glm::abs(min_point_y.y - max_point_y.y) >= world.height / 2.f) {
            auto* label = map_font->gen_text(nation->get_client_hint().alt_name, glm::vec3(-10.f), glm::vec3(-5.f), 1.f);
            nation_labels.push_back(label);
            Eng3D::Log::debug("game", "Extremely big nation: " + nation->ref_name);
            continue;
        }
#endif

        glm::vec2 lab_min, lab_max;

        glm::vec2 center_1 = (max_point_x + max_point_y + min_point_y) / 3.f;
        glm::vec2 center_2 = (max_point_y + min_point_x + min_point_y) / 3.f;

        lab_min = center_1;
        lab_max = center_2;

        glm::vec2 mid_point = (lab_min + lab_max) / 2.f;
        glm::vec3 center = glm::vec3(mid_point, 0.f);
        glm::vec2 x_step = glm::vec2(lab_max.x - mid_point.x, 0.f);
        glm::vec3 left = glm::vec3(mid_point - x_step, 0.f);
        glm::vec3 right = glm::vec3(mid_point + x_step, 0.f);
        float width = glm::length(left - right);

        glm::vec3 right_dir = glm::vec3(mid_point.x + 1.f, mid_point.y, 0.f) - center;
        glm::vec3 top_dir = glm::vec3(mid_point.x, mid_point.y - 1.f, 0.f) - center;
        glm::vec3 normal = glm::cross(top_dir, right_dir);
        normal = glm::normalize(normal);
        float angle = glm::atan(lab_max.y - lab_min.y, lab_max.x - lab_min.x);
        if(angle > (M_PI / 2.0f))
            angle -= M_PI;
        else if(angle < -(M_PI / 2.0f))
            angle += M_PI;

        glm::mat4 rot = glm::rotate(glm::mat4(1.), angle, normal);
        top_dir = rot * glm::vec4(top_dir, 1.);
        right_dir = rot * glm::vec4(right_dir, 1.);

        center.z -= 0.1f;
        auto* label = map_font->gen_text(Eng3D::Locale::translate(nation.get_client_hint().alt_name.get_string()), top_dir, right_dir, width, center);
        nation_labels.push_back(label);
    }
}

void Map::set_view(MapView view) {
    view_mode = view;

    Eng3D::Camera* old_camera = camera;
    if(view == MapView::PLANE_VIEW)
        camera = new Eng3D::FlatCamera(*old_camera);
    else if(view == MapView::SPHERE_VIEW)
        camera = new Eng3D::OrbitCamera(*old_camera, GLOBE_RADIUS);
    delete old_camera;
    // create_labels();
}

// The standard map mode with each province color = country color
std::vector<ProvinceColor> political_map_mode(const World& world) {
    std::vector<ProvinceColor> province_color;
    for(unsigned int i = 0; i < world.provinces.size(); i++) {
        Nation* province_owner = world.provinces[i].controller;
        if(province_owner == nullptr)
            province_color.push_back(ProvinceColor(i, Eng3D::Color::rgba32(0xffdddddd)));
        else
            province_color.push_back(ProvinceColor(i, Eng3D::Color::rgba32(province_owner->get_client_hint().color)));
    }

    // Land
    province_color.push_back(ProvinceColor(Province::invalid(), Eng3D::Color::rgba32(0xffdddddd)));
    return province_color;
}

std::string political_province_tooltip(const World& world, const Province::Id id) {
    return world.provinces[id].name.get_string();
}

std::string empty_province_tooltip(const World&, const Province::Id) {
    return "";
}

void Map::reload_shaders() {
    map_render->reload_shaders();
}

void Map::set_selection(selector_func _selector) {
    selector = _selector;
}

void Map::set_map_mode(mapmode_generator mapmode_generator, mapmode_tooltip tooltip_generator) {
    mapmode_func = mapmode_generator;
    mapmode_tooltip_func = tooltip_generator;
    update_mapmode();
}

void Map::set_selected_province(bool selected, Province::Id id) {
    this->province_selected = selected;
    this->selected_province_id = id;
    map_render->update_visibility();
}

void Map::draw_flag(const Eng3D::OpenGL::Program& shader, const Nation& nation) {
    // Draw a flag that "waves" with some cheap wind effects it
    // looks nice and it's super cheap to make - only using sine
    const float n_steps = 8.f; // Resolution of flag in one side (in vertices)
    const float step = 90.f; // Steps per vertice

    auto flag = Eng3D::Mesh<glm::vec3, glm::vec2>(Eng3D::MeshMode::TRIANGLE_STRIP);
    for(float r = 0.f; r <= (n_steps * step); r += step) {
        float sin_r = (sin(r + wind_osc) / 24.f);

        sin_r = sin(r + wind_osc) / 24.f;
        flag.buffer.push_back(Eng3D::MeshData<glm::vec3, glm::vec2>(
            glm::vec3(((r / step) / n_steps) * 1.5f, sin_r, -2.f),
            glm::vec2((r / step) / n_steps, 0.f)
            ));

        sin_r = sin(r + wind_osc + 160.f) / 24.f;
        flag.buffer.push_back(Eng3D::MeshData<glm::vec3, glm::vec2>(
            glm::vec3(((r / step) / n_steps) * 1.5f, sin_r, -1.f),
            glm::vec2((r / step) / n_steps, 1.f)
            ));
    }
    flag.upload();
    shader.set_texture(0, "diffuse_map", *nation_flags[world.get_id(nation)]);
    flag.draw();
}

void Map::handle_click(GameState& gs, SDL_Event event) {
    Input& input = gs.input;

    if(input.select_pos.first < 0 || input.select_pos.first >= this->world.width || input.select_pos.second < 0 || input.select_pos.second >= this->world.height)
        return;

    if(event.button.button == SDL_BUTTON_LEFT) {
        const Tile& tile = gs.world->get_tile(input.select_pos.first, input.select_pos.second);
        switch(gs.current_mode) {
        case MapMode::COUNTRY_SELECT:
            if(Province::is_valid(tile.province_id)) {
                auto& province = world.provinces[tile.province_id];
                if(province.controller != nullptr)
                    gs.select_nation->change_nation(province.controller->cached_id);
            }
            break;
        case MapMode::NORMAL:
            if(this->selector) {
                /// @todo Good selector function
                //this->selector(this->world, *this, world.provinces[tile.province_id]);
                break;
            }

            // Check if we selected an unit
            is_drag = false;
            if(input.selected_units.empty()) {
                // Show province information when clicking on a province
                if(tile.province_id < gs.world->provinces.size()) {
                    new Interface::ProvinceView(gs, gs.world->provinces[tile.province_id]);
                    return;
                }
            }
            break;
        default:
            break;
        }
        return;
    } else if(event.button.button == SDL_BUTTON_RIGHT) {
        const Tile& tile = gs.world->get_tile(input.select_pos.first, input.select_pos.second);
        if(Province::is_invalid(tile.province_id))
            return;

        Province& province = gs.world->provinces[tile.province_id];
        if(gs.editor) {
            switch(gs.current_mode) {
            case MapMode::NORMAL:
                gs.curr_nation->give_province(province);
                province.nuclei.insert(gs.curr_nation);
                update_mapmode();
                break;
            default:
                break;
            }
        }

        for(const auto& unit : input.selected_units) {
            if(!unit->province->is_neighbour(province) || !unit->can_move())
                continue;
            // Don't change target if...
            if(unit->province == &province || unit->target == &province)
                continue;
            if(province.controller != nullptr && province.controller != gs.curr_nation) {
                // Must either be our ally, have military access with them or be at war
                const NationRelation& relation = gs.world->get_relation(gs.world->get_id(*gs.curr_nation), gs.world->get_id(*province.controller));
                if(!(relation.has_war || relation.has_alliance || relation.has_military_access))
                    continue;
            }

            Eng3D::Networking::Packet packet = Eng3D::Networking::Packet();
            Archive ar = Archive();
            ActionType action = ActionType::UNIT_CHANGE_TARGET;
            ::serialize(ar, &action);
            ::serialize(ar, &unit);
            Province* tmp_province_ref = &province;
            ::serialize(ar, &tmp_province_ref);
            packet.data(ar.get_buffer(), ar.size());
            g_client->send(packet);

            const std::scoped_lock lock2(gs.sound_lock);
            auto entries = Path::get_all_recursive("sfx/land_move");
            if(!entries.empty())
                gs.sound_queue.push_back(new Eng3D::Audio(entries[std::rand() % entries.size()]));
        }
        input.selected_units.clear();
    }
}

void Map::update(const SDL_Event& event, Input& input, UI::Context* ui_ctx, GameState& gs) {
    std::pair<int, int>& mouse_pos = input.mouse_pos;
    // std::pair<float, float>& select_pos = input.select_pos;
    switch(event.type) {
    case SDL_JOYBUTTONDOWN:
    case SDL_MOUSEBUTTONDOWN:
        SDL_GetMouseState(&mouse_pos.first, &mouse_pos.second);

        is_drag = false;
        if(event.button.button == SDL_BUTTON_MIDDLE) {
            glm::ivec2 map_pos;
            if(camera->get_cursor_map_pos(input.mouse_pos, map_pos)) {
                last_camera_drag_pos = map_pos;
                input.last_camera_mouse_pos = mouse_pos;
            }
        } else if(event.button.button == SDL_BUTTON_LEFT) {
            input.drag_coord = input.select_pos;
            input.drag_coord.first = (int)input.drag_coord.first;
            input.drag_coord.second = (int)input.drag_coord.second;
            is_drag = true;
        }
        break;
    case SDL_JOYBUTTONUP:
    case SDL_MOUSEBUTTONUP:
        is_drag = false;
        break;
    case SDL_JOYAXISMOTION: {
        int xrel = SDL_JoystickGetAxis(input.joy, 0);
        int yrel = SDL_JoystickGetAxis(input.joy, 1);

        const float sensivity = Eng3D::State::get_instance().joy_sensivity;

        float x_force = xrel / sensivity;
        float y_force = yrel / sensivity;

        input.mouse_pos.first += x_force;
        input.mouse_pos.second += y_force;

        if(input.middle_mouse_down) {  // Drag the map with middlemouse
            glm::ivec2 map_pos;
            if(camera->get_cursor_map_pos(input.mouse_pos, map_pos)) {
                glm::vec2 current_pos = glm::make_vec2(camera->get_map_pos());
                const glm::vec2 pos = current_pos + last_camera_drag_pos - glm::vec2(map_pos);
                camera->set_pos(pos.x, pos.y);
            }
        }
        glm::ivec2 map_pos;
        if(camera->get_cursor_map_pos(input.mouse_pos, map_pos)) {
            input.select_pos.first = map_pos.x;
            input.select_pos.second = map_pos.y;
        }
    } break;
    case SDL_MOUSEMOTION:
        SDL_GetMouseState(&mouse_pos.first, &mouse_pos.second);
        glm::ivec2 map_pos;

        if(input.middle_mouse_down) {  // Drag the map with middlemouse
            if(camera->get_cursor_map_pos(mouse_pos, map_pos)) {
                glm::vec2 current_pos = glm::make_vec2(camera->get_map_pos());
                const glm::vec2 pos = current_pos + last_camera_drag_pos - glm::vec2(map_pos);
                camera->set_pos(pos.x, pos.y);
            }
        }

        if(camera->get_cursor_map_pos(mouse_pos, map_pos)) {
            if(map_pos.x < 0 || map_pos.x >(int)world.width || map_pos.y < 0 || map_pos.y >(int)world.height)
                break;

            input.select_pos.first = map_pos.x;
            input.select_pos.second = map_pos.y;
            auto prov_id = world.get_tile(map_pos.x, map_pos.y).province_id;
            if(mapmode_tooltip_func != nullptr) {
                std::string text = mapmode_tooltip_func(world, prov_id);
                UI::Tooltip* tooltip = new UI::Tooltip();
                tooltip->text(text);
                ui_ctx->use_tooltip(tooltip, glm::ivec2(mouse_pos.first, mouse_pos.second));
            }
        }
        break;
    case SDL_MOUSEWHEEL:
        SDL_GetMouseState(&mouse_pos.first, &mouse_pos.second);
        camera->move(0.f, 0.f, -event.wheel.y * gs.delta_time * 120.f);
        break;
    case SDL_KEYDOWN:
        switch(event.key.keysym.sym) {
        case SDLK_UP:
            camera->move(0.f, -1.f, 0.f);
            break;
        case SDLK_DOWN:
            camera->move(0.f, 1.f, 0.f);
            break;
        case SDLK_LEFT:
            camera->move(-1.f, 0.f, 0.f);
            break;
        case SDLK_RIGHT:
            camera->move(1.f, 0.f, 0.f);
            break;
        }
        break;
    }
}

// Updates the province color texture with the changed provinces
void Map::update_mapmode() {
    std::vector<ProvinceColor> province_colors = mapmode_func(world);
    map_render->update_mapmode(province_colors);
}

void Map::draw(const GameState& gs) {
    map_render->draw(camera, view_mode);
    // rivers->draw(camera);
    //borders->draw(camera);

    /// @todo We need to better this
    obj_shader->use();
    const glm::mat4 projection = camera->get_projection();
    obj_shader->set_uniform("projection", projection);
    const glm::mat4 view = camera->get_view();
    obj_shader->set_uniform("view", view);

    auto preproc_quad = Eng3D::Quad2D(); // Reused a bunch of times

    // Properly display textures :)
    glm::mat4 base_model = glm::mat4(1.f);
    std::vector<float> province_units_y(world.provinces.size(), 0.f);
    for(const auto& war : world.wars) {
        for(const auto& battle : war->battles) {
            unsigned int i;

            const float y = province_units_y[world.get_id(*battle.province)];
            province_units_y[world.get_id(*battle.province)] += 2.5f;
            const std::pair<float, float> prov_pos = battle.province->get_pos();

            // Attackers on the left side
            i = 0;
            for(const auto& unit : battle.attackers) {
                const std::pair<float, float> pos = std::make_pair(prov_pos.first - (2.f * i) - 3.f, prov_pos.second - y);
                glm::mat4 model = glm::translate(base_model, glm::vec3(pos.first, pos.second, 0.f));
                model = glm::rotate(model, -90.f, glm::vec3(1.f, 0.f, 0.f));
                obj_shader->set_uniform("model", model);
                // Model
                unit_type_models[world.get_id(*unit->type)]->draw(*obj_shader);
                i++;
            }

            // Defenders on the right side
            i = 1;
            for(const auto& unit : battle.defenders) {
                const std::pair<float, float> pos = std::make_pair(prov_pos.first + (2.f * i) + 3.f, prov_pos.second - y);
                glm::mat4 model = glm::translate(base_model, glm::vec3(pos.first, pos.second, 0.f));
                model = glm::rotate(model, -90.f, glm::vec3(1.f, 0.f, 0.f));
                obj_shader->set_uniform("model", model);
                // Model
                unit_type_models[world.get_id(*unit->type)]->draw(*obj_shader);
                i++;
            }
        }
    }

    // Display units that aren't on battles
    size_t unit_index = 0;
    for(const auto& province : world.provinces) {
        const glm::vec2 prov_pos = glm::vec2(province.get_pos().first, province.get_pos().second);
        for(const auto& unit : province.get_units()) {
            if(unit->on_battle)
                continue;

            bool unit_visible = true;
            if(view_mode == MapView::SPHERE_VIEW) {
                glm::vec3 cam_pos = camera->get_world_pos();
                glm::vec3 world_pos = camera->get_tile_world_pos(prov_pos);
                if(glm::dot(cam_pos, world_pos) <= 0)
                    unit_visible = false;
            }

            if(unit_visible) {
                if(unit_index < unit_widgets.size())
                    unit_widgets[unit_index]->set_unit(*unit);
                else
                    unit_widgets.push_back(new Interface::UnitWidget(*unit, *this, map_ui_layer));
                unit_index++;
            }
        }
    }
    for(size_t i = unit_index; i < unit_widgets.size(); i++)
        unit_widgets[i]->kill();
    unit_widgets.resize(unit_index);

    // Display battles on the map (units on a battle are hidden automatically)
    size_t battle_index = 0;
    for(const auto& war : world.wars) {
        size_t war_battle_idx = 0;
        for(const auto& battle : war->battles) {
            const glm::vec2 prov_pos = glm::vec2(battle.province->get_pos().first, battle.province->get_pos().second);
            bool battle_visible = true;
            if(view_mode == MapView::SPHERE_VIEW) {
                glm::vec3 cam_pos = camera->get_world_pos();
                glm::vec3 world_pos = camera->get_tile_world_pos(prov_pos);
                if(glm::dot(cam_pos, world_pos) <= 0)
                    battle_visible = false;
            }

            if(battle_visible) {
                if(battle_index < battle_widgets.size())
                    battle_widgets[battle_index]->set_battle(*war, war_battle_idx);
                else
                    battle_widgets.push_back(new Interface::BattleWidget(*war, war_battle_idx, *this, map_ui_layer));
                battle_index++;
            }
            war_battle_idx++;
        }
    }
    for(size_t i = battle_index; i < battle_widgets.size(); i++)
        battle_widgets[i]->kill();
    battle_widgets.resize(battle_index);

    for(const auto& province : world.provinces) {
        const float y = province_units_y[world.get_id(province)];
        province_units_y[world.get_id(province)] += 2.5f;
        const glm::vec2 prov_pos = glm::vec2(province.get_pos().first, province.get_pos().second);

        unsigned int i = 0;
        for(const auto& unit : province.get_units()) {
            glm::vec2 pos = prov_pos;
            pos.x -= 1.5f * ((province.get_units().size() / 2) - i);
            pos.y -= y;
            glm::mat4 model = glm::translate(base_model, glm::vec3(pos.x, pos.y, 0.f));
            if(unit->target != nullptr) {
                //Eng3D::Line target_line = Eng3D::Line(pos.x, pos.y, );
                const glm::vec2 target_pos = glm::vec2(
                    unit->target->min_x + ((unit->target->max_x - unit->target->min_x) / 2.f),
                    unit->target->min_y + ((unit->target->max_y - unit->target->min_y) / 2.f)
                );

                const float dist = glm::sqrt(glm::pow(glm::abs(pos.x - target_pos.x), 2.f) + glm::pow(glm::abs(pos.y - target_pos.y), 2.f));
                auto line_square = Eng3D::Square(0.f, 0.f, dist, 0.5f);
                glm::mat4 line_model = glm::rotate(model, glm::atan(target_pos.y - pos.y, target_pos.x - pos.x), glm::vec3(0.f, 0.f, 1.f));
                obj_shader->set_texture(0, "diffuse_map", *line_tex);
                obj_shader->set_uniform("model", line_model);
                line_square.draw();
            }
            model = glm::rotate(model, -90.f, glm::vec3(1.f, 0.f, 0.f));
            obj_shader->set_uniform("model", model);

            // Model
            obj_shader->set_uniform("model", model);
            unit_type_models[world.get_id(*unit->type)]->draw(*obj_shader);
            i++;
        }

        for(const auto& building_type : world.building_types) {
            continue;
            if(province.buildings[world.get_id(building_type)].level == 0)
                continue;

            glm::vec2 pos = prov_pos;
            glm::mat4 model = glm::translate(base_model, glm::vec3(pos.x, pos.y, 0.f));
            model = glm::rotate(model, 180.f, glm::vec3(1.f, 0.f, 0.f));
            obj_shader->set_uniform("model", model);
            building_type_models[world.get_id(building_type)]->draw(*obj_shader);
        }
    }
    //*/

    // Highlight for units
    for(const auto& unit : gs.input.selected_units) {
        const std::pair<float, float> pos = unit->get_pos();
        glm::mat4 model = glm::translate(base_model, glm::vec3(pos.first, pos.second, 0.f));
        obj_shader->set_uniform("model", model);
        obj_shader->set_texture(0, "diffuse_map", *gs.tex_man->load(Path::get("gfx/select_border.png")).get());
        preproc_quad.draw();
    }

    // Draw the "drag area" box
    if(is_drag) {
        glm::mat4 model = base_model;
        obj_shader->set_uniform("model", model);
        obj_shader->set_texture(0, "diffuse_map", *line_tex);
        Eng3D::Square dragbox_square = Eng3D::Square(gs.input.drag_coord.first, gs.input.drag_coord.second, gs.input.select_pos.first, gs.input.select_pos.second);
        dragbox_square.draw();
    }

    if(view_mode == MapView::SPHERE_VIEW) {
        // Universe skybox
        const glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 0.f));
        obj_shader->set_uniform("model", model);
        obj_shader->set_texture(0, "diffuse_map", *skybox_tex);
        skybox.draw();
    }

    glm::vec3 map_pos = camera->get_map_pos();
    float distance_to_map = map_pos.z / world.width;
    if(distance_to_map < 0.070)
        map_font->draw(province_labels, camera, view_mode == MapView::SPHERE_VIEW);
    else
        map_font->draw(nation_labels, camera, view_mode == MapView::SPHERE_VIEW);
    wind_osc += 0.1f;
}
