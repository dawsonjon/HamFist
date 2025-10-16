#include "cw_decode.h"
#include "cw_test_data.h"

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(1);  // wait for serial port to connect.

  Serial.println("Pico SSTV Copyright (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");
  
}

void loop() {
  uint32_t t0 = millis();
  decode_cw(cw_signal, sizeof(cw_signal)/sizeof(s_observation), 10);
  Serial.println(millis()-t0);
}