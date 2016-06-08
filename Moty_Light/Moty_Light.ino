#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <IPAddress.h>
#include <PubSubClient.h>

const char* ssid = "your_wifi_network";
const char* password = "your_wifi_password";

const char* clientId = "MotyLight1";
// use public MQTT broker
const char* mqttServer = "broker.hivemq.com";
const char* mqttUsername = "<MQTT_BROKER_USERNAME>";
const char* mqttPassword = "<MQTT_BROKER_PASSWORD>";

const char* lightStatusTopic = "/MotyLight1/LightStatus";
const char* lightCommandTopic = "/MotyLight1/LightCommandTopic";
const char* lightSensorValueTopic = "/MotyLight1/LightSensorValue";

// used for smoothing technique to read light sensor value
// please refer example from menu File\Examples\03.Analog\Smoothing
const int numReadings = 5;
const int lightThreshold = 310;
const unsigned long lightOnDuration = 30000; // max idle time for light without motion detected ~ 30 seconds

const unsigned long reconnectInterval = 120000; // try to reconnect to wifi/broker every 3 mins if disconnected
unsigned long lastReconnect = 0; // last time attempt to reconnect

int readings[numReadings];      // we use smoothing technique to read light sensor value
int currentIndex = 0;           // the index of the current reading
int total = 0;                  // the running total
int lightSensorValue = 0;       // the average light sensor value

const byte dcRelayPin = D7; // for 12V output
const byte acRelayPin = D6; // for AC output
const byte pirPin = D1;
boolean relayOn = false; // light status, false = light off, true = light on

unsigned long lastMotionDetected = 0; // last time motion is detected

WiFiClient espClient;
PubSubClient client(espClient);


void setup() {
    Serial.begin(SERIAL_BAUD);

    // Configure input pins and output pins
    pinMode(pirPin, INPUT);
    pinMode(dcRelayPin, OUTPUT);
    pinMode(acRelayPin, OUTPUT);
  
    digitalWrite(dcRelayPin, LOW);
    digitalWrite(acRelayPin, LOW);
    relayOn = false;
  
    // initialize all the light sensor readings
    for (int i = 0; i < numReadings; i++)
        readings[i] = 0; 

    setup_wifi();

    client.setServer(mqttServer, 1883);
    client.setCallback(onMessageReceived);

    // Waiting for connection ready before sending update
    delay(200); 
}

void loop() {
  if (!client.connected()) {
    unsigned long current = millis();
    if (current - lastReconnect > reconnectInterval) {
      reconnect();
      lastReconnect = millis();
    }
  }
  if (client.connected()) {
      client.loop();
  }

  // get light sensore value
  // subtract the last reading:
  total = total - readings[currentIndex];         
  // read from the sensor:  
  readings[currentIndex] = analogRead(A0); 
  // add the reading to the total:
  total = total + readings[currentIndex];       
  // advance to the next position in the array:  
  currentIndex = currentIndex + 1;                    

  // if we're at the end of the array...
  if (currentIndex >= numReadings)              
    // ...wrap around to the beginning: 
    currentIndex = 0;                           

  // calculate the average:
  lightSensorValue = total / numReadings;
  Serial.print("Light sensor value: "); Serial.println(lightSensorValue);

  int motionDetected = digitalRead(pirPin);

  if (motionDetected) {
    Serial.println("Motion detected");
    if (lightSensorValue < lightThreshold) { // if the light is darker than limit
      if (!relayOn) { // light is currently off, then turn it on
        Serial.println("Motion detected. It's dark so I am turning light on");
        relayOn = true;
        digitalWrite(dcRelayPin, HIGH);
        digitalWrite(acRelayPin, HIGH);
        debugln("Sending status update to broker");
        char status [2];
        sprintf(status, "%d", 1);
        client.publish(lightStatusTopic, status);
      }
      lastMotionDetected = millis(); //save last time motion is detected
    }    
  }
  
  if (relayOn) { // we need to check to turn off light if no motion detected after some time
    unsigned long current = millis();
    if (current - lastMotionDetected > lightOnDuration) {
      Serial.println("No motion detected long time. I am turning light off");
      relayOn = false;
      digitalWrite(dcRelayPin, LOW);
      digitalWrite(acRelayPin, LOW);
      debugln("Sending status update to broker");
      char status [2];
      sprintf(status, "%d", 0);
      client.publish(lightStatusTopic, status);
    }
  }
  delay(50);
}

void setup_wifi() {
  // We start by connecting to a WiFi network
  debug("Connecting to "); debugln(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    debug(".");
  }
  debugln("");
  debugln("WiFi connected");
  debug("IP address: ");
  debugln(WiFi.localIP());
}

void onMessageReceived(char* topic, byte* payload, unsigned int length) {
  debug("Message arrived [");
  debug(topic);
  debug("]: ");
  for (int i = 0; i < length; i++) {
    debug((char)payload[i]);
  }
  debugln();
  
  if (strcmp(topic, lightCommandTopic) == 0) {
    debugln("Received message to control light");
    if ((char)payload[0] == '1') {
      debugln("Command is to turn on");
      digitalWrite(dcRelayPin, HIGH);
      digitalWrite(acRelayPin, HIGH);
      relayOn = true;
    } else if ((char)payload[0] == '0') {
      debugln("Command is to turn off");
      digitalWrite(dcRelayPin, LOW);
      digitalWrite(acRelayPin, LOW);
      relayOn = false;
    } else {
      debug("Invalid command received: "); debugln(payload[0]);
    }
  }
}

void reconnect() {
  debug("Attempting MQTT connection...");
  // Attempt to connect
  if (client.connect(clientId, mqttUsername, mqttPassword)) {
    debugln("connected");
    debug("Subscribe to topic: "); debug(lightCommandTopic);
    client.subscribe(lightCommandTopic);
    debugln("... done");
    delay(100);
    lastReconnect = 0;
  } else {
    debug("failed, rc="); debugln(client.state());
  }
}
