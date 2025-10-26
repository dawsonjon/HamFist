import cw_data
import itertools

def get_code_index(code):
  span = 128
  index = 0
  for symbol in code:
    span //= 2
    if symbol == ".":
      index += 1
    else:
      index += span
  return index
    
def morse_tree():
  tree = ["#" for i in range(128)]

  #generate all possible codes
  possible_codes = []
  for length in range(7):
    possible_codes.extend([''.join(code) for code in itertools.product(".-", repeat=length)])

  #populate tree with all codes
  for code in possible_codes:

    #is a valid code
    if code in cw_data.REVERSE.keys():
      letter = cw_data.REVERSE[code]

    #is a prefix of a valid code
    else:
      letter = '~' #invalid
      for i in cw_data.REVERSE:
        if i.startswith(code):
          letter = '#' #prefix
          break

    tree[get_code_index(code)] = letter

  return tree

cw_abbreviations = ",\n".join(['"%s"'%letter for letter in sorted(cw_data.CW_ABREVIATIONS)])
cw_words = set(cw_data.WORDS)
cw_words = [word for word in cw_words if word not in cw_data.CW_ABREVIATIONS]
len_cw_words = len(cw_words)
cw_words = ",\n".join(['"%s"'%letter for letter in sorted(cw_words)])
morse = ",\n".join(['{\'%s\',"%s"}'%(letter, code) for letter, code in cw_data.MORSE.items()])
prosigns = ",\n".join(['"%s"'%letter for letter in cw_data.PROSIGNS.values()])

content = """
#include "cw_data.h"
const int NUM_CW_ABBREVIATIONS = %u;
const int NUM_CW_WORDS = %u;
const int NUM_MORSE_LETTERS = %u;
const int NUM_PROSIGNS = %u;
const char * CW_ABBREVIATIONS[] = {%s};
const char * CW_WORDS[] = {%s};
const char MORSE[] = "%s";
const char * PROSIGNS[] = {%s};
"""%(
  len(cw_data.CW_ABREVIATIONS),
  len_cw_words,
  len(cw_data.MORSE),
  len(cw_data.PROSIGNS),
  cw_abbreviations,
  cw_words,
  "".join(morse_tree()),
  prosigns,
)

with open("../cw_decoder/cw_data.cpp", "w") as outf:
  outf.write(content)
