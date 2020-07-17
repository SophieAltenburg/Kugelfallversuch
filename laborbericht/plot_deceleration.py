#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import matplotlib.pyplot as plt

with open("../protokolle/twoCycles.csv", "r") as file:
    two_cycles = [[int(x) for x in line.split(",")] for line in file.readlines()]
    two_cycles = np.asarray(two_cycles)
    
with open("../protokolle/fiveCycles.csv", "r") as file:
    five_cycles = [[int(x) for x in line.split(",")] for line in file.readlines()]
    five_cycles = np.asarray(five_cycles)

plt.figure()
plt.plot(two_cycles[:-2,1])    
plt.ylabel("$t_r$ / ms")
plt.xlabel("Umdrehung")
plt.grid(True)
plt.savefig("turn_time_two_cycles.pdf")
            
plt.figure()
plt.plot(two_cycles[:-2,2])    
plt.ylabel("$\Delta t_r$ / ms (gemessen über 2 Zyklen)")
plt.xlabel("Umdrehung")
plt.grid(True)
plt.savefig("decleration_two_cycles.pdf")

plt.figure()
plt.plot(five_cycles[:-2,2])    
plt.ylabel("$\Delta t_r$ / ms (gemessen über 5 Zyklen)")
plt.xlabel("Umdrehung")
plt.grid(True)
plt.savefig("decleration_five_cycles.pdf")

plt.figure()
plt.plot(np.diff(two_cycles[:-2,1]))    
plt.ylabel("$\Delta t_r$ / ms (echt)")
plt.xlabel("Umdrehung")
plt.grid(True)
plt.savefig("true_decleration_two_cycles.pdf")
