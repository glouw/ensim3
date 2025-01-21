/* digit ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
 * letter ::= "a" | "b" | "c" | ... | "z" | "A" | "B" | "C" | ... | "Z" | "_"
 * identifier ::= letter { letter | digit | "_" }
 * number ::= digit { digit } [ "." digit { digit } ]
 * factor ::= number | identifier | "(" expression ")" | "-" factor
 * term ::= factor { ("*" | "%" | "/") factor }
 * expression ::= term { ("+" | "-") term }
 */

struct expression_parser_t
{
    int at = 0;
    std::function<double(std::string)> lookup = nullptr;

    expression_parser_t(std::function<double(std::string)> lookup)
        : lookup{lookup}
        {
        }

    void spin(const std::string& s)
    {
        while(isspace(s[at]))
        {
            at++;
        }
    }

    char peek(const std::string& s)
    {
        spin(s);
        return s[at];
    }

    char read(const std::string& s)
    {
        char c = peek(s);
        at++;
        return c;
    }

    double number(const std::string& s)
    {
        std::string d = "";
        while(std::isdigit(peek(s)) or peek(s) == '.')
        {
            d += read(s);
        }
        return std::stod(d);
    }

    std::string identifier(const std::string& s)
    {
        std::string l = "";
        while(std::isalnum(peek(s)) or peek(s) == '_')
        {
            l += read(s);
        }
        return l;
    }

    double factor(const std::string& s)
    {
        char c = peek(s);
        if(std::isalpha(c))
        {
            std::string ident = identifier(s);
            if(ident == "pi")
            {
                return M_PI;
            }
            else
            if(ident == "otto")
            {
                return sim_n::four_stroke_r;
            }
            else
            if(ident == "k")
            {
                return thermofluidics_n::stp_static_temperature_k;
            }
            else
            if(ident == "atm")
            {
                return thermofluidics_n::ntp_static_pressure_pa;
            }
            else
            if(lookup)
            {
                return lookup(ident);
            }
            else
            {
                return 0.0;
            }
        }
        else
        if(c == '-')
        {
            read(s);
            return -factor(s);
        }
        else
        if(c == '(')
        {
            read(s);
            double val = expression(s);
            read(s);
            return val;
        }
        else
            return number(s);
    }

    double term(const std::string& s)
    {
        double a = factor(s);
        while(peek(s) == '*' or peek(s) == '%' or peek(s) == '/')
        {
            char op = read(s);
            double b = factor(s);
            if(op == '*')
            {
                a *= b;
            }
            else
            if(op == '%')
            {
                a = std::fmod(a, b);
            }
            else
            if(op == '/')
            {
                a /= b;
            }
        }
        return a;
    }

    double expression(const std::string& s)
    {
        double a = term(s);
        while(peek(s) == '-' or peek(s) == '+')
        {
            char op = read(s);
            double b = term(s);
            if(op == '-')
            {
                a -= b;
            }
            else
            if(op == '+')
            {
                a += b;
            }
        }
        return a;
    }

    double parse(const std::string& s)
    {
        at = 0;
        try
        {
            return expression(s);
        }
        catch(...)
        {
            return 0.0;
        }
    }
};

