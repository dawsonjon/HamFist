marks = []
spaces = []

with open("decoder_log") as inf:
    for line in inf:
        if line.startswith("duration:"):
            if "dot" in line:
                duration = line.split()[1]
                logpdot = line.split()[3]
                logpdash = line.split()[5]
                marks.append((duration, logpdot, logpdash))

            if "letter:" in line:
                duration = line.split()[1]
                logpsymbol = line.split()[3]
                logpletter = line.split()[5]
                logpword = line.split()[7]
                spaces.append((duration, logpsymbol, logpletter, logpword))

MORSE = {
 'A':'.-', 'B':'-...',
 'C':'-.-.', 'D':'-..', 'E':'.',
 'F':'..-.', 'G':'--.', 'H':'....',
 'I':'..', 'J':'.---', 'K':'-.-',
 'L':'.-..', 'M':'--', 'N':'-.',
 'O':'---', 'P':'.--.', 'Q':'--.-',
 'R':'.-.', 'S':'...', 'T':'-',
 'U':'..-', 'V':'...-', 'W':'.--',
 'X':'-..-', 'Y':'-.--', 'Z':'--..',
 '1':'.----', '2':'..---', '3':'...--',
 '4':'....-', '5':'.....', '6':'-....',
 '7':'--...', '8':'---..', '9':'----.',
 '0':'-----', ',':'--..--', '.':'.-.-.-',
 '?':'..--..', '/':'-..-.', '-':'-....-',
 '(':'-.--.', ')':'-.--.-', ':':'---...',
 "=":"-...-", " ":" ",
}

REVERSE = {v: k for k, v in MORSE.items()}

def plot_decode(filename, steps, beam_width):

    edges = []
    nodes = [("start", 0, "", "")]
    count = 0
    for i in range(steps):
        new_nodes = []
        for node, logp, decode, partial in nodes:
            duration, logpdot, logpdash = marks[i]
            logpdot = float(logpdot)
            logpdash = float(logpdash)
            new_nodes.append((f"dot{count}", logp+logpdot, decode, partial+"."))
            new_nodes.append((f"dash{count}", logp+logpdash, decode, partial+"-"))
            edges.append((node, f"dot{count}", "dot<br>%.1f"%logpdot, "'%s'<br>%.1f"%(decode+".", logp+logpdot)))
            edges.append((node, f"dash{count}", "dash<br>%.1f"%logpdash, "'%s'<br>%.1f"%(decode+"-", logp+logpdash)))
            count = count + 1
        nodes = new_nodes

        new_nodes = []
        for node, logp, decode, partial in nodes:
            duration, logpsymbol, logpletter, logpword = spaces[i+1]
            logpsymbol = float(logpsymbol)
            logpletter = float(logpletter)
            logpword = float(logpword)
            new_nodes.append((f"symbol{count}", logp+logpsymbol, decode, partial))
            try:
                new_nodes.append((f"letter{count}", logp+logpletter, decode+REVERSE[partial], ""))
                new_nodes.append((f"word{count}", logp+logpword, decode+REVERSE[partial]+" ", ""))
            except KeyError:
                pass
            edges.append((node, f"symbol{count}", "symbol gap<br> %.1f"%logpsymbol, "'%s'<br>%.1f"%(decode+partial, logp+logpsymbol)))
            try:
                edges.append((node, f"letter{count}", "letter gap<br>%.1f"%logpletter, "'%s'<br>%.1f"%(decode+REVERSE[partial], logp+logpletter)))
                edges.append((node, f"word{count}", "word gap<br>%.1f"%logpword, "'%s'<br>%.1f"%(decode+REVERSE[partial]+" ", logp+logpword)))
            except KeyError:
                pass
            count = count + 1

        nodes_sorted = sorted(new_nodes, key=lambda x: x[1])
        nodes = nodes_sorted[-beam_width:]

    with open(filename, "w") as outf:
        outf.write(f"graph LR\n")
        for edge in edges:
            node_from, node_to, edge_label, node_label = edge
            outf.write(f"{node_from} -->|{edge_label}| {node_to}[{node_label}]\n")

plot_decode("plot1.mmd", 1, 2)
plot_decode("plot2.mmd", 3, 1000)
plot_decode("plot3.mmd", 3, 2)
plot_decode("plot4.mmd", 20, 2)
