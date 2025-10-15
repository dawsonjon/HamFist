import cw_data

cw_abbreviations = ",\n".join(['"%s"'%letter for letter in cw_data.CW_ABREVIATIONS])
cw_words = ",\n".join(['"%s"'%letter for letter in cw_data.WORDS])
morse = ",\n".join(['{\'%s\',"%s"}'%(letter, code) for letter, code in cw_data.MORSE.items()])

content = """
#include "cw_data.h"
const int NUM_CW_ABBREVIATIONS = %u;
const int NUM_CW_WORDS = %u;
const int NUM_MORSE_LETTERS = %u;
const char * CW_ABBREVIATIONS[] = {%s};
const char * CW_WORDS[] = {%s};
const s_morse MORSE[] = {%s};
"""%(
  len(cw_data.CW_ABREVIATIONS),
  len(cw_data.WORDS),
  len(cw_data.MORSE),
  cw_abbreviations,
  cw_words,
  morse
)

with open("../cw_data.cpp", "w") as outf:
  outf.write(content)
