"""Generate the sound effects of the game example (examples/assets/*.wav).

The sounds are simple synthesized waveforms, so they are small and free to use.
Run this script again after changing a sound:

    python examples/game/make_sounds.py
"""
import math
import os
import struct
import wave

SAMPLE_RATE = 22050
ASSETS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")


def square(phase):
    return 1.0 if phase % 1.0 < 0.5 else -1.0


def triangle(phase):
    return 4.0 * abs(phase % 1.0 - 0.5) - 1.0


def saw(phase):
    return 2.0 * (phase % 1.0) - 1.0


def tone(wave_fn, duration, f0, f1=None, volume=0.5, decay=8.0):
    """A tone that slides from frequency f0 to f1 and fades out."""
    f1 = f0 if f1 is None else f1
    n = int(SAMPLE_RATE * duration)
    samples = []
    phase = 0.0
    for i in range(n):
        t = i / n
        phase += (f0 + (f1 - f0) * t) / SAMPLE_RATE
        attack = min(1.0, i / (SAMPLE_RATE * 0.002))  # 2 ms, avoids a click.
        envelope = attack * math.exp(-decay * t) * (1.0 - t)
        samples.append(wave_fn(phase) * envelope * volume)
    return samples


def save(name, samples):
    path = os.path.join(ASSETS, name)
    with wave.open(path, "wb") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SAMPLE_RATE)
        f.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s)) * 32767)) for s in samples))
    print(f"{path}: {len(samples) / SAMPLE_RATE:.2f} s")


save("paddle.wav", tone(square, 0.08, 440, 520, volume=0.35, decay=4.0))
save("wall.wav", tone(triangle, 0.05, 300, volume=0.5, decay=6.0))
save("brick.wav", tone(square, 0.10, 660, 990, volume=0.3, decay=5.0))
save("lose.wav", tone(saw, 0.70, 400, 70, volume=0.4, decay=2.0))
