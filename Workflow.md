# Workflow

## Using Simulated Data from ,,Civa 2016a''

* Simulate data using Civa and save it as a \*.civa folder

* You can find the measurements in `*.civa/proc0/model_data/model_Dvoie.bin` as an array of double with shape (position, sender, receiver, sample) for the FMC, and `*.civa/proc0/model_data/model_Dsig.bin` contains an array of double with shape (position, sender, sample) which contains the sum of the signal over all receivers of the FMC.

Sometimes the `model_data` doesn't exist, but you can create it in civa by rightclicking the measurements and saving it as `*.civa`.

* In Civa, you can generate HTML-Reports that contains all the information needed in the data : samples, gate delay (in µs, needs to be multiplied by samplingfrequence to get the offset), number of elements, position of the probe, pitch, etc.

* The reference-signal can be exported under Probe-\>Signal as an csv (containing its own length, so no need for any reports like above)
