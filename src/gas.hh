#pragma once

struct gas_t
: has_prop_table_t
{
    double static_temperature_k = 0.0;
    double bulk_momentum_kg_m_per_s = 0.0;
    double moles = 0.0;
    double air_molar_ratio = 0.0;
    double fuel_molar_ratio = 0.0;
    double combusted_molar_ratio = 0.0;
    double mol_balance = 0.0; /* tracks volumetric_efficiency... todo: only works on 0pi aligned piston... bugged for rest */

    virtual double calc_volume_m3() const = 0;

    void normalize()
    {
        static_temperature_k = thermofluidics::ntp_static_temperature_k;
        bulk_momentum_kg_m_per_s = 0.0;
        moles = thermofluidics::ntp_static_pressure_pa * calc_volume_m3() / (thermofluidics::r_j_per_mol_k * static_temperature_k);
        air_molar_ratio = 1.0;
        fuel_molar_ratio = 0.0;
        combusted_molar_ratio = 0.0;
        mol_balance = 0.0;
    }

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {"gas_static_temperature_k", &static_temperature_k},
            {"gas_bulk_momentum_kg_m_per_s", &bulk_momentum_kg_m_per_s},
            {"gas_moles", &moles},
            {"gas_air_molar_ratio", &air_molar_ratio},
            {"gas_fuel_molar_ratio", &fuel_molar_ratio},
            {"gas_combusted_molar_ratio", &combusted_molar_ratio},
        };
        return prop_table;
    }

    double calc_molar_mass_kg_per_mol() const
    {
        return thermofluidics::molar_mass_combusted_kg_per_mol * combusted_molar_ratio
             + thermofluidics::molar_mass_air_kg_per_mol * air_molar_ratio
             + thermofluidics::molar_mass_fuel_kg_per_mol * fuel_molar_ratio;
    }

    double calc_gamma() const
    {
        return thermofluidics::gamma_combusted * combusted_molar_ratio
             + thermofluidics::gamma_air * air_molar_ratio
             + thermofluidics::gamma_fuel * fuel_molar_ratio;
    }

    /* Rs = R / M */

    double calc_specific_gas_constant_j_per_kg_k() const
    {
        return thermofluidics::r_j_per_mol_k / calc_molar_mass_kg_per_mol();
    }

    /* cv = Rs / (y - 1) */

    double calc_specific_heat_capacity_at_constant_volume_j_per_kg_k() const
    {
        return calc_specific_gas_constant_j_per_kg_k() / (calc_gamma() - 1.0);
    }

    /* cp = y * cv */

    double calc_specific_heat_capacity_at_constant_pressure_j_per_kg_k() const
    {
        return calc_gamma() * calc_specific_heat_capacity_at_constant_volume_j_per_kg_k();
    }

    /*          (y - 1)
     * Td = T * ------- * M ^ 2
     *             2
     */

    double calc_dynamic_temperature_k(double mach_number) const
    {
        return static_temperature_k * (calc_gamma() - 1.0) / 2.0 * std::pow(mach_number, 2.0);
    }

    /* Tt = T + Td */

    double calc_total_temperature_k(double mach_number) const
    {
        return static_temperature_k + calc_dynamic_temperature_k(mach_number);
    }

    /*     n * R * T
     * P = ---------
     *         V
     */

    double calc_static_pressure_pa() const
    {
        return (moles * thermofluidics::r_j_per_mol_k * static_temperature_k) / calc_volume_m3();
    }

    /* P = P - 1 * atm */

    double calc_static_gauge_pressure_pa() const
    {
        return calc_static_pressure_pa() - thermofluidics::ntp_static_pressure_pa;
    }

    /* m = n * M */

    double calc_mass_kg() const
    {
        return moles * calc_molar_mass_kg_per_mol();
    }

    /* v = p / m */

    double calc_bulk_velocity_m_per_s() const
    {
        return bulk_momentum_kg_m_per_s / calc_mass_kg();
    }

    /* ρ = m / V */

    double calc_density_kg_per_m3() const
    {
        return calc_mass_kg() / calc_volume_m3();
    }

    /*          2
     *     ρ * v
     * q = -----
     *       2
     */

    double calc_dynamic_pressure_pa() const
    {
        return calc_density_kg_per_m3() * std::pow(calc_bulk_velocity_m_per_s(), 2.0) / 2.0;
    }

    /* P0 = P + q */

    double calc_total_pressure_pa() const
    {
        return calc_static_pressure_pa() + calc_dynamic_pressure_pa();
    }

    double calc_gauge_pressure_pa() const
    {
        return calc_static_gauge_pressure_pa() + calc_dynamic_pressure_pa();
    }

    /*          ___________
     * vmax = \/ y * Rs * T
     */

    double calc_max_velocity_m_per_s() const
    {
        return std::sqrt(calc_gamma() * calc_specific_gas_constant_j_per_kg_k() * static_temperature_k);
    }

    /* pmax = m * vmax */

    double calc_max_bulk_momentum_kg_m_per_s() const
    {
        return calc_mass_kg() * calc_max_velocity_m_per_s();
    }

    /*           ___________________________
     *          /          y2
     *         /         ------
     * M =    /   (P0,1) y2 - 1         2
     *       /  ( ------        - 1 ) ------
     *     \/     (P0,2)              y2 - 1
     */

    double calc_mach_number(const gas_t& other) const
    {
        double compression_ratio = calc_total_pressure_pa() / other.calc_total_pressure_pa();
        double term1 = std::pow(compression_ratio, other.calc_gamma() / (other.calc_gamma() - 1.0)) - 1.0;
        double term2 = 2.0 / (other.calc_gamma() - 1.0);
        double mach_number = std::sqrt(term1 * term2);
        return std::clamp(mach_number, 0.0, 1.0); /* never supersonic */
    }

    /*                   (y - 1) / y
     *               nnew
     * Tnew = Told * ----
     *               nold
     */

    void add_moles_adiabatically(double delta_moles)
    {
        double new_moles = moles + delta_moles;
        if(new_moles < 0.0)
        {
            throw std::runtime_error("negative mole count encountered");
        }
        mol_balance += delta_moles;
        static_temperature_k *= std::pow(new_moles / moles, (calc_gamma() - 1.0) / calc_gamma());
        moles = new_moles;
    }

    double calc_air_fuel_mass_ratio() const
    {
        double air_fuel_mass_upper_ratio_cap = 999.999999; /* so that it displys visibly on plot */
        if(fuel_molar_ratio == 0.0)
        {
            return air_fuel_mass_upper_ratio_cap;
        }
        double air_mass_kg = air_molar_ratio * moles * thermofluidics::molar_mass_air_kg_per_mol;
        double fuel_mass_kg = fuel_molar_ratio * moles * thermofluidics::molar_mass_fuel_kg_per_mol;
        double air_fuel_mass_ratio = air_mass_kg / fuel_mass_kg;
        air_fuel_mass_ratio = std::clamp(air_fuel_mass_ratio, 0.0, air_fuel_mass_upper_ratio_cap);
        return air_fuel_mass_ratio;
    }

    void add_fuel_moles(double fuel_moles)
    {
        fuel_molar_ratio = (fuel_molar_ratio * moles + fuel_moles) / (moles + fuel_moles);
        air_molar_ratio = 1.0 - fuel_molar_ratio;
        add_moles_adiabatically(fuel_moles);
    }

    void add_momentum(double bulk_momentum_kg_m_per_s)
    {
        this->bulk_momentum_kg_m_per_s += bulk_momentum_kg_m_per_s;
        double max_bulk_momentum_kg_m_per_s = calc_max_bulk_momentum_kg_m_per_s();
        this->bulk_momentum_kg_m_per_s = std::clamp(this->bulk_momentum_kg_m_per_s, -max_bulk_momentum_kg_m_per_s, max_bulk_momentum_kg_m_per_s);
    }

    /*                   (y - 1)
     *               Vold
     * Tnew = Told * ----
     *               Vnew
     */

    void compress_adiabatically(double compression_ratio_m3)
    {
        static_temperature_k *= std::pow(compression_ratio_m3, calc_gamma() - 1.0);
    }

    /* w1 * x1 + w2 * x2
     * -----------------
     *      w1 + w
     */

    double calc_weighted_average(double value1, double weight1, double value2, double weight2)
    {
        return (value1 * weight1 + value2 * weight2) / (weight1 + weight2);
    }

    void mix_in(const gas_t& gas)
    {
        add_moles_adiabatically(gas.moles);
        add_momentum(gas.bulk_momentum_kg_m_per_s);
        air_molar_ratio = calc_weighted_average(air_molar_ratio, moles, gas.air_molar_ratio, gas.moles);
        fuel_molar_ratio = calc_weighted_average(fuel_molar_ratio, moles, gas.fuel_molar_ratio, gas.moles);
        combusted_molar_ratio = calc_weighted_average(combusted_molar_ratio, moles, gas.combusted_molar_ratio, gas.moles);
        static_temperature_k = calc_weighted_average(static_temperature_k, moles, gas.static_temperature_k, gas.moles);
    }

    void burn_fuel(double burning_volume_m3)
    {
        double volume_ratio = burning_volume_m3 / calc_volume_m3();
        double burning_mass_kg = calc_mass_kg() * volume_ratio;
        double burning_moles = burning_mass_kg / calc_molar_mass_kg_per_mol();
        double burning_air_moles = burning_moles * air_molar_ratio;
        double burning_fuel_moles = burning_moles * fuel_molar_ratio;
        double burning_air_mass_kg = burning_air_moles * thermofluidics::molar_mass_air_kg_per_mol;
        double burning_fuel_mass_kg = burning_fuel_moles * thermofluidics::molar_mass_fuel_kg_per_mol;
        if(burning_air_mass_kg / burning_fuel_mass_kg > thermofluidics::air_fuel_stoich_ratio)
        {
            burning_air_mass_kg = burning_fuel_mass_kg * thermofluidics::air_fuel_stoich_ratio;
        }
        else
        {
            burning_fuel_mass_kg = burning_air_mass_kg / thermofluidics::air_fuel_stoich_ratio;
        }
        double air_moles = moles * air_molar_ratio;
        double fuel_moles = moles * fuel_molar_ratio;
        double combusted_moles = moles * combusted_molar_ratio;
        double air_mass_kg = air_moles * thermofluidics::molar_mass_air_kg_per_mol;
        double fuel_mass_kg = fuel_moles * thermofluidics::molar_mass_fuel_kg_per_mol;
        double combusted_mass_kg = combusted_moles * thermofluidics::molar_mass_combusted_kg_per_mol;
        air_mass_kg -= burning_air_mass_kg;
        fuel_mass_kg -= burning_fuel_mass_kg;
        combusted_mass_kg += burning_air_mass_kg + burning_fuel_mass_kg;
        double new_air_moles = air_mass_kg / thermofluidics::molar_mass_air_kg_per_mol;
        double new_fuel_moles = fuel_mass_kg / thermofluidics::molar_mass_fuel_kg_per_mol;
        double new_combusted_moles = combusted_mass_kg / thermofluidics::molar_mass_combusted_kg_per_mol;
        double new_total_moles = new_air_moles + new_fuel_moles + new_combusted_moles;
        air_molar_ratio = new_air_moles / new_total_moles;
        fuel_molar_ratio = new_fuel_moles / new_total_moles;
        combusted_molar_ratio = new_combusted_moles / new_total_moles;
        double energy_released_j = burning_fuel_mass_kg * thermofluidics::fuel_lower_heating_value_j_per_kg;
        static_temperature_k += energy_released_j / (calc_mass_kg() * calc_specific_heat_capacity_at_constant_volume_j_per_kg_k());
    }
};

struct gas_parcel_t
: gas_t
{
    int arrival_cycle = 0;
    double velocity_m_per_s = 0.0;

    double calc_volume_m3() const override
    {
        throw std::logic_error("gas parcels are dimensionless");
    }

    gas_parcel_t(double static_temperature_k, double bulk_momentum_kg_m_per_s, double moles, double air_molar_ratio, double fuel_molar_ratio, double combusted_molar_ratio, int arrival_cycle, double velocity_m_per_s)
        : arrival_cycle{arrival_cycle}
        , velocity_m_per_s{velocity_m_per_s}
        {
            this->static_temperature_k = static_temperature_k;
            this->bulk_momentum_kg_m_per_s = bulk_momentum_kg_m_per_s;
            this->moles = moles;
            this->air_molar_ratio = air_molar_ratio;
            this->fuel_molar_ratio = fuel_molar_ratio;
            this->combusted_molar_ratio = combusted_molar_ratio;
        }

    bool operator>(const gas_parcel_t& other) const
    {
        return arrival_cycle > other.arrival_cycle;
    }
};

struct flowing_gas_t
: gas_t
{
    virtual double calc_flow_area_m2() const = 0;
    virtual double calc_flow_length_m() const = 0;

    /*                                          -(y + 1)
     *                _____                     ---------
     * .    A P0     /  y          (y - 1)  2   2(y - 1)
     * m = ------   / ----- M (1 + ------- M )
     *        __  \/   Rs             2
     *      \/T0
     */

    double calc_mass_flow_rate_kg_per_s(double mach_number) const
    {
        double term1 = calc_flow_area_m2() * calc_total_pressure_pa() / std::sqrt(calc_total_temperature_k(mach_number));
        double term2 = std::sqrt(calc_gamma() / calc_specific_gas_constant_j_per_kg_k()) * mach_number;
        double term3 = std::pow(1.0 + (calc_gamma() - 1.0) / 2.0 * std::pow(mach_number, 2.0), - (calc_gamma() + 1.0) / (2.0 * (calc_gamma() - 1.0)));
        return term1 * term2 * term3;
    }

    /*
     *        P0
     * pt = ------
     *      R * Tt
     */

    double calc_total_density_kg_per_m3(double mach_number) const
    {
        return calc_total_pressure_pa() / (calc_specific_gas_constant_j_per_kg_k() * calc_total_temperature_k(mach_number));
    }

    /*        *
     *        m
     * V = ------
     *     pt * A
     */

    double calc_velocity_m_per_s(double mach_number) const
    {
        return calc_mass_flow_rate_kg_per_s(mach_number) / (calc_total_density_kg_per_m3(mach_number) * calc_flow_area_m2());
    }

    std::optional<gas_parcel_t> package_gas_parcel(double mach_number, int cycle)
    {
        if(calc_flow_area_m2() > 0.0)
        {
            double velocity_m_per_s = calc_velocity_m_per_s(mach_number);
            double mass_flowed_kg = calc_mass_flow_rate_kg_per_s(mach_number) * sim::dt_s;
            double moles_flowed = mass_flowed_kg / calc_molar_mass_kg_per_mol();
            double bulk_momentum_flowed_kg_m_per_s = mass_flowed_kg * velocity_m_per_s;
            int travel_cycles = calc_flow_length_m() / (velocity_m_per_s * sim::dt_s);
            int arrival_cycle = travel_cycles + cycle;
            return gas_parcel_t{static_temperature_k, bulk_momentum_flowed_kg_m_per_s, moles_flowed, air_molar_ratio, fuel_molar_ratio, combusted_molar_ratio, arrival_cycle, velocity_m_per_s};
        }
        return std::nullopt;
    }
};
