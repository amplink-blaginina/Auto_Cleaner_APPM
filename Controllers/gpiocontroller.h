#ifndef GPIOCONTROLLER_H
#define GPIOCONTROLLER_H

#include <gpio_matrix.hpp>
#include <gpio_worker.hpp>


class GPIOController
{
public:
    GPIOController(GPIOWorker *gpio, GPIOMatrix *gpioMatrix);
    void setStarterLight(bool state);
    void setStarter(bool state);
private:
    GPIOWorker *_gpio;
    GPIOMatrix *_gpioMatirx;
};

#endif // GPIOCONTROLLER_H
