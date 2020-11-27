#!/usr/bin/env python
import numpy as np
import matplotlib.pyplot as plt

def gauss(x):
    exponent = -x ** 2
    exponent /= 2
    return np.exp(exponent) / np.sqrt(2 * np.pi)

def hamming(x, alpha = 0.56):
    return alpha - (1 - alpha) * np.cos( (2 * np.pi * x)/(x[-1]))

"""
    gain : Civa reference signal is normalized to 100
    omega1 : bandwith of the window (gauss/hamming)
    freq : frequence of the reference signal in Mhz
    phase_shift : phase shift of cosine
"""
def f(x, gain, omega1, freq, phase_shift = 0):
    s = x - (x[-1] - x[0])/2
    omega2 = 2.0 * np.pi * freq
    return gain * 2.5 * gauss(omega1 * s) * np.cos(omega2 * s + phase_shift)

def f2(x):
    s = x - x[-1]/2
    omega1 = 2
    phase = np.pi / 4
    omega2 = 10.0
    gain = 1
    return gain * hamming(omega1 * x) * np.sin(omega2 * (s + phase))

files = ["gauss_2Mhz_1percent_0degree_signal.csv", "gauss_2Mhz_50percent_0degree_signal.csv", "gauss_2Mhz_60percent_0degree_signal.csv", "gauss_2Mhz_70percent_0degree_signal.csv"]
parameters = [0, 2.7, 3.2, 3.7]

fig, ax = plt.subplots(2, 2)
i = 0

for file, p in zip(files, parameters):
    a = np.loadtxt("./data/simulated_references/" + file, dtype=np.float64, usecols=(0,1,2), delimiter=';')
    original = a[:,2]
    reconstructed = f(a[:,0], gain=100, omega1=p, freq=2)
    ax[i // 2, i % 2].plot(a[:,0], original)
    ax[i // 2, i % 2].plot(a[:,0], reconstructed)
    error = ((original - reconstructed) ** 2).sum() / len(original)
    print("Mean Squared Error is "+ str(error) + " for file " + file)
    i+=1

plt.show()
