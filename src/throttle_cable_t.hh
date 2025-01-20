struct throttle_cable_t
: has_prop_table_t
{
    double pull_ratio_setpoint = 0.5;
    double pull_ratio = 0.0;

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {"throttle_cable_pull_ratio_setpoint", &pull_ratio_setpoint},
        };
        return prop_table;
    }

    void apply()
    {
        pull_ratio = pull_ratio_setpoint;
    }
};
