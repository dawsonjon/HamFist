#ifndef __CW_DATA_H__
#define __CW_DATA_H__


struct s_morse
{
  char letter;
  const char *code;
};

extern const int NUM_MORSE_LETTERS;
extern const char MORSE[];

extern const int NUM_CW_ABBREVIATIONS;
extern const char * CW_ABBREVIATIONS[];

extern const int NUM_CW_WORDS;
extern const char * CW_WORDS[];

#endif
