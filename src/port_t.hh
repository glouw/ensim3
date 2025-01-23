/*         length_m
 * |-----------------------|
 *         +------+
 * --------]      [--------- -
 * |      open_ratio       | | diameter_m
 * --------]      [--------- -
 *         +------+
 */

struct port_t
: has_prop_table_t
, observable_t
{
    std::string name = "";
    double diameter_m = 0.05;
    double length_m = 0.1;
    double open_ratio = 1.0;
    double flow_coefficient = 1.0;
    double flow_threshold_pressure_pa = 0.0;
    cached_t<double> flow_velocity_m_per_s = 0.0;
    double work_time_ns = 0.0;

    port_t(const std::string& name, double flow_threshold_pressure_pa)
        : name{name}
        , flow_threshold_pressure_pa{flow_threshold_pressure_pa}
        {
        }

    port_t()
        : port_t{"port", 100.0}
        {
        }

    prop_table_t get_prop_table() override
    {
        prop_table_t table = {
            {"port_diameter_m", &diameter_m},
            {"port_length_m", &length_m},
            {"port_open_ratio", &open_ratio},
            {"port_flow_threshold_pressure_pa", &flow_threshold_pressure_pa},
            {"port_flow_coefficient", &flow_coefficient},
        };
        return table;
    }

    double calc_flow_area_m2() const
    {
        return open_ratio * flow_coefficient * calc_circle_area_m2(diameter_m);
    }

    virtual void open()
    {
    }
};

struct throttle_port_t
: port_t
{
    throttle_cable_t& throttle_cable;
    double flow_exponent = 2.5;

    throttle_port_t(throttle_cable_t& throttle_cable)
        : port_t{"throttle_port", 10.0}
        , throttle_cable{throttle_cable}
        {
        }

    void open() override
    {
        open_ratio = std::pow(throttle_cable.pull_ratio, flow_exponent);
    }

    void on_create(std::vector<throttle_port_t*>& observers) override
    {
        subscribe_to(observers, this);
    }

    void on_delete(std::vector<throttle_port_t*>& observers) override
    {
        delete_from(observers, this);
    }
};

struct actuated_port_t
: port_t
{
    camshaft_t& camshaft;
    double engage_r = 0.0;
    double ramp_r = 0.0;

    actuated_port_t(camshaft_t& camshaft, double engage_r, double ramp_r)
        : port_t{"actuated_port", 10.0}
        , camshaft{camshaft}
        , engage_r{engage_r}
        , ramp_r{ramp_r}
        {
        }

    actuated_port_t(camshaft_t& camshaft)
        : actuated_port_t{camshaft, 0.0, M_PI}
        {
        }

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {"actuated_port_engage_r", &engage_r},
            {"actuated_port_ramp_r", &ramp_r},
        };
        return port_t::get_prop_table() + prop_table;
    }

    void open() override
    {
        open_ratio = camshaft.calc_lift_ratio(engage_r, ramp_r);
    }
};
