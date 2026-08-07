#include "gpiocontroller.h"

GPIOController::GPIOController(GPIOWorker *gpio, GPIOMatrix *gpioMatrix) {
    _gpio = gpio;
    _gpioMatirx = gpioMatrix;
}
void GPIOController::setStarterLight(bool state){
    _gpio->setOutput(GPIOOutput::OUT_STARTER_LIGHT, state);
}
void GPIOController::setStarter(bool state){
    _gpio->setOutput(GPIOOutput::OUT_STARTER, state);
}
