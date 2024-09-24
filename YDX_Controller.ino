#include "Touch.h"


void serial_print() {
    Serial.print("Touch Serial Com:");
    Serial.println(TouchSer);
}

void setup() {

    Adafruit_USBD_Device device; // 初始化USB设备
    device.begin(); // 启动USB设备

    TouchSer.begin(9600); // 启动 TouchSer
    TouchSer.setTimeout(0);

    Serial.begin(115200);
    //while (!Serial); // 等待串口连接

    // check to see if multiple CDCs are enabled
    if (CFG_TUD_CDC < 2) {
        while (1) {
            Serial.printf("CFG_TUD_CDC must be at least 2, current value is %u\n", CFG_TUD_CDC);
            Serial.println("  Config file is located in Adafruit_TinyUSB_Arduino/src/arduino/ports/{platform}/tusb_config_{platform}.h");
            Serial.println("  where platform is one of: nrf, rp2040, samd");
            delay(1000);
        }
    }

    touch_init();



    


    
}

void loop() {
    touch_update();
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime >= 1000) {
        lastPrintTime = millis();
        serial_print();
    }
}