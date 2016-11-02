#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// Khai báo các chân nối với cảm biến, relay và button
#define PIR_PIN D7

#define BTN1_PIN D5
#define BTN2_PIN D6

#define RELAY1_PIN D1
#define RELAY2_PIN D2

int lightStatus = 0;
boolean relay1Status = false;
boolean relay2Status = false;

unsigned long lastMotionDetected = 0; // last time motion is detected

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
}

void loop() {  
  // Đọc trạng thái ánh sáng hiện tại từ quang trở
  lightStatus = analogRead(A0)*100/1024;
  Serial.print("Trang thai anh sang hien tai: "); Serial.print(lightStatus); Serial.println("%");

  int motionDetected = digitalRead(PIR_PIN);

  if (motionDetected) {
    Serial.println("Phat hien chuyen dong");
    lastMotionDetected = millis(); // lưu thời gian lần cuối phát hiện chuyển động
  }
  delay(200);
}
