#include "DHT.h"
#include <PubSubClient.h>
#include <Regexp.h>

#define DHTPIN 10
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// 硬串口
HardwareSerial espSerial(1);

// MQTT 连接信息
#define MQTT_SERVER "3d7dc6e17b.iot-mqtts.cn-north-4.myhuaweicloud.com"
#define MQTT_PORT 1883
// 三元组：三元组生成链接：https://iot-tool.obs-website.cn-north-4.myhuaweicloud.com/
// 三元组信息
#define CLIENT_ID "63b687c5b7768d66eb705b98_0001_0_0_2023010608"
#define MQTT_USER "63b687c5b7768d66eb705b98_0001"
#define MQTT_PASSWORD "9611e6c421c4f279d7badd97b020000e622a6998c2b4b6b6dedafdf857ef155e"

// 注册设备的ID和密钥
#define DEVICE_ID "63b687c5b7768d66eb705b98_0001"
#define SECRET "faceb85bd48fa756850dab956d0f2f1f"
// 服务ID
#define SERVICE_ID "service01"

// TOPIC 信息（Temperature，Humidity 为服务的两个属性名）
// TOPIC 信息
#define IOT_LINK_BODY_FORMAT "{\"services\":[{\"service_id\":\"" SERVICE_ID "\",\"properties\":{\"voltage\":%d,\"charge_current\":%d,\"discharge_current\":%d }}]}"
// 参考上报格式：{"services":[{"service_id":"Dev_data","properties":{"temp": 39}}]}
// 设备属性上报
#define IOT_LINK_MQTT_TOPIC_REPORT "$oc/devices/" DEVICE_ID "/sys/properties/report"
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
String comdata = "";
char requestId[10];

void setup()
{
    Serial.begin(115200);
    // 串口的开启，传入下面四个参数：波特率，默认SERIAL_8N1为8位数据位、无校验、1位停止位，后面两个分别为 RXD,TXD 引脚
    espSerial.begin(115200, SERIAL_8N1, 33, 34);
    // 传感器初始化
    dht.begin();
    mqttInit();
    // 订阅消息接收
    espSerial.printf("AT+QMTSUB=0,1,\"%s\",0\r\n", IOT_LINK_MQTT_TOPIC_DOWN);
}

void loop()
{
    // Serial.available()判断串口的缓冲区有无数据，当Serial.available()>0时，说明串口接收到了数据，可以读取
    if (espSerial.available())
    {
        comdata = espSerial.readString();
    }
    if (comdata.length() > 0)
    {
        callback(comdata);
        Serial.print(comdata); // 打印接收到的字符
        comdata = "";
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
        // mqttPost();
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
    delay(100);
}

// MQTT 消息发布
void mqttPost()
{
    char properties[32];
    char jsonBuf[128];
    // 获取设备温湿度
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    // 发布温湿度信息 AT指令：AT+QMTPUBEX=<client_idx>,<msgid>,<qos>,<retain>,<topic>,<length>，length 为消息字节数
    // 消息内容格式化
    sprintf(jsonBuf, IOT_LINK_BODY_FORMAT, 3, 1, 0);
    int msgSize = sizeof(jsonBuf);
    espSerial.printf("AT+QMTPUBEX=0,0,0,0,\"%s\",%d\r\n", IOT_LINK_MQTT_TOPIC_REPORT, msgSize);
    delay(50);
    espSerial.println(jsonBuf);
    espSerial.println("                                 ");
    Serial.println("MQTT Publish OK!");
}

// MQTT 消息回调
void callback(String topic)
{
    Serial.printf("Message arrived [%s]", const_cast<char *>(topic.c_str()));

    unsigned long count;
    MatchState ms(const_cast<char *>(topic.c_str()));
    // 请求信息校验
    count = ms.GlobalMatch("\/sys\/properties\/get\/request_id", topicMatchCallback);
    if (count > 0)
    {
        char properties[32];
        char jsonBuf[128];
        char responseTopic[128];
        // 获取请求 ID
        int index = topic.indexOf('=');
        String request = topic.substring(index + 1, topic.length());
        int index0 = request.indexOf('"');
        String requestId = request.substring(0, index0);
        // 发布温湿度信息
        sprintf(jsonBuf, IOT_LINK_BODY_FORMAT, 3, 1, 0);
        sprintf(responseTopic, IOT_LINK_MQTT_TOPIC_GET_RESPONSE, DEVICE_ID, requestId.c_str());
        int msgSize = sizeof(jsonBuf);
        espSerial.printf("AT+QMTPUBEX=0,0,0,0,\"%s\",%d\r\n", responseTopic, msgSize);
        delay(50);
        espSerial.println(jsonBuf);
    }
}

// 正则匹配 Topic 回调
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
        Serial.printf(cap);
    }
}
