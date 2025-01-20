struct flame_t
{
    double diameter_m = 0.0;
    double depth_m = 0.0;
    double pressure_exponent = 1.7;
    double temperature_exponent = 1.35;
    double laminar_flame_speed_m_per_s = 0.375;
    bool is_burning = false;

    double calc_flame_speed_m_per_s(const gas_t& gas) const
    {
        double term1 = std::pow(gas.calc_static_pressure_pa() / thermofluidics_n::ntp_static_pressure_pa, pressure_exponent);
        double term2 = std::pow(gas.static_temperature_k / thermofluidics_n::stp_static_temperature_k, temperature_exponent);
        return laminar_flame_speed_m_per_s * term1 * term2;
    }

    void reset()
    {
        diameter_m = 0.0;
        depth_m = 0.0;
        is_burning = false;
    }

    void ignite()
    {
        if(is_burning == false)
        {
            is_burning = true;
        }
    }

    void grow(const gas_t& gas, double max_diameter_m, double max_depth_m, double depth_growth_rate)
    {
        double flame_speed_m_per_s = calc_flame_speed_m_per_s(gas);
        double flame_displacement_m = flame_speed_m_per_s * sim_n::dt_s;
        diameter_m += 2.0 * flame_displacement_m;
        depth_m += depth_growth_rate * flame_displacement_m;
        diameter_m = std::min(diameter_m, max_diameter_m);
        depth_m = std::min(depth_m, max_depth_m);
        if(diameter_m == max_diameter_m && depth_m == max_depth_m)
        {
            reset();
        }
    }

    double calc_volume_m3()
    {
        return calc_cylinder_volume_m3(diameter_m, depth_m);
    }
};
