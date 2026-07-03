#include <POP32.h>

// ---------- Pin กำหนดตำแหน่งขาอ่านสัญญาณ RC ----------
#define RC_THROTTLE_PIN   PA5
#define RC_STEER_PIN      PA6

// ---------- ค่าคงที่สัญญาณ PWM ของ RC ----------
const int RC_MIN      = 1000;
const int RC_MID      = 1500;
const int RC_MAX      = 2000;
const int RC_DEADZONE =   50;

// ---------- ค่าปรับแต่ง (Calibration) ----------
// ถ้าไม่แตะคันเลี้ยวเลย แต่ค่า STR บนจอไม่เป็น 0 ให้ปรับ STEER_TRIM
// เช่น ถ้า STR ค้างที่ +12 ให้ตั้ง STEER_TRIM = 12 (ลบออกจากค่าที่อ่านได้)
const int STEER_TRIM = 0;

// ถ้าค่า L กับ R บนจอเท่ากัน แต่ล้อหมุนจริงเร็วไม่เท่ากัน ให้ปรับสองค่านี้
// เพิ่มค่าที่ล้อ "หมุนช้ากว่า" เพื่อชดเชย เช่น LEFT_TRIM = 3 ถ้าล้อซ้ายช้ากว่า
const int LEFT_TRIM  = 0;
const int RIGHT_TRIM = 0;

// ---------- ตัวลดแรงเลี้ยว/หมุน ----------
// STEER_GAIN: คูณกับค่า steering ก่อนผสมกับ throttle ยิ่งน้อยยิ่งเลี้ยว/หมุนเบาลง (0.0 - 1.0)
// ไม่กระทบความเร็วเดินหน้า/ถอยหลังตรงๆ (throttle) เลย
const float STEER_GAIN = 0.6;

// ---------- ตัวแปรจับเวลา ----------
unsigned long lastDisplayTime = 0;
const int DISPLAY_INTERVAL = 100;   // อัปเดตจอทุก 100 ms

unsigned long lastRCTime = 0;
const int RC_TIMEOUT = 300;         // ถ้าไม่มีสัญญาณเกิน 300 ms ถือว่าขาดการเชื่อมต่อ

// ---------- อ่านค่าพัลส์จากขา RC แล้วแปลงเป็นช่วง RC_MIN - RC_MAX ----------
int readRC(uint8_t pin) {
  unsigned long pw = pulseIn(pin, HIGH, 25000UL);
  if (pw == 0) return RC_MID;       // อ่านไม่ได้ -> ถือว่าอยู่ตรงกลาง (หยุดนิ่ง)
  lastRCTime = millis();
  return (int)constrain((long)pw, RC_MIN, RC_MAX);
}

// ---------- แปลงค่าพัลส์เป็นความเร็ว (-50 ถึง 50) ----------
int rcToSpeed(int pulse, int trim = 0) {
  int delta = pulse - RC_MID - trim;
  if (abs(delta) < RC_DEADZONE) return 0;

  int range = (RC_MAX - RC_MID) - RC_DEADZONE;
  int speed = map(abs(delta) - RC_DEADZONE, 0, range, 0, 50);
  speed = constrain(speed, 0, 50);

  return (delta > 0) ? speed : -speed;
}

// ---------- สร้างแถบแสดงระดับความเร็วบนจอ OLED ----------
void speedBar(char* buf, int speed) {
  int level = map(abs(speed), 0, 50, 0, 5);
  buf[0] = '[';
  for (int i = 0; i < 5; i++) {
    if (speed >= 0) {
      buf[1 + i] = (i < level) ? '#' : '-';
    } else {
      buf[1 + i] = (i >= (5 - level)) ? '#' : '-';
    }
  }
  buf[6] = ']';
  buf[7] = '\0';
}

void setup() {
  pinMode(RC_THROTTLE_PIN, INPUT);
  pinMode(RC_STEER_PIN,    INPUT);

  motor(1, 0);
  motor(2, 0);

  lastRCTime = millis();

  oled.clear();
  oled.textSize(1);
  oled.text(0, 0, "== RC Soccer Bot ==");
  oled.text(1, 0, "--------------------");
  oled.show();
  delay(500);
}

void loop() {
  int throttle = rcToSpeed(readRC(RC_THROTTLE_PIN));
  int steering = rcToSpeed(readRC(RC_STEER_PIN), STEER_TRIM);

  // ลดแรงเลี้ยว/หมุนด้วย STEER_GAIN ก่อนนำไปผสมกับ throttle
  int steerEffective = (int)round(steering * STEER_GAIN);

  int leftSpeed  = constrain(throttle + steerEffective + LEFT_TRIM,  -50, 50);
  int rightSpeed = constrain(throttle - steerEffective + RIGHT_TRIM, -50, 50);

  motor(1, leftSpeed);
  motor(2, rightSpeed);

  if (millis() - lastDisplayTime >= DISPLAY_INTERVAL) {
    lastDisplayTime = millis();

    bool rcOK = (millis() - lastRCTime) < RC_TIMEOUT;

    oled.text(0, 0, "RC Soccer Bot  %s", rcOK ? "[RC:OK]" : "[RC:LOST]");
    oled.text(1, 0, "--------------------");

    oled.text(2, 0, "THR:%4d   STR:%4d", throttle, steering);

    char barL[8], barR[8];
    speedBar(barL, leftSpeed);
    speedBar(barR, rightSpeed);
    oled.text(3, 0, "L %s %4d", barL, leftSpeed);
    oled.text(4, 0, "R %s %4d", barR, rightSpeed);

    oled.show();
  }

  delay(100);
}