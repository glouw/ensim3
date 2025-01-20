namespace thermofluidics_n
{
    /* todo: one day add different types that will change these marked values */
    const double air_fuel_stoich_ratio = 14.7; /* fuel */
    const double fuel_lower_heating_value_j_per_kg = 44e6; /* fuel */
    const double autoignition_constant_a = 1200.0; /* fuel */
    const double autoignition_constant_b = 0.5; /* fuel */
    const double ntp_static_pressure_pa = 101325.0;
    const double ntp_static_temperature_k = 293.15;
    const double stp_static_temperature_k = 273.15;
    const double molar_mass_combusted_kg_per_mol = 0.02885;
    const double molar_mass_air_kg_per_mol = 0.0289647;
    const double molar_mass_fuel_kg_per_mol = 0.11423; /* fuel */
    const double gamma_fuel = 1.15; /* fuel */
    const double gamma_combusted = 1.3; /* fuel */
    const double gamma_air = 1.4;
    const double r_j_per_mol_k = 8.314;
}
