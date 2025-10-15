import math, random
import cw_data
import numpy as np

if __name__ == "__main__":

  DOT, DASH = 1.0, 3.0
  SIGMA = 0.5

  def simulate_continuous(message):

      """simulate a noisy morse message"""

      signal = []

      words = [";".join(word) for word in message.split()]
      message = " ".join(words)

      for ch in message:

          if ch == " ":
            signal.append((False, random.gauss(7 * DOT, SIGMA))) #word gap
            continue

          if ch == ";":
            signal.append((False, random.gauss(3 * DOT, SIGMA))) #letter gap
            continue

          symbols = ",".join(cw_data.MORSE[ch])
          for symbol in symbols:
            if symbol == ",":
              signal.append((False, random.gauss(1 * DOT, SIGMA))) #letter gap
            else:
              signal.append((True, random.gauss(DOT if symbol == '.' else DASH, SIGMA)))
              
      return signal


  random.seed(7)
  true_message = "HELLO IS IT ME YOU ARE LOOKING FOR"
  signal = simulate_continuous(true_message)
  content = ["{%s, %f}"%("true" if mark else "false", duration) for mark, duration in signal]
  content = ",\n".join(content)

  header = """

#include "cw_decode.h"

s_observation signal[] = {
%s
};

  """%content

  with open("../cw_test_data.h", "w") as outf:
    outf.write(header)
