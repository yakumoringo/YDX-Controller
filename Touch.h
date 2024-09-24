#include "Adafruit_USBD_CDC.h"
#include <Adafruit_I2CDevice.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_TinyUSB.h>

#define SDA_GPIO 26
#define SCL_GPIO 27

typedef struct {
    uint8_t i2c_device_indices;
    uint8_t register_indices;
    uint8_t sensitivity_values;
} TouchUnit;

TouchUnit touch_block[34] = {
    // A区
    {1, 6, 10}, {1, 2, 10}, {0, 6, 10}, {0, 2, 10},
    {3, 5, 10}, {3, 1, 10}, {2, 5, 10}, {2, 1, 10},
    // B区
    {1, 5, 10}, {1, 1, 10}, {0, 5, 10}, {0, 1, 10},
    {3, 4, 10}, {3, 0, 10}, {2, 4, 10}, {2, 0, 10},
    // C区
    {0, 7, 10}, {2, 6, 10},
    // D区
    {1, 8, 10}, {1, 4, 10}, {1, 0, 10}, {0, 4, 10}, 
    {0, 0, 10}, {3, 3, 10}, {2, 8, 10}, {2, 3, 10},
    // E区
    {1, 7, 10}, {1, 3, 10}, {0, 8, 10}, {0, 3, 10},
    {3, 6, 10}, {3, 2, 10}, {2, 7, 10}, {2, 2, 10}
};

// 设备地址
uint8_t device_addresses[] = {0x40, 0x41, 0x42, 0x43};

// 创建I2C设备对象数组
Adafruit_I2CDevice i2c_devices[4] = {
    Adafruit_I2CDevice(device_addresses[0], &Wire1),
    Adafruit_I2CDevice(device_addresses[1], &Wire1),
    Adafruit_I2CDevice(device_addresses[2], &Wire1),
    Adafruit_I2CDevice(device_addresses[3], &Wire1)
};

Adafruit_USBD_CDC TouchSer;

enum {
    commandRSET  = 0x45,
    commandHALT  = 0x4C,
    commandSTAT  = 0x41,
    commandRatio = 0x72,
    commandSens  = 0x6B,
    commandSenCheck = 0x74
};
uint8_t packet_last[6];
uint8_t packet[6];
bool Conditioning = true;

void touch_init() {
    Wire1.setSDA(SDA_GPIO);
    Wire1.setSCL(SCL_GPIO);
    Wire1.begin();
    Wire1.setClock(400000);
  
    // 初始化所有I2C设备
    for (uint8_t i = 0; i < 4; i++) {
        i2c_devices[i].begin();
    }
}

void Cy8cReset(uint8_t index) {
    Adafruit_BusIO_Register cmd_reg = Adafruit_BusIO_Register(&i2c_devices[index], 0x86, 1, LSBFIRST);
    cmd_reg.write(255);
}

void cmd_RSET() {
    for (uint8_t i = 0; i < 4; i++) {
        Cy8cReset(i);
    }
}

void cmd_HALT() {
    Conditioning = true;
}

void cmd_Ratio() {
  TouchSer.write('(');
  TouchSer.write(packet +1,4);
  TouchSer.write(')');
  Serial.write('(');
  Serial.write(packet +1,4);
  Serial.write(')');
}

void cmd_Sens() {
  TouchSer.write('(');
  TouchSer.write(packet +1,4);
  TouchSer.write(')');
  Serial.write('(');
  Serial.write(packet +1,4);
  Serial.write(')');
}

void cmd_SenCk() {
  TouchSer.write('(');
  TouchSer.write(packet +1,4);
  TouchSer.write(')');
  Serial.write('(');
  Serial.write(packet +1,4);
  Serial.write(')');
}

void cmd_STAT() {
    Conditioning = false;
}

bool reading_flag = false;
uint8_t len = 0;
void Recv() {
    
    while (TouchSer.available()) {
        uint8_t r = TouchSer.read();
        if (r == '{') {
            len = 0;
            reading_flag = true;
        }
        if (r == '}') {
            reading_flag = false;
            packet[5] = '}';
            len++;
            break;
        }
        if(reading_flag){
          packet[len] = r;
          len++;
        }
        
    }
    if (len == 6) {
        switch (packet[3]) {
            case commandRSET: cmd_RSET();Serial.println((char*)packet); break;
            case commandHALT: cmd_HALT(); Serial.println((char*)packet);break;
            case commandRatio: cmd_Ratio();Serial.println((char*)packet); break;
            case commandSens: cmd_Sens();Serial.println((char*)packet); break;
            case commandSTAT: cmd_STAT();Serial.println((char*)packet); break;
            case commandSenCheck: cmd_SenCk();Serial.println((char*)packet); break;
        }
        len = 0;
        memset(packet, 0, 6);
    }
}

uint64_t time_delta;
uint64_t TouchData = 0; 
uint64_t TouchData_Last = 0; 
void TouchSend() {
    
    uint16_t temp;
    time_delta = millis();
    // 批量读取数据
    for (int i = 0; i < 34; i++) {
        Adafruit_BusIO_Register reg = Adafruit_BusIO_Register(&i2c_devices[touch_block[i].i2c_device_indices], 
                                                              0xc2 + (11 - touch_block[i].register_indices) * 2, 2, LSBFIRST);
        temp = reg.read();
        
        if (temp > touch_block[i].sensitivity_values) {
            TouchData |= (1 << i);
        }
    }

    time_delta = millis() - time_delta;
    Serial.print("触摸：一次触摸读取时间为：");
    Serial.println(time_delta);


    if(TouchData_Last != TouchData){
      TouchSer.write('(');
      for (uint8_t r = 0; r < 7; r++) {
        TouchSer.write((uint8_t)(TouchData & 0b11111));
        TouchData >>= 5;
      }
      TouchSer.write(')');
      TouchSer.flush();
    }else{
      Serial.println("触摸：前一次扫描触摸和上一次相同，跳过输出。");
    }

    TouchData_Last = TouchData;

}

void touch_update() {
    Recv();
    if (!Conditioning) {
        TouchSend(); 
    }
}