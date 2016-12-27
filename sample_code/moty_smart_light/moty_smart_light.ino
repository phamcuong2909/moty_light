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

#define LIGHT_MIN 20
#define LIGHT_ON_DURATION 10000

// Cấu hình Wifi, sửa lại theo đúng mạng Wifi của bạn
const char* WIFI_SSID = "MiMax";
const char* WIFI_PWD = "0988807067";

int lightStatus = 0;
boolean relay1Status = false;
boolean relay2Status = false;

unsigned long lastMotionDetected = 0; // last time motion is detected

// Cấu hình cho giao thức MQTT
const char* clientId = "MotyLight1";
const char* mqttServer = "192.168.43.55";
const int mqttPort = 1883;
// Username và password để kết nối đến MQTT server nếu server có
// bật chế độ xác thực trên MQTT server
// Nếu không dùng thì cứ để vậy
const char* mqttUsername = "<MQTT_BROKER_USERNAME>";
const char* mqttPassword = "<MQTT_BROKER_PASSWORD>";

// MQTT topic để gửi thông tin về relay
const char* relay1StatusTopic = "/easytech.vn/MotyLight1/Relay1";
// MQTT topic để gửi thông tin về chuyển động khi phát hiện
const char* motionTopic = "/easytech.vn/MotyLight1/Motion";

// Khởi tạo thư viện để kết nối wifi và MQTT server
WiFiClient wclient;
PubSubClient client(wclient);


void setup() {
  Serial.begin(115200);
  // Do cảm biến chuyển động PIR bị nhiễu khi sử dụng gần Esp8266 nên ta cần
  // phải giảm mức tín hiệu của esp8266 để tránh nhiễu cho PIR
  WiFi.setOutputPower(7);

  // Thiết lập chế độ hoạt động của các chân
  pinMode(PIR_PIN, INPUT);

  pinMode(BTN1_PIN, INPUT);
  pinMode(BTN2_PIN, INPUT);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

  // Tắt cả 2 relay khi mới khởi động
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);

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
}

void loop() {
  // Kiểm tra kết nối tới MQTT server, nếu chưa kết nối hoặc
  // bị ngắt kết nối thì thực hiện kết nối
  if (!client.connected()) {
    if (client.connect(clientId, mqttUsername, mqttPassword)) {
      Serial.println("Ket noi den MQTT server thanh cong");
      client.loop();
    } else {
      Serial.print("Ket noi den MQTT server that bai. Error code: ");
      Serial.println(client.state());
    }
  } else {
    client.loop();  
  }

  // Đọc trạng thái ánh sáng hiện tại từ quang trở
  lightStatus = analogRead(A0) * 100 / 1024;
  //Serial.print("Trang thai anh sang hien tai: "); Serial.print(lightStatus); Serial.println("%");

  int motionDetected = digitalRead(PIR_PIN);

  if (motionDetected) {
    Serial.println("Phat hien chuyen dong");

    // Nếu trời tối hơn mức quy định và đèn đang tắt thì bật đèn
    if (lightStatus < LIGHT_MIN && !relay1Status) {
      Serial.println("Phat hien chuyen dong va troi toi, yeu cau bat den");
      digitalWrite(RELAY1_PIN, HIGH);
      relay1Status = true;

      Serial.println("Gui cap nhat trang thai relay ve server");
      char status [2];
      sprintf(status, "%d", 1);
      client.publish(relay1StatusTopic, status);
    }

    Serial.println("Gui cap nhat phat hien chuyen dong ve server");
    char status [2];
    sprintf(status, "%d", 1);
    client.publish(motionTopic, status);
    
    lastMotionDetected = millis(); // Lưu thời gian lần cuối phát hiện chuyển động
  }

  // Nếu đèn đang bật, kiểm tra để tắt nếu không có chuyển động sau 1 thời gian quy định
  if (relay1Status) {
    unsigned long current = millis();
    if (current - lastMotionDetected > LIGHT_ON_DURATION) {
      Serial.println("Khong phat hien chuyen dong sau mot thoi gian, yeu cau tat den");
      digitalWrite(RELAY1_PIN, LOW);
      relay1Status = false;

      Serial.println("Gui cap nhat trang thai relay ve server");
      char status [2];
      sprintf(status, "%d", 0);
      client.publish(relay1StatusTopic, status);
    }
  }

  delay(50);
}

