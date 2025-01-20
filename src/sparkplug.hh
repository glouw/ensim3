struct sparkplug_t
: has_prop_table_t
{
    /* 0π to 1π : intake stroke
     * 1π to 2π : compression stroke
     * 2π to 3π : power stroke
     * 3π to 4π : exhaust stroke
     */

    double ignition_duration_r = M_PI / 8.0;
    double ignition_engage_r = 2.0 * M_PI;
    bool is_enabled = true;
    camshaft_t& camshaft;

    sparkplug_t(camshaft_t& camshaft)
        : camshaft{camshaft}
        {
        }

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {"sparkplug_duration_r", &ignition_duration_r},
            {"sparkplug_engage_r", &ignition_engage_r},
            {"sparkplug_is_enabled", &is_enabled},
        };
        return prop_table;
    }

    double calc_ignition_ratio() const
    {
        if(is_enabled)
        {
            double theta_r = calc_otto_theta_r(camshaft.crankshaft.theta_r - ignition_engage_r);
            if(theta_r < 0.0)
            {
                theta_r += dynamics::four_stroke_r;
            }
            double half_ramp_r = ignition_duration_r / 2.0;
            if(theta_r < half_ramp_r)
            {
                return theta_r / half_ramp_r;
            }
            else
            if(theta_r < ignition_duration_r)
            {
                return 2.0 - theta_r / half_ramp_r;
            }
        }
        return 0.0;
    }
};
