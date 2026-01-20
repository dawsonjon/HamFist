import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, FFMpegWriter

# ----------------------------
# Data
# ----------------------------
words = [
    "apple", "apricot", "avocado",
    "banana", "bilberry", "blackberry", "blueberry", "boysenberry",
    "cherry", "clementine", "coconut", "cranberry",
    "date", "dragonfruit",
    "elderberry",
    "fig",
    "gooseberry", "grape", "grapefruit",
    "honeydew",
    "jackfruit",
    "kiwi", "kumquat",
    "lemon", "lime", "lychee",
    "mandarin", "mango", "mulberry",
    "nectarine",
    "orange",
    "papaya", "passionfruit", "peach", "pear", "persimmon",
    "pineapple", "plum", "pomegranate",
    "quince",
    "raspberry",
    "starfruit", "strawberry",
    "tangerine",
    "ugli",
    "watermelon"
]

target = "banana"

# ----------------------------
# Binary search step capture
# ----------------------------
def binary_search_steps(arr, target):
    steps = []
    low = 0
    high = len(arr) - 1

    while low <= high:
        mid = (low + high) // 2
        steps.append((low, mid, high))

        if arr[mid] == target:
            break
        elif arr[mid] < target:
            low = mid + 1
        else:
            high = mid - 1

    return steps

steps = binary_search_steps(words, target)

# ----------------------------
# 1080p Figure Setup
# ----------------------------
fig, ax = plt.subplots(figsize=(19.2, 10.8), dpi=100)

fig.patch.set_facecolor("#0b1020")
ax.set_facecolor("#0b1020")

ax.set_xlim(0, 1)
ax.set_ylim(-0.5, len(words) - 0.5)
ax.invert_yaxis()
ax.set_xticks([])
ax.set_yticks([])

ax.set_title(
    "Binary Search on Sorted Word List",
    fontsize=20,
    pad=10,
    fontweight="bold",
    color="white"
)

#plt.subplots_adjust(left=0.15, right=0.95, top=0.90, bottom=0.05)

# ----------------------------
# Word labels
# ----------------------------
texts = []
for i, word in enumerate(words):
    t = ax.text(
        0.5, i, word,
        ha="center", va="center",
        fontsize=15,
        color="#e0e0e0"
    )
    texts.append(t)

# ----------------------------
# Pointer markers
# ----------------------------
low_marker  = ax.text(0.4, 0, "low", color="#1f77b4", fontsize=15, fontweight="bold")
mid_marker  = ax.text(0.4, 0, "mid", color="#2ca02c", fontsize=15, fontweight="bold")
high_marker = ax.text(0.4, 0, "high", color="#d62728", fontsize=15, fontweight="bold")

# ----------------------------
# Info text
# ----------------------------
info_text = ax.text(
    0.02, 0.94, "",
    transform=ax.transAxes,
    fontsize=15,
    color="#cccccc"
)

# ----------------------------
# Animation update function
# ----------------------------
def update(frame):
    low, mid, high = steps[frame]

    # Reset all
    for t in texts:
        t.set_color("#444444")
        t.set_fontweight("normal")

    # Active range
    for i in range(low, high + 1):
        texts[i].set_color("#aaaaaa")

    # Mid highlight
    texts[mid].set_color("#00ff66")
    texts[mid].set_fontweight("bold")

    # Move markers
    low_marker.set_position((0.4, low))
    mid_marker.set_position((0.4, mid))
    high_marker.set_position((0.4, high))

    info_text.set_text(
        f"low={low}, mid={mid}, high={high} → checking '{words[mid]}'"
    )

    return texts + [low_marker, mid_marker, high_marker, info_text]

# ----------------------------
# Create animation
# ----------------------------
ani = FuncAnimation(
    fig,
    update,
    frames=len(steps),
    interval=800,
    blit=False,
    repeat=False
)
plt.show()

# Keep reference alive
_ = ani

# ----------------------------
# Save as 1080p MP4
# ----------------------------
writer = FFMpegWriter(fps=1, bitrate=5000)

ani.save(
    "binary_search_animation_1080p.mp4",
    writer=writer,
    dpi=100
)

print("Saved binary_search_animation_1080p.mp4")


