import numpy as np
from matplotlib import pyplot as plt

with open("power") as inf:
  power = inf.read()
power = power.splitlines()
power = [line.split() for line in power]
power = [[int(i) for i in line] for line in power]
plt.imshow(power, aspect="auto")
plt.show()

with open("noise_estimate") as inf:
  noise = inf.read()
noise = noise.splitlines()
noise = [line.split() for line in noise]
noise = [[float(i) for i in line] for line in noise]

with open("signal_estimate") as inf:
  signal = inf.read()
signal = signal.splitlines()
signal = [line.split() for line in signal]
signal = [[float(i) for i in line] for line in signal]

with open("threshold") as inf:
  threshold = inf.read()
threshold = threshold.splitlines()
threshold = [line.split() for line in threshold]
threshold = [[int(i) for i in line] for line in threshold]

with open("decode_bins") as inf:
  decode_bins = inf.read()
decode_bins = decode_bins.splitlines()
decode_bins = [int(line) for line in decode_bins]

power = power[:len(threshold)]

for bin in set(decode_bins):
  plt.title("Frequency Bin %u"%bin)

  max_bins = [np.argmax(x[bin:bin+5]) for x in power]

  bin_threshold = np.array([max(x[bin:bin+5]) for x in threshold])
  plt.plot(bin_threshold, label="threshold")

  bin_noise_estimate = np.array([max(x[bin:bin+5]) for x in noise])
  plt.plot(bin_noise_estimate, label="noise_estimate")

  bin_signal_estimate = np.array([max(x[bin:bin+5]) for x in signal])
  plt.plot(bin_signal_estimate, label="signal_estimate")

  bin_magnitude = np.array([x[bin+y] for x, y in zip(power, max_bins)])
  plt.plot(bin_magnitude, label="magnitude")

  #colors = ["r", "g", "b", "purple", "turquoise", "orange"]
  #for i in [3]:
  #    #plt.plot([x[bin+i] for i in threshold], label="threshold")
  #    plt.plot([x[bin+i] for x in noise], label="noise")#, color=colors[i])
  #    plt.plot([x[bin+i] for x in power], label="power")#, color=colors[i])
  #    noise_bins = np.array([x[bin+i] for x in noise])
  #    power_bins = np.array([x[bin+i] for x in power])
  #    plt.plot(1e8*(noise_bins > 2.0*power_bins), label="is_signal")#, color=colors[i])


  #plt.plot(1e8*(bin_magnitude > bin_threshold), label="signal")
  #plt.plot(1e8*(bin_magnitude > 2.0*bin_noise_estimate), label="is_noise")
  plt.legend()
  plt.show()

