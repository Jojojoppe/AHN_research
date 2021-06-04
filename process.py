import numpy as np
import pandas as pd
import matplotlib.pyplot as pl

print("Processing generated data")

results = pd.read_csv('res.csv')
scalars = results[results.type=='scalar']

# Create figure
pl.figure()

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
    lines_x[delnumber] += [txCount]
    lines_y[delnumber] += [v]
    lines_std[delnumber] += [stddevs[k]]

# Show lines
for k, v in lines_x.items():
    x, y = zip(*sorted(zip(lines_x[k], lines_y[k])))
    x, std = zip(*sorted(zip(lines_x[k], lines_std[k])))
    pl.errorbar(x, y, std, elinewidth=0.5, fmt='o-', label='nr %d'%(k+1))
    pl.legend()
    pl.xlabel('Nr of senders')
    pl.ylabel('Delay to scheduled neighbors [s]')

# Show figure
pl.show()