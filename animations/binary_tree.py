import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, FFMpegWriter

MORSE = ("#EISH54V~3UF~~##2ARL~~##~WP~~J~1TNDB6=X/~KC~~Y(~MGZ7#Q~~O#8~#90#")

# Build tree positions based on your rule
def index_from_path(path):
    idx = 0
    step = 32
    for c in path:
        if c == '.':
            idx += 1
        elif c == '-':
            idx += step
        step //= 2
    return idx

# Example symbols to animate
examples = [
    ("E", "."),
    ("T", "-"),
    ("S", "..."),
    ("O", "---"),
    ("K", "-.-"),
    ("Q", "--.-"),
    ("1", ".----"),
    ("0", "-----"),
]

# Precompute animation frames
frames = []
for char, code in examples:
    idx = 0
    step = 32
    frames.append(("start", char, step, code, idx, None, None))
    for c in code:
        if c == '.':
            idx += 1
            jump = 1
        else:
            idx += step
            jump = step
        frames.append(("step", char, step, code, idx, jump, c))
        step //= 2
    frames.append(("done", char, step, code, idx, jump, c))

# Plot
fig, ax = plt.subplots(figsize=(19.2, 10.8), dpi=100)
ax.set_facecolor("#0b1020")
fig.patch.set_facecolor("#0b1020")
ax.axis("off")
plt.subplots_adjust(left=0.01, right=0.99, top=0.99, bottom=0.01)

# Draw  tree
text_nodes = []
x = 0
y = 0.75
for ch in MORSE:
    t = ax.text(x, y, ch,
                ha="center", va="center",
                fontsize=15, color="#888888")
    x += 0.015
    text_nodes.append(t)

ax.set_xlim(-0.1,1.1)
ax.set_ylim(-0.1,1.1)

title = ax.text(0.5,0.9,"",transform=ax.transAxes,
                fontsize=22,color="white",ha="center")

info = ax.text(0.5,0.5,"",transform=ax.transAxes,
               fontsize=16,color="#cccccc",ha="center")

pointer = ax.text(0,0.7,"⬤",color="#00ff66",fontsize=18,ha="center")

# Animation update
def update(f):
    mode, char, step, code, idx, jump, symbol = frames[f]

    for t in text_nodes:
        t.set_color("#555555")
    x = 0.0 + (0.015*idx)
    y = 0.7
    pointer.set_position((x,y))
    text_nodes[idx].set_color("#00ff66")

    if mode=="start":
        title.set_text(f"Decoding '{char}'  ({code})")
        info.set_text("Start at root")
    elif mode=="step":
        info.set_text(f"Symbol: '{symbol}' add {jump} to index = {idx}")
    else:
        decoded = MORSE[idx]
        info.set_text(f"Decoded character = '{decoded}'")

    return text_nodes+[pointer,title,info]

ani = FuncAnimation(fig, update, frames=len(frames),
                    interval=900, repeat=True)

plt.show()

# ----------------------------
# Export MP4
# ----------------------------
writer = FFMpegWriter(fps=0.75, bitrate=5000)
ani.save("binary_tree_animation.mp4", writer=writer, dpi=100)

print("Saved *_animation.mp4")


