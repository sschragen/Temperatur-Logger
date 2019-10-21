#include <Arduino.h>
//#include <TaskScheduler.h>
#include <Task.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h> 
#include <DallasTemperature.h>

#define ONE_WIRE_BUS D4
OneWire oneWire (ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;
int numberOfDevices;


const char* ssid = "Schragen2.4"; // Enter your WiFi name
const char* password =  "warpdrive"; // Enter WiFi password
const char* mqttServer = "prodesk.fritz.box";
const int   mqttPort = 1883;
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
    sensors.requestTemperatures(); // Send the command to get temperatures
    uint8_t tempDeviceAddress;

    sensors.getAddress(&tempDeviceAddress, 0);
    int vorlauf = sensors.getTempC(&tempDeviceAddress);
    sensors.getAddress(&tempDeviceAddress, 1);
    int ruecklauf = sensors.getTempC(&tempDeviceAddress); 

    // per mqtt weiterreichen
    client.publish ("TempLogger/Vorlauf"  , String  (vorlauf).c_str()); 
    Serial.println ("Temp1 : "+String(vorlauf)+"°C");
    client.publish ("TempLogger/Ruecklauf", String(ruecklauf).c_str());
    Serial.println ("Temp2 : "+String(ruecklauf)+"°C");
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Scheduler TEST");

  

  //runner.init();


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

  Serial.print("1 Locating devices...");
  sensors.begin(); 

  sensors.requestTemperatures();
  Serial.print(sensors.getTempCByIndex(0));
  
  Serial.print("2 Locating devices...");
  numberOfDevices = sensors.getDeviceCount();
  
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");    

  for(int i=0;i<numberOfDevices; i++)
  {
      // Search the wire for address
      if(sensors.getAddress(tempDeviceAddress, i))
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


  taskManager.StartTask(&taskTemperaturMessen); // start with turning it on

  delay(5000);
   
}

void loop() 
{
  // put your main code here, to run repeatedly:
   taskManager.Loop();
}