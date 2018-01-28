/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
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
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik EKblad
 * 
 * DESCRIPTION
 * This sketch provides an example how to implement a humidity/temperature
 * sensor using DHT11/DHT-22 
 * http://www.mysensors.org/build/humidity
 */
 
// Enable debug prints
//#define MY_DEBUG
//#define MY_SENROS_DEBUG

#define MY_NODE_ID 2
#define MY_REPEATER_FEATURE

//#define MY_SIGNING_FEATURE
//#define MY_SIGNING_SOFT
//#define MY_SIGNING_REQUEST_SIGNATURES
//#define MY_RF24_ENABLE_ENCRYPTION

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>
#include <DHT.h>  
#include <RCSwitch.h>
#include <NewPing.h>

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_DISTANCE 2
#define CHILD_ID_LIGHT 3
#define HUMIDITY_SENSOR_DIGITAL_PIN 4
#define TRANSMITTER_433MHZ_DIGITAL_PIN 5
#define DISTANCE_SENSOR_ECHO_PIN 6
#define DISTANCE_SENSOR_TRIGGER_PIN 7
#define MAX_DISTANCE 300 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define DISTANCE_SENSOR_SLEEP_TIME 50
const long interval = 30000;              // interval at which to read (milliseconds)

/*#define LIGHT_ONOFF "111111100000100110000111"
#define LIGHT_BRIGHT_PLUS "111111100000100110000011"
#define LIGHT_BRIGHT_MINUS "111111100000100110000010"*/

DHT dht;
float lastTemp;
float lastHum;

NewPing sonar(DISTANCE_SENSOR_TRIGGER_PIN, DISTANCE_SENSOR_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
int lastDist;
int lastSleep;

boolean metric = true; 
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgDistance(CHILD_ID_DISTANCE, V_DISTANCE);
MyMessage msgLight(CHILD_ID_LIGHT, V_VAR1);
RCSwitch mySwitch = RCSwitch();

void setup()  
{ 
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 
  
  // 433Mhz Transmitter
  mySwitch.enableTransmit(TRANSMITTER_433MHZ_DIGITAL_PIN);
  mySwitch.setProtocol(1);
  mySwitch.setPulseLength(201);
  //mySwitch.setRepeatTransmit(3);

  metric = getControllerConfig().isMetric;
  lastSleep = 0;

  // request last state
  // request(CHILD_ID_LIGHT, V_VAR1);
}

void presentation()  
{ 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Kitchen 1st fl", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
  present(CHILD_ID_DISTANCE, S_DISTANCE);
  present(CHILD_ID_LIGHT, S_CUSTOM);
}

void loop()      
{  
  wait(DISTANCE_SENSOR_SLEEP_TIME);

  // read distance data
  int dist = metric ? sonar.ping_cm() : sonar.ping_in(); 
  if (dist != lastDist) {
      #ifdef MY_SENROS_DEBUG
      Serial.print("Ping: ");
      Serial.print(dist);
      Serial.println(metric ? " cm" : " in");
      #endif

      if (dist >=1 && dist < 20 && lastDist > 30){
        // Serial.println("LIGHTS ON/OFF");
         mySwitch.send("111111100000100110000111");
         send(msgDistance.set(dist, 1));
      }
      
      lastDist = dist; 
  }

  lastSleep += DISTANCE_SENSOR_SLEEP_TIME;

  if (lastSleep < dht.getMinimumSamplingPeriod()+interval){
    return;
  }
     
  // Fetch temperatures from DHT sensor
  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
    #ifdef MY_SENROS_DEBUG
      Serial.println("Failed reading temperature from DHT");
    #endif
  } else if (temperature != lastTemp) {
    lastTemp = temperature;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    send(msgTemp.set(temperature, 1));
    #ifdef MY_SENROS_DEBUG
    Serial.print("T: ");
    Serial.println(temperature);
    #endif
  }
  
  // Fetch humidity from DHT sensor
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
    #ifdef MY_SENROS_DEBUG
      Serial.println("Failed reading humidity from DHT");
    #endif
  } else if (humidity != lastHum) {
      lastHum = humidity;
      send(msgHum.set(humidity, 1));
      #ifdef MY_SENROS_DEBUG
      Serial.print("H: ");
      Serial.println(humidity);
      #endif
  }

  //sleep(INTERRUPT,CHANGE, SLEEP_TIME); //sleep a bit
}

void receive(const MyMessage &message) {
  #ifdef MY_SENROS_DEBUG
  Serial.print("Message received. type: ");
  Serial.print(message.type);
  Serial.print(" data: ");
  Serial.println(message.data);
  #endif
  
  if (message.type==V_VAR1) { 
     unsigned long cmd = message.getULong();
     #ifdef MY_SENROS_DEBUG
     Serial.print("RF Received from Net: ");
     Serial.println(cmd);
     #endif
     mySwitch.send(cmd, 24);
     //mySwitch.send("111111100000100110000111");
   }
}
