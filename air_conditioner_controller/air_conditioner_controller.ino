#include <esp_now.h>
#include <WiFi.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Panasonic.h>

#define CHANNEL 1

#define AC_LED_PIN 10

#define ON_LED_PIN 9

#define AC_N_TRY 1

// air conditioner
IRPanasonicAc ac(AC_LED_PIN);



void init_ac(){
  ac.begin();
}

void ac_print_state() {
  // Display the settings.
  Serial.println("Panasonic A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  // Display the encoded IR sequence.
  unsigned char* ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kPanasonicAcStateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();
}

void ac_send() {
  ac.send();
  ac_print_state();
}


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

  pinMode(AC_LED_PIN, OUTPUT);
  pinMode(ON_LED_PIN, OUTPUT);
  digitalWrite(ON_LED_PIN, LOW);

  init_ac();

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

  for (int i = 0; i < 3; ++i) {
    digitalWrite(ON_LED_PIN, HIGH);
    delay(100);
    digitalWrite(ON_LED_PIN, LOW);
    delay(100);
  }
}







void ac_cool_on(int set_temp){
  for (int i = 0; i < AC_N_TRY; ++i){
    ac.setModel(kPanasonicRkr);
    ac.on();
    ac.setFan(kPanasonicAcFanAuto);
    ac.setMode(kPanasonicAcCool);
    ac.setTemp(set_temp);
    ac.setSwingVertical(kPanasonicAcSwingVHighest);
    ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
    ac_send();
    delay(1000);
  }
}

void ac_dry_on(int set_temp){
  for (int i = 0; i < AC_N_TRY; ++i){
    ac.setModel(kPanasonicRkr);
    ac.on();
    ac.setFan(kPanasonicAcFanAuto);
    ac.setMode(kPanasonicAcDry);
    ac.setTemp(set_temp);
    ac.setSwingVertical(kPanasonicAcSwingVHighest);
    ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
    ac_send();
    delay(1000);
  }
}

void ac_heat_on(int set_temp){
  for (int i = 0; i < AC_N_TRY; ++i){
    ac.setModel(kPanasonicRkr);
    ac.on();
    ac.setFan(kPanasonicAcFanAuto);
    ac.setMode(kPanasonicAcHeat);
    ac.setTemp(set_temp);
    ac.setSwingVertical(kPanasonicAcSwingVHighest);
    ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
    ac_send();
    delay(1000);
  }
}



void ac_off(){
  for (int i = 0; i < 5; ++i){
    ac.setModel(kPanasonicRkr);
    ac.off();
    ac.setFan(kPanasonicAcFanAuto);
    ac.setMode(kPanasonicAcCool);
    ac.setTemp(20);
    ac.setSwingVertical(kPanasonicAcSwingVAuto);
    ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
    ac_send();
    delay(1000);
  }
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  Serial.println("received");
  if (data[0] == 'A' && data[1] == 'K'){ // Air-Kobito
    char mode = data[2];
    char temperature_char = data[3]; // 'A' + temperature(int)
    int temperature = temperature_char - 'A';
    Serial.print(mode);
    Serial.print(' ');
    Serial.print(temperature_char);
    Serial.print(' ');
    Serial.println(temperature);
    if (mode == 'H') { // heat
      ac_heat_on(temperature);
    } else if (mode == 'C') { // cool
      ac_cool_on(temperature);
    } else if (mode == 'D') { // dry
      ac_dry_on(temperature);
    } else if (mode == 'F') { // off
      ac_off();
    }
  }
}

void loop() {
  // digitalWrite(AC_LED_PIN, HIGH);
  // ac_off();
  // Serial.println("off");
  // printState();

  // ac_heat_on(20);
  // Serial.println("heat 20");

  // delay(3000);
}