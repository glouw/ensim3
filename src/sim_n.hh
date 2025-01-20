namespace sim_n
{
    const std::string title = "ensim3";
    const int cycles_per_frame = 1024;
    const int impulse_size = 8192;
    const int render_demo_delay_per_edge_ms = 32;
    const double sample_frequency_hz = 44100.0;
    const double dt_s = 1.0 / sample_frequency_hz;
    const int node_bfs_visited_capacity = 32;
    const double four_stroke_r = 4.0 * M_PI;
}
