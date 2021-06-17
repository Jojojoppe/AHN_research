import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

pkloss = ['0.05', '0.10', '0.15', '0.20', '0.25', '0.30', '0.40']
pklossPf = []

line = []

for bp in pkloss:
    pklossPf += [float(bp)]
    print(bp)

    # open res.csv
    results = pd.read_csv("results/packetloss_rx_%s_retransmissions/schedule.csv"%(bp))
    scalars = results[results.type=='scalar']

    beacons = {}
    first_time = {}
    last_time = {}
    for row in scalars.itertuples():
        # Get module
        module = row.module.split('.')[1]
        # Get run nr
        run = int(row.run.split('-')[1])
        # Get time
        time = row.value
        # Get type
        tp = row.name.split("_")[1]

        if run not in beacons:
            beacons[run] = {}
            first_time[run] = None
            last_time[run] = 0.0

        if tp=="beacon":
            beacon = row.name.split("_")[2]
            beacon_type = beacon.split(" ")[7]
            if beacon_type=="NONE":
                beacon_rx = ""
                beacon_time = ""
            elif beacon_type=="RTS":
                beacon_rx = beacon.split(" ")[9]
                beacon_time = beacon.split(" ")[11]
                if first_time[run] is None:
                    first_time[run] = time
            else:
                beacon_rx = beacon.split(" ")[9]
                beacon_time = beacon.split(" ")[11]

            # print("%s\t%s\tbeacon %s %s %s"%(module, time, beacon_type, beacon_rx, beacon_time))
            
            if module not in beacons[run]:
                beacons[run][module] = {}
                beacons[run][module]['NONE'] = [[], []]
                beacons[run][module]['RTS'] = [[], []]
                beacons[run][module]['CTS'] = [[], []]
                beacons[run][module]['rtscount'] = 0

            beacons[run][module][beacon_type][0] += [time]
            beacons[run][module][beacon_type][1] += [(beacon_rx, beacon_time)]
            if beacon_type=="RTS":
                beacons[run][module]['rtscount'] += 1
        
    # take mean for all runs
    divnr_run = 0
    rtscount_run = 0.0
    for m, b in beacons.items():
        # Take mean for all txers
        divnr = 0
        rtscount = 0
        for n, r in b.items():
            if r['rtscount']>0:
                divnr += 1
                rtscount += r['rtscount']
        rtscount_mean = float(rtscount)/float(divnr)
        # print("run %d: %f"%(m, rtscount_mean))
        divnr_run += 1
        rtscount_run += rtscount_mean
    rtscount_run = rtscount_run/float(divnr_run)
    print("%s: %f"%(bp, rtscount_run))
    line += [rtscount_run]


fig = plt.figure()
ax = fig.add_axes([0.1, 0.1, 0.8, 0.8])
#ax.set_yscale('log')
#ax.set_xscale('log')

ax.plot(pklossPf, line, 'rs-')

ax.set_xlabel('Packet loss ratio');
ax.set_ylabel('Number of RTS beacons');

ax.set_yticks([0, 1, 2, 3, 4, 5, 6])
ax.set_yticklabels(['0', '1', '2', '3', '4', '5', '6'])

ax.set_xticks(pklossPf)
ax.set_xticklabels(pkloss)

ax.grid(True, 'major', 'y', alpha=0.6, ls='--', color='black')
ax.grid(True, 'minor', 'y', alpha=0.6, ls='--')
ax.grid(True, 'major', 'x', alpha=0.6, ls='--')

plt.show()