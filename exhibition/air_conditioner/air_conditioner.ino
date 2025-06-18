const int servoPin = 10;
float DutyCycle = 500;

const int ledPin = 8;

void setup() {
  Serial.begin(115200);
  Serial.println("setup start");
  pinMode(servoPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
}

const int CLOSE = 1200;
const int OPEN = 2200;

void loop() {
  digitalWrite(ledPin, HIGH);
  for (DutyCycle = CLOSE; DutyCycle <= OPEN; DutyCycle += 10) {
    digitalWrite(servoPin, HIGH);
    delayMicroseconds(DutyCycle);
    digitalWrite(servoPin, LOW);
    delayMicroseconds(20*1000 - DutyCycle);
    Serial.println(DutyCycle);
  }
  digitalWrite(ledPin, LOW);
  delay(100);
  for (DutyCycle = OPEN; DutyCycle >= CLOSE; DutyCycle -= 10) {
    digitalWrite(servoPin, HIGH);
    delayMicroseconds(DutyCycle);
    digitalWrite(servoPin, LOW);
    delayMicroseconds(20*1000 - DutyCycle);
    Serial.println(DutyCycle);
  }
  delay(1000);
}