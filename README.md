# Installing Gurobi

Gurobi is complicated to install. Here are the required environment variables :

```
export GUROBI_HOME=path/to/gurobi900/linux64
export PATH="${GUROBI_HOME}/bin:${PATH}"
export GRB_LICENSE_FILE=path/to/gurobi.lic
```

## Linker not happy

In `$GUROBI_HOME/lib` change the symbolic link `libgurobi_c++.a -> ./libgurobi_g++4.2.a` to point to `libgurobi_g++5.2.a`.

# Plots and stuff

* The cells plot was obtained from
```
./build9/visu --cells <config-file>
./build9/visu --circlecells <config-file>
```

* The Master Solver Benchmark Plot:

```
# Execute CG
PANDORA=1 python3 py/master_solver_benchmark.py configs/master_solver_benchmarks/*.cgdump

# Plot Results
python3 py/master_solver_benchmark_plotter.py <Directory Containing all the CGDUMPS from the command before>/PythonData.pkl
```

* The Angle Directivity Term:
```
./build/visu --cos_correction_deviance configs/50_ormh.csv
```

* The executions where multiple tangent-values where tried out

```
# Execute CG
PANDORA=1 MAX_COLUMNS=50 python3 py/tangent_steps.py configs/50_ormh.csv_downsampled_32.csv
# Plot Results
python3 py/tangent_steps_plotter.py <Directory Containing all the CGDUMPS from the command before>
```

* The executions for different number of slaves:
```
# Execute CG
PANDORA=1 python3 py/multi_slave_benchmark.py
# Plot Results
python3 py/multi_slave_benchmark_plotter.py <Directory Containing all the CGDUMPS from the command before>
```

