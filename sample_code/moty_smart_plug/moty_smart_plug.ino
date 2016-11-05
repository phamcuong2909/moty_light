#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <IPAddress.h>
#include <PubSubClient.h>

// Khai báo các chân nối với cảm biến, relay và button
#define PIR_PIN D7

#define BTN1_PIN D5
#define BTN2_PIN D6

#define RELAY1_PIN D1
#define RELAY2_PIN D2

// Cấu hình Wifi, sửa lại theo đúng mạng Wifi của bạn
const char* WIFI_SSID = "Sandiego"; //"your_wifi_network";
const char* WIFI_PWD = "0988807067"; //"your_wifi_password";

// Cấu hình cho giao thức MQTT
const char* clientId = "MotySmartPlug1";
const char* mqttServer = "192.168.1.110";
const int mqttPort = 1883;
// Username và password để kết nối đến MQTT server nếu server có
// bật chế độ xác thực trên MQTT server
// Nếu không dùng thì cứ để vậy
const char* mqttUsername = "<MQTT_BROKER_USERNAME>";
const char* mqttPassword = "<MQTT_BROKER_PASSWORD>";

// MQTT topic để gửi thông tin về relay
const char* relay1StatusTopic = "/easytech.vn/MotySmartPlug1/Status/Relay1";
const char* relay2StatusTopic = "/easytech.vn/MotySmartPlug1/Status/Relay2";

// Topic dùng để nhận lệnh bật tắt từ server
const char* relay1CommandTopic = "/easytech.vn/MotySmartPlug1/Command/Relay1";
const char* relay2CommandTopic = "/easytech.vn/MotySmartPlug1/Command/Relay2";

// Lưu trạng thái bật tắt của 2 relay, false = tắt, true = bật
boolean relay1Status = false;
boolean relay2Status = false;

// Khởi tạo thư viện để kết nối wifi và MQTT server
WiFiClient wclient;
PubSubClient client(wclient);


void setup() {
  Serial.begin(115200);

  // Thiết lập chế độ hoạt động của các chân
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

  // Bật cả 2 relay khi mới khởi động, bạn có thể thay đổi tùy ý
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);

  relay1Status = relay2Status = true;

  // Kết nối Wifi và chờ đến khi kết nối thành công
  WiFi.begin(WIFI_SSID, WIFI_PWD);

  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
    count++;

    if (count > 20) {
      ESP.restart();
    }
  }

  Serial.println("");
  Serial.println("Ket noi WiFi thanh cong");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqttServer, mqttPort);
  client.setCallback(onMessageReceived);
}

void loop() {
  // Kiểm tra kết nối tới MQTT server, nếu chưa kết nối hoặc
  // bị ngắt kết nối thì thực hiện kết nối
  if (!client.connected()) {
    if (client.connect(clientId, mqttUsername, mqttPassword)) {
      Serial.println("Ket noi den MQTT server thanh cong");
      
      Serial.print("Dang ky de nhan lenh dieu khien relay 1 tu server: "); Serial.print(relay1CommandTopic);
      client.subscribe(relay1CommandTopic);
      Serial.println("... done");
      delay(200);

      Serial.print("Dang ky de nhan lenh dieu khien relay 2 tu server: "); Serial.print(relay2CommandTopic);
      client.subscribe(relay2CommandTopic);
      Serial.println("... done");
      delay(200);
      
      client.loop();
    } else {
      Serial.print("Ket noi den MQTT server that bai. Error code: ");
      Serial.println(client.state());
    }
  } else {
    client.loop();  
  }

  // Đọc lệnh từ nút nhấn
  int button1Status = digitalRead(BTN1_PIN);

  if (button1Status == LOW) {
    Serial.println("Nut 1 duoc nhan");

    relay1Status = !relay1Status;
    digitalWrite(RELAY1_PIN, relay1Status);
    
    Serial.println("Gui cap nhat trang thai relay 1 ve server");
    char status [2];
    sprintf(status, "%d", 1);
    client.publish(relay1StatusTopic, status);
    delay(200);
  }

  int button2Status = digitalRead(BTN2_PIN);

  if (button2Status == LOW) {
    Serial.println("Nut 2 duoc nhan");

    relay2Status = !relay2Status;
    digitalWrite(RELAY2_PIN, relay2Status);
    
    Serial.println("Gui cap nhat trang thai relay 2 ve server");
    char status [2];
    sprintf(status, "%d", 1);
    client.publish(relay2StatusTopic, status);
    delay(200);
  }
}


void onMessageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nhan duoc lenh dieu khien tu MQTT server [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Đoạn code dưới đây kiểm tra xem lệnh nhận được là làm việc gì
  // và thực hiện bật tắt relay tương ứng
  if (strcmp(topic, relay1CommandTopic) == 0) {
    if ((char)payload[0] == '1') {
      Serial.println("Nhan duoc lenh bat relay 1");
      digitalWrite(RELAY1_PIN, HIGH);
      relay1Status = true;
    } else if ((char)payload[0] == '0') {
      Serial.println("Nhan duoc lenh tat relay 1");
      digitalWrite(RELAY1_PIN, LOW);
      relay1Status = false;
    }
  } else if (strcmp(topic, relay2CommandTopic) == 0) {
    if ((char)payload[0] == '1') {
      Serial.println("Nhan duoc lenh bat relay 2");
      digitalWrite(RELAY2_PIN, HIGH);
      relay2Status = true;
    } else if ((char)payload[0] == '0') {
      Serial.println("Nhan duoc lenh tat relay 2");
      digitalWrite(RELAY2_PIN, LOW);
      relay2Status = false;
    }
  } else {
    Serial.println("Lenh nhan duoc khong hop le: ");
  }
}

