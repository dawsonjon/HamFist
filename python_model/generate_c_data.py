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

def autocorrect_words():
    with open("autocorrect_words.txt") as inf:
        words = [(index, word.strip().upper()) for index, word in enumerate(inf)]
    words.sort(key=lambda x: x[1])
    ranking = [i for i, w in words]
    words = [w for i, w in words]

    num_autocorrect_words = len(words)
    return num_autocorrect_words, ",\n".join(['"%s"'%i for i in words]), ",\n".join([str(i) for i in ranking])

num_autocorrect_words, autocorrect_words, ranking = autocorrect_words()
morse = ",\n".join(['{\'%s\',"%s"}'%(letter, code) for letter, code in cw_data.MORSE.items()])
prosigns = ",\n".join(['"%s"'%letter for letter in cw_data.PROSIGNS.values()])

content = """
#include "cw_data.h"
const int NUM_AUTOCORRECT_WORDS = %u;
const int NUM_MORSE_LETTERS = %u;
const int NUM_PROSIGNS = %u;
const char * AUTOCORRECT_WORDS[] = {%s};
const uint16_t RANKINGS[] = {%s};
const char MORSE[] = "%s";
const char * PROSIGNS[] = {%s};
"""%(
  num_autocorrect_words,
  len(cw_data.MORSE),
  len(cw_data.PROSIGNS),
  autocorrect_words,
  ranking,
  "".join(morse_tree()),
  prosigns,
)

with open("../cw_decoder/cw_data.cpp", "w") as outf:
  outf.write(content)
