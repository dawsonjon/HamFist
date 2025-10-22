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
noise = [int(line) for line in noise]

with open("threshold") as inf:
  threshold = inf.read()
threshold = threshold.splitlines()
threshold = [int(line) for line in threshold]

with open("decode_bins") as inf:
  decode_bins = inf.read()
decode_bins = decode_bins.splitlines()
decode_bins = [int(line) for line in decode_bins]

for bin in set(decode_bins):
  plt.title("Frequency Bin %u"%bin)
  plt.plot([max(x[bin:bin+5]) for x in data])
  plt.plot(noise)
  plt.plot(threshold)
  plt.plot(1000*(np.array([max(x[bin:bin+5]) for x in data]) > np.array(threshold)))
  plt.show()
  
