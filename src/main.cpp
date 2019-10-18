#include <Arduino.h>
#include <TaskScheduler.h>

// Callback methods prototypes
void t1Callback();

Task t1(2000, 10, &t1Callback);
Scheduler runner;

void t1Callback() 
{
    Serial.print("t1: ");
    Serial.println(millis());
};

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Scheduler TEST");
  runner.init();
  

  Serial.println("Initialized scheduler");
  
  runner.addTask(t1);
  Serial.println("added t1");
  

  delay(5000);
  
  t1.enable();
  Serial.println("Enabled t1");
}

void loop() {
  // put your main code here, to run repeatedly:
   runner.execute();
}