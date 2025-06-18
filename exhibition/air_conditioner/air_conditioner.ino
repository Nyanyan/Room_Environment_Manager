#include <esp_now.h>
#include <WiFi.h>

#define CHANNEL 1

const int servoPin = 10;
float DutyCycle = 500;
const int DEG_CLOSE = 1200;
const int DEG_OPEN = 2200;

const int ledPin = 8;


// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

// config AP SSID
void configDeviceAP() {
  const char *SSID = "Slave_1";
  bool result = WiFi.softAP(SSID, "Slave_1_Password", CHANNEL, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
    Serial.print("AP CHANNEL "); Serial.println(WiFi.channel());
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("setup start");
  pinMode(servoPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);
}


void ac_on() {
  digitalWrite(ledPin, HIGH);
  for (DutyCycle = DEG_CLOSE; DutyCycle <= DEG_OPEN; DutyCycle += 10) {
    digitalWrite(servoPin, HIGH);
    delayMicroseconds(DutyCycle);
    digitalWrite(servoPin, LOW);
    delayMicroseconds(20*1000 - DutyCycle);
    // Serial.println(DutyCycle);
  }
}

void ac_off() {
  for (DutyCycle = DEG_OPEN; DutyCycle >= DEG_CLOSE; DutyCycle -= 10) {
    digitalWrite(servoPin, HIGH);
    delayMicroseconds(DutyCycle);
    digitalWrite(servoPin, LOW);
    delayMicroseconds(20*1000 - DutyCycle);
    // Serial.println(DutyCycle);
  }
  digitalWrite(ledPin, LOW);
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  Serial.println("received");
  if (data[0] == 'A' && data[1] == 'K'){ // Air-Kobito
    char datum = data[2];
    Serial.println(datum);
    if (datum == 'i') { // ON
      ac_on();
    } else if (datum == 'o') { // OFF
      ac_off();
    }
  }
}

void loop() {
  
}