#include <WiFi.h>
#include "DHT.h"
#include <PubSubClient.h>

#define DHTPIN 10    
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);
const char* ssid = "HUAWEI Mate 40";              //wifi名称
const char* password =  "12341234";    //wifi密码
const char* mqttServer = "67d8529e7d.st1.iotda-device.cn-north-4.myhuaweicloud.com";
const int mqttPort = 1883;
//三元组：三元组生成链接：https://iot-tool.obs-website.cn-north-4.myhuaweicloud.com/
const char* ClientId ="639bdb23e70a44473568ba3b_hw_507_0_0_2022122202";         
const char* mqttUser ="639bdb23e70a44473568ba3b_hw_507";
const char* mqttPassword = "92440e7d61b02c667a894aecd11659cdd0a53f1640c27e7bc0879ee911d5c529";
//注册设备的ID和密钥
#define device_id "639bdb23e70a44473568ba3b_hw_507" 
#define secret "XD4mpkhga7MqbkA" 
//注意修改自己的服务ID   
#define Iot_link_Body_Format "{\"services\":[{\"service_id\":\"Dev_data\",\"properties\":{%s" 
//参考上报格式：{"services":[{"service_id":"Dev_data","properties":{"temp": 39}}]}
//设备属性上报
#define Iot_link_MQTT_Topic_Report "$oc/devices/"device_id"/sys/properties/report"

//上报的温湿度值
float temperature ; 
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
  if (!client.connected()){
    MQTT_Init();
  } 
  else client.loop();
  long now = millis();
  if (now - lastMsg > 3000)//定时上报
  {
    lastMsg = now;
    MQTT_POST();
  }
}
void MQTT_Init()
{
  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) 
  {
    Serial.println("Connecting to MQTT...");
    if (client.connect(ClientId, mqttUser, mqttPassword )) 
    {
      Serial.println("connected");
    } 
    else 
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(3000);
    }
  } 
}
void MQTT_POST()
{
  char properties[32];
  char jsonBuf[128];
  //获取设备温度
  temperature = dht.readTemperature();
  Serial.println(temperature);
  //发布温度信息
  sprintf(properties,"\"temperature\":%d}}]}",(int)temperature);
  sprintf(jsonBuf,Iot_link_Body_Format,properties);
  client.publish(Iot_link_MQTT_Topic_Report, jsonBuf);
  Serial.println(Iot_link_MQTT_Topic_Report);
  Serial.println(jsonBuf);

  //获取设备湿度
  humidity = dht.readHumidity();
  Serial.println(humidity);

  //发布湿度信息
  sprintf(properties,"\"Humidity\":%d}}]}",(int)humidity);
  sprintf(jsonBuf,Iot_link_Body_Format,properties);
  client.publish(Iot_link_MQTT_Topic_Report, jsonBuf);
  Serial.println(Iot_link_MQTT_Topic_Report);
  Serial.println(jsonBuf);
  Serial.println("MQTT Publish OK!");
}