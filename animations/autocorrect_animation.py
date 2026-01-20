import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, FFMpegWriter

# ----------------------------
# Word list
# ----------------------------
words = [
    "apple","apricot","avocado","banana","bilberry","blackberry","blueberry",
    "boysenberry","cherry","clementine","coconut","cranberry","date","dragonfruit",
    "elderberry","fig","gooseberry","grape","grapefruit","honeydew","jackfruit",
    "kiwi","kumquat","lemon","lime","lychee","mandarin","mango","mulberry",
    "nectarine","orange","papaya","passionfruit","peach","pear","persimmon",
    "pineapple","plum","pomegranate","quince","raspberry","starfruit",
    "strawberry","tangerine","ugli","watermelon"
]

misspelled = "pomegrante"
WINDOW = 4

# ----------------------------
# Edit distance = 1
# ----------------------------
def edit_distance_one(a, b):
    if abs(len(a) - len(b)) > 1:
        return False

    i = j = diff = 0
    while i < len(a) and j < len(b):
        if a[i] != b[j]:
            diff += 1
            if diff > 1:
                return False
            if len(a) > len(b):
                i += 1
            elif len(a) < len(b):
                j += 1
            else:
                i += 1
                j += 1
        else:
            i += 1
            j += 1

    if i < len(a) or j < len(b):
        diff += 1

    return diff == 1

# ----------------------------
# Binary search + scan steps
# ----------------------------
def binary_search_steps(arr, target):
    steps = []
    low = 0
    high = len(arr) - 1

    while low <= high:
        mid = (low + high) // 2
        steps.append(("binary", low, mid, high))

        if arr[mid] == target:
            break
        elif arr[mid] < target:
            low = mid + 1
        else:
            high = mid - 1

    insert = low
    steps.append(("insert", insert, insert, insert))
    return steps, insert

steps, insert_index = binary_search_steps(words, misspelled)

scan_indices = list(range(
    max(0, insert_index - WINDOW),
    min(len(words), insert_index + WINDOW + 1)
))

for i in scan_indices:
    steps.append(("scan", i, insert_index, insert_index))

# ----------------------------
# 1080p layout
# ----------------------------
fig, ax = plt.subplots(figsize=(19.2, 10.8), dpi=100)
fig.patch.set_facecolor("#0b1020")
ax.set_facecolor("#0b1020")
plt.subplots_adjust(left=0.05, right=0.95, top=0.90, bottom=0.1)

ax.set_xlim(0, 1)
ax.set_ylim(-0.5, len(words)-0.5)
ax.invert_yaxis()
ax.set_xticks([])
ax.set_yticks([])

ax.set_title(
    "Autocorrect: Binary Search + Local Edit Distance Scan",
    fontsize=20, pad=30, fontweight="bold", color="white"
)


# ----------------------------
# Word labels
# ----------------------------
texts = []
for i,w in enumerate(words):
    t = ax.text(0.45, i, w, ha="center", va="center",
                fontsize=14, color="#666666")
    texts.append(t)

# ----------------------------
# Marker
# ----------------------------
marker = ax.text(0.35, 0, "▶", color="#00ff66", fontsize=28, fontweight="bold")

info = ax.text(0, 0.5, "", transform=ax.transAxes,
               fontsize=15, color="#cccccc")

# ----------------------------
# Accepted candidates panel
# ----------------------------
panel_title = ax.text(
    0.80, 0.5, "Accepted candidates",
    transform=ax.transAxes,
    fontsize=15, color="#ffaa00", fontweight="bold"
)

candidate_text = ax.text(
    0.80, 0.47, "",
    transform=ax.transAxes,
    fontsize=15, color="#00ff66",
    va="top"
)

accepted = []

# ----------------------------
# Animation update
# ----------------------------
def update(frame):
    mode, a, b, c = steps[frame]

    # Reset words
    for t in texts:
        t.set_color("#444444")
        t.set_fontweight("normal")


    if mode == "binary":
        low, mid, high = a, b, c
        for i in range(low, high+1):
            texts[i].set_color("#888888")

        texts[mid].set_color("#00ff66")
        texts[mid].set_fontweight("bold")

        marker.set_position((0.35, mid))
        info.set_text(
            f"Binary search: checking '{words[mid]}' for '{misspelled}'"
        )
        candidate_text.set_text("")

    elif mode == "insert":
        idx = a
        texts[idx].set_color("#ffaa00")
        texts[idx].set_fontweight("bold")
        marker.set_position((0.35, idx))
        info.set_text(
            f"Insertion point for '{misspelled}' is index {idx}"
        )

    elif mode == "scan":
        i = a
        marker.set_position((0.35, i))

        if edit_distance_one(misspelled, words[i]):
            texts[i].set_color("#00ff66")
            texts[i].set_fontweight("bold")

            candidate_text.set_text(words[i])

            info.set_text(
                f"Accepted: '{words[i]}' (edit distance 1)"
            )
            accepted.append(words[i])
        else:
            texts[i].set_color("#ff5555")
            info.set_text(
                f"Rejected: '{words[i]}'"
            )

    return texts + [marker, info, candidate_text, panel_title]

# ----------------------------
# Animation
# ----------------------------
ani = FuncAnimation(fig, update, frames=len(steps),
                    interval=900, blit=False, repeat=False)

_ = ani

# ----------------------------
# Export MP4
# ----------------------------
writer = FFMpegWriter(fps=0.75, bitrate=5000)
ani.save("autocorrect_algorithm_animation.mp4", writer=writer, dpi=100)

print("Saved autocorrect_algorithm_animation.mp4")

plt.show()

