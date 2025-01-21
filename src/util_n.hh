namespace util_n
{
    int tile_to_pixel_p(double tile)
    {
        return ui_n::grid_size_p * tile;
    }

    int pixel_to_tile(int pixel_p)
    {
        return pixel_p / ui_n::grid_size_p;
    }

    double calc_otto_theta_r(double theta_r)
    {
        return std::fmod(theta_r, sim_n::four_stroke_r);
    }

    double calc_circle_area_m2(double diameter_m)
    {
        return M_PI * std::pow(diameter_m / 2.0, 2.0);
    }

    double calc_cylinder_volume_m3(double diameter_m, double depth_m)
    {
        return calc_circle_area_m2(diameter_m) * depth_m;
    }

    std::string double_to_string(double value, int precision, int width = 0)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setw(width) << std::setprecision(precision) << value;
        return stream.str();
    }

    double interpolate(double input, double lower_bound, double lower_value, double upper_bound, double upper_value)
    {
        if(input <= lower_bound)
        {
            return lower_value;
        }
        if(input >= upper_bound)
        {
            return upper_value;
        }
        return lower_value + (input - lower_bound) * (upper_value - lower_value) / (upper_bound - lower_bound);
    }

    double calc_weighted_average(double value1, double weight1, double value2, double weight2)
    {
        return (value1 * weight1 + value2 * weight2) / (weight1 + weight2);
    }

    /* cache values that need to be displayed by the ui -
     * a cache value _must not_ for computation */
    template <typename T>
    struct cached_t
    {
        T value;

        cached_t(T value)
            : value{value}
            {
            }

        T& get()
        {
            return value;
        }

        void set(const T& value)
        {
            this->value = value;
        }
    };
}

using namespace util_n;
