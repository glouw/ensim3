#pragma once

struct volume_t
: flowing_gas_t
, observable_t
{
    std::string name = "";
    double diameter_m = 0.0;
    double depth_m = 0.0;
    std::priority_queue<gas_parcel_t, std::vector<gas_parcel_t>, std::greater<>> gas_mail;
    int max_gas_mail_size = 0;
    port_t* edge_port = nullptr;

    volume_t(const std::string& name, double diameter_m, double depth_m)
        : name{name}
        , diameter_m{diameter_m}
        , depth_m{depth_m}
        {
            normalize();
        }

    volume_t(const std::string& name)
        : volume_t{name, 0.05, 0.1}
        {
        }

    volume_t()
        : volume_t{"volume"}
        {
        }

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {ui::volume_key, &name},
            {"volume_diameter_m", &diameter_m},
            {"volume_depth_m", &depth_m},
        };
        return prop_table + gas_t::get_prop_table();
    }

    double calc_volume_m3() const override
    {
        return calc_cylinder_volume_m3(diameter_m, depth_m);
    }

    virtual double calc_displacement_volume_m3() const
    {
        return calc_volume_m3();
    }

    double calc_actual_volume_m3() const
    {
        return mol_balance * thermofluidics::r_j_per_mol_k * static_temperature_k / calc_static_pressure_pa();
    }

    double calc_volumetric_efficiency() const
    {
        return calc_actual_volume_m3() / calc_displacement_volume_m3();
    }

    double calc_flow_area_m2() const override
    {
        return edge_port->calc_flow_area_m2();
    }

    double calc_flow_length_m() const override
    {
        return edge_port->length_m;
    }

    void receive_mail(const gas_parcel_t& parcel)
    {
        gas_mail.push(parcel);
        int size = gas_mail.size();
        max_gas_mail_size = std::max(max_gas_mail_size, size);
    }

    void read_mail(int cycle)
    {
        while(!gas_mail.empty())
        {
            gas_parcel_t mail = gas_mail.top();
            if(cycle < mail.arrival_cycle)
            {
                break;
            }
            gas_mail.pop();
            mix_in(mail);
        }
    }

    virtual void tag_mail(gas_parcel_t&, const volume_t&)
    {
    }

    void send_mail(volume_t& destination, int cycle)
    {
        double mach_number = calc_mach_number(destination);
        if(std::optional<gas_parcel_t> parcel = package_gas_parcel(mach_number, cycle))
        {
            add_moles_adiabatically(-parcel->moles);
            add_momentum(-parcel->bulk_momentum_kg_m_per_s);
            tag_mail(*parcel, destination);
            destination.receive_mail(*parcel); /* todo - give self mail 2x travel cycles to simulate back pressure wave */
            edge_port->flow_velocity_m_per_s.set(parcel->velocity_m_per_s);
        }
        else
        {
            edge_port->flow_velocity_m_per_s.set(0.0);
        }
    }

    virtual std::vector<double> get_plot_datum()
    {
        std::vector<double> datum;
        datum.resize(panel_size, 0.0);
        datum[panel_volume] = calc_volume_m3();
        datum[panel_volumetric_efficiency] = calc_volumetric_efficiency();
        datum[panel_total_pressure] = calc_total_pressure_pa();
        datum[panel_static_temperature] = static_temperature_k;
        datum[panel_gamma] = calc_gamma();
        datum[panel_molar_mass] = calc_molar_mass_kg_per_mol();
        datum[panel_air_fuel_mass_ratio] = calc_air_fuel_mass_ratio();
        return datum;
    }

    virtual void compress()
    {
    }

    virtual void do_work()
    {
    }

    virtual void ignite()
    {
    }

    virtual void autoignite()
    {
    }
};

struct source_t
: volume_t
{
    source_t()
        : volume_t{"source", 1000.0, 1000.0}
        {
        }
};

struct injector_t
: volume_t
{
    bool is_enabled = true;
    throttle_cable_t& throttle_cable;
    pid_controller_t pid_controller{0.1, 0.0, 0.1, 1e6};
    double air_fuel_mass_ratio_setpoint = 14.7;

    injector_t(throttle_cable_t& throttle_cable)
        : volume_t{"injector", 0.12, 0.12}
        , throttle_cable{throttle_cable}
        {
        }

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {"injector_is_enabled", &is_enabled},
            {"injector_injector_controller_kp", &pid_controller.kp},
            {"injector_injector_controller_ki", &pid_controller.ki},
            {"injector_injector_controller_kd", &pid_controller.kd},
            {"injector_air_fuel_mass_ratio_setpoint", &air_fuel_mass_ratio_setpoint},
        };
        return volume_t::get_prop_table() + prop_table;
    }

    void on_create(std::vector<injector_t*>& observers) override
    {
        subscribe_to(observers, this);
    }

    void on_delete(std::vector<injector_t*>& observers) override
    {
        delete_from(observers, this);
    }

    void tag_mail(gas_parcel_t& parcel, const volume_t& to) override
    {
        if(is_enabled)
        {
            double signal = pid_controller.compute(air_fuel_mass_ratio_setpoint, to.calc_air_fuel_mass_ratio());
            if(signal > 0.0)
            {
                /* let more air in */
            }
            else
            {
                double fuel_moles = std::abs(signal);
                parcel.add_fuel_moles(fuel_moles);
            }
        }
    }
};

/* ------- + block_deck_surface_m
 *         | head_clearance_height_m
 * ------- +
 * |     | | head_compression_height_m
 * |  o  | + pin_x_m, pin_y_m
 * |     | |
 * |-----| |
 *   | |   |
 *   | |   | connecting_rod_length_m
 *   | |   |
 *   | |   |
 *   | |   |
 *   |o|   + bearing_x_m, bearing_y_m
 *    |    |
 *    |    | crank_throw_length_m
 *    |    |
 *    o    + origin
 */

struct piston_t
: volume_t
, rotational_mass_t
{
    double pin_x_m = 0.0;
    double pin_y_m = 0.0;
    double bearing_x_m = 0.0;
    double bearing_y_m = 0.0;
    double crankshaft_offset_theta_r = 0.0;
    double crank_throw_length_m = 0.04;
    double connecting_rod_length_m = 0.13;
    double connecting_rod_mass_kg = 0.5;
    cached_t<double> head_mass_kg = 0.0;
    double head_mass_density_kg_per_m3 = 2700.0;
    double head_compression_height_m = 0.025;
    double head_clearance_height_m = 0.01;
    double friction_coefficient = 0.001;
    camshaft_t& camshaft;
    sparkplug_t sparkplug{camshaft};
    flame_t ignition_flame;
    flame_t autoignition_flame;

    piston_t(camshaft_t& camshaft)
        : volume_t{"piston"}
        , camshaft{camshaft}
        {
            rig();
            normalize();
        }

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {"piston_crankshaft_offset_theta_r", &crankshaft_offset_theta_r},
            {"piston_crank_throw_length_m", &crank_throw_length_m},
            {"piston_connecting_rod_length_m", &connecting_rod_length_m},
            {"piston_connecting_rod_mass_kg", &connecting_rod_mass_kg},
            {"piston_head_mass_kg", &head_mass_kg.get()},
            {"piston_head_density_kg_per_m3", &head_mass_density_kg_per_m3},
            {"piston_head_compression_height_m", &head_compression_height_m},
            {"piston_head_clearance_height_m", &head_clearance_height_m},
            {"piston_friction_coefficient", &friction_coefficient},
        };
        return volume_t::get_prop_table() + prop_table + sparkplug.get_prop_table();
    }

    void on_create(std::vector<piston_t*>& observers) override
    {
        subscribe_to(observers, this);
    }

    void on_delete(std::vector<piston_t*>& observers) override
    {
        delete_from(observers, this);
    }

    void on_create(std::vector<rotational_mass_t*>& observers) override
    {
        subscribe_to(observers, static_cast<rotational_mass_t*>(this));
    }

    void on_delete(std::vector<rotational_mass_t*>& observers) override
    {
        delete_from(observers, static_cast<rotational_mass_t*>(this));
    }

    double calc_theta_r() const
    {
        return camshaft.crankshaft.theta_r - crankshaft_offset_theta_r;
    }

    double calc_top_dead_center_m() const
    {
        return connecting_rod_length_m + crank_throw_length_m + head_compression_height_m;
    }

    double calc_bot_dead_center_m() const
    {
        return connecting_rod_length_m - crank_throw_length_m + head_compression_height_m;
    }

    double calc_block_deck_surface_m() const
    {
        return calc_top_dead_center_m() + head_clearance_height_m;
    }

    double calc_chamber_depth_m(double at_pin_y_m) const
    {
        return calc_block_deck_surface_m() - (at_pin_y_m + head_compression_height_m);
    }

    double calc_chamber_depth_m() const
    {
        return calc_chamber_depth_m(pin_y_m);
    }

    double calc_compression_volume_m3() const
    {
        return calc_cylinder_volume_m3(diameter_m, calc_chamber_depth_m(connecting_rod_length_m + crank_throw_length_m));
    }

    double calc_displacement_volume_m3() const override
    {
        return calc_cylinder_volume_m3(diameter_m, calc_chamber_depth_m(connecting_rod_length_m - crank_throw_length_m));
    }

    double calc_gas_torque_n_m() const
    {
        double term1 = calc_static_gauge_pressure_pa() * calc_circle_area_m2(diameter_m) * crank_throw_length_m * std::sin(calc_theta_r());
        double term2 = 1.0 + (crank_throw_length_m / connecting_rod_length_m) * std::cos(calc_theta_r());
        double gas_torque_n_m = term1 * term2;
        return gas_torque_n_m;
    }

    double calc_inertia_torque_n_m() const
    {
        double term1 = 0.25 * std::sin(1.0 * calc_theta_r()) * crank_throw_length_m / connecting_rod_length_m;
        double term2 = 0.50 * std::sin(2.0 * calc_theta_r());
        double term3 = 0.75 * std::sin(3.0 * calc_theta_r()) * crank_throw_length_m / connecting_rod_length_m;
        return calc_moment_of_inertia_kg_per_m2() * std::pow(camshaft.crankshaft.angular_velocity_r_per_s, 2.0) * (term1 - term2 - term3);
    }

    double calc_applied_torque_n_m() const override
    {
        return calc_gas_torque_n_m() + calc_inertia_torque_n_m();
    }

    double calc_head_mass_kg() const
    {
        return head_mass_density_kg_per_m3 * calc_cylinder_volume_m3(diameter_m, 2.0 * head_compression_height_m);
    }

    double calc_moment_of_inertia_kg_per_m2() const override
    {
        double mass_reciprocating_kg = calc_head_mass_kg() + 0.5 * connecting_rod_mass_kg;
        return mass_reciprocating_kg * std::pow(crank_throw_length_m, 2.0);
    }

    double calc_friction_torque_n_m() const override
    {
        return camshaft.crankshaft.angular_velocity_r_per_s * friction_coefficient;
    }

    void update_bearing_position(double theta_r)
    {
        bearing_x_m = crank_throw_length_m * std::sin(theta_r);
        bearing_y_m = crank_throw_length_m * std::cos(theta_r);
    }

    void update_pin_position(double theta_r)
    {
        pin_x_m = 0.0;
        double term1 = std::sqrt(std::pow(connecting_rod_length_m, 2.0) - std::pow(crank_throw_length_m * std::sin(theta_r), 2.0));
        double term2 = crank_throw_length_m * std::cos(theta_r);
        pin_y_m = term1 + term2;
    }

    void rig()
    {
        update_bearing_position(calc_theta_r());
        update_pin_position(calc_theta_r());
        depth_m = calc_chamber_depth_m();
        head_mass_kg = calc_head_mass_kg();
    }

    void compress() override
    {
        double old_volume_m3 = calc_volume_m3();
        rig();
        double new_volume_m3 = calc_volume_m3();
        compress_adiabatically(old_volume_m3 / new_volume_m3);
    }

    std::vector<double> get_plot_datum() override
    {
        std::vector<double> datum = volume_t::get_plot_datum();
        datum[panel_applied_torque] = calc_applied_torque_n_m();
        datum[panel_sparkplug_ignition_ratio] = sparkplug.calc_ignition_ratio();
        return datum;
    }

    void ignite() override
    {
        double ignition_ratio = sparkplug.calc_ignition_ratio();
        if(ignition_ratio > 0.95)
        {
            ignition_flame.ignite();
        }
        if(ignition_flame.is_burning)
        {
            ignition_flame.grow(*this, diameter_m, depth_m, 1.0); /* from top - grows only down */
            burn_fuel(ignition_flame.calc_volume_m3());
        }
    }

    double calc_autoignition_static_temperature_k()
    {
        double static_pressure_mpa = calc_static_pressure_pa() / 1e6;
        if(static_pressure_mpa > 8.0)
        {
            return thermofluidics::autoignition_constant_a * std::pow(static_pressure_mpa, thermofluidics::autoignition_constant_b);
        }
        else
        {
            return std::numeric_limits<double>::max();
        }
    }

    void autoignite() override
    {
        double otto_theta_r = calc_otto_theta_r(calc_theta_r());
        if(otto_theta_r > 1.0 * M_PI && otto_theta_r < 2.0 * M_PI) /* a compression mechanic */
        {
            if(static_temperature_k > calc_autoignition_static_temperature_k())
            {
                autoignition_flame.ignite();
            }
            if(autoignition_flame.is_burning)
            {
                autoignition_flame.grow(*this, diameter_m, depth_m, 2.0); /* from center - grows up and down */
                burn_fuel(autoignition_flame.calc_volume_m3());
            }
        }
        else
        {
            autoignition_flame.reset();
        }
    }
};

struct throttle_t
: volume_t
{
    std::vector<piston_t*>& pistons;
    std::vector<injector_t*>& injectors;
    crankshaft_t& crankshaft;
    double rev_limit_r_per_s = 800.0;
    double rev_limit_hysteresis_r_per_s = 50.0;
    bool is_rev_limiter_enabled = false;

    throttle_t(std::vector<piston_t*>& pistons, std::vector<injector_t*>& injectors, crankshaft_t& crankshaft)
        : volume_t{"throttle", 0.1, 0.1}
        , pistons{pistons}
        , injectors{injectors}
        , crankshaft{crankshaft}
        {
        }

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {"rev_limit_r_per_s", &rev_limit_r_per_s},
            {"rev_limit_hysteresis_r_per_s", &rev_limit_hysteresis_r_per_s},
        };
        return volume_t::get_prop_table() + prop_table;
    }

    void rev_limit()
    {
        is_rev_limiter_enabled = true;
        for(piston_t* piston : pistons)
        {
            piston->sparkplug.is_enabled = false;
        }
        for(injector_t* injector : injectors)
        {
            injector->is_enabled = false;
        }
    }

    void rev_unlimit()
    {
        is_rev_limiter_enabled = false;
        for(piston_t* piston : pistons)
        {
            piston->sparkplug.is_enabled = true;
        }
        for(injector_t* injector : injectors)
        {
            injector->is_enabled = true;
        }
    }

    void do_work() override
    {
        if(crankshaft.angular_velocity_r_per_s > rev_limit_r_per_s)
        {
            rev_limit();
        }
        else
        if(is_rev_limiter_enabled)
        {
            double rev_unlimit_r_per_s = rev_limit_r_per_s - rev_limit_hysteresis_r_per_s;
            if(crankshaft.angular_velocity_r_per_s < rev_unlimit_r_per_s)
            {
                rev_unlimit();
            }
        }
    }
};

struct exhaust_t
: volume_t
{
    audio_processor_t& audio_processor;
    crankshaft_t& crankshaft;

    exhaust_t(audio_processor_t& audio_processor, crankshaft_t& crankshaft)
        : volume_t{"exhaust", 0.1, 0.2}
        , audio_processor{audio_processor}
        , crankshaft{crankshaft}
        {
        }

    void do_work() override
    {
        if(crankshaft.turned())
        {
            double sample = calc_static_pressure_pa();
            audio_processor.sample(sample);
        }
    }

    std::vector<double> get_plot_datum() override
    {
        std::vector<double> datum = volume_t::get_plot_datum();
        datum[panel_audio_signal] = audio_processor.buffer.back();
        return datum;
    }
};

struct sink_t
: volume_t
{
    sink_t()
        : volume_t{"sink", 1000.0, 1000.0}
        {
        }
};
