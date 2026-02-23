#ifndef MIC_MANAGER_H
#define MIC_MANAGER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include "Config.h" // Ensures SAMPLES_PER_BUFFER and I2S_PORT are known

class MicManager {
public:
    MicManager() {} // Constructor

    void initMic();
    int readMicData();

    // Fixes the 'inference_buffer' was not declared error
    int16_t inference_buffer[SAMPLES_PER_BUFFER]; 
};

#endif