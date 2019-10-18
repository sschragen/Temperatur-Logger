#include <Arduino.h>
#include <Task.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Schragen2.4"; // Enter your WiFi name
const char* password =  "warpdrive"; // Enter WiFi password
const char* mqttServer = "prodesk.fritz.box";
const int   mqttPort = 37181;
const char* mqttUser = "otfxknod";
const char* mqttPassword = "nSuUc1dDLygF";

WiFiClient espClient;
PubSubClient client(espClient);

TaskManager taskManager;
void TemperaturMessen(uint32_t deltaTime); 
FunctionTask taskTemperaturMessen(TemperaturMessen, MsToTaskTime(10000)); // turn on the led in 400ms

void TemperaturMessen(uint32_t deltaTime)
{
    // temperaturen messen

    // per mqtt weiterreichen
    client.publish("esp/test", "hello"); //Topic name
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqttServer, mqttPort);
  //client.setCallback(callback);

  while (!client.connected()) 
  {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) 
    {
      Serial.println("connected"); 
    } 
    else 
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  taskManager.StartTask(&taskTemperaturMessen); // start with turning it on

  delay(5000);
};

void loop() 
{
  // put your main code here, to run repeatedly:
   taskManager.Loop();
}