#include "cw_decode.h"
#include "cw_test_data.h"

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(1);  // wait for serial port to connect.

  Serial.println("HamFist Copyright (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");

  pinMode(17, INPUT_PULLUP); 
  
}

void loop() {

  s_observation capture[101];
  uint32_t last_time = millis();
  bool last_value = 0;
  int observations = 0;

  for(int idx=0; idx<101; ++idx) {
    while(1){
      bool value = !digitalRead(17);
      if(value != last_value) {
        Serial.printf("%u %u\n", last_value, millis()-last_time);
        capture[observations++] = {last_value, static_cast<float>(millis()-last_time)};
        last_value = value;
        last_time = millis();
        break;
      }
      if(!value && (millis()-last_time) > 2000) break;
      sleep_ms(10);
    }
  }


  if(observations) {
    uint32_t t0 = millis();
    decode_cw(capture, observations, 3);
    Serial.println(millis()-t0);
  }
}