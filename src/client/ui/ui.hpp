#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <vector>
#include <string>

#ifdef _MSC_VER
#include <SDL_surface.h>
#include <SDL_ttf.h>
#else
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_ttf.h>
#endif

#include <glm/vec2.hpp>
#include "unified_render/rectangle.hpp"
#include "unified_render/color.hpp"

namespace UnifiedRender {
    class Texture;
}

enum class CLICK_STATE {
    NOT_CLICKED,
    NOT_HANDLED,
    HANDLED,
};

namespace UI {
    class Widget;
    class Tooltip;
    typedef void (*Callback)(Widget&, void*);
    class Context {
        int drag_x, drag_y;
        bool is_drag;
        Widget* dragged_widget;
        int width, height;

        glm::ivec2 get_pos(Widget& w, glm::ivec2 offset);
        void check_hover_recursive(Widget& w, const unsigned int mx, const unsigned int my, int x_off, int y_off);
        CLICK_STATE check_click_recursive(Widget& w, const unsigned int mx, const unsigned int my, int x_off, int y_off, CLICK_STATE click_state, bool clickable);
        int check_wheel_recursive(Widget& w, unsigned mx, unsigned my, int x_off, int y_off, int y);
    public:
        Context();
        void load_textures();
        void add_widget(Widget* widget);
        void remove_widget(Widget* widget);

        void render_recursive(Widget& widget, UnifiedRender::Rect viewport);
        void render_all();

        void resize(const int width, const int height);

        void check_hover(unsigned mx, unsigned my);
        bool check_click(unsigned mx, unsigned my);
        void check_drag(unsigned mx, unsigned my);
        int check_wheel(unsigned mx, unsigned my, int y);
        void check_text_input(const char* input);

        int do_tick_recursive(Widget& w);
        void do_tick(void);

        void clear(void);
        void clear_dead();

        void prompt(const std::string& title, const std::string& text);

        const UnifiedRender::Texture* background, * window_top, * button, * tooltip_texture, * piechart_overlay, * border_tex, * button_border;
        TTF_Font* default_font;

        std::vector<Widget*> widgets;
        Tooltip* tooltip_widget = nullptr;
    };
    extern Context* g_ui_context;
};  // namespace UI