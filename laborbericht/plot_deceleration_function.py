#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Jul 30 14:53:09 2020

@author: jakob
"""

import numpy as np
import matplotlib.pyplot as plt

def f(x):
    if x <= 900:
        return 20
    elif x <= 2200:
        return (x-900)/20+20
    elif x <= 2500:
        return (x-2200)/10+85
    
y = [f(x) for x in range(2500)]
plt.plot(y)