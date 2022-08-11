// Eng3D - General purpouse game engine
// Copyright (C) 2021, Eng3D contributors
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
//      map.hpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#pragma once

#include <memory>
#include <vector>
#include <glm/vec2.hpp>

class MapRender;
namespace Eng3D {
    static constexpr auto GLOBE_RADIUS = 100.f;

    class Sphere;
    class Quad2D;
    class Texture;
    class TextureArray;
    class Square;
    class State;

    class BaseMap {
        Eng3D::State& s;

        std::shared_ptr<Eng3D::Texture> water_tex;
        std::shared_ptr<Eng3D::Texture> wave1;
        std::shared_ptr<Eng3D::Texture> wave2;
        std::shared_ptr<Eng3D::Texture> bathymethry;
        std::unique_ptr<Eng3D::TextureArray> terrain_sheet;
        std::unique_ptr<Eng3D::Texture> normal_topo;
        std::unique_ptr<Eng3D::Texture> topo_map;
        std::unique_ptr<Eng3D::Texture> terrain_map;
        std::shared_ptr<Eng3D::Texture> noise_tex;
        std::shared_ptr<Eng3D::Texture> paper_tex;
        std::shared_ptr<Eng3D::Texture> stripes_tex;

        std::vector<Eng3D::Square*> map_quads;
        Eng3D::Sphere* map_sphere = nullptr;
        Eng3D::Quad2D* map_2d_quad = nullptr;
    public:
        BaseMap(Eng3D::State& s, glm::ivec2 size);
        ~BaseMap();

        friend MapRender;
    };
}
