import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

pkloss = ['0.05', '0.10', '0.15', '0.20', '0.25', '0.30', '0.40']
pklossPf = []

tx3_1 = []
tx3_5 = []

for bp in pkloss:
    pklossPf += [float(bp)]
    print(bp)

    # open res.csv
    results = pd.read_csv("results/packetloss_rx_%s_retransmissions/res.csv"%(bp))
    scalars = results[results.type=='scalar']

    delays = {}
    for row in scalars.itertuples():
        # Get module
        module = row.module.split('.')[1]
        # Get run nr
        run = int(row.run.split('-')[1])
        # Get txCount
        txCount = int(row.name.split('_')[1])
        # Get delay number
        delnumber = int(row.name.split('_')[3])
        # Get value
        delay = row.value
        if module not in delays:
            delays[module] = {}
        if (txCount, delnumber) not in delays[module]:
            delays[module][(txCount, delnumber)] = []
        delays[module][(txCount, delnumber)] += [delay]

    # Take mean of runs
    newdelays = {}
    stddevs = {}
    for ki,vi in delays.items():
        newvi = {}
        newsdev = {}
        for kj, vj in vi.items():
            newvi[kj] = np.mean(vj)
            newsdev[kj] = np.std(vj)
        newdelays[ki] = newvi
        stddevs[ki] = newsdev
    delays = newdelays

    # Take mean between txers
    newdelays = {}
    newstddevs = {}
    divnr = len(delays)
    for ki, vi in delays.items():
        for kj, vj in vi.items():
            if kj not in newdelays:
                newdelays[kj] = vj/divnr
                newstddevs[kj] = stddevs[ki][kj]/divnr
            else:
                newdelays[kj] += vj/divnr
                newstddevs[kj] += stddevs[ki][kj]/divnr
    delays = newdelays
    stddevs = newstddevs

    tx3_1 += [delays[(3,0)]]
    tx3_5 += [delays[(3,4)]]

fig = plt.figure()
ax = fig.add_axes([0.1, 0.1, 0.8, 0.8])
ax.set_yscale('log')
#ax.set_xscale('log')

ax.plot(pklossPf, tx3_1, 'rs-', label='3 TX\'ers 1st')
ax.plot(pklossPf, tx3_5, 'rs:', label='3 TX\'ers 5th')

ax.legend()
ax.set_xlabel('Packet loss ratio');
ax.set_ylabel('Delay to scheduled neighbors [s]');

ax.set_yticks([0.1, 0.3, 0.5, 1.0, 3.0, 5.0])
ax.set_yticklabels(['0.1', '0.3', '0.5', '1', '3', '5'])

ax.set_xticks(pklossPf)
ax.set_xticklabels(pkloss)

ax.grid(True, 'major', 'y', alpha=0.6, ls='--', color='black')
ax.grid(True, 'minor', 'y', alpha=0.6, ls='--')
ax.grid(True, 'major', 'x', alpha=0.6, ls='--')

plt.show()