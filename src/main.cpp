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
PubSubClient MQTT(espClient);
#define MQTT_TOPIC_VORLAUF          "TempLogger/Vorlauf"
#define MQTT_TOPIC_RUECKLAUF        "TempLogger/Ruecklauf"
#define MQTT_TOPIC_UPDATE           "TempLogger/Update"
#define MQTT_TOPIC_SENSOR_ANZ       "TempLogger/Anzahl Sensoren"
#define MQTT_TOPIC_SENSOR_ADDRESS   "TempLogger/Adressse Sensor"

#define MQTT_TOPIC_VORLAUF_THERMOSTATMODE     "TempLogger/Vorlauf/thermostatMode"
#define MQTT_TOPIC_VORLAUF_SETPOINT           "TempLogger/Vorlauf/Setpoint"
#define MQTT_TOPIC_VORLAUF_SETPOINTHIGH       "TempLogger/Vorlauf/SetpointHigh"
#define MQTT_TOPIC_VORLAUF_SETPOINTLOW        "TempLogger/Vorlauf/SetpointLow"
#define MQTT_TOPIC_VORLAUF_HUMIDITYAMBIENT    "TempLogger/Vorlauf/HumidityAmbient"


void setup_MQTT (); 

TaskManager taskManager;
void TemperaturMessen(uint32_t deltaTime); 
FunctionTask taskTemperaturMessen(TemperaturMessen, MsToTaskTime(10000)); // turn on the led in 400ms

#define ONE_WIRE_BUS D14                  // connected pin
#define TEMPERATURE_PRECISION 9           // 12 Bit Resolution
int numberOfDevices;                      // Anzahl an Sensoren
OneWire *oneWire;                         // 
DallasTemperature *sensors;               //
DeviceAddress tempDeviceAddress;          // We'll use this variable to store a found device address

void TemperaturMessen(uint32_t deltaTime)
{
    // temperaturen messen
    sensors->requestTemperatures(); // Send the command to get temperatures

    sensors->getAddress(tempDeviceAddress, 0);
    int vorlauf = sensors->getTempC(tempDeviceAddress);
    sensors->getAddress(tempDeviceAddress, 1);
    int ruecklauf = sensors->getTempC(tempDeviceAddress); 

    // per mqtt weiterreichen
    if (!MQTT.connected())
    {
      setup_MQTT (); 
    }
  
    MQTT.publish (MQTT_TOPIC_VORLAUF  , String  (vorlauf).c_str()); 
    MQTT.publish (MQTT_TOPIC_RUECKLAUF, String(ruecklauf).c_str());

    MQTT.publish (MQTT_TOPIC_VORLAUF_SETPOINT, String(vorlauf).c_str());

    Serial.println ("Temp 1 : "+String(vorlauf)+"°C");
    Serial.println ("Temp 2 : "+String(ruecklauf)+"°C");
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
  MQTT.setServer(mqttServer, mqttPort);

  while (!MQTT.connected()) 
  {
    Serial.println("Connecting to MQTT...");
    if (MQTT.connect("ESP8266Client", mqttUser, mqttPassword )) 
    {
      Serial.println("connected");
      MQTT.publish("TempLogger/IP", WiFi.localIP().toString().c_str()); 
      MQTT.publish (MQTT_TOPIC_VORLAUF_THERMOSTATMODE, "auto");
      MQTT.publish (MQTT_TOPIC_VORLAUF_SETPOINT, "44.4");
      MQTT.publish (MQTT_TOPIC_VORLAUF_SETPOINTHIGH, "100.0");
      MQTT.publish (MQTT_TOPIC_VORLAUF_SETPOINTLOW, "0.0");
      MQTT.publish (MQTT_TOPIC_VORLAUF_HUMIDITYAMBIENT, "55.5");
    } 
    else 
    {
      Serial.print("failed with state ");
      Serial.print(MQTT.state());
      delay(2000);
    }
  }
};

void setup_SENSOREN ()
{
  oneWire = new OneWire(ONE_WIRE_BUS);
  //oneWire = new OneWire();
  oneWire->begin(ONE_WIRE_BUS);
  
  sensors = new DallasTemperature(oneWire);
  sensors->begin();
  sensors->setResolution (TEMPERATURE_PRECISION);

  numberOfDevices = sensors->getDeviceCount();
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  MQTT.publish(MQTT_TOPIC_SENSOR_ANZ, String(numberOfDevices).c_str());
  Serial.println(" devices.");    

  for(int i=0;i<numberOfDevices; i++)
  {
      // Search the wire for address
      if(sensors->getAddress(tempDeviceAddress, i))
      {
          String STR_Address = "";
          Serial.print("Found device ");
          Serial.print(i, DEC);
          Serial.print(" with address: ");

          for (uint8_t i = 0; i < 8; i++)
          {
            if (tempDeviceAddress[i] < 16) 
            {
              Serial.print("0");
              STR_Address += "0";
            };
            Serial.print(tempDeviceAddress[i], HEX);
            STR_Address += tempDeviceAddress[i];
            
          }
          MQTT.publish((String("TempLogger/Adressse Sensor")+String(i)).c_str(), STR_Address.c_str() );
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
  //ArduinoOTA.setHostname("TempLogger");
  //MDNS.begin("ESPP");
 
  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"1234");

  ArduinoOTA.onStart([]() 
  {
    Serial.println("Start");
  });

  ArduinoOTA.onEnd([]() 
  {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
  {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    Serial.println();
  });

  ArduinoOTA.onError([](ota_error_t error) 
  {
    Serial.printf("Error[%u]: ", error);
    switch (error)
    {
      case OTA_AUTH_ERROR:
        Serial.println("Begin Failed");
        break;
      case OTA_CONNECT_ERROR:
        Serial.println("Connect Failed");
        break;
      case OTA_RECEIVE_ERROR:
        Serial.println("Receive Failed");
        break;
      case OTA_END_ERROR:
        Serial.println("End Failed");
        break;
      
      default:
        break;
    }
  });
  ArduinoOTA.setHostname("TempLogger");
  ArduinoOTA.begin();
  MDNS.begin("TempLogger");
  MDNS.addService("http", "tcp", 80);
  
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
   MDNS.update();
}