struct render_circle_t
{
    int x_p = 0;
    int y_p = 0;
    int diameter_p = 0;

    render_circle_t() = default;

    render_circle_t(int x_p, int y_p, int diameter_p)
        : x_p{x_p}
        , y_p{y_p}
        , diameter_p{diameter_p}
        {
        }

    int calc_radius_p() const
    {
        return diameter_p / 2;
    }

    void center()
    {
        x_p += diameter_p / 2;
        y_p += diameter_p / 2;
    }
};

struct render_rect_t
{
    int x0_p = 0;
    int y0_p = 0;
    int x1_p = 0;
    int y1_p = 0;

    render_rect_t() = default;

    render_rect_t(int x0_p, int y0_p, int x1_p, int y1_p)
        : x0_p{x0_p}
        , y0_p{y0_p}
        , x1_p{x1_p}
        , y1_p{y1_p}
        {
        }

    bool is_point_inside(int x_p, int y_p) const
    {
        return x_p >= std::min(x0_p, x1_p)
            && x_p <= std::max(x0_p, x1_p)
            && y_p >= std::min(y0_p, y1_p)
            && y_p <= std::max(y0_p, y1_p);
    }

    void clear()
    {
        x0_p = 0;
        y0_p = 0;
        x1_p = 0;
        y1_p = 0;
    }

    void center()
    {
        int w_p = x1_p - x0_p;
        int h_p = y1_p - y0_p;
        x0_p -= w_p / 2;
        x1_p -= w_p / 2;
        y0_p -= h_p / 2;
        y1_p -= h_p / 2;
    }
};
