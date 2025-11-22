#ifndef __CW_DATA_H__
#define __CW_DATA_H__
#include <cstdint>

struct s_morse
{
  char letter;
  const char* code;
};

extern const int NUM_MORSE_LETTERS;
extern const char MORSE[];

extern const int NUM_AUTOCORRECT_WORDS;
extern const char* AUTOCORRECT_WORDS[];
extern const uint16_t RANKINGS[];

extern const int NUM_PROSIGNS;
extern const char* PROSIGNS[];
#endif
