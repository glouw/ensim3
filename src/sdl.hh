#pragma once

struct sdl_t
{
    int xres_p = 0;
    int yres_p = 0;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    void* pixels = nullptr;
    int pitch = 0;
    std::function<void(void)> on_exit;
    std::function<void(int, int)>
        edit_on_left_drag,
        edit_on_shift_left_click_down,
        edit_on_left_click_down,
        edit_on_right_click_up,
        edit_on_left_click_up;
    std::function<void(void)>
        edit_on_ctrl_a_key_down,
        edit_on_ctrl_s_key_down,
        edit_on_ctrl_l_key_down,
        edit_on_shift_g_key_down,
        edit_on_shift_a_key_down,
        edit_on_1_key_down,
        edit_on_2_key_down,
        edit_on_3_key_down,
        edit_on_4_key_down,
        edit_on_b_key_down,
        edit_on_f_key_down,
        edit_on_g_key_down,
        edit_on_p_key_down,
        edit_on_n_key_down,
        edit_on_i_key_down,
        edit_on_h_key_down,
        edit_on_j_key_down,
        edit_on_k_key_down,
        edit_on_q_key_down,
        edit_on_delete_key_down,
        append_on_ctrl_w_key_down,
        append_on_esc_key_down,
        append_on_backspace_key_down,
        append_on_return_key_down;
    std::function<void(char)> append_on_key_down;
    render_rect_t selection_box;
    SDL_AudioSpec spec;
    SDL_AudioDeviceID audio_device;
    bool selection_box_is_valid = false;
    bool is_append_mode = false;
    bool is_pause_mode = false;
    bool is_help_mode = false;
    int append_line = 0;
    int running_animation_index = 0;
    std::array<std::string, 8> running_animation = {"|", "/", "-", "\\", "|", "/", "-", "\\"};
    SDL_Cursor* cursor_arrow = nullptr;
    SDL_Cursor* cursor_wait = nullptr;
    SDL_Cursor* cursor_size_all = nullptr;
    int render_ticks = 0;
    flashing_colo_t red_flash{colo_t::red, colo_t::white};
    std::vector<colo_t> graph_colos = {
        colo_t::bright_red,
        colo_t::bright_green,
        colo_t::bright_yellow,
        colo_t::bright_magenta,
        colo_t::bright_cyan,
        colo_t::bright_blue,
        colo_t::bright_orange,
        colo_t::bright_pink,
    };
    moving_average_filter_t frame_time_ms_smoother{128};
    moving_average_filter_t sim_time_ms_smoother{128};

    sdl_t(int xres_p, int yres_p)
        : xres_p{xres_p}
        , yres_p{yres_p}
        {
            SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
            cursor_arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
            cursor_wait = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
            cursor_size_all = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
            set_cursor_arrow();
            window = SDL_CreateWindow(sim::title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, xres_p, yres_p, SDL_WINDOW_BORDERLESS);
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, xres_p, yres_p);
            spec.freq = sim::sample_frequency_hz;
            spec.format = AUDIO_F32SYS;
            spec.channels = 1;
            spec.samples = sim::cycles_per_frame;
            spec.callback = nullptr;
            audio_device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
            pause_audio();
        }

    ~sdl_t()
    {
        SDL_FreeCursor(cursor_arrow);
        SDL_FreeCursor(cursor_wait);
        SDL_FreeCursor(cursor_size_all);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_CloseAudioDevice(audio_device);
        SDL_Quit();
    }

    void pause_audio()
    {
        SDL_PauseAudioDevice(audio_device, true);
    }

    void play_audio()
    {
        SDL_PauseAudioDevice(audio_device, false);
    }

    void queue_audio(const std::vector<float>& buffer)
    {
        int bytes = buffer.size() * sizeof(float);
        SDL_QueueAudio(audio_device, buffer.data(), bytes);
    }

    int get_audio_queue_size()
    {
        return SDL_GetQueuedAudioSize(audio_device) / sizeof(float);
    }

    void p_controller_delay(double cycle_total_ms)
    {
        double cycle_max_ms = 1000.0 * sim::dt_s * sim::cycles_per_frame;
        double queue_size_setpoint = 4 * sim::cycles_per_frame;
        double queue_size_error = queue_size_setpoint - get_audio_queue_size();
        double p_controller_ms = 2.0 * queue_size_error / sim::cycles_per_frame;
        double delay_ms = cycle_max_ms - cycle_total_ms - p_controller_ms;
        delay(delay_ms);
    }

    void delay(int ms)
    {
        if(ms > 0)
        {
            SDL_Delay(ms);
        }
    }

    void clear()
    {
        for(int y = 0; y < yres_p; y++)
        for(int x = 0; x < xres_p; x++)
        {
            draw_pixel(x, y, colo_t::black);
        }
    }

    void lock()
    {
        SDL_LockTexture(texture, nullptr, &pixels, &pitch);
    }

    void unlock()
    {
        SDL_UnlockTexture(texture);
    }

    void flip()
    {
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    bool in_bounds(int x_p, int y_p) const
    {
        return x_p >= 0 && x_p < xres_p && y_p >= 0 && y_p < yres_p;
    }

    void draw_pixel(int x_p, int y_p, colo_t colo)
    {
        if(in_bounds(x_p, y_p))
        {
            uint32_t pixel = static_cast<uint32_t>(colo);
            int index = y_p * (pitch / sizeof(pixel)) + x_p;
            reinterpret_cast<uint32_t*>(pixels)[index] = pixel;
        }
    }

    enum class align_t
    {
        center, bot_left
    };

    void draw_char(int x_p, int y_p, char c, int scale, colo_t colo)
    {
        int index = c - ' ';
        const uint8_t* at = ui::font[index];
        for(int row = 0; row < ui::font_size_p; ++row)
        for(int col = 0; col < ui::font_size_p; ++col)
        {
            if(at[row] & (1 << col))
            {
                for(int scale_row = 0; scale_row < scale; ++scale_row)
                for(int scale_col = 0; scale_col < scale; ++scale_col)
                    draw_pixel(x_p + col * scale + scale_col, y_p + row * scale + scale_row, colo);
            }
        }
    }

    void draw_render_circle_tangent_lines(const render_circle_t& a, const render_circle_t& b, colo_t colo)
    {
        int dx_p = b.x_p - a.x_p;
        int dy_p = b.y_p - a.y_p;
        int a_radius_p = a.calc_radius_p();
        int b_radius_p = b.calc_radius_p();
        double dist_p = std::sqrt(dx_p * dx_p + dy_p * dy_p);
        double t1_p = std::atan2(dy_p, dx_p);
        double t2_p = std::acos((a_radius_p - b_radius_p) / dist_p);
        draw_line(
            a.x_p + a_radius_p * std::cos(t1_p + t2_p),
            a.y_p + a_radius_p * std::sin(t1_p + t2_p),
            b.x_p + b_radius_p * std::cos(t1_p + t2_p),
            b.y_p + b_radius_p * std::sin(t1_p + t2_p),
            colo);
        draw_line(
            a.x_p + a_radius_p * std::cos(t1_p - t2_p),
            a.y_p + a_radius_p * std::sin(t1_p - t2_p),
            b.x_p + b_radius_p * std::cos(t1_p - t2_p),
            b.y_p + b_radius_p * std::sin(t1_p - t2_p),
            colo);
    }

    int to_otto_cycle(double theta_r)
    {
        return calc_otto_theta_r(theta_r) / M_PI;
    }

    int draw_throttle_port(int x0_p, int y0_p, throttle_port_t* throttle_port)
    {
        double angle_r = throttle_port->throttle_cable.pull_ratio * M_PI_2;
        double diameter_m = throttle_port->diameter_m;
        int diameter_p = ui::throttle_scale * diameter_m;
        int half_diameter_p = diameter_p / 2;
        int side_offset_p = diameter_p / 2 + 0.1 * diameter_p / 2;
        int x_start_p = x0_p - half_diameter_p * std::cos(angle_r);
        int y_start_p = y0_p - half_diameter_p * std::sin(angle_r);
        int x_end_p = x0_p + half_diameter_p * std::cos(angle_r);
        int y_end_p = y0_p + half_diameter_p * std::sin(angle_r);
        draw_line(x_start_p, y_start_p, x_end_p, y_end_p, colo_t::white);
        draw_line(x0_p - side_offset_p, y0_p - half_diameter_p, x0_p - side_offset_p, y0_p + half_diameter_p, colo_t::white);
        draw_line(x0_p + side_offset_p, y0_p - half_diameter_p, x0_p + side_offset_p, y0_p + half_diameter_p, colo_t::white);
        int decorative_circle_diameter_p = diameter_p / 8;
        render_circle_t circle{x0_p, y0_p, decorative_circle_diameter_p};
        draw_render_circle(circle, colo_t::white);
        return diameter_p;
    }

    int draw_piston(int x0_p, int y0_p, piston_t* piston)
    {
        double pin_diameter_m = 0.02;
        int crank_diameter_p = 2.0 * ui::piston_scale * piston->crank_throw_length_m;
        int bearing_x_p = x0_p + ui::piston_scale * piston->bearing_x_m;
        int bearing_y_p = y0_p - ui::piston_scale * piston->bearing_y_m;
        int pin_x_p = x0_p + ui::piston_scale * piston->pin_x_m;
        int pin_y_p = y0_p - ui::piston_scale * piston->pin_y_m;
        int pin_diameter_p = ui::piston_scale * pin_diameter_m;
        int head_x0_p = pin_x_p;
        int head_y0_p = pin_y_p;
        int head_x1_p = pin_x_p + ui::piston_scale * piston->diameter_m;
        int head_y1_p = pin_y_p + ui::piston_scale * 2.0 * piston->head_compression_height_m;
        int top_y_p = y0_p - ui::piston_scale * piston->calc_block_deck_surface_m();
        render_circle_t crank{x0_p, y0_p, crank_diameter_p};
        render_circle_t bearing{bearing_x_p, bearing_y_p, pin_diameter_p};
        render_circle_t pin{pin_x_p, pin_y_p, pin_diameter_p};
        render_rect_t head{head_x0_p, head_y0_p, head_x1_p, head_y1_p};
        head.center();
        std::vector<colo_text_t> cycle = {
            {colo_t::white, double_to_string(to_otto_cycle(piston->calc_theta_r()), 0)}
        };
        draw_texts(x0_p, y0_p, cycle, 1, true);
        draw_render_circle(crank, colo_t::white);
        draw_render_circle(bearing, colo_t::white);
        draw_render_circle(pin, colo_t::white);
        draw_render_circle_tangent_lines(bearing, pin, colo_t::white);
        draw_render_rect(head, colo_t::white);
        draw_line(head.x0_p, top_y_p, head.x1_p, top_y_p, colo_t::white);
        if(piston->ignition_flame.is_burning)
        {
            int flame_depth_p = ui::piston_scale * piston->ignition_flame.depth_m;
            int radius_p = ui::piston_scale * piston->ignition_flame.diameter_m / 2.0;
            int flame_x0_p = pin_x_p - radius_p;
            int flame_y0_p = top_y_p;
            int flame_x1_p = pin_x_p + radius_p;
            int flame_y1_p = top_y_p + flame_depth_p;
            render_rect_t flame{flame_x0_p, flame_y0_p, flame_x1_p, flame_y1_p};
            draw_render_rect(flame, colo_t::red);
        }
        if(piston->autoignition_flame.is_burning)
        {
            int mid_y_p = top_y_p + ui::piston_scale * piston->calc_chamber_depth_m() / 2.0;
            int flame_depth_p = ui::piston_scale * piston->autoignition_flame.depth_m;
            int radius_p = ui::piston_scale * piston->autoignition_flame.diameter_m / 2.0;
            int flame_x0_p = pin_x_p - radius_p;
            int flame_y0_p = mid_y_p - flame_depth_p / 2;
            int flame_x1_p = pin_x_p + radius_p;
            int flame_y1_p = mid_y_p + flame_depth_p / 2;
            render_rect_t flame{flame_x0_p, flame_y0_p, flame_x1_p, flame_y1_p};
            draw_render_rect(flame, colo_t::yellow);
        }
        return crank_diameter_p;
    }

    void draw_pistons(int x0_p, int y0_p, const std::vector<piston_t*>& pistons)
    {
        for(piston_t* piston : pistons)
        {
            int crank_diameter_p = draw_piston(x0_p, y0_p, piston);
            int spacing_p = 0.25 * crank_diameter_p;
            x0_p -= crank_diameter_p + spacing_p;
        }
    }

    void draw_throttle_ports(int x0_p, int y0_p, const std::vector<throttle_port_t*>& throttle_ports)
    {
        for(throttle_port_t* throttle_port : throttle_ports)
        {
            int diameter_p = draw_throttle_port(x0_p, y0_p, throttle_port);
            int spacing_p = 0.25 * diameter_p;
            x0_p -= diameter_p + spacing_p;
        }
    }

    void draw_text(int x_p, int y_p, const std::string& text, int scale, colo_t colo)
    {
        int at = 0;
        for(char c : text)
        {
            draw_char(x_p, y_p, c, scale, colo);
            x_p += ui::font_size_p * scale;
            if(colo == colo_t::green)
            {
                int last = text.size() - 1;
                if(at++ == last)
                {
                    draw_char(x_p, y_p, '_', scale, colo);
                }
            }
        }
    }

    using colo_text_t = std::pair<colo_t, std::string>;

    void draw_texts(int x_p, int y_p, const std::vector<colo_text_t>& texts, int scale, bool center)
    {
        int size_p = ui::font_size_p * scale;
        int h_p = size_p * ui::line_spacing;
        int hh_p = size_p + h_p * (texts.size() - 1);
        for(const colo_text_t& text : texts)
        {
            int w_p = size_p * text.second.size();
            int xx_p = x_p - w_p / 2 + scale;
            int yy_p = y_p - hh_p / 2;
            draw_text(center ? xx_p : x_p, center ? yy_p : y_p, text.second, scale, text.first);
            y_p += h_p;
        }
    }

    void draw_render_circle(const render_circle_t& circle, colo_t colo)
    {
        int radius_p = circle.calc_radius_p();
        int x_center_p = circle.x_p;
        int y_center_p = circle.y_p;
        int circle_x_p = radius_p;
        int circle_y_p = 0;
        int err_p = 0;
        while(circle_x_p >= circle_y_p)
        {
            draw_pixel(x_center_p + circle_x_p, y_center_p + circle_y_p, colo);
            draw_pixel(x_center_p + circle_y_p, y_center_p + circle_x_p, colo);
            draw_pixel(x_center_p - circle_y_p, y_center_p + circle_x_p, colo);
            draw_pixel(x_center_p - circle_x_p, y_center_p + circle_y_p, colo);
            draw_pixel(x_center_p - circle_x_p, y_center_p - circle_y_p, colo);
            draw_pixel(x_center_p - circle_y_p, y_center_p - circle_x_p, colo);
            draw_pixel(x_center_p + circle_y_p, y_center_p - circle_x_p, colo);
            draw_pixel(x_center_p + circle_x_p, y_center_p - circle_y_p, colo);
            circle_y_p++;
            err_p += 1 + 2 * circle_y_p;
            if(err_p > 0)
            {
                circle_x_p--;
                err_p -= 2 * circle_x_p;
            }
        }
    }

    void draw_line(int x0_p, int y0_p, int x1_p, int y1_p, colo_t colo)
    {
        int x_start_p = std::round(x0_p);
        int y_start_p = std::round(y0_p);
        int x_end_p = std::round(x1_p);
        int y_end_p = std::round(y1_p);
        int dx_p = std::abs(x_end_p - x_start_p);
        int dy_p = std::abs(y_end_p - y_start_p);
        int sx_p = (x_start_p < x_end_p) ? 1 : -1;
        int sy_p = (y_start_p < y_end_p) ? 1 : -1;
        int err_p = dx_p - dy_p;
        while(true)
        {
            draw_pixel(x_start_p, y_start_p, colo);
            if(x_start_p == x_end_p && y_start_p == y_end_p)
            {
                break;
            }
            int err2_p = err_p * 2;
            if(err2_p > -dy_p)
            {
                err_p -= dy_p;
                x_start_p += sx_p;
            }
            if(err2_p < dx_p)
            {
                err_p += dx_p;
                y_start_p += sy_p;
            }
        }
    }

    void draw_render_rect(const render_rect_t& rect, colo_t colo)
    {
        draw_line(rect.x0_p, rect.y0_p, rect.x1_p, rect.y0_p, colo);
        draw_line(rect.x1_p, rect.y0_p, rect.x1_p, rect.y1_p, colo);
        draw_line(rect.x1_p, rect.y1_p, rect.x0_p, rect.y1_p, colo);
        draw_line(rect.x0_p, rect.y1_p, rect.x0_p, rect.y0_p, colo);
    }

    void calc_line_endpoints(const render_circle_t& a, const render_circle_t& b, double& theta_r, int& x1_p, int& y1_p, int& x2_p, int& y2_p) const
    {
        theta_r = std::atan2(b.y_p - a.y_p, b.x_p - a.x_p);
        x1_p = a.x_p + a.calc_radius_p() * std::cos(theta_r);
        y1_p = a.y_p + a.calc_radius_p() * std::sin(theta_r);
        x2_p = b.x_p - b.calc_radius_p() * std::cos(theta_r);
        y2_p = b.y_p - b.calc_radius_p() * std::sin(theta_r);
    }

    void draw_arrow_between_points(double theta_r, int x1_p, int y1_p, int x2_p, int y2_p, colo_t colo)
    {
        draw_line(x1_p, y1_p, x2_p, y2_p, colo);
        int arrow_length_p = 15;
        double arrow_theta_r = M_PI / 10.0;
        int arrow_x1_p = x2_p - arrow_length_p * std::cos(theta_r - arrow_theta_r);
        int arrow_y1_p = y2_p - arrow_length_p * std::sin(theta_r - arrow_theta_r);
        int arrow_x2_p = x2_p - arrow_length_p * std::cos(theta_r + arrow_theta_r);
        int arrow_y2_p = y2_p - arrow_length_p * std::sin(theta_r + arrow_theta_r);
        draw_line(x2_p, y2_p, arrow_x1_p, arrow_y1_p, colo);
        draw_line(x2_p, y2_p, arrow_x2_p, arrow_y2_p, colo);
        draw_line(arrow_x1_p, arrow_y1_p, arrow_x2_p, arrow_y2_p, colo);
    }

    void draw_grid()
    {
        for(int x_p = 0; x_p <= xres_p; x_p += ui::grid_size_p) draw_line(x_p, 0, x_p, yres_p, colo_t::grey);
        for(int y_p = 0; y_p <= yres_p; y_p += ui::grid_size_p) draw_line(0, y_p, xres_p, y_p, colo_t::grey);
    }

    void draw_selection_box()
    {
        if(selection_box_is_valid)
        {
            draw_render_rect(selection_box, colo_t::white);
        }
    }

    void draw_node(node_t* node, colo_t colo = colo_t::white)
    {
        render_circle_t circle{
            tile_to_pixel_p(node->x_tile),
            tile_to_pixel_p(node->y_tile),
            ui::grid_size_p
        };
        circle.center();
        draw_render_circle(circle, colo);
        std::vector<colo_text_t> texts = {
            {colo_t::white, double_to_string(node->volume->calc_total_pressure_pa(), 0) + " pa"},
            {colo_t::white, node->volume->name},
            {colo_t::white, double_to_string(node->work_time_ns / 1e6, 4) + " ms"},
            {colo_t::white, double_to_string(node->volume->static_temperature_k, 0) + " k"},
            {colo_t::white, double_to_string(node->children.size(), 0)},
            {colo_t::white, double_to_string(node->volume->max_gas_mail_size, 0)},
        };
        draw_texts(circle.x_p, circle.y_p, texts, ui::node_font_multiplier, true);
    }

    void draw_node_connector(node_t* parent, node_t* child)
    {
        double theta_r = 0.0;
        int x1_p = 0;
        int y1_p = 0;
        int x2_p = 0;
        int y2_p = 0;
        render_circle_t from{
            tile_to_pixel_p(parent->x_tile),
            tile_to_pixel_p(parent->y_tile),
            ui::grid_size_p
        };
        render_circle_t to{
            tile_to_pixel_p(child->x_tile),
            tile_to_pixel_p(child->y_tile),
            ui::grid_size_p
        };
        from.center();
        to.center();
        if(is_pause_mode)
        {
            /* do not change arrow direction so we see directed cyclic graph as is */
        }
        else
        {
            if(child->volume->calc_total_pressure_pa() > parent ->volume->calc_total_pressure_pa())
            {
                std::swap(from, to);
            }
        }
        calc_line_endpoints(from, to, theta_r, x1_p, y1_p, x2_p, y2_p);
        int xm_p = (x1_p + x2_p) / 2;
        int ym_p = (y1_p + y2_p) / 2;
        draw_arrow_between_points(theta_r, x1_p, y1_p, x2_p, y2_p, mix_colos(colo_t::red, colo_t::green, parent->edge_port->open_ratio));
        std::vector<colo_text_t> texts = {
            {colo_t::white, parent->edge_port->name},
            {colo_t::white, double_to_string(parent->edge_port->diameter_m, 3) + " m"},
            {colo_t::white, double_to_string(parent->edge_port->length_m, 3) + " m"},
            {colo_t::white, double_to_string(parent->edge_port->open_ratio, 3) + ""},
        };
        draw_texts(xm_p, ym_p, texts, ui::node_font_multiplier, true);
    }

    void draw_properties(int x_p, int y_p, prop_table_t* prop_table)
    {
        std::vector<colo_text_t> texts;
        for(const prop_t& prop : prop_table->table)
        {
            texts.push_back({colo_t::white, prop.key + " : " + prop.value});
        }
        if(texts.size() > 0)
        {
            texts[append_line].first = is_append_mode ? colo_t::green : colo_t::red;
            draw_texts(x_p, y_p, texts, ui::font_multiplier, false);
        }
    }

    void handle_append_mode(const SDL_Event& event, const SDL_Keymod& mod) const
    {
        auto& sym = event.key.keysym.sym;
        auto& type = event.type;
        if(type == SDL_KEYDOWN)
        {
            if(sym == SDLK_9 && (mod & KMOD_LSHIFT)) /* todo - specify append handler in function name */
            {
                append_on_key_down('(');
            }
            else
            if(sym == SDLK_8 && (mod & KMOD_LSHIFT))
            {
                append_on_key_down('*');
            }
            else
            if(sym == SDLK_5 && (mod & KMOD_LSHIFT))
            {
                append_on_key_down('%');
            }
            else
            if(sym == SDLK_0 && (mod & KMOD_LSHIFT))
            {
                append_on_key_down(')');
            }
            else
            if(sym == SDLK_EQUALS && (mod & KMOD_SHIFT))
            {
                append_on_key_down('+');
            }
            else
            if(sym == SDLK_MINUS && (mod & KMOD_SHIFT))
            {
                append_on_key_down('_');
            }
            else
            if(sym == SDLK_w && (mod & KMOD_CTRL))
            {
                append_on_ctrl_w_key_down();
            }
            else
            if(sym == SDLK_ESCAPE)
            {
                append_on_esc_key_down();
            }
            else
            if(sym == SDLK_BACKSPACE)
            {
                append_on_backspace_key_down();
            }
            else
            if(sym == SDLK_RETURN)
            {
                append_on_return_key_down();
            }
            else
            if(sym == SDLK_PERIOD)
            {
                append_on_key_down('.');
            }
            else
            if(sym == SDLK_SLASH)
            {
                append_on_key_down('/');
            }
            else
            if(sym == SDLK_MINUS)
            {
                append_on_key_down('-');
            }
            else
            if(sym == SDLK_SPACE)
            {
                append_on_key_down(' ');
            }
            else
            if(sym >= SDLK_0 && sym <= SDLK_9)
            {
                append_on_key_down('0' + (sym - SDLK_0));
            }
            else
            if(sym >= SDLK_a && sym <= SDLK_z)
            {
                append_on_key_down('a' + (sym - SDLK_a));
            }
        }
    }

    void handle_edit_mode(const SDL_Event& event, const SDL_Keymod& mod) const
    {
        int x_p = 0;
        int y_p = 0;
        SDL_GetMouseState(&x_p, &y_p);
        auto& sym = event.key.keysym.sym;
        auto& type = event.type;
        auto& button = event.button.button;
        if(type == SDL_MOUSEMOTION) /* todo - specify edit handler in function name */
        {
            if(button == SDL_BUTTON_LEFT)
            {
                edit_on_left_drag(x_p, y_p);
            }
        }
        else
        if(type == SDL_MOUSEBUTTONDOWN)
        {
            if(button == SDL_BUTTON_LEFT)
            {
                if(mod & KMOD_LSHIFT)
                {
                    edit_on_shift_left_click_down(x_p, y_p);
                }
                else
                {
                    edit_on_left_click_down(x_p, y_p);
                }
            }
        }
        else
        if(type == SDL_MOUSEBUTTONUP)
        {
            if(button == SDL_BUTTON_RIGHT)
            {
                edit_on_right_click_up(x_p, y_p);
            }
            else
            if(button == SDL_BUTTON_LEFT)
            {
                edit_on_left_click_up(x_p, y_p);
            }
        }
        else
        if(type == SDL_KEYDOWN)
        {
            if(sym == SDLK_a && (mod & KMOD_CTRL))
            {
                edit_on_ctrl_a_key_down();
            }
            else
            if(sym == SDLK_s && (mod & KMOD_CTRL))
            {
                edit_on_ctrl_s_key_down();
            }
            else
            if(sym == SDLK_l && (mod & KMOD_CTRL))
            {
                edit_on_ctrl_l_key_down();
            }
            else
            if(sym == SDLK_g && (mod & KMOD_LSHIFT))
            {
                edit_on_shift_g_key_down();
            }
            else
            if(sym == SDLK_a && (mod & KMOD_LSHIFT))
            {
                edit_on_shift_a_key_down();
            }
            else
            if(sym == SDLK_1)
            {
                edit_on_1_key_down();
            }
            else
            if(sym == SDLK_2)
            {
                edit_on_2_key_down();
            }
            else
            if(sym == SDLK_3)
            {
                edit_on_3_key_down();
            }
            else
            if(sym == SDLK_4)
            {
                edit_on_4_key_down();
            }
            else
            if(sym == SDLK_b)
            {
                edit_on_b_key_down();
            }
            else
            if(sym == SDLK_f)
            {
                edit_on_f_key_down();
            }
            else
            if(sym == SDLK_g)
            {
                edit_on_g_key_down();
            }
            else
            if(sym == SDLK_p)
            {
                edit_on_p_key_down();
            }
            else
            if(sym == SDLK_n)
            {
                edit_on_n_key_down();
            }
            else
            if(sym == SDLK_i)
            {
                edit_on_i_key_down();
            }
            else
            if(sym == SDLK_h)
            {
                edit_on_h_key_down();
            }
            else
            if(sym == SDLK_j)
            {
                edit_on_j_key_down();
            }
            else
            if(sym == SDLK_k)
            {
                edit_on_k_key_down();
            }
            else
            if(sym == SDLK_q)
            {
                edit_on_q_key_down();
            }
            else
            if(sym == SDLK_DELETE)
            {
                edit_on_delete_key_down();
            }
        }
    }

    void handle_input() const
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            SDL_Keymod mod = SDL_GetModState();
            if(event.type == SDL_QUIT)
            {
                on_exit();
            }
            else
            if(is_append_mode)
            {
                handle_append_mode(event, mod);
            }
            else
            {
                handle_edit_mode(event, mod);
            }
        }
    }

    const std::string& get_running_animation_frame()
    {
        std::string postfix = "";
        if(is_pause_mode == false)
        {
            if(render_ticks % ui::info_render_ticks == 0)
            {
                running_animation_index++;
            }
        }
        int index = running_animation_index % running_animation.size();
        return running_animation[index];
    }

    void draw_running_animation_frame(int x_p, int y_p, double frame_time_ms, double sim_time_ms, bool is_slowmo_mode)
    {
        frame_time_ms = frame_time_ms_smoother.filter(frame_time_ms);
        sim_time_ms = sim_time_ms_smoother.filter(sim_time_ms);
        double audio_time_ms = 1000.0 * sim::cycles_per_frame / sim::sample_frequency_hz;
        std::string frame_time_ms_string = double_to_string(frame_time_ms, 1, 4);
        std::string sim_time_ms_string = double_to_string(sim_time_ms, 1, 4);
        std::string audio_time_ms_string = double_to_string(audio_time_ms, 1, 4);
        std::string audio_queue_size = double_to_string(get_audio_queue_size(), 0, 4);
        std::string frame = get_running_animation_frame();
        std::string text = sim::title + " " + frame + " " + frame_time_ms_string + " + " + sim_time_ms_string + " / " + audio_time_ms_string + " : " + audio_queue_size;
        if(is_slowmo_mode)
        {
            text += " slowmo!";
        }
        colo_t colo = colo_t::white;
        if(frame_time_ms > audio_time_ms)
        {
            colo = red_flash.colo;
        }
        draw_text(x_p, y_p, text, ui::title_font_multiplier, colo);
    }

    void draw_command_message(int x_p, int y_p, const std::string& message)
    {
        draw_text(x_p, y_p, message, ui::font_multiplier, colo_t::white);
    }

    void set_cursor_busy()
    {
        SDL_SetCursor(cursor_wait);
    }

    void set_cursor_arrow()
    {
        SDL_SetCursor(cursor_arrow);
    }

    void set_cursor_size_all()
    {
        SDL_SetCursor(cursor_size_all);
    }

    void draw_plot_values(const plot_values_t& x, const plot_values_t& y, const render_rect_t& rect, colo_t colo)
    {
        int samples = x.values.size();
        for(int i = 0; i < samples; i++)
        {
            int x_p = (0.0 + x.values[i]) * (rect.x1_p - rect.x0_p) + rect.x0_p;
            int y_p = (1.0 - y.values[i]) * (rect.y1_p - rect.y0_p) + rect.y0_p;
            draw_pixel(x_p, y_p, colo);
        }
    }

    void draw_plot(plot_t& plot)
    {
        int margin_p = tile_to_pixel_p(1) / 4;
        int x0 = plot.rect.x0_p + margin_p;
        int y0 = plot.rect.y0_p + margin_p;
        int ym = y0 + (plot.rect.y1_p - plot.rect.y0_p) / 2;
        int channels = plot.front.size();
        double y_max = 0.0;
        double y_average = 0.0;
        double y_min = 0.0;
        for(int channel = 0; channel < channels; channel++)
        {
            plot_channel_t& plot_channel = plot.front[channel];
            y_max += plot_channel.y.max;
            y_average += plot_channel.y.calc_average();
            y_min += plot_channel.y.min;
        }
        if(channels > 0)
        {
            y_max /= channels;
            y_average /= channels;
            y_min /= channels;
        }
        for(int channel = 0; channel < channels; channel++)
        {
            plot_channel_t& plot_channel = plot.front[channel];
            colo_t colo = graph_colos[channel % graph_colos.size()];
            draw_plot_values(plot_channel.x, plot_channel.y, plot.rect, colo);
        }
        std::vector<colo_text_t> texts = {
            {colo_t::white, double_to_string(y_max, plot.precision, plot.width) + " " + plot.y_units},
            {colo_t::white, double_to_string(y_average, plot.precision, plot.width) + " " + plot.y_units},
            {colo_t::white, double_to_string(y_min, plot.precision, plot.width) + " " + plot.y_units},
        };
        draw_texts(x0, ym, texts, ui::node_font_multiplier, false);
        draw_render_rect(plot.rect, colo_t::white);
        draw_text(x0, y0, plot.name, ui::graph_title_font_multiplier, colo_t::white);
    }

    void draw_plot_panel(plot_panel_t& plot_panel)
    {
        for(plot_t& plot : plot_panel.panel)
        {
            draw_plot(plot);
        }
    }

    void draw_help_screen()
    {
        std::vector<colo_text_t> texts = {
            {colo_t::white, "ensim3 - open internal combustion engine simulation research"},
            {colo_t::white,  ""},
            {colo_t::white, "louw, gustav copyright (c) 2023-2025 gplv3 - this software"},
            {colo_t::white, "nigus, hailemariam copyright (c) 2015 cc-by-4.0 - engine load and kinematics"},
            {colo_t::white, "lantinga, sam et. al copyright (c) 1997-2025 zlib - sdl2"},
            {colo_t::white, "sondaar, marcel in public domain - this font"},
            {colo_t::white,  ""},
            {colo_t::white,  "left click + drag : moves nodes"},
            {colo_t::white,  " shift left click : select multiple nodes"},
            {colo_t::white,  "  left click down : select node"},
            {colo_t::white,  "   right click up : create node or connect node"},
            {colo_t::white,  "         ctrl + a : select all nodes"},
            {colo_t::white,  "         ctrl + s : save to disk"},
            {colo_t::white,  "         ctrl + l : load from disk"},
            {colo_t::white,  "        shift + g : go to last property"},
            {colo_t::white,  "        shift + a : enter append mode"},
            {colo_t::white,  "                1 : throttle (almost) closed"},
            {colo_t::white,  "                2 : throttle open somewhat"},
            {colo_t::white,  "                3 : throttle open halfway"},
            {colo_t::white,  "                4 : throttle fully open"},
            {colo_t::white,  "                b : make node simulation-entry-node (node circle color becomes red)"},
            {colo_t::white,  "                f : toggle slo-mo mode"},
            {colo_t::white,  "                g : go to first property"},
            {colo_t::white,  "                p : toggle pause mode"},
            {colo_t::white,  "                n : normalize all nodes to standard pressure and temperature conditions"},
            {colo_t::white,  "                i : enter append mode"},
            {colo_t::white,  "                j : go to next property"},
            {colo_t::yellow, "                h : this help screen"}, /* highlighted, so the user knows where they are */
            {colo_t::white,  "                k : go to previous property"},
            {colo_t::white,  "                q : demo breadth first graph execution"},
            {colo_t::white,  "         ctwl + w : delete a line"},
            {colo_t::white,  "           escape : exit append mode"},
            {colo_t::white,  "           return : exit append mode"},
        };
        draw_texts(tile_to_pixel_p(0.5), tile_to_pixel_p(0.5), texts, ui::help_font_multiplier, false);
    }
};
