// Enable debug prints to serial monitor
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#define MY_NODE_ID 10
//#define MY_REPEATER_FEATURE
#include <MySensors.h>

#define CHILD_ID_DOOR_CONTROL 0
#define CHILD_ID_OPENED_REED 1
#define CHILD_ID_CLOSED_REED 2

#define DOOR_CONTROL_PIN 3 
#define DOOR_OPENED_PIN 4  // Top reed (yellow)
#define DOOR_CLOSED_PIN 5  // Bottom reed (orange)

int oldOpenedReedState=-1;
int oldClosedReedState=-1;

unsigned long DOOR_ACTIVATION_PERIOD = 800; // [ms] - emulate button hold down

MyMessage msgDoorCtrl(CHILD_ID_DOOR_CONTROL, V_STATUS);
MyMessage msgOpened(CHILD_ID_OPENED_REED, V_TRIPPED);
MyMessage msgClosed(CHILD_ID_CLOSED_REED, V_TRIPPED);

// Sleep time between sensor updates (in milliseconds)
unsigned long UPDATE_INTERVAL = 30000; // 30 sec
unsigned long lastRefreshTime;

bool firstStart = true;

void before()
{
		pinMode(DOOR_CONTROL_PIN, OUTPUT);
    pinMode(DOOR_CLOSED_PIN, INPUT_PULLUP);
    pinMode(DOOR_OPENED_PIN, INPUT_PULLUP);

    lastRefreshTime = millis();
}

void setup()
{

}

void presentation()
{
	sendSketchInfo("Garage Door control", "3.0");
  present(CHILD_ID_DOOR_CONTROL, S_BINARY);
  present(CHILD_ID_OPENED_REED, S_DOOR);
  present(CHILD_ID_CLOSED_REED, S_DOOR);
}

void loop()
{
  if (firstStart) {
    send(msgDoorCtrl.set(0));
    request(CHILD_ID_DOOR_CONTROL, V_STATUS);
    wait(2000, C_SET, V_STATUS);
  }

  boolean needRefresh = (millis() - lastRefreshTime) > UPDATE_INTERVAL;
  if (needRefresh) {
     lastRefreshTime = millis();
  }
  
 int proximity = digitalRead(DOOR_CLOSED_PIN);
 if (proximity != oldClosedReedState || needRefresh){
    oldClosedReedState = proximity;
    send(msgClosed.set(proximity==HIGH ? 1 : 0));
    
    if (proximity == HIGH) // If the pin reads high, the switch is opened.
      Serial.println("Door closed reed (orange) state: OPENED");
    else
      Serial.println("Door closed reed (orange) state: CLOSED");
 } 
  
 proximity = digitalRead(DOOR_OPENED_PIN);
 if (proximity != oldOpenedReedState || needRefresh){
     oldOpenedReedState = proximity;
     send(msgOpened.set(proximity==HIGH ? 1 : 0));
    
     if (proximity == HIGH) // If the pin reads high, the switch is opened.
       Serial.println("Door opened reed (yellow) state: OPENED");
     else
       Serial.println("Door opened reed (yellow) state: CLOSED");
  } 
}

void receive(const MyMessage &message)
{
	// We only expect one type of message from controller. But we better check anyway.
	if (message.type == V_STATUS) 
	{
     if (firstStart) {
      Serial.println("Received initial value from controller");
      firstStart = false;
    } else { 
      // Open/Close door
      Serial.println("Garage door activated");
      digitalWrite(DOOR_CONTROL_PIN, HIGH);
      wait(DOOR_ACTIVATION_PERIOD);
      digitalWrite(DOOR_CONTROL_PIN, LOW);
    }

    send(msgDoorCtrl.set(0));
	}
}
