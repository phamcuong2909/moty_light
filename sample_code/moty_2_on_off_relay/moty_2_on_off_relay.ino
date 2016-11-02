#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// Khai báo các chân nối với cảm biến, relay và button
#define PIR_PIN D7

#define BTN1_PIN D5
#define BTN2_PIN D6

#define RELAY1_PIN D1
#define RELAY2_PIN D2

#define LIGHT_MIN 20
#define LIGHT_ON_DURATION 15000

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
  lightStatus = analogRead(A0) * 100 / 1024;
  Serial.print("Trang thai anh sang hien tai: "); Serial.print(lightStatus); Serial.println("%");

  int motionDetected = digitalRead(PIR_PIN);

  if (motionDetected) {
    Serial.println("Phat hien chuyen dong");

    // Nếu trời tối hơn mức quy định và đèn đang tắt thì bật đèn
    if (lightStatus < LIGHT_MIN && !relay1Status) {
      Serial.println("Phat hien chuyen dong va troi toi, yeu cau bat den");
      digitalWrite(RELAY1_PIN, HIGH);
      relay1Status = true;
    }

    lastMotionDetected = millis(); // Lưu thời gian lần cuối phát hiện chuyển động
  }

  // Nếu đèn đang bật, kiểm tra để tắt nếu không có chuyển động sau 1 thời gian quy định
  if (relay1Status) {
    unsigned long current = millis();
    if (current - lastMotionDetected > LIGHT_ON_DURATION) {
      Serial.println("Khong phat hien chuyen dong sau mot thoi gian, yeu cau tat den");
      digitalWrite(RELAY1_PIN, LOW);
      relay1Status = false;
    }
  }

  delay(200);
}
