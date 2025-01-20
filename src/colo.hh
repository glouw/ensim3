enum class colo_t
: uint32_t
{
    black          = 0x000000,
    white          = 0xFFFFFF,
    blue           = 0x005F99,
    red            = 0xD93A3A,
    grey           = 0x2E2E2E,
    green          = 0x3C9D5A,
    yellow         = 0xF1C232,
    cyan           = 0x00A6A6,
    bright_red     = 0xFF4C4C,
    bright_green   = 0x4CFF4C,
    bright_blue    = 0x4C4CFF,
    bright_yellow  = 0xFFFF4C,
    bright_cyan    = 0x4CFFFF,
    bright_magenta = 0xFF4CFF,
    bright_orange  = 0xFFA64C,
    bright_pink    = 0xFF4CA6,
};

colo_t mix_colos(colo_t colo1, colo_t colo2, double ratio)
{
    uint32_t c1 = static_cast<uint32_t>(colo1);
    uint32_t c2 = static_cast<uint32_t>(colo2);
    uint8_t r1 = (c1 >> 16) & 0xFF;
    uint8_t g1 = (c1 >> 8) & 0xFF;
    uint8_t b1 = c1 & 0xFF;
    uint8_t r2 = (c2 >> 16) & 0xFF;
    uint8_t g2 = (c2 >> 8) & 0xFF;
    uint8_t b2 = c2 & 0xFF;
    uint8_t r = r1 * (1.0 - ratio) + r2 * ratio;
    uint8_t g = g1 * (1.0 - ratio) + g2 * ratio;
    uint8_t b = b1 * (1.0 - ratio) + b2 * ratio;
    uint32_t mixed_color = (r << 16) | (g << 8) | b;
    return static_cast<colo_t>(mixed_color);
}

struct flashing_colo_t
{
    int flip_tick = ui::info_render_ticks;
    colo_t on = colo_t::red;
    colo_t off = colo_t::white;
    colo_t colo = off;

    flashing_colo_t(colo_t on, colo_t off)
        : on{on}
        , off{off}
        {
        }

    void tick(int render_ticks)
    {
        if(render_ticks % flip_tick == 0)
        {
            colo = (colo == on) ? off : on;
        }
    }
};
