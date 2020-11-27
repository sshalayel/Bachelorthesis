import numpy as np
import data_reader
import sys
import scipy.signal as sig

if __name__ == "__main__":
    for f in sys.argv[1:]:
        decimate = 6
        output_file = f"{f}_decimate_{decimate}.bin"
        a = data_reader.load_data(f)
        a = a.reshape(16,16,-1)

        # create offset of 100, as the sent impulse is part of the signal
        offset = 100
        new_shape = list(a.shape)
        new_shape[-1] -= offset
        new_shape = tuple(new_shape)

        new_a = np.zeros(new_shape)
        new_a[:] = a[:,:,offset:]

        new_a = sig.decimate(new_a, decimate, ftype='fir', axis=-1)

        print(f"Created file <<{output_file}>> with shape {new_a.shape}")

        new_a.astype(np.float64).tofile(output_file)