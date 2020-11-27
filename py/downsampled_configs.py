#!/usr/bin/python

"""
Creates copies of configs with downsampled data. Arguments should be configs, but creates also corresponding data/references.
"""

from simple_down_sampling import simple_down_sampling
from enum import IntEnum
import sys

class config_columns(IntEnum):
    INPUT_FILE = 0
    REFERENCE_FILE = 1
    PITCH = 4
    SPEED = 6
    ELEMENTS = 7
    SAMPLES = 10
    REFERENCE_SAMPLES = 11
    OFFSET = 15

def main():
    for current_config in sys.argv[1:]:
        with open(current_config) as f:
            last_line = f.readlines()[-1]

            values = last_line.split(";")
            """ Load data """
            elements = values[config_columns.ELEMENTS]
            sds = simple_down_sampling(elements=elements, offset=values[config_columns.OFFSET])
            sds.read(values[0])
            sds_reference = simple_down_sampling(elements=1)
            sds_reference.read(values[1])
            original_speed = values[config_columns.SPEED]

            #for sampling_size in [2**x for x in range(1,6)]:
            for sampling_size in [6]:
                sds.stepsize = sampling_size
                sds_reference.stepsize = sampling_size

                values[config_columns.INPUT_FILE] = str(sds.to())
                values[config_columns.REFERENCE_FILE] = str(sds_reference.to())
                values[config_columns.SAMPLES] = str(sds.result.shape[-1])
                values[config_columns.REFERENCE_SAMPLES] = str(sds_reference.result.shape[-1])
                values[config_columns.OFFSET] = str(sds.new_offset)
                values[config_columns.SPEED] = str(int(float(original_speed) * sampling_size))

                new_config = ";".join(values)
                new_config_name = current_config + f"_downsampled_{sampling_size}.csv" if not sds._decimate else current_config + f"_decimate_{sampling_size}.csv"
                with open(current_config + f"_downsampled_{sampling_size}.csv", "w") as output:
                    output.write(new_config)

if __name__ == "__main__":
    main()