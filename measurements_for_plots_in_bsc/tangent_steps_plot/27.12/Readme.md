This plot was achieved by running

```
# Execute CG
PANDORA=1 MAX_COLUMNS=50 python3 tangent_steps.py configs/50_ormh.csv_downsampled_32.csv
# Plot Results
python3 tangent_steps_plotter.py <Directory Containing all the CGDUMPS from the command before>
```

on pandora.