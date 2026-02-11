#include <IRremoteESP8266.h>
#include <IRsend.h>

// true: Mitsubishi / false: Panasonic (default)
#define AC_USE_MITSUBISHI false

// IR LED output pin
#define AC_LED_PIN D7

#if AC_USE_MITSUBISHI
#include <ir_Mitsubishi.h>
IRMitsubishiAC ac(AC_LED_PIN);
#else
#include <ir_Panasonic.h>
IRPanasonicAc ac(AC_LED_PIN);
#endif

void sendAcOff() {
#if AC_USE_MITSUBISHI
  ac.off();
#else
  ac.setModel(kPanasonicRkr);
  ac.off();
  ac.setFan(kPanasonicAcFanHigh);
  ac.setMode(kPanasonicAcCool);
  ac.setTemp(20);
  ac.setSwingVertical(kPanasonicAcSwingVAuto);
  ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
#endif
  ac.send();
}

void setup() {
  ac.begin();
  delay(500);
}

void loop() {
  sendAcOff();       // 送信するのはオフのみ
  delay(1000);       // 連続送信しないよう少し待つ
}
