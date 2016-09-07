#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <IPAddress.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>


const char* ssid = "Sandiego";
const char* password = "0988807067";

const char* clientId = "MotyLight1";
// use public MQTT broker
//const char* mqttServer = "broker.hivemq.com";
const char* mqttServer = "192.168.1.110";
const char* mqttUsername = "<MQTT_BROKER_USERNAME>";
const char* mqttPassword = "<MQTT_BROKER_PASSWORD>";

const char* lightStatusTopic = "/MotyLight1/LightStatus";
const char* ipInfoTopic = "/MotyLight1/IPInfo";

// used for smoothing technique to read light sensor value
// please refer example from menu File\Examples\03.Analog\Smoothing
const int numReadings = 5;
int lightThreshold = 310;
int lightOnDuration = 10000; // max idle time for light without motion detected ~ 30 seconds

const unsigned long reconnectInterval = 60000; // try to reconnect to wifi/broker every 3 mins if disconnected
unsigned long lastReconnect = 0; // last time attempt to reconnect

int readings[numReadings];      // we use smoothing technique to read light sensor value
int currentIndex = 0;           // the index of the current reading
int total = 0;                  // the running total
int lightSensorValue = 0;       // the average light sensor value

const byte relayPin = D1; 
const byte buttonPin = D5;
const byte pirPin = D2;
boolean relayOn = false; // light status, false = light off, true = light on

unsigned long lastMotionDetected = 0; // last time motion is detected

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);


void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("Startup");

    // Configure input pins and output pins
    pinMode(pirPin, INPUT);
    pinMode(buttonPin, INPUT);
    pinMode(relayPin, OUTPUT);
  
    digitalWrite(relayPin, HIGH); // turn off relay by default
    relayOn = false;
  
    // initialize all the light sensor readings
    for (int i = 0; i < numReadings; i++)
        readings[i] = 0;

    // read eeprom for ssid and pass
    EEPROM.begin(512);
    Serial.println("Reading ligh threshold from EEPROM");
    String lightThresholdStr;
    for (int i = 0; i < 32; ++i)
    {
      lightThresholdStr += char(EEPROM.read(i));
    }
    Serial.print("lightThresholdStr: ");
    Serial.println(lightThresholdStr);
    if (lightThresholdStr.length() > 0)
    {
      lightThreshold = lightThresholdStr.toInt();
    }
 
    Serial.println("Reading duration from EEPROM");
    String lightOnDurationStr = "";
    for (int i = 32; i < 96; ++i)
    {
      lightOnDurationStr += char(EEPROM.read(i));
    }
    Serial.print("lightOnDurationStr: ");
    Serial.println(lightOnDurationStr); 
    if (lightOnDurationStr.length() > 0)
    {
      lightOnDuration = lightOnDurationStr.toInt()*1000;
    } 

    setup_wifi();

    client.setServer(mqttServer, 1883);
    client.setCallback(onMessageReceived);

    launchWeb();
    Serial.println("Setup done");
}

void loop() {

  if (client.connected()) {
      client.loop();
  } else {
    Serial.println("Not connected to MQTT server");
    unsigned long current = millis();
 
    if (lastReconnect == 0 || current - lastReconnect > reconnectInterval) {
      reconnect();
      lastReconnect = millis();
    }
  }

  // get light sensor value
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
  //Serial.print("Light sensor value: "); Serial.println(lightSensorValue);

  int motionDetected = digitalRead(pirPin);

  if (motionDetected) {
    //Serial.println("Motion detected");
    if (lightSensorValue < lightThreshold) { // if the light is darker than limit
      if (!relayOn) { // light is currently off, then turn it on
        Serial.println("Motion detected. It's dark so I am turning light on");
        relayOn = true;
        digitalWrite(relayPin, LOW);
        Serial.println("Sending status update to broker");
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
      digitalWrite(relayPin, HIGH);
      Serial.println("Sending status update to broker");
      char status [2];
      sprintf(status, "%d", 0);
      client.publish(lightStatusTopic, status);
    }
  }
  
  server.handleClient();
  delay(5);
}

void setup_wifi() {
  //WiFi.setPhyMode(WIFI_PHY_MODE_11G); 
  WiFi.setOutputPower(8);
  // We start by connecting to a WiFi network
  Serial.print("Connecting to "); Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void onMessageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  /*
  if (strcmp(topic, lightCommandTopic) == 0) {
    Serial.println("Received message to control light");
    if ((char)payload[0] == '1') {
      Serial.println("Command is to turn on");
      digitalWrite(dcRelayPin, HIGH);
      digitalWrite(acRelayPin, HIGH);
      relayOn = true;
    } else if ((char)payload[0] == '0') {
      Serial.println("Command is to turn off");
      digitalWrite(dcRelayPin, LOW);
      digitalWrite(acRelayPin, LOW);
      relayOn = false;
    } else {
      Serial.print("Invalid command received: "); Serial.println(payload[0]);
    }
  }
  */
}

void reconnect() {
  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
  if (client.connect(clientId, mqttUsername, mqttPassword)) {
    Serial.println("connected");
    delay(500);
    lastReconnect = 0;

    IPAddress ip = WiFi.localIP();
    String ipStr = "http://" + String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    char msg[200];
    ipStr.toCharArray(msg, 200);
    client.publish(ipInfoTopic, msg);
  } else {
    Serial.print("failed, rc="); Serial.println(client.state());
  }
}

/*---------------------- Web server setup ----------------------*/

void launchWeb() {
  
  server.on("/", []() {
    IPAddress ip = WiFi.localIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    String content;
    content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
    content += ipStr;
    content += "<p>Light sensor: " + String(lightSensorValue) + "</p>";
    content += "<p>Light status: " + String(relayOn) + "</p>";
    content += "<form method='post' action='setting'><label>Light Threshold (0-1023): </label><input name='light_threshold' value='" + String(lightThreshold) + "'length=4><br><label>Light Duration (seconds): </label><input name='light_duration' value='" + String(lightOnDuration/1000) + "' length=4><input type='submit'></form>";
    content += "</html>";
    server.send(200, "text/html", content);
  });

  server.on("/setting", []() {
    IPAddress ip = WiFi.localIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    String content;
    String lightThresholdStr = server.arg("light_threshold");
    String lightDurationOnStr = server.arg("light_duration");
    
    if (lightThresholdStr.length() > 0 && lightDurationOnStr.length() > 0) {
      Serial.println("clearing eeprom");
      for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
      Serial.println(lightThresholdStr);
      Serial.println(lightDurationOnStr);
        
      Serial.println("writing threshold values to eeprom:");
      for (int i = 0; i < lightThresholdStr.length(); ++i)
      {
        EEPROM.write(i, lightThresholdStr[i]);
        Serial.print("Wrote: ");
        Serial.println(lightThresholdStr[i]); 
      }
 
      Serial.println("writing duration to eeprom:"); 
      for (int i = 0; i < lightDurationOnStr.length(); ++i)
      {
        EEPROM.write(32+i, lightDurationOnStr[i]);
        Serial.print("Wrote: ");
        Serial.println(lightDurationOnStr[i]); 
      }
      
      EEPROM.commit();

      lightOnDuration = lightDurationOnStr.toInt()*1000;
      lightThreshold = lightThresholdStr.toInt();
      
      content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
      content += ipStr;
      content += "<p>Light sensor: " + String(lightSensorValue) + "</p>";
      content += "<p>Light status: " + String(relayOn) + "</p>";
      content += "<form method='post' action='setting'><label>Light Threshold (0-1023): </label><input name='light_threshold' value='" + String(lightThreshold) + "'length=4><br><label>Light Duration (seconds): </label><input name='light_duration' value='" + String(lightOnDuration/1000) + "' length=4><input type='submit'></form>";
      content += "<p>Success saved to eeprom</p>";
      content += "</html>";
      server.send(200, "text/html", content);
    } else {
      Serial.println("Sending error message");
      content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
      content += ipStr;
      content += "<p>Light sensor: " + String(lightSensorValue) + "</p>";
      content += "<p>Light status: " + String(relayOn) + "</p>";
      content += "<form method='post' action='setting'><label>Light Threshold (0-1023): </label><input name='light_threshold' length=4><br><label>Light Duration (seconds): </label><input name='light_duration' length=4><input type='submit'></form>";
      content += "<p>Error: Invalid value</p>";
      content += "</html>";
      server.send(200, "text/html", content);
    }    
  });

  server.on("/cleareeprom", []() {
    String content;
    content = "<!DOCTYPE HTML>\r\n<html>";
    content += "<p>Clearing the EEPROM</p></html>";
    server.send(200, "text/html", content);
    Serial.println("clearing eeprom");
    for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
    EEPROM.commit();
  });
  
  // Start the server
  server.begin();
  Serial.println("Server started"); 
}
