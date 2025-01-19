#pragma once

struct filter_t
{
    virtual double filter(double value) = 0;
    virtual ~filter_t() = default;
};

struct moving_average_filter_t
: filter_t
{
    std::deque<double> window;
    int size = 0;
    double sum = 0.0;

    moving_average_filter_t(int size)
        : size{size}
        {
        }

    double filter(double value) override
    {
        int window_size = window.size();
        if(window_size == size)
        {
            sum -= window.front();
            window.pop_front();
        }
        window.push_back(value);
        sum += value;
        return sum / window_size;
    }
};

struct lowpass_filter_t
: filter_t
{
    double cutoff_frequency_hz = 10000.0;
    double prev_output = 0.0;

    double filter(double value) override
    {
        double rc_s = 1.0 / (2.0 * M_PI * cutoff_frequency_hz);
        double alpha = sim::sample_frequency_hz * rc_s / (sim::sample_frequency_hz * rc_s + 1.0);
        prev_output = alpha * value + (1.0 - alpha) * prev_output;
        return prev_output;
    }
};

struct highpass_filter_t
: filter_t
{
    double cutoff_frequency_hz = 60.0;
    double prev_input = 0.0;
    double prev_output = 0.0;

    double filter(double value) override
    {
        double rc_s = 1.0 / (2.0 * M_PI * cutoff_frequency_hz);
        double alpha = rc_s / (rc_s + sim::dt_s);
        double output = alpha * (prev_output + value - prev_input);
        prev_input = value;
        prev_output = output;
        return output;
    }
};

struct dc_filter_t
: highpass_filter_t
{
    dc_filter_t()
    {
        cutoff_frequency_hz = 10.0;
    }
};

struct static_noise_filter_t
: lowpass_filter_t
{
    static_noise_filter_t()
    {
        cutoff_frequency_hz = 250.0;
    }
};

struct gain_filter_t
: filter_t
{
    double gain = 1e-6;

    double filter(double value) override
    {
        return std::clamp(gain * value, -1.0, 1.0);
    }
};

struct convolution_filter_t
: filter_t
{
    int at = 0;
    double buffer[sim::impulse_size] = {};
    double impulse[sim::impulse_size] = {};

    convolution_filter_t(const std::string& impulse_filename)
    {
        load_impulse(impulse_filename);
    }

    double filter(double sample) override
    {
        buffer[at] = sample;
        double result = 0;
        int y = sim::impulse_size;
        int x = y - at;
        for(int i = 0; i < x; i++)
        {
            result += impulse[i] * buffer[i + at];
        }
        for(int i = x; i < y; i++)
        {
            result += impulse[i] * buffer[i - x];
        }
        at = (at - 1 + y) % y;
        return result;
    }

    void load_impulse(const std::string& filename)
    {
        std::ifstream file(filename);
        int index = 0;
        if(file.good())
        {
            double value;
            while(file >> value)
            {
                impulse[index++] = value;
            }
        }
        assert(index == sim::impulse_size);
    }
};

struct derivative_filter_t
: filter_t {

    double prev_value = 0.0;

    double filter(double value) override
    {
        double derivative = value - prev_value;
        prev_value = value;
        return derivative;
    }
};

struct brightness_filter_t
: filter_t
{
    derivative_filter_t derivative_filter;
    double mix_ratio = 0.5;

    double filter(double value) override
    {
        double derivative = derivative_filter.filter(value);
        return (1.0 - mix_ratio) * value + mix_ratio * derivative;
    }
};

struct agc_filter_t
: filter_t
{
    moving_average_filter_t window{512};
    double gain = 0.5;

    double filter(double value)
    {
        double magnitude = std::abs(value);
        double average = window.filter(magnitude);
        value /= average;
        value *= gain;
        return std::clamp(value, -1.0, 1.0);
    }
};
