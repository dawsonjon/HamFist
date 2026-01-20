#include <EEPROM.h>

#include "cw_dsp.h"
#include "cw_encoder.h"
#include "ADCAudio.h"
#include "PWMAudio.h"
#include "fft.h"
#include "font_8x5.h"
#include "font_16x12.h"
#include "ili934x.h"
#include "splash.h"
#include "header.h"
#include "gui_components.h"
#include "utils.h"

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

struct s_settings {
  int tx_wpm;
  int tx_channel;
  char my_callsign[17];
  char their_callsign[17];
  s_messages messages;
};

s_settings settings = {
  12, //WPM
  1, //active channel
  "XYZ123",
  "ZYX321",
  { //messages
  "CQ CQ CQ DE @MYCALL @MYCALL @MYCALL K",
  "CQ CQ DE @MYCALL K",
  "@CALLSIGN DE @MYCALL @MYCALL K",
  "@CALLSIGN DE @MYCALL RST 599 NAME OM QTH QTH K",
  "@CALLSIGN DE @MYCALL UR RST 599 599 K",
  "@CALLSIGN DE @MYCALL RIG 5W ANT DIPOLE K",
  "@CALLSIGN DE @MYCALL WX HR SUNNY TEMP 15C K",
  "@CALLSIGN DE @MYCALL OP OM HW CPY? K",
  "@CALLSIGN DE @MYCALL TNX FER QSO OM K",
  "@CALLSIGN DE @MYCALL PSE QRS K",
  "@CALLSIGN DE @MYCALL PSE AGN K",
  "R R R FB OM K",
  "QRZ DE @MYCALL K",
  "@CALLSIGN DE @MYCALL BK",
  "@CALLSIGN DE @MYCALL 73 ES TNX FER QSO SK",
  "73 73 DE @MYCALL SK"
  }
};

class my_cw_dsp : public c_cw_dsp
{
  bool m_update_display = true;
  std::string decoded_text[NUM_CHANNELS];
  std::string partial_decoded_text[7];
  virtual void decode(uint16_t cluster, std::string text, std::string partial);
  public:
  void set_update_display(bool update_display);
};

void setup() {
  Serial.begin(115200);
  //while (!Serial) delay(1);  // wait for serial port to connect.

  Serial.println("HamFist Copyright (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");

  gpio_init(17); gpio_set_dir(17, GPIO_IN);  gpio_pull_up(17);
  gpio_init(20); gpio_set_dir(20, GPIO_IN);  gpio_pull_up(20);
  gpio_init(21); gpio_set_dir(21, GPIO_IN);  gpio_pull_up(21);
  gpio_init(22); gpio_set_dir(22, GPIO_IN);  gpio_pull_up(22);

  configure_display();
  fft_initialise();
  EEPROM.begin(8192);
  load();
  display->writeImage(0, 0, 320, 240, splash);
  sleep_ms(4000);
}

void loop() {
  
  //Audio input and output
  ADCAudio adc_audio;
  adc_audio.begin(28, 15000);
  PWMAudio audio_output;
  audio_output.begin(0, 15000, rp2040.f_cpu());

  //CW encoder and decoder
  my_cw_dsp cw_dsp;
  c_cw_encoder cw_encoder(15000);
  cw_dsp.set_update_display(true);

  bool enable_waterfall = true;
  
  while(1) {
    static uint16_t output_buffer[2*ADC_BLOCK_SIZE];
    send_cw(audio_output, output_buffer, cw_encoder);
    int16_t *samples = adc_audio.input_samples();

    uint32_t start1 = millis();
    for(int sample_number=0; sample_number<ADC_BLOCK_SIZE; ++sample_number)
    {
      cw_dsp.process_sample(samples[sample_number]);
      if(enable_waterfall && sample_number%256 == 0) draw_waterfall(cw_dsp);
    }
    //Serial.print("processing time: ");
    //Serial.println(millis() - start1);

    update_ui(cw_dsp, cw_encoder, enable_waterfall);
    while(1) {
      if((millis() - start1) > 67 && (millis() - start1) < 69) update_ui(cw_dsp, cw_encoder, enable_waterfall);
      sleep_ms(1);
      if((millis() - start1) >= 69) break;
    }

  }
}

///////////////////////////////////////////////////////////////////////////////
//User Interfacce

//COLOUR SCHEME
const uint16_t BACKGROUND        = display->colour565(  8,  12,  20);
const uint16_t PANEL_BG          = COLOUR_NAVY;
const uint16_t DIVIDER_LINE      = display->colour565(  0, 180, 255);
const uint16_t INACTIVE_BORDER   = display->colour565( 32,  48,  80);
const uint16_t ACTIVE_BORDER     = COLOUR_AQUA;
const uint16_t TEXT_HEADING      = COLOUR_AQUA;
const uint16_t TEXT_PRIMARY      = COLOUR_WHITE;
const uint16_t TEXT_RECENT       = COLOUR_AQUA;
const uint16_t TEXT_DIM          = COLOUR_GREY;
const uint16_t TEXT_PARTIAL      = COLOUR_ORANGE;
const uint16_t TX_BORDER         = COLOUR_ORANGE;
const uint16_t TX_BG             = COLOUR_NAVY;
const uint16_t TX_TEXT           = COLOUR_ORANGE;

//LAYOUT
const uint16_t PADDING = 2;
const uint16_t HEADING_Y = 0;
const uint16_t HEADING_HEIGHT = 24;
const uint16_t PANEL_X = 4;
const uint16_t PANEL_Y = HEADING_Y + HEADING_HEIGHT + 4;
const uint16_t PANEL_WIDTH = width - 8;
const uint16_t WATERFALL_HEIGHT = 5;
const uint16_t WATERFALL_WIDTH = PANEL_WIDTH - (2*PADDING);
const uint16_t TEXT_HEIGHT = 8;
const uint16_t PANEL_HEIGHT = PADDING + TEXT_HEIGHT + PADDING + TEXT_HEIGHT + PADDING + WATERFALL_HEIGHT + PADDING;
const uint16_t PANEL_INTERVAL = PANEL_HEIGHT + PADDING;
const uint16_t TX_PANEL_Y = PANEL_Y+PANEL_INTERVAL*NUM_CHANNELS + 4;
const uint16_t TX_PANEL_HEIGHT = PADDING + TEXT_HEIGHT + PADDING + TEXT_HEIGHT;



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
  const int active_channel = settings.tx_channel;
  display->drawRect(PANEL_X, PANEL_Y + PANEL_INTERVAL*active_channel, PANEL_HEIGHT, PANEL_WIDTH, ACTIVE_BORDER);
  display->fillRect(PANEL_X, TX_PANEL_Y, TX_PANEL_HEIGHT, PANEL_WIDTH, TX_BG);
  display->drawRect(PANEL_X, TX_PANEL_Y, TX_PANEL_HEIGHT, PANEL_WIDTH, TX_BORDER);
  display->writeImage(0, 0, 320, HEADING_HEIGHT, header);
  
}

void update_ui(my_cw_dsp &cw_dsp, c_cw_encoder &cw_encoder, bool &enable_waterfall)
{
  static button button_up(17);
  static button button_down(20);
  static button button_right(21);
  static button button_left(22);
  static c_text_entry text_entry(display, button_left, button_right, button_down, button_up);
  static c_multi_text_entry messages(display, button_left, button_right, button_down, button_up, settings.messages);

  static uint8_t main_menu_selection = 0;
  static const char* const main_menu_items[] = {"Transmit WPM", "Transmit Channel", "My Callsign", "Their Callsign"};
  static c_menu main_menu(display, button_left, button_right, button_down, button_up, "Menu", main_menu_selection, main_menu_items, 4);
  static c_int_entry wpm_entry(display, button_left, button_right, button_down, button_up, "Transmit Speed (WPM)", settings.tx_wpm, 10, 50);
  static c_int_entry channel_entry(display, button_left, button_right, button_down, button_up, "Transmit Channel", settings.tx_channel, 0, 5);
  static c_text_entry my_callsign_entry(display, button_left, button_right, button_down, button_up);
  static c_text_entry their_callsign_entry(display, button_left, button_right, button_down, button_up);


  static const uint16_t text_len = 250;
  static char text[text_len] = "";
  static char send_text[text_len] = "";
  static bool needs_draw = true;
  static bool button_bar_active = false;
  static uint8_t button_bar_timeout = 255;

  if(my_callsign_entry.is_active()) { 
    my_callsign_entry.run(); 
    if(!my_callsign_entry.is_active()) save(); 
  }
  else if(their_callsign_entry.is_active()) { 
    their_callsign_entry.run(); 
    if(!their_callsign_entry.is_active()) save(); 
  }
  else if(wpm_entry.is_active()) { 
    wpm_entry.run(); 
    if(!wpm_entry.is_active() && wpm_entry.is_ok()) save(); 
  }
  else if(channel_entry.is_active()) {
    channel_entry.run();
    if(!channel_entry.is_active() && channel_entry.is_ok()) save();
  }
  else if(main_menu.is_active()) {
    cw_dsp.set_update_display(false);
    enable_waterfall = false;
    main_menu.run();
    if(!main_menu.is_active() && main_menu.is_ok()) {
      if(main_menu_selection == 0) wpm_entry.raise();
      if(main_menu_selection == 1) channel_entry.raise();
      if(main_menu_selection == 2) my_callsign_entry.raise(settings.my_callsign, 16);
      if(main_menu_selection == 3) their_callsign_entry.raise(settings.their_callsign, 16);
    }
    needs_draw = true;
  } else if(text_entry.is_active()) {
    cw_dsp.set_update_display(false);
    enable_waterfall = false;
    needs_draw = true;
    text_entry.run();
    if(!text_entry.is_active()) {
      strcpy(send_text, text);        
      str_replace_all(send_text, 250, "@MYCALL", settings.my_callsign);
      str_replace_all(send_text, 250, "@CALLSIGN", settings.their_callsign);
    }
  } else if(messages.is_active()) {
    cw_dsp.set_update_display(false);
    enable_waterfall = false;
    needs_draw = true;
    messages.run();
    if(!messages.is_active()) {
      strcpy(send_text, settings.messages.text[messages.get_selection()]);
      str_replace_all(send_text, 250, "@MYCALL", settings.my_callsign);
      str_replace_all(send_text, 250, "@CALLSIGN", settings.their_callsign);
      save();
    }
  } else {
    cw_dsp.set_update_display(true);
    enable_waterfall = true;
    if(button_bar_active) {
      if(button_left.is_pressed()) {
        button_bar_active = false;
        text_entry.raise(text, text_len);
      } else if (button_right.is_pressed()) {
        button_bar_active = false;
        messages.raise();
      } else if (button_up.is_pressed()) {
        button_bar_active = false;
        main_menu.raise();
      } else if (button_down.is_pressed()) {
        button_bar_active = false;
        needs_draw = true;
        if(cw_encoder.is_done()) {
          cw_encoder.set_string(send_text);
        }
      }
    } else {

      if(button_left.is_pressed() || button_right.is_pressed() || button_up.is_pressed() || button_down.is_pressed()) {
        if(cw_encoder.is_done()) {
          button_bar_active = true;
          button_bar_timeout = 15;
          draw_button_bar(display, "TX TEXT", "TX MEMORY", "SEND", "MENU");
        } else {
          cw_encoder.terminate();
        }
      }
    }
    cw_encoder.set_wpm(settings.tx_wpm);
    cw_encoder.set_frequency_Hz(((2 * settings.tx_channel * 586)+586)/2);
    if(needs_draw){
      needs_draw = false;
      draw();
    }
    draw_channel_status(cw_dsp);
    if(button_bar_active) {
      if(!button_bar_timeout--){
        button_bar_active = false;
        needs_draw = true;
      }
    } else { 
      draw_tx_panel(send_text, cw_encoder);
    }
  }
}

void draw_channel_status(my_cw_dsp &cw_dsp)
{
  //int x = 8;
  for(int channel=0; channel<NUM_CHANNELS; ++channel)
  {
    uint16_t TEXT_Y = PANEL_Y + (PANEL_INTERVAL * channel) + PADDING; 
    char buffer[50];
    uint16_t frequency = ((2 * channel * 586)+586)/2; 
    snprintf(buffer, 50, "chan: %u +%3uHz %3u%% %3.0fwpm %3.0fdB(500 Hz)", channel, frequency, cw_dsp.get_buffer_percent(channel), cw_dsp.get_WPM(channel), cw_dsp.get_snr(channel));
    int TEXT_X = (width - (strlen(buffer) * 6))/2;
    display->drawString(TEXT_X, TEXT_Y, font_8x5, buffer, TEXT_HEADING, PANEL_BG);
  }
}

void draw_tx_panel(const char send_text[], c_cw_encoder &cw_encoder)
{

  const uint16_t length = strlen(send_text);
  const uint16_t cursor = cw_encoder.get_cursor();
  const uint16_t left_to_send = length-cursor;
  const int font_width = 6;
  const int font_height = 8;
  const int text_width = 2*((PANEL_WIDTH - (2*PADDING))/(2*font_width));
  const int half_text_width = text_width/2;

  char buffer[51];
  uint16_t amount_to_copy = half_text_width<left_to_send?half_text_width:left_to_send;
  memcpy(buffer, send_text+cursor, amount_to_copy); buffer[amount_to_copy] = 0;
  if(cw_encoder.is_done()) {
    display->drawRect(PANEL_X, TX_PANEL_Y, TX_PANEL_HEIGHT, PANEL_WIDTH, TX_BORDER);
  } else {
    display->drawRect(PANEL_X, TX_PANEL_Y, TX_PANEL_HEIGHT, PANEL_WIDTH, COLOUR_RED);
  }
  display->fillRect(PANEL_X + PADDING + font_width*(half_text_width+amount_to_copy), TX_PANEL_Y + PADDING, font_height, font_width*(half_text_width-amount_to_copy), TX_BG);
  display->drawString(PANEL_X + PADDING+font_width*half_text_width, TX_PANEL_Y+PADDING, font_8x5, buffer, TX_TEXT, PANEL_BG);
  amount_to_copy = half_text_width<cursor?half_text_width:cursor;
  memcpy(buffer, send_text+cursor-amount_to_copy, amount_to_copy); buffer[amount_to_copy] = 0;
  display->drawString(PANEL_X + PADDING + (half_text_width-amount_to_copy)*font_width, TX_PANEL_Y + PADDING, font_8x5, buffer, COLOUR_WHITE, TX_BG);
  display->fillRect(PANEL_X + PADDING, TX_PANEL_Y + PADDING, font_height, font_width*(half_text_width-amount_to_copy), TX_BG);
}

void draw_waterfall(c_cw_dsp &cw_dsp)
{
  static uint16_t waterfall_newest = 0;
  static uint16_t waterfall[FRAME_SIZE/2][WATERFALL_WIDTH] = {0};

  uint32_t *magnitudes = cw_dsp.get_magnitudes();
  
  for(uint16_t bin=0; bin<FRAME_SIZE/2; ++bin) {
    uint32_t magnitude = magnitudes[bin];
    float scaled_magnitude = std::max(0.0, 20*log10(magnitude+1)-20);
    uint8_t pixel = scaled_magnitude * 3.5;

    waterfall[bin][waterfall_newest] = display->colour565(0, pixel, pixel/2);
  }

  static uint32_t refresh_count = 0;
  if(++refresh_count == 2) {
    refresh_count = 0;
    uint8_t channel = 0;
    uint8_t sub_y = 0;
    for(uint16_t waterfall_y=0; waterfall_y<30; ++waterfall_y) {         
      uint8_t y = PANEL_Y + (channel * PANEL_INTERVAL) + PADDING+TEXT_HEIGHT+PADDING+TEXT_HEIGHT+PADDING + sub_y;
      display->writeHLine(PANEL_X + PADDING, y, WATERFALL_WIDTH - waterfall_newest - 1, &waterfall[waterfall_y][waterfall_newest + 1]);
      display->writeHLine(PANEL_X + PADDING + WATERFALL_WIDTH - waterfall_newest, y, waterfall_newest+1, waterfall[waterfall_y]);
      if(++sub_y > 4) {
        sub_y = 0;
        channel++;
      }
    }
  }
    
  if(++waterfall_newest == WATERFALL_WIDTH) waterfall_newest = 0;
}

///////////////////////////////////////////////////////////////////////////////
//CW Encoding and Decoding

void my_cw_dsp :: decode(uint16_t cluster, std::string text, std::string partial)
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
    display->drawString(TEXT_X,                                                           TEXT_Y, font_8x5, decoded_text[cluster].c_str(), TEXT_PRIMARY, PANEL_BG);
    display->drawString(TEXT_X+(font_width*decoded_text[cluster].size()),                 TEXT_Y, font_8x5, text.c_str(), TEXT_RECENT, PANEL_BG);
    display->drawString(TEXT_X+(font_width*(decoded_text[cluster].size() + text.size())), TEXT_Y, font_8x5, partial.c_str(), TEXT_PARTIAL, PANEL_BG);
  }
  decoded_text[cluster]+=text;
}

void my_cw_dsp :: set_update_display(bool update_display)
{
  m_update_display = update_display;
}

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

///////////////////////////////////////////////////////////////////////////////
//Non-Volatile Storage

const int version = 300;

void load() {
  uint32_t settings_stored = 0;
  EEPROM.get(0, settings_stored);
  if(settings_stored == version){
    uint32_t address = 4;
    EEPROM.get(address, settings);
  }
}

void save() {
  uint32_t address = 4;
  EEPROM.put(address, settings);
  uint32_t settings_stored = 0;
  EEPROM.get(0, settings_stored);
  if(settings_stored != version){ EEPROM.put(0, version); }
  EEPROM.commit();
}



///////////////////////////////////////////////////////////////////////////////
//Display Configuration
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