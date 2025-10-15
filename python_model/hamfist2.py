import numpy as np
import scipy.signal
import wave
import sys
import time
from matplotlib import pyplot as plt

import cw_decode

filename = sys.argv[1]

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
        

def get_cw(samples, rate, plot_map=False):

    num_frequency_bins = 128
    frequency_bin = 72
    time_per_sample = 128/rate

    smoothed_freq_frame = np.zeros(num_frequency_bins)
    noise_estimate_capture = []
    freq_frame_capture = []
    threshold_capture = []
    detection_capture = []
    freq_frame_d4 = np.zeros(num_frequency_bins)
    freq_frame_d3 = np.zeros(num_frequency_bins)
    freq_frame_d2 = np.zeros(num_frequency_bins)
    freq_frame_d1 = np.zeros(num_frequency_bins)
    freq_frame = np.zeros(num_frequency_bins)

    threshold = 0
    sample_number = 0
    last_change = 0
    detection = False
    mark_space_lengths = []
    while(len(samples)) > 128:

      frame = samples[:num_frequency_bins]
      samples = samples[num_frequency_bins:]
      freq_frame_d4 = freq_frame_d3
      freq_frame_d3 = freq_frame_d2
      freq_frame_d2 = freq_frame_d1
      freq_frame_d1 = freq_frame
      freq_frame = np.abs(np.fft.fftshift(np.fft.fft(frame)))
      freq_frame_smoothed = np.mean([freq_frame_d1, freq_frame_d2, freq_frame_d3], 0)

      noise_estimate = np.mean(freq_frame_smoothed[frequency_bin-11:frequency_bin-2]+freq_frame_smoothed[frequency_bin+2:frequency_bin+11])
      threshold = 0.98*threshold + 0.02*(noise_estimate * 2)
      old_detection = detection
      detection = freq_frame_smoothed[frequency_bin] > threshold

      if old_detection != detection:
        length = sample_number - last_change
        if old_detection:
          print(old_detection, length)
        last_change = sample_number
        mark_space_lengths.append((old_detection, length))

    
      sample_number += 1
      freq_frame_capture.append(freq_frame_smoothed)
      noise_estimate_capture.append(noise_estimate)
      threshold_capture.append(threshold)
      detection_capture.append(detection)

    plt.plot([i[frequency_bin] for i in freq_frame_capture])
    plt.plot(noise_estimate_capture)
    plt.plot(threshold_capture)
    plt.plot([1e6 if i else 0 for i in detection_capture])
    plt.show()

    marks = [l for v, l in mark_space_lengths if v]
    dits, dahs = cluster(marks, 2, 5)
    dit_mu = np.mean(dits)
    dah_mu = np.mean(dahs)

    print("speed WPM:", 1.2/(mu*time_per_sample))
    print("dit mu sigma:", mu*time_per_sample, dit_sigma*time_per_sample)

    spaces = [l for v, l in mark_space_lengths if not v]
    spaces = [l for l in spaces if l < 10*dit_mu] #remove outliers
    short_space, long_space = cluster(spaces, 2, 5)
    short_space_mu = np.mean(short_space)
    long_space_mu = np.mean(long_space)
    short_space_sigma = np.std(short_space)
    long_space_sigma = np.std(long_space)

    print("short space sigma:", short_space_mu*time_per_sample, short_space_sigma*time_per_sample)
    print("long space sigma:", long_space_mu*time_per_sample, long_space_sigma*time_per_sample)




    print("total_time:", len(samples)//rate)
    print("%6.1f frequency bins of %6.1f Hz"%(num_frequency_bins, frequency_bin_size))
    print("%6.1f time bins of %6.1f ms"%(num_time_bins, time_bin_size*1000))
    print("")

if __name__ == "__main__":
    wav = wave.open(filename)
    rate = wav.getframerate()
    assert wav.getnchannels() == 1 ## one channel
    assert wav.getsampwidth() == 2 ## 16-bit audio

    frames = wav.readframes(rate*60)
    samples = np.frombuffer(frames, np.int16)
    get_cw(samples, rate)
