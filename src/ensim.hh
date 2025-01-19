#pragma once

struct ensim_t
{
    std::string filename = "test.ensim3";
    std::string command_message = "";
    int cycle = 0;
    int cycles_per_frame = sim::cycles_per_frame;
    bool is_slowmo_mode = false;
    bool is_done = false;
    int x_tiles = 38;
    int y_tiles = 20;
    int plot_panel_tiles = 7;
    sdl_t sdl{tile_to_pixel_p(x_tiles), tile_to_pixel_p(y_tiles)};
    plot_panel_t plot_panel{tile_to_pixel_p(x_tiles - plot_panel_tiles), sdl.yres_p, tile_to_pixel_p(plot_panel_tiles)};
    crankshaft_t crankshaft;
    camshaft_t camshaft{crankshaft};
    audio_processor_t audio_processor{crankshaft}; /* todo: collect audio from multiple collectors (like dual exhaust) */
    flywheel_t flywheel;
    starter_motor_t starter_motor{crankshaft, flywheel};
    throttle_cable_t throttle_cable;
    std::vector<piston_t*> pistons;
    std::vector<injector_t*> injectors;
    std::vector<rotational_mass_t*> rotational_masses = {&crankshaft, &camshaft, &flywheel, &starter_motor};
    std::vector<throttle_port_t*> throttle_ports;
    node_table_t node_table{x_tiles, y_tiles, pistons, injectors, rotational_masses, throttle_ports};
    node_t* graph = nullptr;
    node_t* select = nullptr;

    ensim_t()
    {
        sdl.play_audio();
        setup_input_handlers();
        int x_tile = x_tiles - 8;
        int y_tile = y_tiles / 2;
        std::unique_ptr<node_t> source = make_node(x_tile, y_tile, "source");
        graph = source.get();
        select = graph;
        node_table.create_node(x_tile, y_tile, std::move(source));
        load_nodes_from_disk();
    }

    void move_selected_nodes_to(int dx_tile, int dy_tile)
    {
        node_table.iterate(
            [&table = node_table, dx_tile, dy_tile](std::unique_ptr<node_t>& node)
            {
                if(node->is_selected)
                {
                    int x_tile = node->x_tile + dx_tile;
                    int y_tile = node->y_tile + dy_tile;
                    if(node_t* exists = table.get(x_tile, y_tile))
                    {
                        return;
                    }
                    if(node->was_moved)
                    {
                        /* do not move again */
                    }
                    else
                    {
                        node->was_moved = true;
                        table.move(node, x_tile, y_tile);
                    }
                }
            }
        );
        node_table.iterate(
            [](node_t* node)
            {
                node->was_moved = false;
            }
        );
    }

    void select_all_nodes()
    {
        node_table.iterate(
            [](node_t* node)
            {
                node->is_selected = true;
            }
        );
    }

    void deselect_all_nodes()
    {
        node_table.iterate(
            [](node_t* node)
            {
                node->is_selected = false;
            }
        );
        select = nullptr;
    }

    node_t* select_node_at(int x_tile, int y_tile)
    {
        if(node_t* exists = node_table.get(x_tile, y_tile))
        {
            exists->is_selected = true;
            return exists;
        }
        return nullptr;
    }

    void select_nodes_in(const render_rect_t& render_rect)
    {
        node_table.iterate(
            [&render_rect](node_t* node)
            {
                int x_p = tile_to_pixel_p(node->x_tile);
                int y_p = tile_to_pixel_p(node->y_tile);
                if(render_rect.is_point_inside(x_p, y_p))
                {
                    node->is_selected = true;
                }
            }
        );
    }

    prop_table_t* get_selected_prop_table()
    {
        if(select)
        {
            return &select->prop_table;
        }
        return nullptr;
    }

    void add_child_to_selected(node_t* child)
    {
        node_table.iterate(
            [child](node_t* node)
            {
                if(node->is_selected)
                {
                    node->add_child(child);
                }
            }
        );
    }

    void selected_prop_table_operate(std::function<void(prop_table_t*)> operate)
    {
        if(prop_table_t* selected_prop_table = get_selected_prop_table())
        {
            int size = selected_prop_table->table.size();
            sdl.append_line = std::clamp(sdl.append_line, 0, size - 1);
            operate(selected_prop_table);
        }
    }

    void save_nodes_to_disk()
    {
        int uid = 0;
        std::unordered_map<node_t*, int> node_to_uid;
        std::ofstream file{filename};
        if(file.is_open())
        {
            graph->iterate(
                [&file, &uid, &node_to_uid](node_t* parent)
                {
                    file
                        << "make" << ":"
                        << std::to_string(uid) << ":"
                        << std::to_string(parent->x_tile) << ":"
                        << std::to_string(parent->y_tile) << ":"
                        << parent->volume->name << ":";
                    for(const prop_t& prop : parent->prop_table.table)
                    {
                        file << prop.key << "=" << prop.value << ",";
                    }
                    file << "\n";
                    node_to_uid[parent] = uid++;
                    return false;
                });
            graph->iterate(
                [&file, &node_to_uid](node_t* parent, node_t* child)
                {
                    file
                        << "join" << ":"
                        << std::to_string(node_to_uid[parent]) << ":"
                        << std::to_string(node_to_uid[child]) << "\n";
                    return false;
                });
            file.close();
            command_message = "saved to " + filename;
        }
    }

    void unpack_props(const std::string& props, node_t* node)
    {
        std::string pair = "";
        std::stringstream stream{props};
        while(std::getline(stream, pair, ','))
        {
            size_t pos = pair.find('=');
            if(pos != std::string::npos)
            {
                std::string key = pair.substr(0, pos);
                std::string value = pair.substr(pos + 1);
                node->prop_table.set_prop(key, value);
            }
        }
    }

    void load_nodes_from_disk()
    {
        node_table.clear();
        std::unordered_map<int, node_t*> uid_to_node;
        std::ifstream file{filename};
        if(file.is_open())
        {
            std::string line = "";
            while(std::getline(file, line))
            {
                std::vector<std::string> tokens;
                std::stringstream stream{line};
                std::string token = "";
                while(std::getline(stream, token, ':'))
                {
                    tokens.push_back(token);
                }
                const std::string& command = tokens[0];
                if(command == "make")
                {
                    int uid = std::stoi(tokens[1]);
                    int x_tile = std::stoi(tokens[2]);
                    int y_tile = std::stoi(tokens[3]);
                    const std::string& name = tokens[4];
                    const std::string& props = tokens[5];
                    std::unique_ptr<node_t> node = make_node(x_tile, y_tile, name);
                    unpack_props(props, node.get());
                    uid_to_node[uid] = node.get();
                    node_table.create_node(x_tile, y_tile, std::move(node));
                }
                else
                if(command == "join")
                {
                    int parent_uid = std::stoi(tokens[1]);
                    int child_uid = std::stoi(tokens[2]);
                    node_t* parent = uid_to_node[parent_uid];
                    node_t* child = uid_to_node[child_uid];
                    parent->add_child(child);
                }
            }
            command_message = "loaded " + filename;
        }
        graph = uid_to_node[0];
        select = graph;
    }

    void setup_input_handlers()
    {
        sdl.on_exit =
            [this]()
            {
                is_done = true;
            };

        /* edit mode handlers */

        sdl.edit_on_left_drag =
            [this](int x_p, int y_p)
            {
                int x_tile = pixel_to_tile(x_p);
                int y_tile = pixel_to_tile(y_p);
                if(node_table.in_bounds(x_tile, y_tile))
                {
                    node_t* node = select;
                    if(node)
                    {
                        int dx_tile = x_tile - node->x_tile;
                        int dy_tile = y_tile - node->y_tile;
                        move_selected_nodes_to(dx_tile, dy_tile);
                    }
                    else
                    {
                        sdl.selection_box_is_valid = true;
                        sdl.selection_box.x1_p = x_p;
                        sdl.selection_box.y1_p = y_p;
                    }
                }
            };

        sdl.edit_on_shift_left_click_down =
            [this](int x_p, int y_p)
            {
                int x_tile = pixel_to_tile(x_p);
                int y_tile = pixel_to_tile(y_p);
                if(node_t* node = select_node_at(x_tile, y_tile))
                {
                    select = node;
                }
                else
                {
                    deselect_all_nodes();
                    sdl.selection_box_is_valid = false;
                    sdl.selection_box.x0_p = x_p;
                    sdl.selection_box.y0_p = y_p;
                }
            };

        sdl.edit_on_left_click_down =
            [this](int x_p, int y_p)
            {
                deselect_all_nodes();
                sdl.edit_on_shift_left_click_down(x_p, y_p);
            };

        sdl.edit_on_right_click_up =
            [this](int x_p, int y_p)
            {
                int x_tile = pixel_to_tile(x_p);
                int y_tile = pixel_to_tile(y_p);
                node_t* child = node_table.get(x_tile, y_tile);
                if(child)
                {
                    add_child_to_selected(child);
                }
                else
                {
                    deselect_all_nodes();
                    std::unique_ptr<node_t> node = make_node(x_tile, y_tile, "volume");
                    node->is_selected = true;
                    node_table.create_node(x_tile, y_tile, std::move(node));
                }
            };

        sdl.edit_on_left_click_up =
            [this](int x_p, int y_p)
            {
                if(sdl.selection_box_is_valid)
                {
                    sdl.selection_box.x1_p = x_p;
                    sdl.selection_box.y1_p = y_p;
                    select_nodes_in(sdl.selection_box);
                    sdl.selection_box_is_valid = false;
                    sdl.selection_box.clear();
                }
            };

        sdl.edit_on_1_key_down =
            [this]()
            {
                throttle_cable.pull_ratio_setpoint = 0.10;
            };

        sdl.edit_on_2_key_down =
            [this]()
            {
                throttle_cable.pull_ratio_setpoint = 0.33;
            };

        sdl.edit_on_3_key_down =
            [this]()
            {
                throttle_cable.pull_ratio_setpoint = 0.66;
            };

        sdl.edit_on_4_key_down =
            [this]()
            {
                throttle_cable.pull_ratio_setpoint = 0.99;
            };

        sdl.edit_on_ctrl_a_key_down =
            [this]()
            {
                select_all_nodes();
            };

        sdl.edit_on_ctrl_s_key_down =
            [this]()
            {
                save_nodes_to_disk();
            };

        sdl.edit_on_ctrl_l_key_down =
            [this]()
            {
                load_nodes_from_disk();
            };

        sdl.edit_on_shift_g_key_down =
            [this]()
            {
                selected_prop_table_operate(
                    [this](prop_table_t* prop_table)
                    {
                        int size = prop_table->table.size();
                        sdl.append_line = size - 1;
                    }
                );
            };

        sdl.edit_on_shift_a_key_down =
            [this]()
            {
                sdl.is_append_mode = true;
                sdl.is_pause_mode = true;
                command_message = "-- APPEND --";
            };

        sdl.edit_on_b_key_down =
            [this]()
            {
                if(select)
                {
                    graph = select;
                }
            };

        sdl.edit_on_f_key_down =
            [this]()
            {
                if(is_slowmo_mode)
                {
                    command_message = "entered regular motion mode";
                    is_slowmo_mode = false;
                    cycles_per_frame = sim::cycles_per_frame;
                }
                else
                {
                    command_message = "entered slow motion mode";
                    is_slowmo_mode = true;
                    cycles_per_frame = 1;
                }
            };

        sdl.edit_on_g_key_down =
            [this]()
            {
                selected_prop_table_operate(
                    [this](prop_table_t*)
                    {
                        sdl.append_line = 0;
                    }
                );
            };

        sdl.edit_on_p_key_down =
            [this]()
            {
                if(sdl.is_pause_mode)
                {
                    sdl.is_pause_mode = false;
                    command_message = "played simulation";
                }
                else
                {
                    sdl.is_pause_mode = true;
                    command_message = "paused simulation";
                }
            };

        sdl.edit_on_n_key_down =
            [this]()
            {
                int count = normalize_selected_nodes();
                command_message = "normalized " + double_to_string(count, 0) + " node(s)";
            };

        sdl.edit_on_i_key_down =
            [this]()
            {
                sdl.edit_on_shift_a_key_down();
            };

        sdl.edit_on_h_key_down =
            [this]()
            {
                if(sdl.is_help_mode)
                {
                    sdl.is_help_mode = false;
                }
                else
                {
                    sdl.is_help_mode = true;
                }
            };

        sdl.edit_on_j_key_down =
            [this]()
            {
                sdl.append_line++;
                selected_prop_table_operate(
                    [](prop_table_t*)
                    {
                        /* clamp */
                    }
                );
            };

        sdl.edit_on_k_key_down =
            [this]()
            {
                sdl.append_line--;
                selected_prop_table_operate(
                    [](prop_table_t*)
                    {
                        /* clamp */
                    }
                );
            };

        sdl.edit_on_q_key_down =
            [this]()
            {
                draw_execution_flow_demo();
                command_message = "drew execution flow";
            };

        sdl.edit_on_delete_key_down =
            [this]()
            {
                int deleted = delete_selected_nodes();
                command_message = "deleted " + double_to_string(deleted, 0) + " node(s)";
            };

        /* append mode handlers */

        sdl.append_on_ctrl_w_key_down =
            [this]()
            {
                selected_prop_table_operate(
                    [this](prop_table_t* prop_table)
                    {
                        prop_table->at(sdl.append_line)->value.clear();
                    }
                );
            };

        sdl.append_on_esc_key_down =
            [this]()
            {
                sdl.append_on_return_key_down();
            };

        sdl.append_on_backspace_key_down =
            [this]()
            {
                selected_prop_table_operate(
                    [this](prop_table_t* prop_table)
                    {
                        prop_t* prop = prop_table->at(sdl.append_line);
                        if(prop->value.empty() == false)
                        {
                            prop->value.pop_back();
                        }
                    }
                );
            };

        sdl.append_on_return_key_down =
            [this]()
            {
                selected_prop_table_operate(
                    [this](prop_table_t* prop_table)
                    {
                        prop_t* prop = prop_table->at(sdl.append_line);
                        if(prop->key == ui::volume_key)
                        {
                            if(select)
                            {
                                int x_tile = select->x_tile;
                                int y_tile = select->y_tile;
                                const std::string& name = prop->value;
                                node_table.polymorph(select, make_node(x_tile, y_tile, name));
                            }
                        }
                        else
                        {
                            prop->commit(
                                [prop_table](const std::string& key)
                                {
                                    if(std::optional<prop_t::real_t> real = prop_table->find(key))
                                    {
                                        if(std::holds_alternative<double*>(*real))
                                        {
                                            return *std::get<double*>(*real);
                                        }
                                    }
                                    return 0.0;
                                }
                            );
                        }
                        sdl.is_append_mode = false;
                    }
                );
            };

        sdl.append_on_key_down =
            [this](char character)
            {
                selected_prop_table_operate(
                    [this, character](prop_table_t* prop_table)
                    {
                        prop_table->at(sdl.append_line)->value += character;
                    }
                );
            };
    }

    std::unique_ptr<node_t> make_node(int x_tile, int y_tile, const std::string& name)
    {
        std::unique_ptr<volume_t> volume;
        std::unique_ptr<port_t> port;
        if(name == "source")
        {
            volume = std::make_unique<source_t>();
            port = std::make_unique<port_t>();
        }
        else
        if(name == "throttle")
        {
            volume = std::make_unique<throttle_t>(pistons, injectors, crankshaft);
            port = std::make_unique<throttle_port_t>(throttle_cable);
        }
        else
        if(name == "injector")
        {
            volume = std::make_unique<injector_t>(throttle_cable);
            port = std::make_unique<actuated_port_t>(camshaft, 0.0 * M_PI, M_PI);
        }
        else
        if(name == "piston")
        {
            volume = std::make_unique<piston_t>(camshaft);
            port = std::make_unique<actuated_port_t>(camshaft, 3.0 * M_PI, M_PI);
        }
        else
        if(name == "exhaust")
        {
            volume = std::make_unique<exhaust_t>(audio_processor, crankshaft);
            port = std::make_unique<port_t>();
        }
        else
        if(name == "sink")
        {
            volume = std::make_unique<sink_t>();
            port = std::make_unique<port_t>();
        }
        else
        {
            volume = std::make_unique<volume_t>();
            port = std::make_unique<port_t>();
        }
        std::unique_ptr<node_t> node = std::make_unique<node_t>(x_tile, y_tile, std::move(volume), std::move(port));
        node->prop_table
            = node->prop_table
            + crankshaft.get_prop_table()
            + camshaft.get_prop_table()
            + flywheel.get_prop_table()
            + throttle_cable.get_prop_table()
            + starter_motor.get_prop_table()
            + audio_processor.get_prop_table();
        return node;
    }

    int count_selected_nodes()
    {
        int count = 0;
        node_table.iterate(
            [&count](node_t* node)
            {
                if(node->is_selected)
                {
                    count++;
                }
            }
        );
        return count;
    }

    int delete_selected_nodes()
    {
        int count = 0;
        node_table.iterate(
            [this, &count](std::unique_ptr<node_t>& node)
            {
                if(node.get() != graph)
                {
                    if(node.get() == select)
                    {
                        select = nullptr;
                    }
                    if(node->is_selected)
                    {
                        count++;
                        node_table.delete_node(node);
                    }
                }
            }
        );
        return count;
    }

    int normalize_selected_nodes()
    {
        int count = 0;
        node_table.iterate(
            [&count](node_t* node)
            {
                if(node->is_selected)
                {
                    node->volume->normalize();
                    count++;
                }
            }
        );
        return count;
    }

    void normalize_all_nodes()
    {
        node_table.iterate(
            [](node_t* node)
            {
                node->volume->normalize();
            }
        );
    }

    void sync_all_nodes()
    {
        node_table.iterate(
            [](node_t* node)
            {
                node->prop_table.sync();
            }
        );
    }

    void reset_all_nodes_work_time()
    {
        node_table.iterate(
            [](node_t* node)
            {
                node->work_time_ns = 0.0;
            }
        );
    }

    double calc_moment_of_inertia_kg_per_m2()
    {
        double moment_of_inertia_kg_per_m2 = 0.0;
        for(rotational_mass_t* mass : rotational_masses)
        {
            moment_of_inertia_kg_per_m2 += mass->calc_moment_of_inertia_kg_per_m2();
        }
        return moment_of_inertia_kg_per_m2;
    }

    double calc_applied_torque_n_m()
    {
        double applied_torque_n_m = 0.0;
        for(rotational_mass_t* mass : rotational_masses)
        {
            applied_torque_n_m += mass->calc_applied_torque_n_m();
        }
        return applied_torque_n_m;
    }

    double calc_friction_torque_n_m()
    {
        double friction_torque_n_m = 0.0;
        for(rotational_mass_t* mass : rotational_masses)
        {
            friction_torque_n_m += mass->calc_friction_torque_n_m();
        }
        return friction_torque_n_m;
    }

    void run_sim_once()
    {
        throttle_cable.apply();
        double moment_of_inertia_kg_per_m2 = calc_moment_of_inertia_kg_per_m2();
        double applied_torque_n_m = calc_applied_torque_n_m();
        double friction_torque_n_m = calc_friction_torque_n_m();
        double torque_n_m = applied_torque_n_m - friction_torque_n_m;
        double angular_acceleration_r_per_s = torque_n_m / moment_of_inertia_kg_per_m2;
        crankshaft.accelerate(angular_acceleration_r_per_s);
        int channels = count_selected_nodes();
        plot_panel.set_channels(channels);
        int channel = 0;
        graph->iterate(
            [this, &channel](node_t* parent)
            {
                parent->volume->do_work();
                parent->volume->compress();
                parent->volume->autoignite();
                parent->volume->ignite();
                parent->volume->read_mail(cycle);
                parent->edge_port->open();
                if(parent->is_selected)
                {
                    if(crankshaft.turned())
                    {
                        std::vector<double> datum = parent->volume->get_plot_datum();
                        datum[panel_port_open_ratio] = parent->edge_port->open_ratio;
                        datum[panel_port_flow_velocity] = parent->edge_port->flow_velocity_m_per_s.get();
                        plot_panel.buffer(channel++, crankshaft.theta_r, datum);
                    }
                }
                if(crankshaft.finished_rotation())
                {
                    parent->volume->mol_balance = 0.0;
                }
                return false;
            },
            [this](node_t* parent, node_t* child)
            {
                /* convention defines edge_port is always parent edge port regardless of flow direction */
                /* todo: 1dcfd will remove this convention - each volume will have input and output port */
                parent->volume->edge_port = parent->edge_port.get();
                child->volume->edge_port = parent->edge_port.get();
                double delta_total_pressure_pa = parent->volume->calc_total_pressure_pa() - child->volume->calc_total_pressure_pa();
                if(std::abs(delta_total_pressure_pa) > parent->edge_port->flow_threshold_pressure_pa)
                {
                    if(delta_total_pressure_pa > 0.0)
                    {
                        parent->volume->send_mail(*child->volume.get(), cycle);
                    }
                    else
                    {
                        child->volume->send_mail(*parent->volume.get(), cycle);
                    }
                }
                return false;
            }
        );
        if(crankshaft.finished_rotation())
        {
            plot_panel.flip();
        }
        cycle++;
    }

    void run_sim()
    {
        for(int i = 0; i < cycles_per_frame; i++)
        {
            try
            {
                run_sim_once();
            }
            catch(...)
            {
                normalize_all_nodes();
                command_message = "simulation reset !!! nan encountered !!!";
            }
        }
        if(is_slowmo_mode == false)
        {
            sdl.queue_audio(audio_processor.buffer);
        }
        audio_processor.buffer.clear();
    }

    void draw_execution_flow_demo()
    {
        sdl.is_pause_mode = true;
        int frame_time_s = 200;
        sdl.lock();
        sdl.clear();
        sdl.draw_grid();
        sdl.unlock();
        sdl.flip();
        sdl.delay(frame_time_s);
        graph->iterate(
            [this, frame_time_s](node_t* parent)
            {
                sdl.lock();
                sdl.draw_node(parent, colo_t::white);
                sdl.unlock();
                sdl.flip();
                sdl.delay(frame_time_s);
                return false;
            },
            [this, frame_time_s](node_t* parent, node_t* child)
            {
                sdl.lock();
                sdl.draw_node_connector(parent, child);
                sdl.unlock();
                sdl.flip();
                sdl.delay(frame_time_s);
            }
        );
        int watch_time_s = 3 * frame_time_s;
        sdl.delay(watch_time_s);
        sdl.is_pause_mode = false;
    }

    void render_help_screen()
    {
        sdl.lock();
        sdl.clear();
        sdl.draw_grid();
        sdl.draw_help_screen();
        sdl.unlock();
        sdl.flip();
    }

    void render_ui(double frame_time_ms, double sim_time_ms)
    {
        int x_margin_p = tile_to_pixel_p(0.5); /* todo: top level DSL for GUI coords */
        int y_margin_p = tile_to_pixel_p(0.5);
        sdl.lock();
        sdl.clear();
        sdl.draw_grid();
        node_table.iterate(
            [this](node_t* parent)
            {
                sdl.draw_node(parent, parent->is_selected ? colo_t::white : colo_t::blue);
                for(node_t* child : parent->children)
                {
                    sdl.draw_node_connector(parent, child);
                }
            }
        );
        selected_prop_table_operate(
            [this](prop_table_t* prop_table)
            {
                sdl.draw_properties(tile_to_pixel_p(0.5), tile_to_pixel_p(1.5), prop_table);
            }
        );
        if(select)
        {
            sdl.draw_node(select, select->is_selected ? colo_t::white : colo_t::blue);
        }
        sdl.draw_node(graph, graph->is_selected ? colo_t::white : colo_t::red);
        sdl.draw_selection_box();
        sdl.draw_running_animation_frame(x_margin_p, y_margin_p, frame_time_ms, sim_time_ms, is_slowmo_mode); /* todo: put filename in title (it will eventually be from command line) */
        sdl.draw_command_message(x_margin_p, sdl.yres_p - tile_to_pixel_p(1), command_message);
        sdl.draw_plot_panel(plot_panel);
        sdl.draw_pistons(tile_to_pixel_p(x_tiles - 9), tile_to_pixel_p(y_tiles - 2), pistons);
        sdl.draw_throttle_ports(tile_to_pixel_p(x_tiles - 9), tile_to_pixel_p(1), throttle_ports);
        sdl.unlock();
        sdl.flip();
        sdl.render_ticks++;
        sdl.red_flash.tick(sdl.render_ticks);
    }

    void run()
    {
        double frame_time_ms = 0;
        run_sim_once();
        int frames [[maybe_unused]] = 0;
        while(!is_done)
        {
            auto t0 = std::chrono::high_resolution_clock::now();
            sdl.handle_input();
            if(sdl.is_pause_mode == false)
            {
                reset_all_nodes_work_time();
                sync_all_nodes();
                run_sim();
            }
            auto t1 = std::chrono::high_resolution_clock::now();
            double sim_time_ms = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() / 1e6;
            if(sdl.is_help_mode)
            {
                render_help_screen();
            }
            else
            {
                render_ui(frame_time_ms, sim_time_ms);
            }
            auto t2 = std::chrono::high_resolution_clock::now();
            frame_time_ms = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count() / 1e6;
            sdl.p_controller_delay(frame_time_ms + sim_time_ms);
#ifdef PERF
            frames++;
            if(frames == 120) /* todo: this is a magic number */
            {
                break;
            }
#endif
        }
    }
};
