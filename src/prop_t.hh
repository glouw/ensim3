struct prop_t
{
    using real_t = std::variant<double*, int*, bool*, std::string*>;
    std::string key = "";
    std::string value = "";
    real_t real;

    prop_t(const std::string& key, const real_t& real)
        : key{key}
        , real{real}
        {
            sync();
        }

    void sync()
    {
        if(std::holds_alternative<double*>(real))
        {
            std::ostringstream stream;
            double double_value = *std::get<double*>(real);
            double precision = double_value == 0.0 ? 1 : ui_n::prop_table_double_precision;
            value = double_to_string(double_value, precision);
        }
        else
        if(std::holds_alternative<int*>(real))
        {
            value = std::to_string(*std::get<int*>(real));
        }
        else
        if(std::holds_alternative<bool*>(real))
        {
            value = *std::get<bool*>(real) ? "true" : "false";
        }
        else
        if(std::holds_alternative<std::string*>(real))
        {
            value = *std::get<std::string*>(real);
        }
    }

    void commit(std::function<double(std::string)> lookup = nullptr)
    {
        if(std::holds_alternative<double*>(real))
        {
            expression_parser_t parser{lookup};
            *std::get<double*>(real) = parser.parse(value);
        }
        else
        if(std::holds_alternative<int*>(real))
        {
            expression_parser_t parser{lookup};
            *std::get<int*>(real) = parser.parse(value);
        }
        else
        if(std::holds_alternative<bool*>(real))
        {
            if(value == "true")
            {
                *std::get<bool*>(real) = true;
            }
            else
            if(value == "false")
            {
                *std::get<bool*>(real) = false;
            }
        }
        else
        if(std::holds_alternative<std::string*>(real))
        {
            *std::get<std::string*>(real) = value;
        }
        sync();
    }
};

struct prop_table_t
{
    std::vector<prop_t> table;

    prop_table_t() = default;

    prop_table_t(const std::initializer_list<prop_t>& init)
        : table{init}
        {
        }

    int last() const
    {
        int size = table.size() - 1;
        return size;
    }

    prop_t* at(int index)
    {
        return &table[index];
    }

    std::optional<prop_t::real_t> find(const std::string& key)
    {
        for(prop_t& prop : table)
        {
            if(prop.key == key)
            {
                return prop.real;
            }
        }
        return std::nullopt;
    }

    void set_prop(const std::string& key, const std::string& value)
    {
        for(prop_t& prop : table)
        {
            if(prop.key == key)
            {
                prop.value = value;
                prop.commit();
                break;
            }
        }
    }

    void sync()
    {
        for(prop_t& prop : table)
        {
            prop.sync();
        }
    }

    bool equals(const prop_table_t& other) const
    {
        if(table.size() != other.table.size())
        {
            return false;
        }
        return std::equal(
            table.begin(), table.end(), other.table.begin(),
            [](const prop_t& a, const prop_t& b)
            {
                return a.key == b.key;
            }
        );
    }

    prop_table_t operator+(const prop_table_t& other)
    {
        prop_table_t sum;
        sum.table.insert(sum.table.end(), table.begin(), table.end());
        sum.table.insert(sum.table.end(), other.table.begin(), other.table.end());
        return sum;
    }
};

struct has_prop_table_t
{
    virtual prop_table_t get_prop_table() = 0;
    virtual ~has_prop_table_t() = default;
};
