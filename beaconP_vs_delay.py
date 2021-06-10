import matplotlib.pyplot as plt

# TODO get those values from real data instead of graphs
beaconP = [0.1, 0.2, 0.3, 0.4, 0.5, 1.0]
tx1_1 = [0.096, 0.145, 0.175, 0.225, 0.275, 0.475]
tx3_1 = [0.15, 0.2, 0.26, 0.31, 0.37, 0.65]
tx1_5 = [0.275, 0.305, 0.36, 0.43, 0.52, 0.92]
tx3_5 = [0.45, 0.58, 0.68, 0.81, 0.96, 1.80]

fig = plt.figure()
ax = fig.add_axes([0.1, 0.1, 0.8, 0.8])
ax.set_yscale('log')
# ax.set_xscale('log')

ax.plot(beaconP, tx1_1, 'bo-', label='1 TX\'er 1st')
ax.plot(beaconP, tx1_5, 'bo:', label='1 TX\'er 5th')
ax.plot(beaconP, tx3_1, 'rs-', label='3 TX\'ers 1st')
ax.plot(beaconP, tx3_5, 'rs:', label='3 TX\'ers 5th')

ax.legend()
ax.set_xlabel('Beacon period [s]')
ax.set_ylabel('Delay to scheduled neighbors [s]');

ax.set_yticks([0.1, 0.3, 0.5, 1.0, 3.0, 5.0])
ax.set_yticklabels(['0.1', '0.3', '0.5', '1', '3', '5'])

ax.set_xticks([0.1, 0.2, 0.3, 0.4, 0.5, 1.0])
ax.set_xticklabels(['0.1', '0.2', '0.3', '0.4', '0.5', '1.0'])

ax.grid(True, 'major', 'y', alpha=0.6, ls='--', color='black')
ax.grid(True, 'minor', 'y', alpha=0.6, ls='--')
ax.grid(True, 'major', 'x', alpha=0.6, ls='--')

plt.show()