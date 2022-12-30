#include <WiFi.h>
#include "DHT.h"
#include <PubSubClient.h>
#include <Regexp.h>

#define DHTPIN 10
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

//WIFI 连接信息
#define WIFI_SSID "HUAWEI Mate 40"
#define WIFI_PASSWORD "12341234"

//MQTT 连接信息
#define MQTT_SERVER "3d7dc6e17b.iot-mqtts.cn-north-4.myhuaweicloud.com"
#define MQTT_PORT 1883
//三元组：三元组生成链接：https://iot-tool.obs-website.cn-north-4.myhuaweicloud.com/
#define CLIENT_ID "63a8fee2c4efcc747bd6ee06_dht11_0_0_2022122602"
#define MQTT_USER "63a8fee2c4efcc747bd6ee06_dht11"
#define MQTT_PASSWORD "e165ab9be744be09205aa06d9e3424f13d2b479146cef6c5c778ced297ef1b6f"

//注册设备的ID和密钥
#define DEVICE_ID "63a8fee2c4efcc747bd6ee06_dht11"
#define SECRET "XD4mpkhga7MqbkA"
#define SERVICE_ID "Dev_data"

//TOPIC 信息
#define IOT_LINK_BODY_FORMAT "{\"services\":[{\"service_id\":" SERVICE_ID ",\"properties\":{\"Temperature\":%d,\"Humidity\":%d }}]}"
//参考上报格式：{"services":[{"service_id":"Dev_data","properties":{"temp": 39}}]}
//设备上报属性
#define IOT_LINK_MQTT_TOPIC_GET_RESPONSE "$oc/devices/%s/sys/properties/get/response/request_id=%s"
//设备属性上报
#define IOT_LINK_MQTT_TOPIC_REPORT "$oc/devices/" DEVICE_ID "/sys/properties/report"
//平台消息下发
#define IOT_LINK_MQTT_TOPIC_DOWN "$oc/devices/" DEVICE_ID "/sys/messages/down"

//上报的温湿度值
float temperature;
float humidity;
long lastMsg = 0;

void setup() {
  Serial.begin(115200);
  //WIFI 初始化
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  //传感器初始化
  dht.begin();
  mqttInit();
}

void loop() {
  if (!client.connected()) {
    mqttInit();
  } else client.loop();
  long now = millis();
  //定时上报
  if (now - lastMsg > 3000) {
    lastMsg = now;
    mqttPost();
  }
}

//MQTT 初始化
void mqttInit() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(3000);
    }
  };
  client.subscribe(IOT_LINK_MQTT_TOPIC_DOWN);
  client.setCallback(callback);
}

//MQTT 消息回调
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  unsigned long count;
  MatchState ms(topic);
  //请求信息校验
  count = ms.GlobalMatch("\/sys\/properties\/get", topicMatchCallback);
  if (count > 0) {
    char properties[32];
    char jsonBuf[128];
    char responseTopic[128];
    //获取请求 ID
    String topicStr;
    topicStr = topic;
    int index = topicStr.indexOf('=');
    String requestId = topicStr.substring(index + 1, topicStr.length());
    //发布温湿度信息
    sprintf(responseTopic, IOT_LINK_MQTT_TOPIC_GET_RESPONSE, DEVICE_ID, requestId.c_str());
    sprintf(jsonBuf, IOT_LINK_BODY_FORMAT, (int)temperature, (int)humidity);
    Serial.println(jsonBuf);
    client.publish(responseTopic, jsonBuf);
  }
}

//MQTT 消息发布
void mqttPost() {
  char properties[32];
  char jsonBuf[128];
  //获取设备温湿度
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  //发布温湿度信息
  sprintf(jsonBuf, IOT_LINK_BODY_FORMAT, (int)temperature, (int)humidity);
  client.publish(IOT_LINK_MQTT_TOPIC_REPORT, jsonBuf);
  Serial.println(IOT_LINK_MQTT_TOPIC_REPORT);
  Serial.println(jsonBuf);
  Serial.println("MQTT Publish OK!");
}

//匹配 Topic 回调
void topicMatchCallback(const char* match,
                        const unsigned int length,
                        const MatchState& ms) {
  char cap[10];
  Serial.write((byte*)match, length);
  for (word i = 0; i < ms.level; i++) {
    Serial.print("Capture ");
    Serial.print(i, DEC);
    Serial.print(" = ");
    ms.GetCapture(cap, i);
    Serial.println(cap);
  }
}