struct rotational_mass_t
{
    virtual double calc_moment_of_inertia_kg_per_m2() const = 0;
    virtual double calc_applied_torque_n_m() const = 0;
    virtual double calc_friction_torque_n_m() const = 0;
    virtual ~rotational_mass_t() = default;
};

struct crankshaft_t
: has_prop_table_t
, rotational_mass_t
{
    double theta_r = 0.0;
    double last_theta_r = 0.0;
    double angular_velocity_r_per_s = 0.0;
    double mass_kg = 2.5;
    double diameter_m = 0.1;
    double friction_coefficient = 0.01;
    double static_friction_coefficient = 0.8;

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {"crankshaft_theta_r", &theta_r},
            {"crankshaft_angular_velocity_r_per_s", &angular_velocity_r_per_s},
            {"crankshaft_mass_kg", &mass_kg},
            {"crankshaft_diameter_m", &diameter_m},
            {"crankshaft_friction_coefficient", &friction_coefficient},
            {"crankshaft_static_friction_coefficient", &static_friction_coefficient},
        };
        return prop_table;
    }

    void accelerate(double angular_acceleration_r_per_s)
    {
        angular_velocity_r_per_s += angular_acceleration_r_per_s * sim::dt_s;
        last_theta_r = theta_r;
        theta_r += angular_velocity_r_per_s * sim::dt_s;
    }

    bool finished_rotation()
    {
        return calc_otto_theta_r(last_theta_r) > calc_otto_theta_r(theta_r);
    }

    bool turned()
    {
        return theta_r != last_theta_r;
    }

    double calc_moment_of_inertia_kg_per_m2() const override
    {
        return 0.5 * mass_kg * std::pow(diameter_m / 2.0, 2.0);
    }

    double calc_applied_torque_n_m() const override
    {
        return 0.0;
    }

    double calc_friction_torque_n_m() const override
    {
        if(angular_velocity_r_per_s < 1.0)
        {
            return angular_velocity_r_per_s * static_friction_coefficient;
        }
        else
        {
            return angular_velocity_r_per_s * friction_coefficient;
        }
    }
};

struct camshaft_t
: has_prop_table_t
, rotational_mass_t
{
    crankshaft_t& crankshaft;
    double mass_kg = 1.0;
    double diameter_m = 0.025;
    double friction_coefficient = 0.001;

    camshaft_t(crankshaft_t& crankshaft)
        : crankshaft{crankshaft}
        {
        }

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {"camshaft_mass_kg", &mass_kg},
            {"camshaft_diameter_m", &diameter_m},
            {"camshaft_friction_coefficient", &friction_coefficient},
        };
        return prop_table;
    }

    double calc_lift_ratio(double engage_r, double ramp_r) const
    {
        double otto_engage_r = calc_otto_theta_r(engage_r);
        double otto_theta_r = calc_otto_theta_r(crankshaft.theta_r);
        if(otto_theta_r < otto_engage_r)
        {
            otto_theta_r += dynamics::four_stroke_r;
        }
        double open_r = otto_theta_r - otto_engage_r;
        double term1 = 35.0 * std::pow(open_r / ramp_r, 4.0);
        double term2 = 84.0 * std::pow(open_r / ramp_r, 5.0);
        double term3 = 70.0 * std::pow(open_r / ramp_r, 6.0);
        double term4 = 20.0 * std::pow(open_r / ramp_r, 7.0);
        double lift_ratio = term1 - term2 + term3 - term4;
        return std::clamp(lift_ratio, 0.0, 1.0);
    }

    double calc_moment_of_inertia_kg_per_m2() const override
    {
        return 0.5 * mass_kg * std::pow(diameter_m / 2.0, 2.0);
    }

    double calc_applied_torque_n_m() const override
    {
        return 0.0;
    }

    double calc_friction_torque_n_m() const override
    {
        return crankshaft.angular_velocity_r_per_s * friction_coefficient;
    }
};

struct flywheel_t
: has_prop_table_t
, rotational_mass_t
{
    double mass_kg = 7.5;
    double diameter_m = 0.5;

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {"flywheel_mass_kg", &mass_kg},
            {"flywheel_diameter_m", &diameter_m},
        };
        return prop_table;
    }

    double calc_moment_of_inertia_kg_per_m2() const override
    {
        return 0.5 * mass_kg * std::pow(diameter_m / 2.0, 2.0);
    }

    double calc_applied_torque_n_m() const override
    {
        return 0.0;
    }

    double calc_friction_torque_n_m() const override
    {
        return 0.0;
    }
};
