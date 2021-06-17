import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

beaconP = ['0.05', '0.075', '0.1', '0.2', '0.3', '0.4', '0.5', '1.0', '1.5', '2.0']
beaconPf = []

tx1_1 = []
tx3_1 = []
tx1_5 = []
tx3_5 = []

for bp in beaconP:
    beaconPf += [float(bp)]
    print(bp)

    # open res.csv
    results = pd.read_csv("results/beaconp_%s/res.csv"%(bp))
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

    # Create lines
    lines_x = {}
    lines_y = {}
    lines_std = {}
    for k,v in delays.items():
        txCount = k[0]
        delnumber = k[1]
        if delnumber not in lines_x:
            lines_x[delnumber] = []
            lines_y[delnumber] = []
            lines_std[delnumber] = []
        lines_x[delnumber] += [txCount/7.0 * 100.0]
        lines_y[delnumber] += [v]
        lines_std[delnumber] += [stddevs[k]]

    print(lines_y)

    # Add to tx<x>_<x>
    tx1_1 += [lines_y[0][0]]
    tx3_1 += [lines_y[4][0]]
    tx1_5 += [lines_y[0][3]]
    tx3_5 += [lines_y[4][3]]
    print(tx1_1)
    print(tx3_1)
    print(tx1_5)
    print(tx3_5)


# # TODO get those values from real data instead of graphs
# beaconP = [0.1, 0.2, 0.3, 0.4, 0.5, 1.0]
# tx1_1 = [0.096, 0.145, 0.175, 0.225, 0.275, 0.475]
# tx3_1 = [0.15, 0.2, 0.26, 0.31, 0.37, 0.65]
# tx1_5 = [0.275, 0.305, 0.36, 0.43, 0.52, 0.92]
# tx3_5 = [0.45, 0.58, 0.68, 0.81, 0.96, 1.80]

fig = plt.figure()
ax = fig.add_axes([0.1, 0.1, 0.8, 0.8])
ax.set_yscale('log')
#ax.set_xscale('log')

ax.plot(beaconPf, tx1_1, 'bo-', label='1 TX\'er 1st')
ax.plot(beaconPf, tx1_5, 'bo:', label='1 TX\'er 5th')
ax.plot(beaconPf, tx3_1, 'rs-', label='3 TX\'ers 1st')
ax.plot(beaconPf, tx3_5, 'rs:', label='3 TX\'ers 5th')

ax.legend()
ax.set_xlabel('Beacon period [s]')
ax.set_ylabel('Delay to scheduled neighbors [s]');

ax.set_yticks([0.1, 0.3, 0.5, 1.0, 3.0, 5.0])
ax.set_yticklabels(['0.1', '0.3', '0.5', '1', '3', '5'])

ax.set_xticks(beaconPf)
ax.set_xticklabels(beaconP)

ax.grid(True, 'major', 'y', alpha=0.6, ls='--', color='black')
ax.grid(True, 'minor', 'y', alpha=0.6, ls='--')
ax.grid(True, 'major', 'x', alpha=0.6, ls='--')

plt.show()