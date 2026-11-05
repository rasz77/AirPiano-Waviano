#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// HARDWARE PIN CONFIGURATION
// ============================================

// Ultrasonic Sensor Pins
#define ECHO_PIN 3
#define TRIGGER_PIN 4

// Shift Register Pins (74HC595) for controlling 8 LEDs
#define SHIFT_DATA_PIN 5    // DS (Serial Data Input)
#define SHIFT_LATCH_PIN 6   // ST_CP (Storage Register Clock / Latch)
#define SHIFT_CLOCK_PIN 7   // SH_CP (Shift Register Clock)

// Buzzer Pins
#define BUZZER_PIN 2         // Buzzer 1: Used for playback of recorded songs (Timer2 via tone())
#define BUZZER_PIN_2 9       // Buzzer 2: Used for free play mode (Timer1 OC1A)

// Potentiometer Pins (Analog)
#define VOLUME_POT_PIN A0    // Volume control potentiometer
#define PITCH_POT_PIN A1     // Pitch adjustment potentiometer

// ============================================
// TIMING CONSTANTS
// ============================================

// Ultrasonic sensor trigger interval (ms)
#define ULTRASONIC_TRIGGER_DELAY 100

// Note duration for free play mode (ms)
#define NOTE_DURATION_MS 500

// Debounce time for note detection (ms)
#define NOTE_DEBOUNCE_MS 50

// ============================================
// SCHEDULER TIMING (NEW - for discrete-time simulation)
// ============================================

// Main loop cycle time (ms) - target 10ms for 100Hz operation
#define SCHEDULER_TICK_MS 10

// IR receiver polling interval (ms) - CRITICAL for reliability
// Must be < 50ms to catch NEC protocol signals
#define IR_POLL_INTERVAL_MS 10  // 100Hz polling rate

// Audio state machine update interval (ms)
// Higher frequency for smooth volume pulsing
#define AUDIO_UPDATE_INTERVAL_MS 5  // 200Hz for smooth PWM-like pulsing

// Serial input processing interval (ms)
#define SERIAL_POLL_INTERVAL_MS 10  // Process serial every cycle

// Maximum allowed loop duration before warning (ms)
#define MAX_LOOP_DURATION_MS 20  // Warn if loop takes > 20ms

// ============================================
// RECORDING CONFIGURATION
// ============================================

// Number of recording slots available
// REDUCED from 4 to 3 to save RAM (saves ~60 bytes)
#define NUM_RECORDING_SLOTS 3

// Maximum notes per recording slot
// Note: Each note takes ~2 bytes (note_index + duration)
// INCREASED from 15 to 30 for longer recordings (uses 120 bytes more RAM)
#define MAX_NOTES_PER_SLOT 30

// Duration unit for recording (ms)
// Durations stored as multiples of this value
#define DURATION_UNIT_MS 100

// Maximum duration per note in units (255 × 100ms = 25.5 seconds)
#define MAX_NOTE_DURATION_UNITS 255

// ============================================
// PLAYBACK CONFIGURATION
// ============================================

// Overlap resolution strategies
enum OverlapStrategy {
  OVERLAP_PRIORITY_HIGH = 0,  // Play highest note when overlap
  OVERLAP_PRIORITY_LOW = 1,   // Play lowest note when overlap
  OVERLAP_ALTERNATE = 2,      // Rapidly alternate between notes
  OVERLAP_DROP = 3            // First note takes priority, drop others
};

// Default overlap strategy
#define DEFAULT_OVERLAP_STRATEGY OVERLAP_PRIORITY_HIGH

// Alternate mode switching interval (ms)
#define ALTERNATE_SWITCH_INTERVAL_MS 50

// ============================================
// SYSTEM MODES
// ============================================

enum SystemMode {
  MODE_MENU = 0,        // Main menu / idle
  MODE_FREE_PLAY = 1,   // Playing notes freely
  MODE_RECORDING = 2,   // Recording in progress
  MODE_PLAYBACK = 3     // Playing back recording(s)
};

// ============================================
// POTENTIOMETER CONFIGURATION
// ============================================

// Pitch adjustment range (in semitones up/down from base note)
#define PITCH_SHIFT_SEMITONES 2

// Volume control range (0-100%)
#define MIN_VOLUME_PERCENT 0
#define MAX_VOLUME_PERCENT 100

// ============================================
// FEATURE FLAGS
// ============================================

// Enable EEPROM storage (future feature)
#define ENABLE_EEPROM false

// Enable debug output (legacy - kept for compatibility)
#define ENABLE_DEBUG false

// ============================================
// DEBUG CONFIGURATION (NEW)
// ============================================

// Debug verbose mode - set to true for comprehensive debugging
// false = Production mode (no debug output, zero overhead)
// true = Verbose mode (full diagnostics with timestamps)
#define DEBUG_VERBOSE false  // SET TO true TO ENABLE DEBUGGING!

// ============================================
// GLOBAL STATE VARIABLES
// ============================================

// Current system mode
extern SystemMode current_mode;

// Currently selected recording slot for recording
extern int current_recording_slot;

// Overlap strategy for multi-track playback
extern OverlapStrategy overlap_strategy;

#endif // CONFIG_H
