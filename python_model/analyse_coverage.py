import bisect
import string

from matplotlib import pyplot as plt

def generate_distance1_variants(word):
    """Generate all edit-distance-1 variants of a word."""
    letters = string.ascii_lowercase
    variants = set()

    # Substitutions
    for i, c in enumerate(word):
        for l in letters:
            if l != c:
                variants.add(word[:i] + l + word[i+1:])

    # Insertions
    for i in range(len(word) + 1):
        for l in letters:
            variants.add(word[:i] + l + word[i:])

    # Deletions
    if len(word) > 1:
        for i in range(len(word)):
            variants.add(word[:i] + word[i+1:])

    return variants


def compute_window_coverage(dictionary, window_size=15, max_variants_per_word=None):
    """Simulate autocorrect coverage for distance-1 variants."""
    words_sorted = sorted(set(w.strip().lower() for w in dictionary if w.strip()))
    N = len(words_sorted)

    total_variants = 0
    covered = 0

    for w in words_sorted:
        variants = list(generate_distance1_variants(w))
        if max_variants_per_word:
            variants = variants[:max_variants_per_word]  # sampling for speed

        w_index = bisect.bisect_left(words_sorted, w)

        for v in variants:
            total_variants += 1
            # Find where the misspelling would land in lexicographic order
            pos = bisect.bisect_left(words_sorted, v)
            if abs(pos - w_index) <= window_size:
                covered += 1
            elif v[0] != w[0] and len(v) == len(w): #one word substitutions covered seperately
                covered += 1

    coverage = (covered / total_variants) * 100 if total_variants else 100
    return coverage



with open("autocorrect_words.txt") as inf:
    dictionary = [i.strip() for i in inf]
dictionary.sort()
windows = []
coverages = []
for window in range(5, 100, 5):
    cov = compute_window_coverage(dictionary, window_size=window, max_variants_per_word=500)
    windows.append(window)
    coverages.append(cov)
    print(f"Coverage (±{window} window): {cov:.2f}%")

plt.plot(windows, coverages)
plt.show()
