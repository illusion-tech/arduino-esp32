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
// 设备上报属性
#define IOT_LINK_MQTT_TOPIC_GET_RESPONSE "$oc/devices/%s/sys/properties/get/response/request_id=%s"
// 设备属性上报
#define IOT_LINK_MQTT_TOPIC_REPORT "$oc/devices/" DEVICE_ID "/sys/properties/report"
// 平台消息下发
#define IOT_LINK_MQTT_TOPIC_DOWN "$oc/devices/" DEVICE_ID "/sys/messages/down"

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
    long now = millis();
    // 定时上报
    if (now - lastMsg > 3000)
    {
        lastMsg = now;
        mqttPost();
    }
}

// MQTT 初始化
void mqttInit()
{
    // 确认 AT 指令开启
    espSerial.println("AT");
    // 配置接收模式
    espSerial.println("AT+QMTCFG=\"recv\/mode\",0,0,1");
    // 配置华为云设备信息：AT+QMTCFG="hwauth",<client_idx>[,<product id>,<device secret>]
    espSerial.println("AT+QMTCFG=\"hwauth\",\"63a8fee2c4efcc747bd6ee06\",\"5d2f04c861424507e4c6191abc9c5275\"");
    // 设置并打开MQTT客户端：AT+QMTOPEN=<client_idx>,<host_name>,<port>
    espSerial.println("AT+QMTOPEN=0,\"3d7dc6e17b.iot-mqtts.cn-north-4.myhuaweicloud.com\",1883");
    // 设置客户端连接：AT+QMTCONN=<client_idx>,<clientid>,<username>,<password>
    espSerial.println("AT+QMTCONN=0,\"63a8fee2c4efcc747bd6ee06_EC600N_0_0_2023010507\",\"63a8fee2c4efcc747bd6ee06_EC600N\",\"98508a6cb86d7d17e74a6a46d47c421b9d3ad1d0d22f257f4aab4493b1c90e30\"");
}

// MQTT 消息回调
void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.println("] ");

    unsigned long count;
    MatchState ms(topic);
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
    espSerial.println("AT+QMTPUBEX=0,0,0,0," IOT_LINK_MQTT_TOPIC_REPORT ",300," jsonBuf "");

    // client.publish(IOT_LINK_MQTT_TOPIC_REPORT, jsonBuf);
    // Serial.println(IOT_LINK_MQTT_TOPIC_REPORT);
    // Serial.println(jsonBuf);
    // Serial.println("MQTT Publish OK!");
}

// 匹配 Topic 回调
void topicMatchCallback(const char *match,
                        const unsigned int length,
                        const MatchState &ms)
{
    char cap[10];
    Serial.write((byte *)match, length);
    for (word i = 0; i < ms.level; i++)
    {
        Serial.print("Capture ");
        Serial.print(i, DEC);
        Serial.print(" = ");
        ms.GetCapture(cap, i);
        Serial.println(cap);
    }
}