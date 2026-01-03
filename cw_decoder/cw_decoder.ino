#include "cw_dsp.h"
#include "cw_encoder.h"
#include "ADCAudio.h"
#include "PWMAudio.h"
#include "fft.h"
#include "font_8x5.h"
#include "font_16x12.h"
#include "ili934x.h"
#include "splash.h"
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

//COLOUR SCHEME
uint16_t BACKGROUND        = display->colour565(  8,  12,  20);  // very dark blue-black
uint16_t PANEL_BG          = COLOUR_NAVY;//display->colour565( 16,  24,  40);  // RX area background
uint16_t DIVIDER_LINE      = display->colour565(  0, 180, 255);  // subtle separators
uint16_t INACTIVE_BORDER   = display->colour565( 32,  48,  80);
uint16_t ACTIVE_BORDER     = COLOUR_AQUA; //display->colour565(  0, 180, 255);  // cyan highlight
uint16_t TEXT_HEADING      = COLOUR_AQUA; //display->colour565(  0, 180, 255);  // cyan highlight
uint16_t TEXT_PRIMARY      = COLOUR_WHITE; //display->colour565(220, 230, 240);  // confirmed decode
uint16_t TEXT_RECENT       = COLOUR_AQUA; //display->colour565(  0, 180, 255);  // last decoded text
uint16_t TEXT_DIM          = COLOUR_GREY; //display->colour565(120, 140, 170);  // inactive channels
uint16_t TEXT_PARTIAL      = COLOUR_ORANGE;//display->colour565(230, 170,  90);  // tentative / partial decode
uint16_t TX_BORDER         = COLOUR_ORANGE;//display->colour565(255, 160,   0);   // slightly deeper amber
uint16_t TX_BG             = COLOUR_NAVY;//display->colour565( 32,  24,  12);   // dark warm background
uint16_t TX_TEXT           = COLOUR_ORANGE;//display->colour565(230, 170,  90);   // muted amber

uint16_t PADDING = 2;
uint16_t HEADING_Y = 4;
uint16_t HEADING_HEIGHT = PADDING + 16 + PADDING;
uint16_t PANEL_X = 4;
uint16_t PANEL_Y = HEADING_Y + HEADING_HEIGHT + 4;
uint16_t PANEL_WIDTH = width - 8;
uint16_t WATERFALL_HEIGHT = 5;
uint16_t WATERFALL_WIDTH = PANEL_WIDTH - (2*PADDING);
uint16_t TEXT_HEIGHT = 8;
uint16_t PANEL_HEIGHT = PADDING + TEXT_HEIGHT + PADDING + TEXT_HEIGHT + PADDING + WATERFALL_HEIGHT + PADDING;
uint16_t PANEL_INTERVAL = PANEL_HEIGHT + PADDING;
uint16_t TX_PANEL_Y = PANEL_Y+PANEL_INTERVAL*NUM_CHANNELS + 4;
uint16_t TX_PANEL_HEIGHT = PADDING + TEXT_HEIGHT + PADDING + TEXT_HEIGHT;

void draw() {
  display->clear(BACKGROUND);
  
  for(int channel=0; channel<NUM_CHANNELS; ++channel)
  {
    uint16_t Y = PANEL_Y + (PANEL_INTERVAL * channel); 
    display->fillRect(PANEL_X, Y, PANEL_HEIGHT, PANEL_WIDTH, PANEL_BG);
  }
  for(int channel=1; channel<NUM_CHANNELS; ++channel)
  {
    uint16_t Y = PANEL_Y + (PANEL_INTERVAL * channel) - 1; 
  }
  const int active_channel = 1;
  display->drawRect(PANEL_X, PANEL_Y + PANEL_INTERVAL*active_channel, PANEL_HEIGHT, PANEL_WIDTH, ACTIVE_BORDER);
  display->fillRect(PANEL_X, TX_PANEL_Y, TX_PANEL_HEIGHT, PANEL_WIDTH, TX_BG);
  display->drawRect(PANEL_X, TX_PANEL_Y, TX_PANEL_HEIGHT, PANEL_WIDTH, TX_BORDER);
  const char heading[] = "Pico CW Decoder";
  int TEXT_X = (width - (strlen(heading) * 12))/2;
  display->drawString(TEXT_X, HEADING_Y, font_16x12, heading, COLOUR_WHITE, BACKGROUND);
  display->drawString(TEXT_X, HEADING_Y, font_16x12, "Pico", COLOUR_GREEN, BACKGROUND);
  //display->drawFastHline(PANEL_X, PANEL_X+PANEL_WIDTH-1, HEADING_Y + HEADING_HEIGHT, DIVIDER_LINE);
}

void setup() {
  Serial.begin(115200);
  //while (!Serial) delay(1);  // wait for serial port to connect.

  Serial.println("HamFist Copyright (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");

  configure_display();
  fft_initialise();
  display->writeImage(0, 0, 320, 240, splash);
  sleep_ms(4000);

}

class my_cw_dsp : public c_cw_dsp
{
  bool m_update_display = true;
  std::string decoded_text[NUM_CHANNELS];
  std::string partial_decoded_text[7];
  virtual void decode(uint16_t cluster, std::string text, std::string partial)
  {
    const int decoded_text_size = decoded_text[cluster].size();
    const int new_text_size = text.size();
    const int partial_size = partial.size();

    const int font_width = 6;
    const int text_width = (PANEL_WIDTH - (2*PADDING))/font_width;
    int excess_length = decoded_text_size + new_text_size + partial_size - text_width;
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
      const uint16_t TEXT_Y = PANEL_Y + (PANEL_INTERVAL * cluster) + PADDING + TEXT_HEIGHT + PADDING; 
      const uint16_t TEXT_X = PANEL_X + PADDING;
      display->drawString(TEXT_X,                                                  TEXT_Y, font_8x5, decoded_text[cluster].c_str(), TEXT_PRIMARY, PANEL_BG);
      display->drawString(TEXT_X+(font_width*decoded_text[cluster].size()),                 TEXT_Y, font_8x5, text.c_str(), TEXT_RECENT, PANEL_BG);
      display->drawString(TEXT_X+(font_width*(decoded_text[cluster].size() + text.size())), TEXT_Y, font_8x5, partial.c_str(), TEXT_PARTIAL, PANEL_BG);
    }
    decoded_text[cluster]+=text;
  }
  public:
  void set_update_display(bool update_display){
    m_update_display = update_display;
  }
};

void send_cw(PWMAudio &audio_output, uint16_t *output_buffer, c_cw_encoder &cw_encoder) {
  static uint8_t ping_pong = 0;
  for(int sample_number=0; sample_number<ADC_BLOCK_SIZE; ++sample_number)
  {
    uint16_t scaled_sample = ((cw_encoder.get_sample() >> 2) + 32767) >> 6;
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
  cw_encoder.set_wpm(12);
  //cw_encoder.set_string("the quick brown fox jumps over the lazy dog");
  cw_encoder.set_frequency_Hz(879);

  uint16_t waterfall_newest = 0;
  static uint8_t waterfall[320][FRAME_SIZE/2] = {0};
  cw_dsp.set_update_display(true);

  button button_up(17);
  button button_down(20);
  button button_right(21);
  button button_left(22);
  c_text_entry text_entry(display, button_left, button_right, button_down, button_up);
  c_multi_text_entry messages(display, button_left, button_right, button_down, button_up);
  
  s_messages messages_text = {
    {
    "CQ CQ CQ DE MYCALL MYCALL MYCALL K",
    "CQ CQ DE MYCALL K",
    "CALLSIGN DE MYCALL MYCALL K",
    "CALLSIGN DE MYCALL RST 599 NAME OM QTH QTH K",
    "CALLSIGN DE MYCALL UR RST 599 599 K",
    "CALLSIGN DE MYCALL RIG 5W ANT DIPOLE K",
    "CALLSIGN DE MYCALL WX HR SUNNY TEMP 15C K",
    "CALLSIGN DE MYCALL OP OM HW CPY? K",
    "CALLSIGN DE MYCALL TNX FER QSO OM K",
    "CALLSIGN DE MYCALL PSE QRS K",
    "CALLSIGN DE MYCALL PSE AGN K",
    "R R R FB OM K",
    "QRZ DE MYCALL K",
    "CALLSIGN DE MYCALL BK",
    "CALLSIGN DE MYCALL 73 ES TNX FER QSO SK",
    "73 73 DE MYCALL SK"}
  };
  messages.m_messages = messages_text;

  const uint16_t text_len = 250;
  char text[text_len] = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG";
  char send_text[text_len] = "";
  static uint16_t output_buffer[2*ADC_BLOCK_SIZE];
  bool new_text = false;
  bool draw_waterfall = true;
  bool needs_draw = true;
  
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
          uint16_t waterfall_line[WATERFALL_WIDTH];
          for(uint16_t pixel_x=0; pixel_x<WATERFALL_WIDTH; ++pixel_x){
            uint16_t waterfall_x;
            if(pixel_x <= waterfall_newest){
              waterfall_x = waterfall_newest-pixel_x;
            } else {
              waterfall_x = WATERFALL_WIDTH+waterfall_newest-pixel_x;
            }
            waterfall_line[WATERFALL_WIDTH-1-pixel_x] = display->colour565(0, 0, waterfall[waterfall_x][waterfall_y]);
          }
          uint8_t channel = waterfall_y/5;
          uint8_t sub_y = waterfall_y%5; 
          uint8_t y = PANEL_Y + (channel * PANEL_INTERVAL) + PADDING+TEXT_HEIGHT+PADDING+TEXT_HEIGHT+PADDING + sub_y;

          display->writeHLine(PANEL_X + PADDING, y, WATERFALL_WIDTH, waterfall_line);
        }
          
        if(++waterfall_newest == WATERFALL_WIDTH) waterfall_newest = 0;
      }

    }

    if(text_entry.is_active()) {
      cw_dsp.set_update_display(false);
      draw_waterfall = false;
      text_entry.run();
      new_text = true;
      needs_draw = true;
    } else if(messages.is_active()) {
      cw_dsp.set_update_display(false);
      draw_waterfall = false;
      messages.run();
      needs_draw = true;
    } else {
      cw_dsp.set_update_display(true);
      draw_waterfall = true;
      if(button_left.is_pressed()) {
        text_entry.raise(text, text_len);
      } else if (button_right.is_pressed()) {
        messages.raise();
      }
      if(new_text) {
        new_text = false;
        strcpy(send_text, text);
        cw_encoder.set_string(send_text);
      }
      if(needs_draw){
        needs_draw = false;
        draw();
      }

      int x = 8;
      for(int channel=0; channel<NUM_CHANNELS; ++channel)
      {
        uint16_t TEXT_Y = PANEL_Y + (PANEL_INTERVAL * channel) + PADDING; 
        char buffer[50];
        uint16_t frequency = ((2 * channel * 586)+586)/2; 
        snprintf(buffer, 50, "chan: %u +%3uHz %3u%% %3.0fwpm %3.0fdB(500 Hz)", channel, frequency, cw_dsp.get_buffer_percent(channel), cw_dsp.get_WPM(channel), cw_dsp.get_snr(channel));
        int TEXT_X = (width - (strlen(buffer) * 6))/2;
        display->drawString(TEXT_X, TEXT_Y, font_8x5, buffer, TEXT_HEADING, PANEL_BG);
      }

      uint16_t length = strlen(send_text);
      uint16_t cursor = cw_encoder.get_cursor();
      uint16_t left_to_send = length-cursor;
      const int font_width = 6;
      const int font_height = 8;
      const int text_width = 2*((PANEL_WIDTH - (2*PADDING))/(2*font_width));
      const int half_text_width = text_width/2;
      
      char buffer[51];
      uint16_t amount_to_copy = half_text_width<left_to_send?half_text_width:left_to_send;
      memcpy(buffer, send_text+cursor, amount_to_copy); buffer[amount_to_copy] = 0;
      display->fillRect(PANEL_X + PADDING + font_width*(half_text_width+amount_to_copy), TX_PANEL_Y + PADDING, font_height, font_width*(half_text_width-amount_to_copy), TX_BG);
      display->drawString(PANEL_X + PADDING+font_width*half_text_width, TX_PANEL_Y+PADDING, font_8x5, buffer, TX_TEXT, PANEL_BG);
      amount_to_copy = half_text_width<cursor?half_text_width:cursor;
      memcpy(buffer, send_text+cursor-amount_to_copy, amount_to_copy); buffer[amount_to_copy] = 0;
      display->drawString(PANEL_X + PADDING + (half_text_width-amount_to_copy)*font_width, TX_PANEL_Y + PADDING, font_8x5, buffer, COLOUR_WHITE, TX_BG);



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