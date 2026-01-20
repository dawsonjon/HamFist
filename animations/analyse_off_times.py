import sys
with open(sys.argv[1]) as inf:
    state = 0
    stats = 0
    histogram_data = []
    histograms = []
    dot_mus = []
    dot_sigmas = []
    dot_mu = None
    dash_mus = []
    dash_sigmas = []
    dash_mu = None
    gap7_mus = []
    gap7_sigmas = []
    gap7_mu = None
    for line in inf:

        if state:
            try:
                nbin, ncount = line.split()
                histogram_data.append((int(nbin), int(ncount)))
            except ValueError:
                state = 0
                histogram = 0
                if histogram_data:
                    histograms.append(histogram_data)
                histogram_data = []

        if stats:
            if line.startswith("gap1 mu") or line.startswith("gap1 sigma") or line.startswith("gap3 mu") or line.startswith("gap3 sigma") or line.startswith("gap7 mu") or line.startswith("gap7 sigma"):
                if line.startswith("gap1 mu"):
                    dot_mu = float(line.split()[2])
                    dot_mus.append(dot_mu)
                if line.startswith("gap1 sigma"):
                    dot_sigma = float(line.split()[2])
                    dot_sigmas.append(dot_sigma)
                if line.startswith("gap3 mu"):
                    dash_mu = float(line.split()[2])
                    dash_mus.append(dash_mu)
                if line.startswith("gap3 sigma"):
                    dash_sigma = float(line.split()[2])
                    dash_sigmas.append(dash_sigma)
                if line.startswith("gap7 mu"):
                    gap7_mu = float(line.split()[2])
                    gap7_mus.append(gap7_mu)
                if line.startswith("gap7 sigma"):
                    gap7_sigma = float(line.split()[2])
                    gap7_sigmas.append(gap7_sigma)
            else:
                stats = 0

        if line.startswith("channel 1 off"):
            state = 1

        if line.startswith("channel 1"):
            stats = 1

from matplotlib import pyplot as plt
from matplotlib.animation import FuncAnimation, FFMpegWriter

# Initial data
x = [i[0]+5 for i in histograms[0]]
y = [i[1] for i in histograms[0]]

#fig, ax = plt.subplots()
fig, ax = plt.subplots(figsize=(19.2, 10.8), dpi=100)

bars = ax.bar(x, y, width=10.0)

# Mean line
dot_mean_line = ax.axvline(
    dot_mus[0],
    color="red",
    linewidth=2,
    label="Symbol Gap Mean"
)
dash_mean_line = ax.axvline(
    dash_mus[0],
    color="green",
    linewidth=2,
    label="Letter Gap Mean"
)
gap7_mean_line = ax.axvline(
    gap7_mus[0],
    color="purple",
    linewidth=2,
    label="Word Gap Mean"
)

# Std-dev band (initially)
dot_std_band = ax.axvspan(
    dot_mus[0] - dot_sigmas[0],
    dot_mus[0] + dot_sigmas[0],
    color="red",
    alpha=0.25,
    label="Symbol Gap ±1σ"
)
dash_std_band = ax.axvspan(
    dash_mus[0] - dash_sigmas[0],
    dash_mus[0] + dash_sigmas[0],
    color="green",
    alpha=0.25,
    label="Letter Gap ±1σ"
)
gap7_std_band = ax.axvspan(
    gap7_mus[0] - gap7_sigmas[0],
    gap7_mus[0] + gap7_sigmas[0],
    color="purple",
    alpha=0.25,
    label="Word Gap ±1σ"
)

ax.set_title("Classifier CW 'off' Times")
ax.set_ylim(0, max(max(i[1] for i in h) for h in histograms))
ax.set_xlabel("Duration (ms)")
ax.set_ylabel("Count (smoothed)")
ax.legend(loc="upper right")

def update(frame):
    histogram = histograms[frame]

    # Update bar heights
    for bar, (_, value) in zip(bars, histogram):
        bar.set_height(value)

    # Update mean line
    dot_mu = dot_mus[frame]
    dot_sigma = dot_sigmas[frame]
    dash_mu = dash_mus[frame]
    dash_sigma = dash_sigmas[frame]
    gap7_mu = gap7_mus[frame]
    gap7_sigma = gap7_sigmas[frame]

    dot_mean_line.set_xdata([dot_mu, dot_mu])
    dash_mean_line.set_xdata([dash_mu, dash_mu])
    gap7_mean_line.set_xdata([gap7_mu, gap7_mu])

    # Remove old std band and draw new one
    global dot_std_band
    dot_std_band.remove()
    dot_std_band = ax.axvspan(
        dot_mu - dot_sigma,
        dot_mu + dot_sigma,
        color="red",
        alpha=0.25
    )
    global dash_std_band
    dash_std_band.remove()
    dash_std_band = ax.axvspan(
        dash_mu - dash_sigma,
        dash_mu + dash_sigma,
        color="green",
        alpha=0.25
    )
    global gap7_std_band
    gap7_std_band.remove()
    gap7_std_band = ax.axvspan(
        gap7_mu - gap7_sigma,
        gap7_mu + gap7_sigma,
        color="purple",
        alpha=0.25
    )

    return bars, dot_mean_line, dot_std_band, dash_mean_line, dash_std_band, gap7_mean_line, gap7_std_band

ani = FuncAnimation(
    fig,
    update,
    frames=len(histograms),
    interval=100,
    blit=False
)


plt.show()

# ----------------------------
# Export MP4
# ----------------------------
writer = FFMpegWriter(fps=10, bitrate=5000)
ani.save("off_time_animation.mp4", writer=writer, dpi=100)

print("Saved *animation.mp4")
