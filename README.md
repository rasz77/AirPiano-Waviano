# AirPiano - Waviano: Air Piano with Multi-Track Recording

An interactive air piano that uses an ultrasonic sensor to detect hand position and play musical notes. Features guided song playback, free play mode, and a sophisticated 4-track recording system.

## Features

- **Guided Mode**: Follow along with pre-programmed songs (Mary Had a Little Lamb, Twinkle Twinkle, etc.)
- **Free Play Mode**: Play any notes freely by moving your hand
- **Multi-Track Recording**: Record up to 4 separate tracks (30 notes each)
- **Smart Playback**: Play back recordings individually or merged together
- **Overlap Resolution**: 4 different strategies for handling overlapping notes in multi-track playback
- **Real-time Feedback**: LED indicators and buzzer output

## Hardware Requirements

### Components
- **Arduino Uno** (or compatible board)
- **HC-SR04 Ultrasonic Sensor** (1x)
- **LEDs** (8x) - for note indication
- **Buzzer** (1x) - for audio output
- **Resistors** (8x 220Ω) - for LEDs
- **Breadboard and jumper wires**

### Wiring Diagram

#### Ultrasonic Sensor (HC-SR04)
| Sensor Pin | Arduino Pin |
|------------|-------------|
| VCC        | 5V          |
| GND        | GND         |
| TRIG       | Pin 4       |
| ECHO       | Pin 3       |

#### LEDs (Note Indicators)
| Note    | Arduino Pin | Resistor |
|---------|-------------|----------|
| Do (C5) | Pin 13      | 220Ω     |
| Re (D5) | Pin 12      | 220Ω     |
| Mi (E5) | Pin 11      | 220Ω     |
| Fa (F5) | Pin 10      | 220Ω     |
| Sol (G5)| Pin 9       | 220Ω     |
| La (A5) | Pin 8       | 220Ω     |
| Si (B5) | Pin 7       | 220Ω     |
| Do (C6) | Pin 6       | 220Ω     |

#### Buzzer
| Component | Arduino Pin |
|-----------|-------------|
| Buzzer +  | Pin 2       |
| Buzzer -  | GND         |

## Installation

1. **Clone or download** this repository
2. **Open** `PianoAir.ino` in Arduino IDE
3. **Connect** your Arduino board
4. **Select** your board type (Tools → Board → Arduino Uno)
5. **Select** your port (Tools → Port)
6. **Upload** the sketch (Ctrl+U or Upload button)
7. **Open** Serial Monitor (Ctrl+Shift+M) at **115200 baud**

## Usage Guide

### Getting Started

Once uploaded, open the Serial Monitor at **115200 baud**. You'll see the main menu with all available commands.

### Distance-to-Note Mapping

Move your hand above the ultrasonic sensor at different distances:

| Distance (cm) | Note    | Frequency |
|---------------|---------|-----------|
| 2-10 cm       | Do (C5) | 523 Hz    |
| 10-20 cm      | Re (D5) | 587 Hz    |
| 20-30 cm      | Mi (E5) | 659 Hz    |
| 30-40 cm      | Fa (F5) | 698 Hz    |
| 40-50 cm      | Sol (G5)| 784 Hz    |
| 50-60 cm      | La (A5) | 880 Hz    |
| 60-70 cm      | Si (B5) | 988 Hz    |
| 70-80 cm      | Do (C6) | 1046 Hz   |

### Command Reference

#### Guided Mode Commands
Play along with pre-programmed songs. The LED shows which note to play next.

| Command | Action                          |
|---------|---------------------------------|
| `1`     | Mary Had a Little Lamb          |
| `2`     | Twinkle Twinkle Little Star     |
| `3`     | The Wheels on the Bus           |
| `4`     | Yankee Doodle                   |

#### Free Play & Recording Commands

| Command | Action                          |
|---------|---------------------------------|
| `0`     | Enter free play mode            |
| `R1`    | Record to slot 1                |
| `R2`    | Record to slot 2                |
| `R3`    | Record to slot 3                |
| `R4`    | Record to slot 4                |
| `S`     | Stop recording                  |

#### Playback Commands

| Command | Action                          |
|---------|---------------------------------|
| `P1`    | Play recording from slot 1      |
| `P2`    | Play recording from slot 2      |
| `P3`    | Play recording from slot 3      |
| `P4`    | Play recording from slot 4      |
| `PA`    | Play all slots (merged)         |
| `X`     | Stop playback                   |

#### Management Commands

| Command | Action                          |
|---------|---------------------------------|
| `L`     | List all recordings             |
| `C1`    | Clear slot 1                    |
| `C2`    | Clear slot 2                    |
| `C3`    | Clear slot 3                    |
| `C4`    | Clear slot 4                    |
| `CA`    | Clear all recordings            |

#### Overlap Mode Commands

When playing multiple recordings together, these modes control how overlapping notes are handled:

| Command | Mode          | Behavior                              |
|---------|---------------|---------------------------------------|
| `M1`    | Priority High | Play highest note when overlap        |
| `M2`    | Priority Low  | Play lowest note when overlap         |
| `M3`    | Alternate     | Rapidly switch between notes (50ms)   |
| `M4`    | Drop          | First note wins, skip others          |

### Example Workflows

#### Creating a Simple Recording
```
1. Type: 0          → Enter free play mode
2. Type: R1         → Start recording to slot 1
3. Play some notes  → Move hand to different distances
4. Type: S          → Stop recording
5. Type: P1         → Play back your recording
```

#### Creating a Multi-Track Composition
```
1. Type: R1         → Record bass line to slot 1
2. Play bass notes
3. Type: S          → Stop recording
4. Type: R2         → Record melody to slot 2
5. Play melody
6. Type: S          → Stop recording
7. Type: M1         → Set to "Priority High" mode
8. Type: PA         → Play both tracks merged!
```

#### Listing & Managing Recordings
```
Type: L             → View all recordings
Output:
  Slot 1: 15 notes, 4.5s
  Slot 2: 8 notes, 2.4s
  Slot 3: [Empty]
  Slot 4: [Empty]

Type: C2            → Clear slot 2
Type: L             → Verify it's cleared
```

## Technical Architecture

### Project Structure

```
PianoAir/
├── PianoAir.ino      # Main sketch (setup & loop)
├── config.h          # Hardware pins & constants
├── note_mapping.h    # Note frequencies & distance mapping
├── utils.h           # Sensor, LED, buzzer utilities
├── songs.h           # Pre-programmed song data
├── recording.h       # Recording system
├── playback.h        # Playback engine with merging
├── ui.h              # Serial command interface
└── README.md         # This file
```

### System Modes

The system operates in five distinct modes:

1. **MODE_MENU** - Idle, waiting for user input
2. **MODE_GUIDED** - Following a pre-programmed song
3. **MODE_FREE_PLAY** - Playing notes freely
4. **MODE_RECORDING** - Recording notes to a slot
5. **MODE_PLAYBACK** - Playing back recorded notes

### Memory Usage

- **Program Storage**: 14,024 bytes (43% of Arduino Uno's 32KB)
- **Dynamic Memory**: 1,832 bytes (89% of Arduino Uno's 2KB)
  - Recording slots: ~240 bytes (4 slots × 30 notes)
  - Playback timeline: ~720 bytes (120 events max)
  - Pre-programmed songs: ~200 bytes
  - State variables: ~672 bytes

### Recording System

Each recording slot stores:
- **Note index** (0-7): Which note was played
- **Duration** (0-255 units): How long the note lasted (100ms per unit)

Maximum capacity:
- **4 slots** total
- **30 notes** per slot
- **25.5 seconds** max duration per note
- **~75 seconds** total recording time (if distributed evenly)

### Multi-Track Playback Algorithm

1. **Collect Events**: Gather all note events from selected slots
2. **Sort by Time**: Sort events chronologically
3. **Resolve Overlaps**: Apply selected overlap strategy
4. **Play Timeline**: Execute merged timeline through buzzer

Since the Arduino has only one buzzer, true polyphony isn't possible. The overlap resolution strategies provide different artistic approaches to merging tracks.

## Configuration Options

Edit [config.h](config.h) to customize:

```cpp
// Recording capacity
#define NUM_RECORDING_SLOTS 4      // Number of recording slots
#define MAX_NOTES_PER_SLOT 30      // Notes per slot

// Timing
#define NOTE_DURATION_MS 500       // Free play note duration
#define NOTE_DEBOUNCE_MS 50        // Debounce time

// Overlap behavior
#define DEFAULT_OVERLAP_STRATEGY OVERLAP_PRIORITY_HIGH

// Debug output
#define ENABLE_DEBUG false         // Enable verbose logging
```

## Troubleshooting

### No sound from buzzer
- Check buzzer polarity (+ to Pin 2, - to GND)
- Verify buzzer is working (some buzzers need specific frequencies)
- Check Serial Monitor for debug output

### Sensor not detecting hand
- Ensure sensor is powered (VCC to 5V, GND to GND)
- Check TRIG connected to Pin 4, ECHO to Pin 3
- Sensor works best in range 2-80cm
- Avoid reflective or absorbent surfaces

### LEDs not lighting up
- Check resistors are 220Ω (Red-Red-Brown)
- Verify LED polarity (long leg = positive)
- Check pin connections (Pins 6-13)

### Recording not saving
- Memory might be full (check Serial output)
- Ensure you typed `S` to stop recording
- Maximum 30 notes per slot

### Playback sounds wrong
- Try different overlap modes (M1-M4)
- Verify recordings exist with `L` command
- Check that recordings aren't empty

## Advanced Customization

### Adding More Songs

Edit [songs.h](songs.h) to add new pre-programmed melodies:

```cpp
// Your new song
int melody_new_song[] = {
  LED_Do, LED_Re, LED_Mi, LED_Fa, LED_Sol
};
int length_new_song = sizeof(melody_new_song) / sizeof(melody_new_song[0]);
```

Then add to the `songs` array and update `NUM_SONGS`.

### Changing Note Frequencies

Edit [note_mapping.h](note_mapping.h) to use different frequencies or distance ranges:

```cpp
const int note_frequencies[NUM_NOTES] = {
  // Customize these frequencies
};

const DistanceRange distance_ranges[NUM_NOTES] = {
  // Adjust distance ranges
};
```

### Adding More Recording Slots

Edit [config.h](config.h):

```cpp
#define NUM_RECORDING_SLOTS 6  // Increase from 4 to 6
```

Note: Each slot uses ~60 bytes of RAM. Monitor memory usage!

## Compilation Stats

Compiled for **Arduino Uno**:
- ✓ No errors or warnings
- ✓ 14,024 bytes program storage (43% used)
- ✓ 1,832 bytes dynamic memory (89% used)
- ✓ Ready for upload

## License

This project is released under the MIT License. Feel free to modify and distribute!

## Credits

IOT project(CSC 473): Rashika, Noah, and Niva.
* Hardware: Arduino Uno + HC-SR04 Ultrasonic Sensor
* Features: Guided mode, Free play, Multi-track recording & playback
