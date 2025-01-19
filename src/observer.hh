#pragma once

struct piston_t;
struct rotational_mass_t;
struct throttle_port_t;
struct injector_t;

struct observable_t
{
    template <typename T>
    void subscribe_to(std::vector<T*>& observers, T* observer)
    {
        observers.push_back(observer);
    }

    template <typename T>
    void delete_from(std::vector<T*>& observers, T* observer)
    {
        observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
    }

    virtual void on_create(std::vector<piston_t*>&) {}
    virtual void on_delete(std::vector<piston_t*>&) {}
    virtual void on_create(std::vector<rotational_mass_t*>&) {}
    virtual void on_delete(std::vector<rotational_mass_t*>&) {}
    virtual void on_create(std::vector<throttle_port_t*>&) {}
    virtual void on_delete(std::vector<throttle_port_t*>&) {}
    virtual void on_create(std::vector<injector_t*>&) {}
    virtual void on_delete(std::vector<injector_t*>&) {}

    virtual ~observable_t() = default;
};
