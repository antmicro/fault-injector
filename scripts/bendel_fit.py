#!/usr/bin/env -S uv run --script
#
# /// script
# requires-python = ">3.12,<3.14"
# dependencies = ["numpy", "matplotlib"]
# ///

import numpy as np
import matplotlib.pyplot as plt

# =============================================================================

class Cell:
    def __init__(self, area):
        self.area = area

        # DOF/DEC/GER/RP7.222
        # https://escies.org/download/webDocumentFile?id=1036
        # MT4LC4M4B1D28M, 3.3V
        # self.bendel_a = 2.4705 # [MeV]
        # self.bendel_b = 0.2427 # [MeV]
        # per cell:
        self.bendel_a = 2.4704718 # [MeV]
        self.bendel_b = 0.7964778 # [MeV]

        # # MT4LC4M4B1D28M, 4.5V
        # self.bendel_a = 6.0363 # [MeV]
        # self.bendel_b = 0.5851 # [MeV]

        # # "An empirical model for predicting proton induced upset"
        # # 62256R
        # self.bendel_a   = 9.0    * 1e6 # [Mev]
        # self.bendel_b   = 8.6    * 1e6 # [Mev]
        # #self.bendel_g0  = 1.0    * 1e6 # [cm^2]

class Stream:
    def __init__(self, model, phi, let_or_energy, theta=0.0):
        self.model          = model
        self.flux_phi       = phi
        self.flux_theta     = theta
        self.let_or_energy  = let_or_energy

    def __call__(self, cell):
        cos = np.cos(np.deg2rad(self.flux_theta))

        if self.model == "bendel":
            # Bendel curve
            # TODO: This model uses energy, not LET. Should I divide by cos(th) ??
            y = np.sqrt(18.0 / cell.bendel_a) * (self.let_or_energy * 1e-6 - cell.bendel_a)
            y = max([y, 0.0])
            if hasattr(self, "bendel_g0"):
                g0 = self.bendel_g0
            else:
                g0 = np.power(cell.bendel_b / cell.bendel_a, 14.0)
            g = g0 * np.power(1.0 - np.exp(-0.18 * np.sqrt(y)), 4.0)

            print(y, g, g0)
        else:
            assert False, self.model

        # Lambda for exponential distribution
        h = g * self.flux_phi * cell.area * cos

        p  = np.random.rand()
        dt = -np.log(1 - p) / h
        print(h, p, dt)
        return dt

# =============================================================================

def run_simulation(**kw):

    # Artificially correct cell area for bit count and num_cells
    # Cell area given
    if "cell_area" in kw:
        cell_area = kw["cell_area"] * (kw["bit_count"] / kw["num_cells"])
    # Device area given
    elif "device_area" in kw:
        cell_area = kw["device_area"] / kw["num_cells"]
    else:
        assert False

    cell = Cell(
        cell_area,
    )

    if "let" in kw:
        let_or_energy = kw["let"]
    if "energy" in kw:
        let_or_energy = kw["energy"]

    streams = [Stream(
        kw["model"],
        kw["flux_phi"],
        let_or_energy,
        kw.get("flux_theta", 0.0),
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
    # ......................................

    # DOF/DEC/GER/RP7.222
    # MT4LC4M4B1D28M
    # https://escies.org/download/webDocumentFile?id=1036

    # Proton
    cases = [
        # MT4LC4M4B1D28M 3.3V
        # SN2
        {
            "name":       "run55",
            "energy":     20.0   * 1e6,    # [Mev]
            "flux_phi":   1.12e8 * 1e4,    # [s^-1 * cm^-2]
            "fluence":    1e10   * 1e4,    # [cm^-2]
            "seu_count":  344,
        },
        {
            "name":       "run52",
            "energy":     40.0   * 1e6,    # [Mev]
            "flux_phi":   1.19e8 * 1e4,    # [s^-1 * cm^-2]
            "fluence":    1e10   * 1e4,    # [cm^-2]
            "seu_count":  621,
        },
        {
            "name":       "run47",
            "energy":     60.0   * 1e6,    # [Mev]
            "flux_phi":   9.17e7 * 1e4,    # [s^-1 * cm^-2]
            "fluence":    1e10   * 1e4,    # [cm^-2]
            "seu_count":  862,
        },

        # # MT4LC4M4B1D28M 4.5V
        # {
        #     "name":       "run27",
        #     "energy":     20.0   * 1e6,    # [Mev]
        #     "flux_phi":   1.12e8 * 1e4,    # [s^-1 * cm^-2]
        #     "fluence":    1e10   * 1e4,    # [cm^-2]
        #     "seu_count":  93,
        # },
        # {
        #     "name":       "run26",
        #     "energy":     40.0   * 1e6,    # [Mev]
        #     "flux_phi":   1.19e8 * 1e4,    # [s^-1 * cm^-2]
        #     "fluence":    1e10   * 1e4,    # [cm^-2]
        #     "seu_count":  361,
        # },
        # {
        #     "name":       "run19",
        #     "energy":     60.0   * 1e6,    # [Mev]
        #     "flux_phi":   9.17e7 * 1e4,    # [s^-1 * cm^-2]
        #     "fluence":    1e10   * 1e4,    # [cm^-2]
        #     "seu_count":  471,
        # },
    ]

        # # Heavy ion
        # cases = [
        #     {
        #         "name":       "run11",
        #         "energy":     316.0,         # [Mev]
        #         "flux_phi":   150.0  * 1e4,  # [s^-1 * cm^-2]
        #         "fluence":    23316,         # [cm^-2]
        #     },
        #     {
        #         "name":       "run12",
        #         "energy":     316.0,         # [Mev]
        #         "flux_phi":   150.0  * 1e4,  # [s^-1 * cm^-2]
        #         "fluence":    13038,         # [cm^-2]
        #     },
        #     {
        #         "name":       "run36",
        #         "energy":     150.0,         # [Mev]
        #         "flux_phi":   270.0  * 1e4,  # [s^-1 * cm^-2]
        #         "fluence":    28257,         # [cm^-2]
        #     },
        #     {
        #         "name":       "run37",
        #         "energy":     150.0,         # [Mev]
        #         "flux_phi":   270.0  * 1e4,  # [s^-1 * cm^-2]
        #         "fluence":    25980,         # [cm^-2]
        #     },
        #     {
        #         "name":       "run50",
        #         "energy":     78.0,          # [Mev]
        #         "flux_phi":   350.0  * 1e4,  # [s^-1 * cm^-2]
        #         "fluence":    11478,         # [cm^-2]
        #     },
        #     {
        #         "name":       "run51",
        #         "energy":     78.0,          # [Mev]
        #         "flux_phi":   350.0  * 1e4,  # [s^-1 * cm^-2]
        #         "fluence":    12611,         # [cm^-2]
        #     },
        #     {
        #         "name":       "run56",
        #         "energy":     78.0,          # [Mev]
        #         "flux_phi":   1000.0  * 1e4,  # [s^-1 * cm^-2]
        #         "flux_theta": 54.0,
        #         "fluence":    180518         # [cm^-2]
        #     },
        #     {
        #         "name":       "run57",
        #         "energy":     78.0,          # [Mev]
        #         "flux_phi":   1000.0  * 1e4,  # [s^-1 * cm^-2]
        #         "flux_theta": 54.0,
        #         "fluence":    91917,         # [cm^-2]
        #     },
        # ]
        # ......................................

        # # HRX/SEE/0073
        # # https://escies.org/download/webDocumentFile?id=847

        # cases = [
        #     {
        #         "name":       "run108",
        #         "let":        1.70   * 1e5,  # [Mev * cm^2 / mg]
        #         "flux_phi":   6993.0 * 1e4,  # [s^-1 * cm^-2]
        #         "flux_theta": 0.0,
        #         "max_time":   143,           # [s]
        #         "seu_count":  435,
        #     },
        #     {
        #         "name":       "run107",
        #         "let":        1.70   * 1e5,  # [Mev * cm^2 / mg]
        #         "flux_phi":   7143.0 * 1e4,  # [s^-1 * cm^-2]
        #         "flux_theta": 0.0,
        #         "max_time":   140,           # [s]
        #         "seu_count":  429,
        #     },
        #     # {
        #     #     "name":       "run109",
        #     #     "let":        1.70   * 1e5,  # [Mev * cm^2 / mg]
        #     #     "flux_phi":   4975.0 * 1e4,  # [s^-1 * cm^-2]
        #     #     "flux_theta": 45.0,
        #     #     "max_time":   201,           # [s]
        #     #     "seu_count":  743,
        #     # },
        #     # {
        #     #     "name":       "run110",
        #     #     "let":        1.70   * 1e5,  # [Mev * cm^2 / mg]
        #     #     "flux_phi":   3390.0 * 1e4,  # [s^-1 * cm^-2]
        #     #     "flux_theta": 60.0,
        #     #     "max_time":   295,           # [s]
        #     #     "seu_count":  1914,
        #     # },

        #     {
        #         "name":       "run73",
        #         "let":        5.85   * 1e5,  # [Mev * cm^2 / mg]
        #         "flux_phi":   489.0  * 1e4,  # [s^-1 * cm^-2]
        #         "flux_theta": 0.0,
        #         "max_time":   159,           # [s]
        #     },
        #     {
        #         "name":       "run72",
        #         "let":        5.85   * 1e5,  # [Mev * cm^2 / mg]
        #         "flux_phi":   81.0   * 1e4,  # [s^-1 * cm^-2]
        #         "flux_theta": 0.0,
        #         "max_time":   291,           # [s]
        #     },
        #     # {
        #     #     "name":       "run74",
        #     #     "let":        5.85   * 1e5,  # [Mev * cm^2 / mg]
        #     #     "flux_phi":   835.0  * 1e4,  # [s^-1 * cm^-2]
        #     #     "flux_theta": 45.0,
        #     #     "max_time":   160,           # [s]
        #     # },
        #     # {
        #     #     "name":       "run75",
        #     #     "let":        5.85   * 1e5,  # [Mev * cm^2 / mg]
        #     #     "flux_phi":   594.0  * 1e4,  # [s^-1 * cm^-2]
        #     #     "flux_theta": 60.0,
        #     #     "max_time":   237,           # [s]
        #     # },

        #     {
        #         "name":       "run23",
        #         "let":        14.10  * 1e5,  # [Mev * cm^2 / mg]
        #         "flux_phi":   419.0  * 1e4,  # [s^-1 * cm^-2]
        #         "flux_theta": 0.0,
        #         "max_time":   225,           # [s]
        #     },
        #     {
        #         "name":       "run22",
        #         "let":        14.10  * 1e5,  # [Mev * cm^2 / mg]
        #         "flux_phi":   511.0  * 1e4,  # [s^-1 * cm^-2]
        #         "flux_theta": 0.0,
        #         "max_time":   186,           # [s]
        #     },
        #     # {
        #     #     "name":       "run25",
        #     #     "let":        14.10  * 1e5,  # [Mev * cm^2 / mg]
        #     #     "flux_phi":   242.0  * 1e4,  # [s^-1 * cm^-2]
        #     #     "flux_theta": 45.0,
        #     #     "max_time":   248,           # [s]
        #     # },
        #     # {
        #     #     "name":       "run24",
        #     #     "let":        14.10  * 1e5,  # [Mev * cm^2 / mg]
        #     #     "flux_phi":   244.0  * 1e4,  # [s^-1 * cm^-2]
        #     #     "flux_theta": 45.0,
        #     #     "max_time":   248,           # [s]
        #     # },
        #     # {
        #     #     "name":       "run26",
        #     #     "let":        14.10  * 1e5,  # [Mev * cm^2 / mg]
        #     #     "flux_phi":   142.0  * 1e4,  # [s^-1 * cm^-2]
        #     #     "flux_theta": 60.0,
        #     #     "max_time":   341,           # [s]
        #     # },
        # ]

    # Derive max time from fluence and flux (t = fluence / flux)
    for case in cases:
        case["max_time"] = case["fluence"] / case["flux_phi"]

    # die size:  5.6mm x 10.2mm
    # cell size: 0.875um x 1.75um
    params = {
        "model":        "bendel",
        "bit_count":    1 << 24,     # 16Mb
        "device_area":  57.12e-6,    # [mm^2]
        "num_cells":    64,          # Artificial. 1 cell represents N device cells
    }


    # ......................................

    for i, case in enumerate(cases):
        name = case.get("name", f"#{i}")

        kw = dict(params.items())
        kw.update(case)

        data = []
        for j in range(10):
            total_hits, vis = run_simulation(**kw)
            data.append(total_hits)

        if "seu_count" in case:
            s = f"vs. {case['seu_count']}"
        else:
            s = ""

        print(name, np.min(data), np.average(data), np.max(data), s)

if __name__ == "__main__":
    main()
