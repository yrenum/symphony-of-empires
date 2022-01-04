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
//      client/interface/top_window.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include "client/interface/top_window.hpp"
#include "client/game_state.hpp"
#include "unified_render/path.hpp"
#include "client/map.hpp"
#include "world.hpp"
#include "unified_render/texture.hpp"
#include "client/interface/policies.hpp"
#include "client/interface/army.hpp"
#include "client/interface/technology.hpp"
#include "io_impl.hpp"
#include "client/ui/components.hpp"

using namespace Interface;

TopWindow::TopWindow(GameState& _gs)
    : UI::Group(0, 0, _gs.width, 400),
    gs{ _gs }
{
    this->is_scroll = false;
    this->is_pinned = true;

    new TimeControlView(gs);

    new UI::Image(0, 99, 43, 260, "ui/border_bar.png", this);
    new UI::Image(0, 0, 159, 127, "ui/flag_border.png", this);

    auto nation_flag = &gs.get_nation_flag(*gs.curr_nation);
    auto* flag_img = new UI::Image(9, 9, 138, 84, nation_flag, this);
    flag_img->on_each_tick = ([](UI::Widget& w, void*) {
        auto& state = static_cast<TopWindow&>(*w.parent);

        w.current_texture = &state.gs.get_nation_flag(*state.gs.curr_nation);
    });

    auto* policy_ibtn = new UI::Image(9, 115, 25, 25, "ui/icons/top_bar/book.png", this);
    policy_ibtn->on_click = (UI::Callback)([](UI::Widget& w, void*) {
        auto& o = static_cast<TopWindow&>(*w.parent);
        new Interface::PoliciesScreen(o.gs);
    });
    policy_ibtn->tooltip = new UI::Tooltip(policy_ibtn, 512, 24);
    policy_ibtn->tooltip->text("Laws & Policies");

    auto* economy_ibtn = new UI::Image(9, 155, 25, 25, "ui/icons/top_bar/economy.png", this);
    economy_ibtn->on_click = (UI::Callback)([](UI::Widget& w, void*) {
        // auto& o = static_cast<TopWindow&>(*w.parent);

    });
    economy_ibtn->tooltip = new UI::Tooltip(economy_ibtn, 512, 24);
    economy_ibtn->tooltip->text("Economy & World Market");

    auto* military_ibtn = new UI::Image(9, 195, 25, 25, "ui/icons/military_score.png", this);
    military_ibtn->on_click = (UI::Callback)([](UI::Widget& w, void*) {
        auto& o = static_cast<TopWindow&>(*w.parent);
        new Interface::ArmyView(o.gs);
    });
    military_ibtn->tooltip = new UI::Tooltip(military_ibtn, 512, 24);
    military_ibtn->tooltip->text("Military");

    auto* research_ibtn = new UI::Image(9, 235, 25, 25, "ui/icons/top_bar/tech.png", this);
    research_ibtn->on_click = (UI::Callback)([](UI::Widget& w, void*) {
        auto& o = static_cast<TopWindow&>(*w.parent);
        new Interface::TechTreeView(o.gs);
    });
    research_ibtn->tooltip = new UI::Tooltip(research_ibtn, 512, 24);
    research_ibtn->tooltip->text("Research");

    auto* save_ibtn = new UI::Image(9, 275, 25, 25, "ui/icons/top_bar/save.png", this);
    save_ibtn->on_click = (UI::Callback)([](UI::Widget& w, void*) {
        auto& o = static_cast<TopWindow&>(*w.parent);
        Archive ar = Archive();
        ::serialize(ar, o.gs.world);
        ar.to_file(o.gs.curr_nation->ref_name + ".scv");
        o.gs.ui_ctx->prompt("Save", "Saved sucessfully!");
    });
    save_ibtn->tooltip = new UI::Tooltip(save_ibtn, 512, 24);
    save_ibtn->tooltip->text("Saves the current game; TODO: SAVE LUA STATE");

    auto* exit_ibtn = new UI::Image(9, 315, 25, 25, "ui/icons/top_bar/exit.png", this);
    exit_ibtn->on_click = (UI::Callback)([](UI::Widget& w, void*) {
        auto& o = static_cast<TopWindow&>(*w.parent);
        o.gs.run = false;
    });
    exit_ibtn->tooltip = new UI::Tooltip(exit_ibtn, 512, 24);
    exit_ibtn->tooltip->text("Exits");
}

TimeControlView::TimeControlView(GameState& _gs)
    : UI::Group(-192, 0, 192, 24),
    gs{ _gs }
{
    this->is_scroll = false;
    this->is_pinned = true;
    this->origin = UI::Origin::UPPER_RIGHT_SCREEN;

    auto* speed0_btn = new UI::Button(0, 0, 48, 24, this);
    speed0_btn->text("||");
    speed0_btn->on_click = ([](UI::Widget& w, void*) {
        auto& o = static_cast<TimeControlView&>(*w.parent);
        o.gs.paused = true;
    });
    speed0_btn->tooltip = new UI::Tooltip(speed0_btn, 512, 24);
    speed0_btn->tooltip->text("Pause");

    auto* speed1_btn = new UI::Button(0, 0, 48, 24, this);
    speed1_btn->right_side_of(*speed0_btn);
    speed1_btn->text(">");
    speed1_btn->on_click = ([](UI::Widget& w, void*) {
        auto& o = static_cast<TimeControlView&>(*w.parent);
        o.gs.paused = false;
        o.gs.ms_delay_speed = 1000;
    });
    speed1_btn->tooltip = new UI::Tooltip(speed1_btn, 512, 24);
    speed1_btn->tooltip->text("Turtle speed");

    auto* speed2_btn = new UI::Button(0, 0, 48, 24, this);
    speed2_btn->right_side_of(*speed1_btn);
    speed2_btn->text(">>");
    speed2_btn->on_click = ([](UI::Widget& w, void*) {
        auto& o = static_cast<TimeControlView&>(*w.parent);
        o.gs.paused = false;
        o.gs.ms_delay_speed = 500;
    });
    speed2_btn->tooltip = new UI::Tooltip(speed2_btn, 512, 24);
    speed2_btn->tooltip->text("Horse speed");

    auto* speed3_btn = new UI::Button(0, 0, 48, 24, this);
    speed3_btn->right_side_of(*speed2_btn);
    speed3_btn->text(">>>");
    speed3_btn->on_click = ([](UI::Widget& w, void*) {
        auto& o = static_cast<TimeControlView&>(*w.parent);
        o.gs.paused = false;
        o.gs.ms_delay_speed = 50;
    });
    speed3_btn->tooltip = new UI::Tooltip(speed3_btn, 512, 24);
    speed3_btn->tooltip->text("Fire speed");
}