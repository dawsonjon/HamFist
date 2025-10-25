#include "cw_dsp.h"
#include "ADCAudio.h"
#include "fft.h"
#include "font_8x5.h"
#include "font_16x12.h"
#include "ili934x.h"

//CONFIGURATION SECTION
///////////////////////////////////////////////////////////////////////////////

#define PIN_MISO 12 //not used by TFT but part of SPI bus
#define PIN_CS   13
#define PIN_SCK  14
#define PIN_MOSI 15 
#define PIN_DC   11
#define SPI_PORT spi1

//#define ROTATION R0DEG
//#define ROTATION R90DEG
//#define ROTATION R180DEG
#define ROTATION R270DEG
//#define ROTATION MIRRORED0DEG
//#define ROTATION MIRRORED90DEG
//#define ROTATION MIRRORED180DEG
//#define ROTATION MIRRORED270DEG

//#define INVERT_COLOURS false
#define INVERT_COLOURS true

#define INVERT_DISPLAY false
//#define INVERT_DISPLAY true

#define DISPLAY_TYPE 0 //ILI934x 320x240 TFT DIsplay
//#define DISPLAY_TYPE 1 //ILI934x (driver 2) 320x240 TFT DIsplay
//#define DISPLAY_TYPE 2 //ST7796 480x320
//#define DISPLAY_TYPE 3 //ILI9488 480x320

#if DISPLAY_TYPE == 0
  const uint16_t width = 320u;
  const uint16_t height = 240u;
  e_display_type display_type = ILI9341;
#elif DISPLAY_TYPE == 1
  const uint16_t width = 320u;
  const uint16_t height = 240u;
  e_display_type display_type = ILI9341_2;
#elif DISPLAY_TYPE == 2
  const uint16_t width = 480u;
  const uint16_t height = 320u;
  e_display_type display_type = ST7796;
#else
  const uint16_t width = 480u;
  const uint16_t height = 320u;
  e_display_type display_type = ILI9488;
#endif


ILI934X *display;
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(1);  // wait for serial port to connect.

  Serial.println("HamFist Copyright (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");

  configure_display();
  fft_initialise();
}

class my_cw_dsp : public c_cw_dsp
{
  std::string decoded_text[7];
  virtual void decode(uint16_t cluster, std::string text)
  {
    decoded_text[cluster]+=text;
    int excess_length = decoded_text[cluster].size() - 50;
    if(excess_length > 0) decoded_text[cluster].erase(0, excess_length);
    display->drawString(20, 20*cluster, font_8x5, decoded_text[cluster].c_str(), COLOUR_WHITE, COLOUR_BLACK);
  }
};

void loop() {
  
  ADCAudio adc_audio;
  adc_audio.begin(28, 15000);
  my_cw_dsp cw_dsp;

  uint16_t frame_line = 0;
  while(1) {
    static int16_t *samples;

    //fetch a new block of 1024 samples
    samples = adc_audio.input_samples();
    for(int sample_number=0; sample_number<1024; ++sample_number)
    {
      cw_dsp.process_sample(samples[sample_number]);

      if(sample_number%128 == 0) {
        uint32_t *magnitudes = cw_dsp.get_magnitudes();
        uint16_t waterfall_line[FRAME_SIZE/2];
        for(uint16_t idx=0; idx<FRAME_SIZE/2; ++idx) {
          uint32_t magnitude = magnitudes[idx];
          float scaled_magnitude = magnitude>0?20*log10(magnitude):0;
          uint8_t pixel = scaled_magnitude * 3.5;
          waterfall_line[idx] = display->colour565(0, 0, pixel);
        }
        display->writeVLine(frame_line++, 239-FRAME_SIZE/2-2, FRAME_SIZE/2, waterfall_line);
        if(frame_line == 320) frame_line = 0;
      }
    }


  }

}

void configure_display()
{
  spi_init(SPI_PORT, 75000000);
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
  gpio_init(PIN_CS);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_init(PIN_DC);
  gpio_set_dir(PIN_DC, GPIO_OUT);
  display = new ILI934X(SPI_PORT, PIN_CS, PIN_DC, width, height);
  display->init(ROTATION, INVERT_COLOURS, INVERT_DISPLAY, display_type);
  display->powerOn(true);
  display->clear();
}