struct audio_processor_t
: has_prop_table_t
{
    std::vector<float> buffer;
    bool use_convolution = true;
    std::unique_ptr<dc_filter_t> dc_filter = std::make_unique<dc_filter_t>();
    std::unique_ptr<convolution_filter_t> convolution_filter = std::make_unique<convolution_filter_t>("impulses/impulse.dat");
    std::unique_ptr<brightness_filter_t> brightness_filter = std::make_unique<brightness_filter_t>();
    std::unique_ptr<agc_filter_t> agc_filter = std::make_unique<agc_filter_t>();
    crankshaft_t& crankshaft;
    double lower_brightness_mix_ratio = 0.1;
    double upper_brightness_mix_ratio = 1.0;
    double lower_gain = 0.2;
    double upper_gain = 0.6;
    double lower_angular_velocity_r_per_s = 100.0;
    double upper_angular_velocity_r_per_s = 1000.0;

    audio_processor_t(crankshaft_t& crankshaft)
        : crankshaft{crankshaft}
        {
        }

    prop_table_t get_prop_table() override
    {
        prop_table_t prop_table = {
            {"audio_processor_use_convolution", &use_convolution},
            {"audio_processor_brightness_ratio", &brightness_filter->mix_ratio},
            {"audio_processor_gain", &agc_filter->gain},
        };
        return prop_table;
    }

    void sample(double value)
    {
        agc_filter->gain = interpolate(crankshaft.angular_velocity_r_per_s, lower_angular_velocity_r_per_s, lower_gain, upper_angular_velocity_r_per_s, upper_gain);
        brightness_filter->mix_ratio = interpolate(crankshaft.angular_velocity_r_per_s, lower_angular_velocity_r_per_s, lower_brightness_mix_ratio, upper_angular_velocity_r_per_s, upper_brightness_mix_ratio);
        value = dc_filter->filter(value);
        if(use_convolution)
        {
            value = convolution_filter->filter(value);
        }
        value = brightness_filter->filter(value);
        value = agc_filter->filter(value);
        buffer.push_back(value);
    }
};
