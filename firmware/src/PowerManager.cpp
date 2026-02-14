#include "PowerManager.h"

void enterDeepSleep(Adafruit_SSD1306 &display) {
    Serial.println("[POWER]: Deep Sleep Entry...");
    display.clearDisplay();
    display.setCursor(20, 30);
    display.println("SLEEPING...");
    display.display();
    delay(500);

    display.ssd1306_command(SSD1306_DISPLAYOFF);

    rtc_gpio_init(GPIO_NUM_27);
    rtc_gpio_set_direction(GPIO_NUM_27, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(GPIO_NUM_27);
    rtc_gpio_pulldown_dis(GPIO_NUM_27);

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);
    esp_deep_sleep_start();
}