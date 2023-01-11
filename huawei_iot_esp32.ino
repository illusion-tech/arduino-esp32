#include "DHT.h"
#include <PubSubClient.h>
#include <Regexp.h>

#define DHTPIN 10
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// 硬串口
HardwareSerial espSerial(1);

String comdata = "";

// MQTT 连接信息
#define MQTT_SERVER "3d7dc6e17b.iot-mqtts.cn-north-4.myhuaweicloud.com"
#define MQTT_PORT 1883
// 三元组：三元组生成链接：https://iot-tool.obs-website.cn-north-4.myhuaweicloud.com/
#define CLIENT_ID "63a8fee2c4efcc747bd6ee06_dht11_0_0_2022122602"
#define MQTT_USER "63a8fee2c4efcc747bd6ee06_dht11"
#define MQTT_PASSWORD "e165ab9be744be09205aa06d9e3424f13d2b479146cef6c5c778ced297ef1b6f"

// 注册设备的ID和密钥
#define DEVICE_ID "63a8fee2c4efcc747bd6ee06_dht11"
#define SECRET "XD4mpkhga7MqbkA"
#define SERVICE_ID "Dev_data"

// TOPIC 信息
#define IOT_LINK_BODY_FORMAT "{\"services\":[{\"service_id\":\"" SERVICE_ID "\",\"properties\":{\"Temperature\":%d,\"Humidity\":%d }}]}"
// 参考上报格式：{"services":[{"service_id":"Dev_data","properties":{"temp": 39}}]}
// 设备属性上报
#define IOT_LINK_MQTT_TOPIC_REPORT "$oc/devices/" DEVICE_ID "/sys/properties/report"

// 上报的温湿度值
float temperature;
float humidity;
long lastMsg = 0;

void setup()
{
    Serial.begin(115200);
    // 串口的开启，传入下面四个参数：波特率，默认SERIAL_8N1为8位数据位、无校验、1位停止位，后面两个分别为 RXD,TXD 引脚
    espSerial.begin(115200, SERIAL_8N1, 33, 34);
    // 传感器初始化
    dht.begin();
    mqttInit();
}

void loop()
{
    // Serial.available()判断串口的缓冲区有无数据，当Serial.available()>0时，说明串口接收到了数据，可以读取
    if (espSerial.available())
    {
        Serial.write(espSerial.read());
    }
    if (Serial.available())
    {
        espSerial.write(Serial.read());
    }
    long now = millis();
    // 定时上报
    if (now - lastMsg > 5000)
    {
        lastMsg = now;
        mqttPost();
    }
}

// MQTT 初始化
void mqttInit()
{
    // 确认 AT 指令开启
    espSerial.print("AT\r\n");
    delay(50);
    // 关闭之前的连接
    espSerial.println("AT+QMTCLOSE=0");
    delay(50);
    // 配置接收模式
    espSerial.print("AT+QMTCFG=\"recv\/mode\",0,0,1\r\n");
    delay(50);
    // 配置MQTT协议版本（ 版本为 3.1.1 ）
    espSerial.print("AT+QMTCFG=\"version\",0,4\r\n");
    delay(50);
    // 设置并打开MQTT客户端：AT+QMTOPEN=<client_idx>,<host_name>,<port>
    espSerial.printf("AT+QMTOPEN=0,%s,1883\r\n", MQTT_SERVER);
    delay(50);
    // 设置客户端连接：AT+QMTCONN=<client_idx>,<clientid>,<username>,<password>
    espSerial.printf("AT+QMTCONN=0,%s,%s,%s\r\n", CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
    delay(50);
}

// MQTT 消息发布
void mqttPost()
{
    char properties[32];
    char jsonBuf[128];
    // 获取设备温湿度
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    // 发布温湿度信息 AT指令：AT+QMTPUBEX=<client_idx>,<msgid>,<qos>,<retain>,<topic>,<length>
    sprintf(jsonBuf, IOT_LINK_BODY_FORMAT, (int)temperature, (int)humidity);
    espSerial.printf("AT+QMTPUBEX=0,0,0,0,\"%s\",%d\r\n", IOT_LINK_MQTT_TOPIC_REPORT, sizeof(jsonBuf));
    delay(50);
    espSerial.println(jsonBuf);
    Serial.println("MQTT Publish OK!");
}
