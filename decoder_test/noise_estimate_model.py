import numpy as np
from matplotlib import pyplot as plt

with open("power") as inf:
  power = inf.read()
power = power.splitlines()
power = [line.split() for line in power]
power = [[int(i) for i in line] for line in power]

power = [i[5+3] for i in power]

on_estimate = []
off_estimate = []
n = power[0]


on = power[0]
off = power[0]
for p in power:
    if p > on or p > off+ 0.25*(on+off):
        on = 0.99*on + 0.01*p
    else:
        off = 0.99*off + 0.01*p

    on_estimate.append(on)
    off_estimate.append(off)

plt.plot(power)
plt.plot(on_estimate)
plt.plot(off_estimate)
plt.show()
