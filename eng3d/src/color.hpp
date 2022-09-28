// Eng3D - General purpouse game engine
// Copyright (C) 2021, Eng3D contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------
// Name:
//      color.hpp
//
// Abstract:
//      Primitive color type used through the engine
// ----------------------------------------------------------------------------

#pragma once

#include <cstdint>

namespace Eng3D {
    /// @brief Primitive color type used through the engine
    class Color {
    public:
        float r = 0.f, g = 0.f, b = 0.f;
        float a = 1.f;
    public:
        constexpr Color()
            : r{ 0.f },
            g{ 0.f },
            b{ 0.f },
            a{ 0.f }
        {

        }

        constexpr Color(float red, float green, float blue, float alpha = 1.f)
            : r{ red },
            g{ green },
            b{ blue },
            a{ alpha }
        {

        }

        ~Color() = default;

        /// @brief Create a color from RGBA components
        /// @param red Red component
        /// @param green Green component
        /// @param blue Blue component
        /// @param alpha Alpha component
        /// @return Color Resulting color
        constexpr static Color rgba8(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
            return Color(red / 255.f, green / 255.f, blue / 255.f, alpha / 255.f);
        }

        /// @brief Create a color from RGB components
        /// @param red Red component
        /// @param green Green component
        /// @param blue Blue component
        /// @return Color Resulting color
        constexpr static Color rgb8(uint8_t red, uint8_t green, uint8_t blue) {
            return rgba8(red, green, blue, 255);
        }

        /// @brief Create a color from RGBA32 components
        /// @param argb The ARGB32 color
        /// @return Color Resulting color
        constexpr static Color rgba32(uint32_t argb) {
            const auto a = ((argb >> 24) % 255) / 255.f;
            const auto b = ((argb >> 16) % 255) / 255.f;
            const auto g = ((argb >> 8) % 255) / 255.f;
            const auto r = (argb % 255) / 255.f;
            return Color(r, g, b, a);
        }

        /// @brief Get the raw value of the color
        /// @return uint32_t The raw value
        constexpr uint32_t get_value() const {
            const auto alpha = static_cast<uint8_t>(a * 255);
            const auto red = static_cast<uint8_t>(r * 255);
            const auto green = static_cast<uint8_t>(g * 255);
            const auto blue = static_cast<uint8_t>(b * 255);
            uint32_t color = (alpha << 24) | (blue << 16) | (green << 8) | (red);
            return color;
        }

        /// @brief Combine two colors with LERP
        /// @param color1 Color 1
        /// @param color2 Color 2
        /// @param lamda Intensity of merge in respect to Color 2
        /// @return Color Resulting color
        constexpr static Color lerp(Color color1, Color color2, float lamda) {
            const auto r = color1.r * (1 - lamda) + color2.r * lamda;
            const auto g = color1.g * (1 - lamda) + color2.g * lamda;
            const auto b = color1.b * (1 - lamda) + color2.b * lamda;
            return Color(r, g, b);
        }
    };
};
