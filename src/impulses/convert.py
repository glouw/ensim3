import matplotlib.pyplot as plt
from scipy.io import wavfile
import numpy as np

wav = 'minimal_muffling_01.wav'

rate, data = wavfile.read(wav)
data = data[:8192]
data = data / np.max(np.abs(data), axis=0)

plt.figure(figsize=(10, 4))
plt.plot(data)
plt.title('Waveform of the WAV file')
plt.xlabel('Samples')
plt.ylabel('Amplitude')
plt.grid(True)
plt.show()

for x in data:
    print(x)
