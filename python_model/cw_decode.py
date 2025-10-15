import math, random
import cw_data
import numpy as np

SKIP_PENALTY = -10000.0
DELETE_PENALTY = -10000.0


output_debug = False
def debug(*args):
  if output_debug:
    print(*args)

def log_gaussian(x, mu, sigma):

    """return the log likelihood x in a normal distribution mu/sigma"""

    return -0.5 * ((x - mu) / sigma) ** 2

def log_likelihood_duration(x, mu, dah_mu, sigma, short_mu, medium_mu, long_mu, short_sigma, medium_sigma, long_sigma):

    """return the log likelihood of each symbol given the duration"""

    return {
        'dot': log_gaussian(x, mu, sigma),
        'dash': log_gaussian(x, dah_mu, sigma),
        'gap1': log_gaussian(x, short_mu, short_sigma),
        'gap3': log_gaussian(x, medium_mu, medium_sigma),
        'gap7': log_gaussian(x, long_mu, long_sigma) if x < long_mu else 0
    }

def is_valid(pattern):

    """Is this pattern part of a morse symbol?"""

    for test_pattern in cw_data.REVERSE:
      if test_pattern.startswith(pattern):
        return True
    return False

def language_log_prob(word):

    """What is the log probability that this is a word?"""

    if word in cw_data.CW_ABREVIATIONS:
      return math.log(6)
    elif word in cw_data.WORDS:
      return math.log(4)
    else:
      return 0

def cluster(x, k, iterations):
    clusters = [[] for i in range(k)]
    for item in range(len(x)): 
        clusters[item%k].append(x[item])

    for iteration in range(iterations):
        means = [np.mean(cluster) for cluster in clusters]
        clusters = [[] for i in range(k)]
        for item in x:
            distances = [abs(item-mean)*abs(item-mean) for mean in means]
            cluster = np.argmin(distances)
            clusters[cluster].append(item)

    clusters = np.array(clusters)[np.argsort(means)]
    return clusters
  

def decode_cw(signal, beam_width=5):

    """find the n most likely decodes"""


    #global clustering
    marks = [l for v, l in signal if v]
    dits, dahs = cluster(marks, 2, 5)
    mu = np.mean(dits)
    dah_mu = np.mean(dahs)
    sigma = np.std(dits)
    print("dit mu sigma:", mu, sigma)
    print("dah mu sigma:", dah_mu, sigma)

    spaces = [l for v, l in signal if not v and l < 10*mu]
    short, medium, long = cluster(spaces, 3, 5)

    short_mu = np.mean(short)
    medium_mu = np.mean(medium)
    long_mu = np.mean(long)

    short_sigma = np.std(short)
    medium_sigma = np.std(medium)
    long_sigma = np.std(long)

    print("short mu sigma:", short_mu, short_sigma)
    print("medium mu sigma:", medium_mu, medium_sigma)
    print("long mu sigma:", long_mu, long_sigma)

    #assert False


    message = ""
    beam = [("", "", 0.0)]  # (decoded_text, current_pattern, log_prob)
    for mark, duration in signal:
        probs = log_likelihood_duration(duration, mu, dah_mu, sigma, short_mu, medium_mu, long_mu, short_sigma, medium_sigma, long_sigma)
        print(probs)
        candidates = []

        debug("\n\n\n")
        debug("candidates")
        debug(mark, duration)

        for text, pattern, logp in beam:

            debug("beam", text, pattern, logp)

            if mark:
              # Case 1: dot
              if is_valid(pattern+"."):
                candidates.append((text, pattern + ".", logp + probs["dot"]))
                debug("dot", duration, mu, probs["dot"], candidates[-1])
              elif pattern in cw_data.REVERSE:
                candidates.append((text+cw_data.REVERSE[pattern], ".", logp + probs["dot"]))
                debug("dot force end", duration, mu, probs["dot"], candidates[-1])

              # Case 2: dash
              if is_valid(pattern+"-"):
                candidates.append((text, pattern + "-", logp + probs["dash"]))
                debug("dash", duration, mu, probs["dash"], candidates[-1])
              elif pattern in cw_data.REVERSE:
                candidates.append((text+cw_data.REVERSE[pattern], "-", logp + probs["dash"]))
                debug("dash force end", duration, mu, probs["dash"], candidates[-1])

            else:

              # Case 3: symbol gap
              candidates.append((text, pattern, logp + probs["gap1"]))
              debug("symbol gap", probs["gap1"], candidates[-1])

              # Case 4: letter gap
              if pattern in cw_data.REVERSE:
                  letter = cw_data.REVERSE[pattern]
                  candidates.append((text+letter, "", logp + probs["gap3"]))
                  debug("letter gap", probs["gap3"], candidates[-1])

              # Case 5: word gap
              if pattern in cw_data.REVERSE:
                  letter = cw_data.REVERSE[pattern]
                  last_word = (text+letter).split()[-1]
                  language_bonus = language_log_prob(last_word)
                  candidates.append((text+letter+" ", "", logp + probs["gap7"] + language_bonus))
                  debug("word gap", probs["gap7"], candidates[-1])

              test = {}
              for path in candidates:
                if path in test:
                   print("duplicate found", path)
                   for path in candidates:
                     print(path)
                   assert False
                test[path] = 1

        candidates.sort(key=lambda x: x[2], reverse=True)
        beam = candidates[:beam_width]

        #prune
        #print("check for prune")
        #for i in beam:
        #  print(i)
        #words = beam[0][0].split()
        #print("message", beam[0][0])
        #if len(words) > 3:
        #  prefix = " ".join(words[:-2]) + " "
        #  new_beam = []
        #  for text, pattern, logp in beam:
        #    if text.startswith(prefix):
        #      new_beam.append((text[len(prefix):], pattern, logp))
        #  beam = new_beam
        #  message += prefix
        #  print("pruning", prefix)
        #  print("message", message)
        #  for i in beam:
        #    print(i)


    # finalize
    final = []
    for text, pattern, logp in beam:
        if pattern in cw_data.REVERSE:
            letter = cw_data.REVERSE[pattern]
            text += letter
            final.append((text, logp))
    final.sort(key=lambda x: x[1], reverse=True)

    text = ""
    for v, l in signal:
      if v:
        if l > 2*mu:
          text += "-"
        else:
          text += "."
      else:
        if l > 2*mu:
          text += " "
        else:
          text += ";"
    print(text)

    print(message)

    return final[:beam_width]


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
  results = decode_cw(signal, beam_width=5)

  print("True message:", true_message)
  print("Top beam results with LM:")
  for txt, lp in results:
      rel = math.exp(lp - results[0][1])
      print(f"  {txt:10s}  (rel prob ≈ {rel:.3f})")
