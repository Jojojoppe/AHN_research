import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from pandas.core.accessor import register_index_accessor

print("Processing generated data")

results = pd.read_csv('schedule.csv')
scalars = results[results.type=='scalar']

beacons = {}
transmissions = {}
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
        transmissions[run] = {}
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

        print("%s\t%s\tbeacon %s %s %s"%(module, time, beacon_type, beacon_rx, beacon_time))
        
        if module not in beacons[run]:
            beacons[run][module] = {}
            beacons[run][module]['NONE'] = [[], []]
            beacons[run][module]['RTS'] = [[], []]
            beacons[run][module]['CTS'] = [[], []]

        beacons[run][module][beacon_type][0] += [time]
        beacons[run][module][beacon_type][1] += [(beacon_rx, beacon_time)]

    elif tp=="tx" or tp=="rx":
        rxer = row.name.split("_")[2]
        duration = row.name.split("_")[3]

        if module not in transmissions[run]:
            transmissions[run][module] = [[], []]

        if tp=="tx":
            s = '>'+rxer
        else:
            s = '<'+rxer

        transmissions[run][module][0] += [[float(time), float(time)+float(duration)]]
        transmissions[run][module][1] += [s]

        last_time[run] = float(time) + float(duration)


beacons_bkp = beacons
transmissions_bkp = transmissions

for tri in range(len(beacons_bkp)):
    tri = 1200
    beacons = beacons_bkp[tri]
    transmissions = transmissions_bkp[tri]

    fig = plt.figure()
    ax = fig.add_axes([0.1, 0.1, 0.8, 0.8])

    i = 0
    mods = [0]
    mods_names = [""]
    ypositions = {}
    clri = {}

    # Plot beacons
    for m, b in beacons.items():
        i += 1
        mods += [i]
        mods_names += [m]
        ypositions[m] = i
        clri[m] = 0

        ax.scatter(b["NONE"][0], [i]*len(b["NONE"][0]), 50.0, 'black', '.')
        ax.scatter(b["RTS"][0], [i]*len(b["RTS"][0]), 50.0, 'red', '^')
        ax.scatter(b["CTS"][0], [i]*len(b["CTS"][0]), 50.0, 'blue', 'v')

        for j, d in enumerate(zip(b["RTS"][0], b["RTS"][1])):
            p = d[0]
            beacon = d[1]
            ax.annotate('%s\n%s'%(beacon[0], beacon[1]), xy=(p, i), xytext=(p, i-0.3), c='red', size='xx-small')

        for j, d in enumerate(zip(b["CTS"][0], b["CTS"][1])):
            p = d[0]
            beacon = d[1]
            ax.annotate('%s\n%s'%(beacon[0], beacon[1]), xy=(p, i), xytext=(p, i-0.5), c='blue', size='xx-small')

    # Plot transmissions
    clrs = ['#696969', '#BEBEBE']
    for m, t in transmissions.items():
        for i, transmission in enumerate(t[0]):
            ax.plot(transmission, [ypositions[m]]*2, lw=6, zorder=-1, c=clrs[clri[m]])
            ax.annotate(t[1][i], xy=(transmission[0], ypositions[m]+0.1), c=clrs[clri[m]], size='x-small')
            clri[m] = (clri[m]+1)%2

    ax.set_yticks(mods)
    ax.set_yticklabels(mods_names)

    ax.grid(True, 'major', 'x', alpha=0.6, ls='--')
    ax.grid(True, 'minor', 'x', alpha=0.4, ls='--')
    ax.grid(True, 'major', 'y', alpha=0.6, ls='--', color='black')

    ax.set_xlabel("time [s]")
    ax.set_ylabel("node")

    ax.set_title('Schedule for run %s'%tri);

    ax.set_xlim([first_time[tri]-0.02, last_time[tri]+0.02])

    plt.show()