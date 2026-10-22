import numpy as np

serial = np.loadtxt("serial_results.csv", delimiter=",")
parallel = np.loadtxt("mpi_results_4.csv", delimiter=",")

print("Maximum absolute error:", np.max(np.abs(serial - parallel)))
print("Results match:", np.allclose(serial, parallel, rtol=1e-12, atol=1e-12))