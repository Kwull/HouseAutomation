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

//#define MY_SIGNING_FEATURE
//#define MY_SIGNING_SOFT
//#define MY_SIGNING_REQUEST_SIGNATURES
//#define MY_RF24_ENABLE_ENCRYPTION

#define MY_NODE_ID 3
#define MY_REPEATER_FEATURE

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>  
#include <DHT.h>  

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define HUMIDITY_SENSOR_DIGITAL_PIN 3
 
DHT dht;
boolean metric = true; 
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

// Sleep time between sensor updates (in milliseconds)
// Must be >1000ms for DHT22 and >2000ms for DHT11
unsigned long UPDATE_INTERVAL = 60000; // 1 min
unsigned long lastRefreshTime;

void setup()  
{ 
  // Setup locally attached sensors
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 
  metric = getControllerConfig().isMetric;

  // wait(dht.getMinimumSamplingPeriod());
  lastRefreshTime = millis(); // delay first update for DHT22
}

void presentation()  
{ 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Terassa", "2.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
}

void loop()      
{  
  boolean needRefresh = (millis() - lastRefreshTime) > UPDATE_INTERVAL;
  if (!needRefresh)
    return;

  lastRefreshTime = millis();

  // Fetch temperatures from DHT sensor
  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT");
  } else {
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    send(msgTemp.set(temperature, 1));
  //  Serial.print("T: ");
  //  Serial.println(temperature);
  }
  
  // Fetch humidity from DHT sensor
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
  } else {
     send(msgHum.set(humidity, 1));
  //   Serial.print("H: ");
  //   Serial.println(humidity);
  } 
}
