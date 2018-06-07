// Enable debug prints
//#define MY_DEBUG

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
#define MAX_DISTANCE 50 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define DISTANCE_SENSOR_SLEEP_TIME 50

/*#define LIGHT_ONOFF "111111100000100110000111"
#define LIGHT_BRIGHT_PLUS "111111100000100110000011"
#define LIGHT_BRIGHT_MINUS "111111100000100110000010"*/

DHT dht;

NewPing sonar(DISTANCE_SENSOR_TRIGGER_PIN, DISTANCE_SENSOR_ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
int lastDist;

// Sleep time between sensor updates (in milliseconds)
// Must be >1000ms for DHT22 and >2000ms for DHT11
unsigned long UPDATE_INTERVAL = 60000; // 1 min for temp and hum
unsigned long lastRefreshTime;

// To make a fading motion on the led distDebounce time
unsigned long distDebounce = 30; 
unsigned long lastDebouncePeriod;

#define LOCK_THRESHOLD 5 // distance sensensor lock threshold in cm
#define LOCK_TIME 1000 // lock time for distance sensor 3 sec
unsigned long last_lock_time;
boolean needChangeState;

boolean metric = true; 
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgDistance(CHILD_ID_DISTANCE, V_DISTANCE);
MyMessage msgLight(CHILD_ID_LIGHT, V_STATUS);
RCSwitch mySwitch = RCSwitch();

bool isLightsOn = false;
#define LIGHT_ON 1
#define LIGHT_OFF 0 

bool firstStart = true;

void setup()  
{ 
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 
  
  // 433Mhz Transmitter
  mySwitch.enableTransmit(TRANSMITTER_433MHZ_DIGITAL_PIN);
  mySwitch.setProtocol(1);
  mySwitch.setPulseLength(201);
  //mySwitch.setRepeatTransmit(3);

  metric = getControllerConfig().isMetric;
  
  lastRefreshTime = millis(); // delay first update for DHT22
  last_lock_time = millis();
  needChangeState = true;
}

void presentation()  
{ 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Kitchen 1st fl", "3.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
  present(CHILD_ID_DISTANCE, S_DISTANCE);
  present(CHILD_ID_LIGHT, S_BINARY);
}

void loop()      
{  
  if (firstStart) {
    send(msgLight.set(isLightsOn ? LIGHT_ON : LIGHT_OFF));
    request(CHILD_ID_LIGHT, V_STATUS);
    wait(2000, C_SET, V_STATUS);
  }

  unsigned long now = millis();
  
  // read distance data
  int dist = sonar.ping_median(5); 
  dist = sonar.convert_cm(dist);

  // Check if it is time to alter the leds
  if (now-lastDebouncePeriod > distDebounce) {
    lastDebouncePeriod = millis();

    if (dist>0){
       Serial.print("Distance: ");
       Serial.print(dist);
       Serial.println(" cm");

       if (dist < 20) {
          if (needChangeState) {
            isLightsOn = !isLightsOn;

            Serial.println("TURN ON/OF");
            mySwitch.send("111111100000100110000111");

            send(msgLight.set(isLightsOn ? LIGHT_ON : LIGHT_OFF));
            //send(msgDistance.set(dist, 1));
            
            needChangeState = false;
          }
        } 

        last_lock_time = millis();
    } else {
      needChangeState = true;
    }
  }
  
  boolean needRefresh = (millis() - lastRefreshTime) > UPDATE_INTERVAL;
  if (!needRefresh)
    return;

  lastRefreshTime = millis();

  // Fetch temperatures from DHT sensor
  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
    #ifdef MY_DEBUG
      Serial.println("Failed reading temperature from DHT");
    #endif
  } else {
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    send(msgTemp.set(temperature, 1));
    #ifdef MY_DEBUG
    Serial.print("T: ");
    Serial.println(temperature);
    #endif
   } 
       
  // Fetch humidity from DHT sensor
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
    #ifdef MY_DEBUG
      Serial.println("Failed reading humidity from DHT");
    #endif
  } else {
      send(msgHum.set(humidity, 1));
      #ifdef MY_DEBUG
      Serial.print("H: ");
      Serial.println(humidity);
      #endif
   }
}

void receive(const MyMessage &message) {
  if (message.isAck()) {
     Serial.println("This is an ack from gateway");
  }  

  #ifdef MY_DEBUG
  Serial.print("Message received. type: ");
  Serial.print(message.type);
  Serial.print(" data: ");
  Serial.println(message.data);
  #endif
  
  if (message.type == V_STATUS) { 
    if (firstStart) {
      Serial.println("Received initial value from controller");
      firstStart = false;
    } else {
      Serial.println("TURN ON/OF");
      isLightsOn = !isLightsOn;
      mySwitch.send("111111100000100110000111");
    }

    send(msgLight.set(isLightsOn ? LIGHT_ON : LIGHT_OFF));
   }
}
