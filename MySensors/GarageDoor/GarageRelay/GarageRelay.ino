/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to noddes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

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
// Must be >1000ms for DHT22 and >2000ms for DHT11
unsigned long UPDATE_INTERVAL = 60000; // 1 min
unsigned long lastRefreshTime;

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
	sendSketchInfo("Garage Door control", "2.0");
  present(CHILD_ID_DOOR_CONTROL, S_BINARY);
  present(CHILD_ID_OPENED_REED, S_DOOR);
  present(CHILD_ID_CLOSED_REED, S_DOOR);
}

void loop()
{
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
	if (message.type==V_STATUS) 
	{
	  // Open/Close door
    Serial.println("Garage door activated");
    digitalWrite(DOOR_CONTROL_PIN, HIGH);
    wait(DOOR_ACTIVATION_PERIOD);
    digitalWrite(DOOR_CONTROL_PIN, LOW);
	}
}

