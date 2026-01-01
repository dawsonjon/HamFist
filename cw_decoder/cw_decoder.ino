#include "cw_dsp.h"
#include "cw_encoder.h"
#include "ADCAudio.h"
#include "PWMAudio.h"
#include "fft.h"
#include "font_8x5.h"
#include "font_16x12.h"
#include "ili934x.h"

#include "text_entry.h"

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

#if DISPLAY_TYPE == 0
  const uint16_t width = 320u;
  const uint16_t height = 240u;
  e_display_type display_type = ILI9341;
#elif DISPLAY_TYPE == 1
  const uint16_t width = 320u;
  const uint16_t height = 240u;
  e_display_type display_type = ILI9341_2;
#endif


ILI934X *display;
void setup() {
  Serial.begin(115200);
  //while (!Serial) delay(1);  // wait for serial port to connect.

  Serial.println("HamFist Copyright (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");

  configure_display();
  fft_initialise();
}

class my_cw_dsp : public c_cw_dsp
{
  bool m_update_display = true;
  std::string decoded_text[NUM_CHANNELS];
  std::string partial_decoded_text[7];
  virtual void decode(uint16_t cluster, std::string text, std::string partial)
  {
    int decoded_text_size = decoded_text[cluster].size();
    int new_text_size = text.size();
    int partial_size = partial.size();

    int excess_length = decoded_text_size + new_text_size + partial_size - 50;
    if(excess_length > 0) {
      int chars_to_remove = std::min(excess_length, decoded_text_size);
      decoded_text[cluster].erase(0, chars_to_remove);
      excess_length -= chars_to_remove;
    }
    if(excess_length > 0) {
      int chars_to_remove = std::min(excess_length, new_text_size);
      text.erase(0, chars_to_remove);
      excess_length -= chars_to_remove;
    }
    
    if(m_update_display) {
      display->drawString(20,                                                  37*cluster + 2+16, font_8x5, decoded_text[cluster].c_str(), COLOUR_WHITE, COLOUR_BLACK);
      display->drawString(20+(6*decoded_text[cluster].size()),                 37*cluster + 2+16, font_8x5, text.c_str(), COLOUR_AQUA, COLOUR_BLACK);
      display->drawString(20+(6*(decoded_text[cluster].size() + text.size())), 37*cluster + 2+16, font_8x5, partial.c_str(), COLOUR_ORANGE, COLOUR_BLACK);
    }
    decoded_text[cluster]+=text;
  }
  public:
  void set_update_display(bool update_display){
    Serial.print("waterfall");
    Serial.println(update_display);
    m_update_display = update_display;
  }
};

void send_cw(PWMAudio &audio_output, uint16_t *output_buffer, c_cw_encoder &cw_encoder) {
  static uint8_t ping_pong = 0;
  for(int sample_number=0; sample_number<ADC_BLOCK_SIZE; ++sample_number)
  {
    uint16_t scaled_sample = (cw_encoder.get_sample() + 32767) >> 4;
    output_buffer[ping_pong * ADC_BLOCK_SIZE + sample_number] = scaled_sample;
  }
  
  audio_output.output_samples(&output_buffer[ping_pong*ADC_BLOCK_SIZE], ADC_BLOCK_SIZE);
  ping_pong ^= 1;
}

void loop() {
  
  ADCAudio adc_audio;
  adc_audio.begin(28, 15000);
  PWMAudio audio_output;
  audio_output.begin(0, 15000, rp2040.f_cpu());

  my_cw_dsp cw_dsp;
  c_cw_encoder cw_encoder(15000);
  cw_encoder.set_wpm(30);
  //cw_encoder.set_string("the quick brown fox jumps over the lazy dog");
  cw_encoder.set_frequency_Hz(1000);

  uint16_t waterfall_newest = 0;
  static uint8_t waterfall[320][FRAME_SIZE/2] = {0};
  cw_dsp.set_update_display(true);

  button button_up(17);
  button button_down(20);
  button button_right(21);
  button button_left(22);
  c_text_entry text_entry(display, button_left, button_right, button_down, button_up);

  const uint16_t text_len = 250;
  char text[text_len] = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG";
  char send_text[text_len];
  static uint16_t output_buffer[2*ADC_BLOCK_SIZE];
  bool new_text = false;
  bool draw_waterfall = true;

  display->clear(COLOUR_BLACK);
  
  while(1) {
    int16_t *samples;

    send_cw(audio_output, output_buffer, cw_encoder); 

    //fetch a new block of ADC_BLOCK_SIZE samples
    samples = adc_audio.input_samples();
    for(int sample_number=0; sample_number<ADC_BLOCK_SIZE; ++sample_number)
    {
      //uint32_t start = millis();
      cw_dsp.process_sample(samples[sample_number]);
      //Serial.print("processing time: ");
      //Serial.println(millis() - start);

      if(draw_waterfall && sample_number%256 == 0) {
        uint32_t *magnitudes = cw_dsp.get_magnitudes();
        
        for(uint16_t bin=0; bin<FRAME_SIZE/2; ++bin) {
          uint32_t magnitude = magnitudes[bin];
          float scaled_magnitude = std::max(0.0, 20*log10(magnitude+1)-20);
          uint8_t pixel = scaled_magnitude * 3.5;        
          waterfall[waterfall_newest][bin] = pixel;
        }

        for(uint16_t waterfall_y=0; waterfall_y<30; ++waterfall_y) {
          uint16_t waterfall_line[320];
          for(uint16_t pixel_x=0; pixel_x<320; ++pixel_x){
            uint16_t waterfall_x;
            if(pixel_x <= waterfall_newest){
              waterfall_x = waterfall_newest-pixel_x;
            } else {
              waterfall_x = 320+waterfall_newest-pixel_x;
            }
            waterfall_line[319-pixel_x] = display->colour565(0, 0, waterfall[waterfall_x][waterfall_y]);
          }
          uint8_t channel = waterfall_y/5;
          uint8_t sub_y = waterfall_y%5; 
          uint8_t y = channel * 37 + 2+28 + sub_y;
          display->writeHLine(0, y, 320, waterfall_line);
        }
          
        if(++waterfall_newest == 320) waterfall_newest = 0;
      }

    }

    if(text_entry.is_active()) {
      cw_dsp.set_update_display(false);
      draw_waterfall = false;
      text_entry.run();
      new_text = true;
    }
    else {
      cw_dsp.set_update_display(true);
      draw_waterfall = true;
      if(button_left.is_pressed()) {
        Serial.printf("pressed\n");
        text_entry.raise(text, text_len);
      }
      if(new_text) {
        new_text = false;
        strcpy(send_text, text);
        cw_encoder.set_string(send_text);
      }

      int x = 8;
      for(int channel=0; channel<NUM_CHANNELS; ++channel)
      {
        char buffer[50];
        uint16_t frequency = ((2 * channel * 586)+586)/2; 
        snprintf(buffer, 50, "chan: %u +%3uHz %3u%% %3.0fwpm %3.0fdB(500 Hz)", channel, frequency, cw_dsp.get_buffer_percent(channel), cw_dsp.get_WPM(channel), cw_dsp.get_snr(channel));
        display->drawString(10, channel * 37 + 2 + 4, font_8x5, buffer, COLOUR_GREEN, COLOUR_BLACK);
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