#!/usr/bin/env python

import numpy as np
import dakota.interfacing as di


def re_func(x):
#     if x[0] < 0:
#         return [np.exp(5 * x[0])]
#     else:
#         return [np.exp(-5 * x[0])]

# Genz Continuous 1D
#     a = 5
#     u = 0.5
#     return [np.exp(-a * abs(x[0] - u))]

# Genz discontinuous 1D
#     a = 5
#     u = 0.5
#     if x[0] > u:
#         return [0]
#     else:
#         return [np.exp(a * x[0])]

# Genz Corner Peak
    a = 5
    u = 0.5
    return [(1 + a * x[0]) ** (-(1 + 1))]

# tensorMonomial
#     prod = 1
#     for d in range(len(x)):
#         prod *= (x[d] + 1) 
#     return [prod]

# attenuation
#     dim = len(x) 
#     prod = 1
#     for d in range(dim):
#         prod *= np.exp(-x[d] / dim) 
#     return [prod]

# borehole
#     rw = x[0]; r = x[1]; Tu = x[2]; Hu = x[3]
#     Tl = x[4]; Hl = x[5]; L = x[6]; Kw = x[7]
#     return [(2 * np.pi * Tu * (Hu - Hl)) / (np .log(r / rw) * (1 + ((2 * L * Tu / np.log(r / rw) * rw * rw * Kw) + (Tu / Tl))))]


params, results = di.read_parameters_file()

continuous_vars = [ params['x1']]
# continuous_vars = [ params['x1'], params['x2']]
# continuous_vars = [ params['x1'], params['x2'], params['x3'] ]
# continuous_vars = [ params['x1'], params['x2'], params['x3'], params['x4'], params['x5'], params['x6'] ]
# continuous_vars = [ params['x1'], params['x2'], params['x3'], params['x4'], params['x5'], params['x6'], params['x7'], params['x8'] ]
# continuous_vars = [ params['x1'], params['x2'], params['x3'], params['x4'], params['x5'], params['x6'], params['x7'], params['x8'], params['x9'] ]
evaluations = re_func(continuous_vars)

for i, r in enumerate(results.responses()):
    if r.asv.function:
        r.function = evaluations[i]

results.write()
