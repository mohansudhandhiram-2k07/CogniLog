#include "MicManager.h"

void MicManager::initMic() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, 
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
    
    Serial.println("I2S Microphone Initialized.");
}

int MicManager::readMicData() {
    size_t bytes_read;
    int32_t raw_samples[SAMPLES_PER_BUFFER]; 

    // Use a small timeout (10ms) to ensure we wait for a full buffer
    i2s_read(I2S_PORT, raw_samples, sizeof(raw_samples), &bytes_read, 10 / portTICK_PERIOD_MS);
    int samples_count = bytes_read / sizeof(int32_t);

    for (int i = 0; i < samples_count; i++) {
        // ELITE FIX: Shift by 14 and cast. 
        // This removes the DC offset buzz and aligns 24-bit to 16-bit.
        int32_t processed = raw_samples[i] >> 14;
        
        // Manual Clipping Guard
        if (processed > 32767) processed = 32767;
        if (processed < -32768) processed = -32768;
        
        inference_buffer[i] = (int16_t)processed;
    }
    return samples_count;
}
