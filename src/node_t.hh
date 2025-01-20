struct node_t
{
    int x_tile = 0;
    int y_tile = 0;
    std::unique_ptr<volume_t> volume = nullptr;
    std::unique_ptr<port_t> port = nullptr;
    std::vector<node_t*> children;
    prop_table_t prop_table;
    bool is_selected = false;
    bool was_moved = false;
    double work_time_ns = 0.0;

    node_t(int x_tile, int y_tile, std::unique_ptr<volume_t>&& volume, std::unique_ptr<port_t>&& port)
        : x_tile{x_tile}
        , y_tile{y_tile}
        , volume{std::move(volume)}
        , port{std::move(port)}
        , prop_table{this->port->get_prop_table() + this->volume->get_prop_table()}
        {
        }

    void polymorph(std::unique_ptr<node_t>&& other)
    {
        volume = std::move(other->volume);
        port = std::move(other->port);
        prop_table = std::move(other->prop_table);
        /* ... and keep children as is */
    }

    void add_child(node_t* child)
    {
        /* do not add self as child */
        if(child == this)
        {
            return;
        }
        /* do not add the same child twice */
        std::vector<node_t*>::iterator iterator = find_if(
            children.begin(),
            children.end(),
            [child](node_t* node)
            {
                return child == node;
            }
        );
        if(iterator == children.end())
        {
            children.push_back(child);
        }
    }

    void iterate(std::function<bool(node_t*)> handle_node, std::function<void(node_t*, node_t*)> handle_edge)
    {
        std::queue<node_t*> queue;
        std::unordered_set<node_t*> visited;
        visited.reserve(sim_n::node_bfs_visited_capacity);
        queue.push(this);
        visited.insert(this);
        while(queue.empty() == false)
        {
            node_t* parent = queue.front();
            queue.pop();
            auto t0 = std::chrono::high_resolution_clock::now();
            if(handle_node(parent))
            {
                break;
            }
            for(node_t* child : parent->children)
            {
                handle_edge(parent, child);
                if(visited.contains(child) == false)
                {
                    queue.push(child);
                    visited.insert(child);
                }
            }
            auto t1 = std::chrono::high_resolution_clock::now();
            auto dt_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            parent->work_time_ns += dt_ns;
        }
    }

    void iterate(std::function<bool(node_t*)> handle_node) /* ease of use */
    {
        iterate(
            [handle_node](node_t* parent)
            {
                return handle_node(parent);
            },
            [](node_t*, node_t*)
            {
            }
        );
    }

    void iterate(std::function<void(node_t*, node_t*)> handle_edge) /* ease of use */
    {
        iterate(
            [](node_t*)
            {
                return false;
            },
            [handle_edge](node_t* parent, node_t* child)
            {
                handle_edge(parent, child);
            }
        );
    }
};

struct node_table_t
{
    std::vector<std::unique_ptr<node_t>> nodes;
    int x_tiles = 0;
    int y_tiles = 0;
    std::vector<piston_t*>& pistons;
    std::vector<injector_t*>& injectors;
    std::vector<rotational_mass_t*>& rotational_masses;
    std::vector<throttle_port_t*>& throttle_ports;

    node_table_t(
        int x_tiles,
        int y_tiles,
        std::vector<piston_t*>& pistons,
        std::vector<injector_t*>& injectors,
        std::vector<rotational_mass_t*>& rotational_masses,
        std::vector<throttle_port_t*>& throttle_ports)
            : x_tiles{x_tiles}
            , y_tiles{y_tiles}
            , pistons{pistons}
            , injectors{injectors}
            , rotational_masses{rotational_masses}
            , throttle_ports{throttle_ports}
            {
                nodes.resize(x_tiles * y_tiles);
            }

    void delete_observers(node_t* node)
    {
        node->volume->on_delete(pistons);
        node->volume->on_delete(injectors);
        node->volume->on_delete(rotational_masses);
        node->port->on_delete(throttle_ports);
    }

    void create_observers(node_t* node)
    {
        node->volume->on_create(pistons);
        node->volume->on_create(injectors);
        node->volume->on_create(rotational_masses);
        node->port->on_create(throttle_ports);
    }

    std::unique_ptr<node_t>& at(int x_tile, int y_tile)
    {
        return nodes[x_tile + x_tiles * y_tile];
    }

    node_t* get(int x_tile, int y_tile)
    {
        if(in_bounds(x_tile, y_tile))
        {
            return at(x_tile, y_tile).get();
        }
        else
        {
            return nullptr;
        }
    }

    void create_node(int x_tile, int y_tile, std::unique_ptr<node_t>&& node)
    {
        create_observers(node.get());
        nodes[x_tile + x_tiles * y_tile] = std::move(node);
    }

    bool in_bounds(int x_tile, int y_tile) const
    {
        return x_tile < x_tiles && x_tile >= 0 && y_tile < y_tiles && y_tile >= 0;
    }

    void move(std::unique_ptr<node_t>& node, int x_tile, int y_tile)
    {
        if(in_bounds(x_tile, y_tile))
        {
            std::unique_ptr<node_t>& to = at(x_tile, y_tile);
            to = std::move(node);
            to->x_tile = x_tile;
            to->y_tile = y_tile;
        }
    }

    void polymorph(node_t* node, std::unique_ptr<node_t>&& other)
    {
        delete_observers(node);
        create_observers(other.get());
        node->polymorph(std::move(other));
    }

    void delete_edges(node_t* node)
    {
        iterate(
            [&node](node_t* parent)
            {
                parent->children.erase(std::remove(parent->children.begin(), parent->children.end(), node), parent->children.end());
            }
        );
    }

    void delete_node(std::unique_ptr<node_t>& node)
    {
        delete_observers(node.get());
        delete_edges(node.get());
        node.reset();
    }

    void iterate(std::function<void(node_t*)> handle)
    {
        int size = nodes.size();
        for(int x = 0; x < size; x++)
        {
            if(node_t* node = nodes[x].get())
            {
                handle(node);
            }
        }
    }

    void iterate(std::function<void(std::unique_ptr<node_t>&)> handle)
    {
        int size = nodes.size();
        for(int x = 0; x < size; x++)
        {
            if(std::unique_ptr<node_t>& node = nodes[x])
            {
                handle(node);
            }
        }
    }

    void clear()
    {
        iterate(
            [this](std::unique_ptr<node_t>& node)
            {
                delete_node(node);
            }
        );
    }
};
