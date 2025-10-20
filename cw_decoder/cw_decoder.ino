#include "cw_dsp.h"
#include "ADCAudio.h"
#include "fft.h"


void setup() {
  Serial.begin(115200);
  while (!Serial) delay(1);  // wait for serial port to connect.

  Serial.println("HamFist Copyright (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");

  fft_initialise();
}

void loop() {
  
  ADCAudio adc_audio;
  adc_audio.begin(28, 15000);
  c_cw_dsp cw_dsp;

  while(1) {
    static int16_t *samples;

    //fetch a new block of 1024 samples
    samples = adc_audio.input_samples();
    for(int sample_number=0; sample_number<1024; ++sample_number)
    {
      cw_dsp.process_sample(samples[sample_number]);
    }
  }

}