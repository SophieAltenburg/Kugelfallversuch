#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import matplotlib.pyplot as plt

with open("../protokolle/twoCycles.csv", "r") as file:
    two_cycles = [[int(x) for x in line.split(",")] for line in file.readlines()]
    two_cycles = np.asarray(two_cycles)
    
def f(x):
    if x <= 900:
        return 20
    elif x <= 2200:
        return (x-900)/20+20
    elif x <= 2500:
        return (x-2200)/10+85
    
y = [f(x) for x in range(2500)]
plt.plot(two_cycles[:-3,1],
         np.diff(two_cycles[:-2,1]),
         label="Abbremsung ohne Außeneinwirkung")
plt.plot(y,
         label="Akzeptierte Abbremsung")
plt.xlim(min(two_cycles[:-3,1])-100, 2600)
plt.ylim(0, 120)
plt.grid(True)

plt.xlabel("$tr$ / ms")
plt.ylabel("$delta tr$ / ms")
plt.legend()

plt.savefig("acceptable_deceleration.pdf")