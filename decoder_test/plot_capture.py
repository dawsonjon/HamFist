from matplotlib import pyplot as plt

with open("capture.txt") as inf:
    data = [int(line) for line in inf if line.strip().isdigit()]

data = [i%4096 for i in data]
plt.plot(data)
plt.show()

