// 17: 給水LED
// 18: 弱
// 23: 強
// 4: ボタン (オフ→弱→強を繰り返す)

#define LED_NEED_WATER 17
#define LED_WORKING_WEAK 18
#define LED_WORKING_STRONG 23
#define BUTTON 4

#define LED_STATE_LIGHT LOW // LOWで光っている状態

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_NEED_WATER, INPUT);
  pinMode(LED_WORKING_WEAK, INPUT);
  pinMode(LED_WORKING_STRONG, INPUT);
  pinMode(BUTTON, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  // Serial.print(digitalRead(17));
  // Serial.print('\t');
  // Serial.print(digitalRead(18));
  // Serial.print('\t');
  // Serial.println(digitalRead(23));
}
