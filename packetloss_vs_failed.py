import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

pkloss = ['0.05', '0.10', '0.15', '0.20', '0.25', '0.30', '0.40']
pklossPf = []

data = {}

for bp in pkloss:
    pklossPf += [float(bp)]
    print(bp)

    # open res.csv
    results = pd.read_csv("results/packetloss_rx_%s_retransmissions/logres.csv"%(bp))
    scalars = results[results.type=='scalar']

    data[bp] = {}

    previousName = ''

    for row in scalars.itertuples():
        # Get module
        module = row.module.split('.')[1]
        # Get run nr
        run = int(row.run.split('-')[1])

        name = row.name

        # Save data
        if module not in data[bp]:
            data[bp][module] = {}
        if run not in data[bp][module]:
            data[bp][module][run] = {}

        # Fix naming of rx/tx (second of same name is tx)
        if name == previousName:
            name = name.replace('_rx_', '_tx_')
        previousName = name

        data[bp][module][run][name] = int(row.value)
        # print(bp, module, run, name, row.value, data[bp][module][run])

plotdata_successes = []
plotdata_fails = []
plotdata_notscheduled = []
plotdata_collisions = []

# Take mean of all runs for each node
for bp, modules in data.items():
    # Loop over each module

    # Take mean for all variables
    totvals = {}
    totvals['log_rx_rejects'] = 0
    totvals['log_tx_rejects'] = 0
    totvals['log_rx_success'] = 0
    totvals['log_tx_success'] = 0
    #totvals['log_rx_lost'] = 0
    #totvals['log_tx_lost'] = 0
    nrruns = 0

    for module, moduleData in modules.items():
        for run, d in moduleData.items():
            totvals['log_rx_rejects'] += d['log_rx_rejects']
            totvals['log_tx_rejects'] += d['log_tx_rejects']
            totvals['log_rx_success'] += d['log_rx_success']
            totvals['log_tx_success'] += d['log_tx_success']
            #totvals['log_rx_lost'] += d['log_rx_lost']
            #totvals['log_tx_lost'] += d['log_tx_lost']
            nrruns += 1

    plotdata_notscheduled += [float(7500 - totvals['log_rx_success'] - totvals['log_tx_rejects'] - totvals['log_rx_rejects'])/75.0]
    plotdata_collisions += [float(totvals['log_rx_rejects']+totvals['log_tx_rejects'])/75.0]

    # In 500 runs 7500 transmissions!

    totvals['log_rx_success'] = float(totvals['log_rx_success'])/75.0 
    totvals['log_tx_success'] = float(totvals['log_tx_success'])/75.0 
    print(bp, module, nrruns, totvals)

    plotdata_successes += [totvals['log_rx_success']]
    plotdata_fails += [100.0-totvals['log_rx_success']]

print(plotdata_successes)
print(plotdata_fails)
print(plotdata_notscheduled)
print(plotdata_collisions)

fig = plt.figure()
ax = fig.add_axes([0.1,0.1,0.8,0.8])
#ax.set_yscale('log')
#ax.set_xscale('log')

#ax.plot(pklossPf, plotdata_notscheduled, 'bo-')
ax.plot(pklossPf, plotdata_fails, 'bo-')
#ax.plot(pklossPf, plotdata_collisions, 'bo-')

ax.legend()
ax.set_xlabel('Packet loss rate')
#ax.set_ylabel('Non-scheduled mmWave transmissions [%]');
ax.set_ylabel('Failed mmWave transmissions [%]');
#ax.set_ylabel('Collided mmWave transmissions [%]');

#ax.set_yticks([0.1, 0.3, 0.5, 1.0, 3.0, 5.0])
#ax.set_yticklabels(['0.1', '0.3', '0.5', '1', '3', '5'])

ax.set_xticks(pklossPf)
ax.set_xticklabels(pkloss)

ax.grid(True, 'major', 'y', alpha=0.6, ls='--', color='black')
ax.grid(True, 'minor', 'y', alpha=0.6, ls='--')
ax.grid(True, 'major', 'x', alpha=0.6, ls='--')

plt.show()