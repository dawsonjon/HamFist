import numpy as np
import scipy.signal
import wave
import sys
import time
import math
from matplotlib import pyplot as plt
from mpl_toolkits import mplot3d

import cw_decode

filename = sys.argv[1]

def cluster_frequencies(max_hold, threshold):
    best_frequencies = np.argsort(max_hold[:64])[::-1]

    # group together adjacent frequency bins
    frequency_clusters = []
    for f in best_frequencies:
        if max_hold[f] > threshold:
          for i, fc in enumerate(frequency_clusters):
              if f < max(fc)+2 and f > min(fc)-2:
                  frequency_clusters[i].append(f)
                  break
          else:
              frequency_clusters.append([f])

    return [np.argmax(max_hold[cluster]) for cluster in frequency_clusters]


threshold_setting = 2

def get_cw(samples, rate, plot_map=False):

    num_frequency_bins = 128
    frequency_bin = 56
    time_per_sample = 128/rate

    smoothed_freq_frame = np.zeros(num_frequency_bins)
    freq_frame_d4 = np.zeros(num_frequency_bins)
    freq_frame_d3 = np.zeros(num_frequency_bins)
    freq_frame_d2 = np.zeros(num_frequency_bins)
    freq_frame_d1 = np.zeros(num_frequency_bins)
    freq_frame = np.zeros(num_frequency_bins)
    max_hold = np.zeros(num_frequency_bins)

    noise_estimate_capture = []
    freq_frame_capture = []
    threshold_capture = []
    detection_capture = []
    max_hold_capture = []

    threshold = 0
    sample_number = 0
    last_change = 0
    detection = False
    mark_space_lengths = []

    clusters = []
    while(len(samples)) > 128:

      frame = samples[:num_frequency_bins]
      samples = samples[num_frequency_bins:]
      freq_frame_d1 = freq_frame
      freq_frame = np.abs(np.fft.fftshift(np.fft.fft(frame * np.blackman(num_frequency_bins))))
      freq_frame_smoothed = np.mean([freq_frame, freq_frame_d1], 0)
      
      noise_estimate = np.mean(freq_frame_smoothed[frequency_bin-11:frequency_bin-2]+freq_frame_smoothed[frequency_bin+2:frequency_bin+11])
      threshold = 0.98*threshold + 0.02*(noise_estimate * threshold_setting)
      old_detection = detection
      detection = freq_frame_smoothed[frequency_bin] > threshold

      max_hold = max_hold * 0.999
      max_hold[freq_frame > max_hold] = freq_frame[freq_frame > max_hold]

      clusters =  cluster_frequencies(max_hold, threshold)
      print(clusters)

      if old_detection != detection:
        length = sample_number - last_change
        last_change = sample_number
        mark_space_lengths.append((old_detection, length))
      sample_number += 1

      freq_frame_capture.append(freq_frame_smoothed)
      noise_estimate_capture.append(noise_estimate)
      threshold_capture.append(threshold)
      detection_capture.append(detection)
      max_hold_capture.append(max_hold)

    plt.plot([i[frequency_bin] for i in freq_frame_capture])
    plt.plot(noise_estimate_capture)
    plt.plot(threshold_capture)
    plt.plot([1e6 if i else 0 for i in detection_capture])
    #plt.plot(max_hold_capture[500])
    plt.show()


    results = cw_decode.decode_cw(mark_space_lengths, beam_width=3)

    print("Top beam results with LM:")
    for txt, lp in results[:1]:
        rel = math.exp(lp - results[0][1])
        print(f"  {txt:10s}  (rel prob ≈ {rel:.3f})")


if __name__ == "__main__":
    wav = wave.open(filename)
    rate = wav.getframerate()
    assert wav.getnchannels() == 1 ## one channel
    assert wav.getsampwidth() == 2 ## 16-bit audio

    frames = wav.readframes(rate*60)
    samples = np.frombuffer(frames, np.int16)
    get_cw(samples, rate)
