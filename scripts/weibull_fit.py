#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

import numpy as np
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt

# =============================================================================

def weibull(x, g0, L0, W, s):
    return g0 * (1.0 - np.exp(-np.power((x - L0) / W, s)))

def weibull_fit(xs, ys):
    p0    = np.zeros(4, dtype=np.float32)

    p0[0] = max(ys)
    p0[1] = 0.0
    p0[2] = 1.0
    p0[3] = 2.0

    popt, pcov  = curve_fit(weibull, xs, ys, p0, method="dogbox")
    return popt

# =============================================================================

if __name__ == "__main__":
    xs = [1.1, 3.0, 10.2, 20.4, 32.6, 40.4, 67.7]
    ys = [2.67e-5, 1.06e-2, 4.74e-2, 1.08e-1, 1.52e-1, 1.80e-1, 2.38e-1]

    popt = weibull_fit(xs, ys)
    print(popt)

    t = np.linspace(min(xs), max(xs), 100)
    f = weibull(t, *popt)

    plt.figure()
    plt.semilogy(xs, ys, '*')
    plt.semilogy(t,  f)

    plt.xlabel("LET")
    plt.ylabel("Cross-section")

    plt.show()
    
