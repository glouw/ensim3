#pragma once

struct pid_controller_t
{
    double kp = 0.0;
    double ki = 0.0; /* todo: deal with integral windup? */
    double kd = 0.0;
    double scale = 0.0;
    double previous_error = 0.0;
    double integral = 0.0;

    pid_controller_t(double kp, double ki, double kd, double scale)
        : kp{kp}
        , ki{ki}
        , kd{kd}
        , scale{scale}
        {
        }

    double compute(double setpoint, double actual)
    {
        double error = setpoint - actual;
        integral += error;
        double derivative = error - previous_error;
        previous_error = error;
        return (kp / scale) * error + (ki / scale) * integral + (kd / scale) * derivative;
    }
};
