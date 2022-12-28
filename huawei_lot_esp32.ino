#include <WiFi.h>
#include "DHT.h"
#include <PubSubClient.h>

#define DHTPIN 10
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);
const char* ssid = "HUAWEI Mate 40";  //wifi名称
const char* password = "12341234";    //wifi密码
const char* mqttServer = "3d7dc6e17b.iot-mqtts.cn-north-4.myhuaweicloud.com";
const int mqttPort = 1883;
//三元组：三元组生成链接：https://iot-tool.obs-website.cn-north-4.myhuaweicloud.com/
const char* ClientId = "63a8fee2c4efcc747bd6ee06_dht11_0_0_2022122602";
const char* mqttUser = "63a8fee2c4efcc747bd6ee06_dht11";
const char* mqttPassword = "e165ab9be744be09205aa06d9e3424f13d2b479146cef6c5c778ced297ef1b6f";
//注册设备的ID和密钥
#define device_id "63a8fee2c4efcc747bd6ee06_dht11"
#define secret "XD4mpkhga7MqbkA"
//注意修改自己的服务ID
#define Iot_link_Body_Format "{\"services\":[{\"service_id\":\"Dev_data\",\"properties\":{%s"
//参考上报格式：{"services":[{"service_id":"Dev_data","properties":{"temp": 39}}]}
//平台查询设备属性
#define Iot_link_MQTT_Topic_Get "$oc/devices/" device_id "/sys/properties/get"
//设备属性上报
#define Iot_link_MQTT_Topic_Report "$oc/devices/" device_id "/sys/properties/report"
//平台消息下发
#define Iot_link_MQTT_Topic_Down "$oc/devices/" device_id "/sys/messages/down"

//上报的温湿度值
float temperature;
float humidity;

long lastMsg = 0;
void setup() {
  //wifi初始化
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  //传感器初始化
  dht.begin();
  //MQTT初始化
  MQTT_Init();
}

void loop() {
  if (!client.connected()) {
    MQTT_Init();
  } else client.loop();
  long now = millis();
  if (now - lastMsg > 3000)  //定时上报
  {
    lastMsg = now;
    MQTT_POST();
  }
}
void MQTT_Init() {
  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(ClientId, mqttUser, mqttPassword)) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(3000);
    }
  };
  client.subscribe(Iot_link_MQTT_Topic_Down);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");
  char properties[32];
  char jsonBuf[128];
  String split_result[10];
  //获取设备温湿度
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  //发布温度信息
  sprintf(properties, "\"Temperature\":%d}}]}", (int)temperature);
  sprintf(jsonBuf, Iot_link_Body_Format, properties);
  
  Split(opic, '/', split_result);
  char request_id = split_result[split_result.length - 1];
  char response_topic = "$oc/devices/{device_id}/sys/properties/get/response/request_id=" request_id "";

  client.publish(response_topic, jsonBuf);

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
}

//字符串分割
void Split(String str, String character, String result[]) {
  int data;
  String temps;
  int i = 0;
  do {
    data = str.indexOf(character);
    if (data != -1) {
      temps = str.substring(0, data);
      str = str.substring(data + character.length(), str.length());

    } else {
      if (str.length() > 0)
        temps = str;
    }
    result[i++] = temps;
    temps = "";
  } while (data >= 0);
}

void MQTT_POST() {
  char properties[32];
  char jsonBuf[128];
  //获取设备温度
  temperature = dht.readTemperature();
  Serial.println(temperature);
  //发布温度信息
  sprintf(properties, "\"Temperature\":%d}}]}", (int)temperature);
  sprintf(jsonBuf, Iot_link_Body_Format, properties);
  client.publish(Iot_link_MQTT_Topic_Report, jsonBuf);
  Serial.println(Iot_link_MQTT_Topic_Report);
  Serial.println(jsonBuf);

  //获取设备湿度
  humidity = dht.readHumidity();
  Serial.println(humidity);

  //发布湿度信息
  sprintf(properties, "\"Humidity\":%d}}]}", (int)humidity);
  sprintf(jsonBuf, Iot_link_Body_Format, properties);
  client.publish(Iot_link_MQTT_Topic_Report, jsonBuf);
  Serial.println(Iot_link_MQTT_Topic_Report);
  Serial.println(jsonBuf);
  Serial.println("MQTT Publish OK!");
}