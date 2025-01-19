#pragma once

struct plot_values_t
{
    std::vector<double> values;

    /* left in an invalid state after normalization */
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();
    double sum = 0.0;
    /* --- */

    void normalize()
    {
        for(double& value : values)
        {
            double denom = max - min;
            if(denom == 0.0)
            {
                value = 1.0;
            }
            else
            {
                value = (value - min) / denom;
            }
        }
    }

    void push(double value)
    {
        values.push_back(value);
        min = std::min(min, value);
        max = std::max(max, value);
        sum += value;
    }

    double calc_average()
    {
        return sum / values.size();
    }
};

struct plot_channel_t
{
    plot_values_t x;
    plot_values_t y;

    void push(double xx, double yy)
    {
        x.push(xx);
        y.push(yy);
    }

    void normalize()
    {
        x.normalize();
        y.normalize();
    }
};

struct plot_t
{
    render_rect_t rect;
    int precision = 0;
    int width = 0;
    std::string x_units = "";
    std::string y_units = "";
    std::string name = "";
    std::vector<plot_channel_t> front;
    std::vector<plot_channel_t> back;
    int channels = 0;

    plot_t(int precision, int width, const std::string& x_units, const std::string& y_units, const std::string& name)
        : precision{precision}
        , width{width}
        , x_units{x_units}
        , y_units{y_units}
        , name{name}
        {
        }

    void push(int channel, double x, double y)
    {
        back[channel].push(x, y);
    }

    void set_channels(int channels)
    {
        if(this->channels != channels)
        {
            this->channels = channels;
            back.resize(channels);
        }
    }

    void flip()
    {
        for(plot_channel_t& plot : back)
        {
            plot.normalize();
        }
        front = std::move(back);
        back.resize(channels);
    }
};

enum
{
    panel_port_open_ratio,
    panel_volume,
    panel_volumetric_efficiency,
    panel_port_flow_velocity,
    panel_total_pressure,
    panel_static_temperature,
    panel_gamma,
    panel_molar_mass,
    panel_air_fuel_mass_ratio,
    panel_applied_torque,
    panel_total_pressure_volume,
    panel_sparkplug_ignition_ratio,
    panel_audio_signal,
    panel_size
};

struct plot_panel_t
{
    std::array<plot_t, panel_size> panel = {
        plot_t{5, 15, "rad", "ratio", "port open"},
        plot_t{5, 15, "rad", "m3", "volume"},
        plot_t{5, 15, "rad", "", "volumetric efficiency"},
        plot_t{5, 15, "rad", "m/s", "port flow velocity"},
        plot_t{5, 15, "rad", "pa", "total pressure"},
        plot_t{5, 15, "rad", "k", "static temperature"},
        plot_t{5, 15, "rad", "", "gamma"},
        plot_t{5, 15, "rad", "kg/mol", "molar mass"},
        plot_t{5, 15, "rad", "", "air fuel mass ratio"},
        plot_t{5, 15, "rad", "n*m", "applied torque"},
        plot_t{5, 15, "m3", "pa", "total pressure - volume"},
        plot_t{5, 15, "rad", "", "sparkplug ignition"},
        plot_t{5, 15, "rad", "", "audio signal"},
    };

    plot_panel_t(int x_p, int yres_p, int w_p)
    {
        int y_p = 0;
        int h_p = yres_p / panel.size();
        for(plot_t& plot : panel)
        {
            plot.rect = {x_p, y_p, x_p + w_p, y_p + h_p};
            y_p += h_p;
        }
    }

    void set_channels(int channels)
    {
        for(plot_t& plot : panel)
        {
            plot.set_channels(channels);
        }
    }

    void buffer(double channel, double theta_r, const std::vector<double>& plot_values)
    {
        panel[panel_port_open_ratio].push(channel, theta_r, plot_values[panel_port_open_ratio]);
        panel[panel_volume].push(channel, theta_r, plot_values[panel_volume]);
        panel[panel_volumetric_efficiency].push(channel, theta_r, plot_values[panel_volumetric_efficiency]);
        panel[panel_port_flow_velocity].push(channel, theta_r, plot_values[panel_port_flow_velocity]);
        panel[panel_total_pressure].push(channel, theta_r, plot_values[panel_total_pressure]);
        panel[panel_static_temperature].push(channel, theta_r, plot_values[panel_static_temperature]);
        panel[panel_gamma].push(channel, theta_r, plot_values[panel_gamma]);
        panel[panel_molar_mass].push(channel, theta_r, plot_values[panel_molar_mass]);
        panel[panel_air_fuel_mass_ratio].push(channel, theta_r, plot_values[panel_air_fuel_mass_ratio]);
        panel[panel_applied_torque].push(channel, theta_r, plot_values[panel_applied_torque]);
        panel[panel_total_pressure_volume].push(channel, plot_values[panel_volume], plot_values[panel_total_pressure]);
        panel[panel_sparkplug_ignition_ratio].push(channel, theta_r, plot_values[panel_sparkplug_ignition_ratio]);
        panel[panel_audio_signal].push(channel, theta_r, plot_values[panel_audio_signal]);
    }

    void flip()
    {
        for(plot_t& plot : panel)
        {
            plot.flip();
        }
    }
};
