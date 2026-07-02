#!/usr/bin/env -S uv run --script
#
# /// script
# requires-python = ">3.12,<3.14"
# dependencies = ["numpy", "matplotlib"]
# ///

# SPDX-License-Identifier: Apache-2.0

import numpy as np
import matplotlib.pyplot as plt

# =============================================================================

class Cell:
    def __init__(self, area):
        self.area = area
        
        self.weibull_L0 = 1.09   * 1e5  # [Mev * cm^2 / mg]
        self.weibull_W  = 39.25  * 1e5  # [Mev * cm^2 / mg]
        self.weibull_s  = 1.116
        self.weibull_g0 = 0.284  * 1e-4 # [s * cm^2 / bit]


class Stream:
    def __init__(self, phi, let, theta=0.0):
        self.flux_phi   = phi
        self.flux_theta = theta
        self.let        = let

    def __call__(self, cell):
        cos = np.cos(np.deg2rad(self.flux_theta))

        g = cell.weibull_g0 * (1.0 - np.exp(-np.power((self.let / cos - cell.weibull_L0) / cell.weibull_W, cell.weibull_s)))
        h = g * self.flux_phi * cell.area * cos

        p  = np.random.rand()
        dt = -np.log(1 - p) / h
        return dt

# =============================================================================

def run_simulation(**kw):

    cell = Cell(
        kw["cell_area"],
    )

    # Artificially correct g0 for bit count and num_cells
    cell.weibull_g0 *= kw["bit_count"] / kw["num_cells"]

    streams = [Stream(
        kw["flux_phi"],
        kw["let"],
    )]

    # Initialize
    event_time = [
        [stream(cell) for stream in streams]
        for i in range(kw["num_cells"])
    ]

    history    = [list() for i in range(kw["num_cells"])]
    curr_time  = 0.0
    total_hits = 0

    # Run
    while True:

        # Next event
        time_list = [min(l) for l in event_time]
        indx_list = [l.index(t) for t, l in zip(time_list, event_time)]

        cell_id   = time_list.index(min(time_list))
        stream_id = indx_list[cell_id]

        prev_time = curr_time
        curr_time = event_time[cell_id][stream_id]

        # Finish
        if curr_time >= kw["max_time"]:
            break

        # Store event history
        history[cell_id].append((curr_time, stream_id))
        total_hits += 1

        # Schedule next one
        event_time[cell_id][stream_id] += streams[stream_id](cell)

    # Visualize
    vis = np.zeros((kw["num_cells"], 512, 3), dtype=np.uint8)
    for i, events in enumerate(history):
        y = i
        for j, hit in events:
            x = int(j * vis.shape[1] / kw["max_time"])
            vis[y, x, :] = [
                (255, 0, 0),
                (0, 255, 0),
                (0, 0, 255),
            ][hit]

    return total_hits, vis

# =============================================================================

def main():

    # TRAD/TI/R1RW0416DSB/1346/ESA/LG/1409
    # https://escies.org/download/webDocumentFile?id=63660

    cases = [
        {
            "let":        67.7   * 1e5,  # [Mev * cm^2 / mg]
            "flux_phi":   9.15e3 * 1e4,  # [s^-1 * cm^-2]
            "max_time":   1094,          # [s]
        },
        {
            "let":        67.7   * 1e5,
            "flux_phi":   1.01e3 * 1e4,
            "max_time":   996,
        },
        {
            "let":        67.7   * 1e5,
            "flux_phi":   1.04e3 * 1e4,
            "max_time":   409,
        },
        {
            "let":        67.7   * 1e5,
            "flux_phi":   1.05e3 * 1e4,
            "max_time":   399,
        },
        {
            "let":        67.7   * 1e5,
            "flux_phi":   5.04e2 * 1e4,
            "max_time":   166,
        },
        {
            "let":        40.4   * 1e5,
            "flux_phi":   1.01e3 * 1e4,
            "max_time":   536,
        },
        {
            "let":        40.4   * 1e5,
            "flux_phi":   1.01e3 * 1e4,
            "max_time":   551,
        },

        {
            "let":        32.6   * 1e5,
            "flux_phi":   1.58e3 * 1e4,
            "max_time":   417,
        },
        {
            "let":        32.6   * 1e5,
            "flux_phi":   1.51e3 * 1e4,
            "max_time":   411,
        },
        {
            "let":        32.6   * 1e5,
            "flux_phi":   1.45e3 * 1e4,
            "max_time":   12,
        },
        {
            "let":        20.4   * 1e5,
            "flux_phi":   2.00e3 * 1e4,
            "max_time":   433,
        },
        {
            "let":        20.4   * 1e5,
            "flux_phi":   2.05e3 * 1e4,
            "max_time":   452,
        },
        {
            "let":        10.2   * 1e5,
            "flux_phi":   2.32e3 * 1e4,
            "max_time":   433,
        },
        {
            "let":        10.2   * 1e5,
            "flux_phi":   2.77e3 * 1e4,
            "max_time":   636,
        },
        {
            "let":        3.0    * 1e5,
            "flux_phi":   5.03e3 * 1e4,
            "max_time":   201,
        },
        {
            "let":        3.0    * 1e5,
            "flux_phi":   5.11e3 * 1e4,
            "max_time":   197,
        },
        {
            "let":        1.1    * 1e5,
            "flux_phi":   7.60e3 * 1e4,
            "max_time":   133,
        },
        {
            "let":        1.1    * 1e5,
            "flux_phi":   8.17e3 * 1e4,
            "max_time":   124,
        },
        {
            "let":        32.6   * 1e5,
            "flux_phi":   9.99e3 * 1e4,
            "max_time":   102,
        },
        {
            "let":        32.6   * 1e5,
            "flux_phi":   5.17e1 * 1e4,
            "max_time":   1275,
        },

    ]

    params = {
        "bit_count":    256000 * 16, # 4MB
        "cell_area":    0.25e-6,     # [mm^2] (tuned manually!)
        "num_cells":    64,          # Artificial. 1 cell represents N device cells
    }

    for i, case in enumerate(cases):
        kw = dict(params.items())
        kw.update(case)

        total_hits, vis = run_simulation(**kw)

        print(f"#{i}: {total_hits}")

if __name__ == "__main__":
    main()
