int snap_p(int value)
{
    int mid_p = ui::grid_size_p / 2;
    return value >= 0
        ? ((value + mid_p) / ui::grid_size_p) * ui::grid_size_p
        : ((value - mid_p) / ui::grid_size_p) * ui::grid_size_p;
}

int tile_to_pixel_p(double tile)
{
    return ui::grid_size_p * tile;
}

int pixel_to_tile(int pixel_p)
{
    return pixel_p / ui::grid_size_p;
}

double calc_otto_theta_r(double theta_r)
{
    return std::fmod(theta_r, dynamics::four_stroke_r);
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

double frand()
{
    return 2.0 * (std::rand() / (double) RAND_MAX - 0.5);
}

/* cache values that need to be displayed by the ui -
 * a cache value _must not_ for computation */
template <typename T>
struct cached_t
{
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

private:
    T value;
};
