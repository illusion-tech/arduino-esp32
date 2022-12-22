#include <Wire.h>
#include <WiFi.h>
#include "DHT.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define DHTPIN 10    
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

/*MQTT连接配置*/
/*-----------------------------------------------------*/
const char* ssid = "ESP32连接的WiFi名称";
const char* password = "WiFi密码";
const char* mqttServer = "华为云MQTT接入地址";
const int   mqttPort = 1883;
//以下3个参数可以由HMACSHA256算法生成，为硬件通过MQTT协议接入华为云IoT平台的鉴权依据
const char* clientId = "";
const char* mqttUser = "";
const char* mqttPassword = "";
 
WiFiClient espClient; //ESP32WiFi模型定义
PubSubClient client(espClient);
 
const char* topic_properties_report = "属性上报topic";
 
//接收到命令后上发的响应topic

char* topic_Commands_Response = "$oc/devices/639bdb23e70a44473568ba3b_hw_507/sys/commands/response/request_id=";
 
 
/*******************************************************/
/*
 * 作用：  ESP32的WiFi初始化以及与MQTT服务器的连接
 * 参数：  无
 * 返回值：无
 */
void MQTT_Init()
{
//WiFi网络连接部分
  WiFi.begin(ssid, password); //开启ESP32的WiFi
  while (WiFi.status() != WL_CONNECTED) { //ESP尝试连接到WiFi网络
    delay(3000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to the WiFi network");
 
 
//MQTT服务器连接部分
  client.setServer(mqttServer, mqttPort); //设置连接到MQTT服务器的参数
 
  client.setKeepAlive (60); //设置心跳时间
 
  while (!client.connected()) { //尝试与MQTT服务器建立连接
    Serial.println("Connecting to MQTT...");
  
    if (client.connect(clientId, mqttUser, mqttPassword )) {
  
      Serial.println("connected");  
  
    } else {
  
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(6000);
  
    }
  }
 
 
//接受平台下发内容的初始化
  client.setCallback(callback); //可以接受任何平台下发的内容
 
}
