import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from pandas.core.accessor import register_index_accessor

print("Processing generated data")

results = pd.read_csv('trans.csv')
scalars = results[results.type=='scalar']

transmissions = {}

for row in scalars.itertuples():
    v = int(row.value)
    if v not in transmissions:
        transmissions[v] = 0
    transmissions[v] += 1

print(transmissions)
