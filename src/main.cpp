#include <Arduino.h>
//#include <TaskScheduler.h>
#include <Task.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// aktuell

const char* ssid = "Schragen2.4";               // your WiFi name
const char* password =  "warpdrive";            // your WiFi password
const char* mqttServer = "prodesk.fritz.box";   // Adresse vom MQTT Server
const int   mqttPort = 1883;                    // Port vom MQTT Server
const char* mqttUser = "TempLogger";            // MQTT Anmeldename
const char* mqttPassword = "none";              // MQTT Passwort

WiFiClient espClient;
PubSubClient client(espClient);
#define MQTT_TOPIC_VORLAUF "TempLogger/Vorlauf"
#define MQTT_TOPIC_RUECKLAUF "TempLogger/Ruecklauf"
#define MQTT_TOPIC_UPDATE "TempLogger/Update"

TaskManager taskManager;
void TemperaturMessen(uint32_t deltaTime); 
FunctionTask taskTemperaturMessen(TemperaturMessen, MsToTaskTime(10000)); // turn on the led in 400ms

#define ONE_WIRE_BUS 16                   // connected pin
#define TEMPERATURE_PRECISION 12          // 12 Bit Resolution
int numberOfDevices;                      // Anzahl an Sensoren
OneWire *oneWire;                         // 
DallasTemperature *sensors;               //
DeviceAddress tempDeviceAddress;          // We'll use this variable to store a found device address

void TemperaturMessen(uint32_t deltaTime)
{
    // temperaturen messen
    sensors->requestTemperatures(); // Send the command to get temperatures

    sensors->getAddress(tempDeviceAddress, 0);
    float vorlauf = sensors->getTempC(tempDeviceAddress);
    sensors->getAddress(tempDeviceAddress, 1);
    float ruecklauf = sensors->getTempC(tempDeviceAddress); 

    // per mqtt weiterreichen
    client.publish(MQTT_TOPIC_VORLAUF  , String  (vorlauf,'2').c_str()); 
    client.publish(MQTT_TOPIC_RUECKLAUF, String(ruecklauf,'2').c_str());
    Serial.println ("Temp 1 : "+String(vorlauf,'2')+"°C");
    Serial.println ("Temp 2 : "+String(vorlauf,'2')+"°C");
}

void setup_WIFI ()
{
  Serial.print("Connecting");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
};

void setup_MQTT ()
{
  client.setServer(mqttServer, mqttPort);

  while (!client.connected()) 
  {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) 
    {
      Serial.println("connected");
      client.publish("TempLogger/IP", WiFi.localIP().toString().c_str()); 
    } 
    else 
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
};

void setup_SENSOREN ()
{
  oneWire = new OneWire(ONE_WIRE_BUS);
  sensors = new DallasTemperature(oneWire);
  sensors->setResolution (TEMPERATURE_PRECISION);

  numberOfDevices = sensors->getDeviceCount();
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");    

  for(int i=0;i<numberOfDevices; i++)
  {
      // Search the wire for address
      if(sensors->getAddress(tempDeviceAddress, i))
      {
          Serial.print("Found device ");
          Serial.print(i, DEC);
          Serial.print(" with address: ");
          for (uint8_t i = 0; i < 8; i++)
          {
            if (tempDeviceAddress[i] < 16) 
              Serial.print("0");
            Serial.print(tempDeviceAddress[i], HEX);
          }
          Serial.println();
      } 
      else 
      {
          Serial.print("Found ghost device at ");
          Serial.print(i, DEC);
          Serial.print(" but could not detect address. Check power and cabling");
      }
    }
};

void setup_OTA ()
{
  // OTA Settings
 
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
 
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("TempLogger"); 
 
  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"1234");

  ArduinoOTA.onStart([]() 
  {
    Serial.println("Start");
    client.publish(MQTT_TOPIC_UPDATE  , "gestartet ..."); 
  });

  ArduinoOTA.onEnd([]() 
  {
    Serial.println("\nEnd");
    client.publish(MQTT_TOPIC_UPDATE  , "beendet.");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
  {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    Serial.println();
    client.publish(MQTT_TOPIC_UPDATE  , String  (progress / (total / 100),'2').c_str()); 
  });

  ArduinoOTA.onError([](ota_error_t error) 
  {
    Serial.printf("Error[%u]: ", error);
    switch (error)
    {
      case OTA_AUTH_ERROR:
        Serial.println("Begin Failed");
        client.publish(MQTT_TOPIC_UPDATE  , "Begin Failed");
        break;
      case OTA_CONNECT_ERROR:
        Serial.println("Connect Failed");
        client.publish(MQTT_TOPIC_UPDATE  , "Connect Failed");
        break;
      case OTA_RECEIVE_ERROR:
        Serial.println("Receive Failed");
        client.publish(MQTT_TOPIC_UPDATE  , "Receive Failed");
        break;
      case OTA_END_ERROR:
        Serial.println("End Failed");
        client.publish(MQTT_TOPIC_UPDATE  , "End Failed");
        break;
      
      default:
        break;
    }
  });
  ArduinoOTA.begin();
};

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  setup_WIFI ();
  setup_MQTT ();
  setup_SENSOREN ();
  setup_OTA ();

  taskManager.StartTask(&taskTemperaturMessen); // start with turning it on
  delay(5000);
   
}

void loop() 
{
  // put your main code here, to run repeatedly:
   taskManager.Loop();
   ArduinoOTA.handle();
}