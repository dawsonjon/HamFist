import numpy as np
from matplotlib import pyplot as plt

with open("magnitudes") as inf:
  data = inf.read()
data = data.splitlines()
data = [line.split() for line in data]
data = [[int(i) for i in line] for line in data]
plt.imshow(data, aspect="auto")
plt.show()

with open("noise_estimate") as inf:
  noise = inf.read()
noise = noise.splitlines()
noise = [line.split() for line in noise]
noise = [[float(i) for i in line] for line in noise]

with open("threshold") as inf:
  threshold = inf.read()
threshold = threshold.splitlines()
threshold = [line.split() for line in threshold]
threshold = [[int(i) for i in line] for line in threshold]

with open("decode_bins") as inf:
  decode_bins = inf.read()
decode_bins = decode_bins.splitlines()
decode_bins = [int(line) for line in decode_bins]

data = data[:len(threshold)]

for bin in set(decode_bins):
  plt.title("Frequency Bin %u"%bin)

  max_bins = [np.argmax(x[bin:bin+5]) for x in data]

  bin_threshold = np.array([max(x[bin:bin+5]) for x, y in zip(threshold, max_bins)])
  plt.plot(bin_threshold, label="threshold")

  bin_noise_estimate = [x[bin+y] for x, y in zip(noise, max_bins)]
  plt.plot(bin_noise_estimate, label="noise_estimate")

  bin_magnitude = np.array([x[bin+y] for x, y in zip(data, max_bins)])
  plt.plot(bin_magnitude, label="magnitude")

  plt.plot(1000*(bin_magnitude > bin_threshold), label="signal")
  plt.legend()
  plt.show()

