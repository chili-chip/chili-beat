# Sound

Chili Beat has two kinds of sound: **tunes** that play on the beat, and **blips**
that rules play. Both are made from a few choices, so whatever you pick sounds fine.

## Tunes

A tune is a loop of 16 steps. One step plays on each clock beat, so the music
always follows the game's tempo. Each screen picks one tune, or none. A new tune
starts from its first step when you enter a screen that has it.

| Voice | Sound | Values |
|-------|-------|--------|
| `lead` | Square wave, the melody | `0` rest, `1`-`15` notes of the scale (two octaves from C4) |
| `beat` | Triangle bass and drums | `0` rest, `1`-`7` bass notes (from C3), `8` kick, `9` snare, `10` hi-hat |

Each note fades out over one beat. A rest lets the last note finish.

### Scales

Notes are scale steps, not piano keys. Step `1` is the root (C).

| Scale | Steps 1-8 | Feels |
|-------|-----------|-------|
| `major` | C D E F G A B C | bright |
| `minor` | C D E♭ F G A♭ B♭ C | dark |
| `pentatonic-major` | C D E G A C' D' E' | happy, can't go wrong |
| `pentatonic-minor` | C E♭ F G B♭ C' E♭' F' | bluesy, can't go wrong |

```json
{ "id": "main", "scale": "pentatonic-major",
  "lead": [1, 0, 3, 0, 5, 0, 3, 0, 6, 0, 5, 3, 2, 0, 1, 0],
  "beat": [8, 0, 1, 0, 9, 0, 1, 0, 8, 0, 5, 0, 9, 0, 10, 10] }
```

## Blips

A blip is a preset plus three sliders, each 0 to 7.

| Preset | Sound | Wave |
|--------|-------|------|
| `coin` | Two quick high notes, the second higher | square |
| `jump` | A rising chirp | square |
| `hit` | A short crunch | noise |
| `boom` | A long rumble | noise |
| `step` | A soft tap | triangle |
| `power` | A long rising sweep | square |
| `blip` | A short beep | square |

| Slider | 0 | 4 (default for pitch) | 7 |
|--------|---|---|---|
| `pitch` | half the preset's pitch | the preset's pitch | 1.4 × |
| `length` | 0.4 × | 1.2 × (3 is the preset's length) | 1.8 × |
| `slide` | no bend | | bends nearly a whole octave |

Two blips can play at once. A third one replaces the one that started first.

## What the engine sends to the player

The engine does not make sound waves itself. On every step it tells the player
what each of 4 channels should play:

| Channel | Plays |
|---------|-------|
| 0 | Tune lead |
| 1 | Tune beat |
| 2, 3 | Blips |

```cpp
struct Channel {
  enum class Wave : uint8_t { Square, Triangle, Noise };
  Wave wave;
  uint16_t frequency;  // Hz
  uint8_t volume;      // 0 silent, 255 full
};
```

The console player maps these to 32blit audio channels. The web player maps them
to Web Audio oscillators. Because the engine decides every value, a game sounds
the same on both.
