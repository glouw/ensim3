#pragma once

struct starter_motor_t
: has_prop_table_t
, rotational_mass_t
{
    double diameter_m = 0.05;
    double rated_torque_n_m = 20.0;
    double rated_angular_velocity_r_per_s = 400.0;
    bool is_enabled = false;
    crankshaft_t& crankshaft;
    flywheel_t& flywheel;

    starter_motor_t(crankshaft_t& crankshaft, flywheel_t& flywheel)
        : crankshaft{crankshaft}
        , flywheel{flywheel}
        {
        }

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {"starter_motor_diameter_m", &diameter_m},
            {"starter_motor_rated_torque_n_m", &rated_torque_n_m},
            {"starter_motor_rated_angular_velocity_r_per_s", &rated_angular_velocity_r_per_s},
            {"starter_motor_is_enabled", &is_enabled},
        };
        return prop_table;
    }

    double calc_moment_of_inertia_kg_per_m2() const override
    {
        return 0.0;
    }

    double calc_angular_velocity_r_per_s() const
    {
        return crankshaft.angular_velocity_r_per_s * flywheel.diameter_m / diameter_m;
    }

    double calc_effective_torque_n_m() const
    {
        double effective_torque_n_m = rated_torque_n_m * (1.0 - calc_angular_velocity_r_per_s() / rated_angular_velocity_r_per_s);
        return std::max(0.0, effective_torque_n_m);
    }

    double calc_applied_torque_n_m() const override
    {
        if(is_enabled)
        {
            return calc_effective_torque_n_m() * flywheel.diameter_m / diameter_m;
        }
        else
        {
            return 0.0;
        }
    }

    double calc_friction_torque_n_m() const override
    {
        return 0.0;
    }
};
