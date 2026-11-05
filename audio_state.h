#ifndef AUDIO_STATE_H
#define AUDIO_STATE_H

#include <Arduino.h>
#include "config.h"
#include "note_mapping.h"
#include "utils.h"
#include "debug.h"  // Include debug system

// ============================================
// NON-BLOCKING AUDIO STATE MACHINE
// ============================================

// Audio state machine states
enum AudioState {
  AUDIO_IDLE,           // No audio playing
  AUDIO_PLAYING,        // Playing continuous tone
  AUDIO_PULSING_ON,     // Volume pulsing - tone ON phase
  AUDIO_PULSING_OFF     // Volume pulsing - tone OFF phase
};

// Audio state machine data
struct AudioStateMachine {
  AudioState state;
  int current_note;
  unsigned long start_time;
  unsigned long duration_ms;
  unsigned long pulse_start_time;
  int pulse_on_ms;
  int pulse_off_ms;
  int volume_percent;
  bool led_on;
};

// Global audio state machine instance
AudioStateMachine audio_sm = {AUDIO_IDLE, -1, 0, 0, 0, 0, 0, 100, false};

/**
 * Request to play a note (non-blocking)
 * This replaces the blocking playNoteWithDuration() function
 * @param note_index Note to play (0-7)
 * @param duration_ms Duration in milliseconds
 */
void requestPlayNote(int note_index, unsigned int duration_ms) {
  // If already playing, stop current note
  if (audio_sm.state != AUDIO_IDLE) {
    debugAudioState(audio_sm.state == AUDIO_PLAYING ? "PLAYING" :
                    audio_sm.state == AUDIO_PULSING_ON ? "PULSING_ON" :
                    audio_sm.state == AUDIO_PULSING_OFF ? "PULSING_OFF" : "UNKNOWN",
                    "IDLE");
    stopNote(BUZZER_PIN_2);  // Free play uses buzzer 2
    if (audio_sm.led_on) {
      turnOffAllLEDs();
      audio_sm.led_on = false;
    }
  }

  // Initialize state machine for new note
  audio_sm.current_note = note_index;
  audio_sm.start_time = millis();
  audio_sm.duration_ms = duration_ms;
  audio_sm.volume_percent = getVolumePercent();

  // Debug: Log note request
  debugAudioRequest(note_index, duration_ms, audio_sm.volume_percent);

  // Calculate pulse parameters based on volume
  if (audio_sm.volume_percent < 100) {
    // Volume control via PWM-like pulsing
    audio_sm.pulse_on_ms = map(audio_sm.volume_percent, 0, 100, 5, 50);
    audio_sm.pulse_off_ms = map(audio_sm.volume_percent, 0, 100, 45, 0);
    audio_sm.pulse_start_time = audio_sm.start_time;

    // Debug: Log pulse parameters
    debugAudioPulse(audio_sm.pulse_on_ms, audio_sm.pulse_off_ms);
    debugAudioState("IDLE", "PULSING_ON");

    audio_sm.state = AUDIO_PULSING_ON;
  } else {
    // Full volume - continuous tone
    debugAudioState("IDLE", "PLAYING");
    audio_sm.state = AUDIO_PLAYING;
  }

  // Start playing the note on buzzer 2 (free play)
  playNote(note_index, BUZZER_PIN_2);
  setNoteLED(note_index);
  audio_sm.led_on = true;
}

/**
 * Update the audio state machine (called every cycle)
 * Must be called frequently (5-10ms) for smooth operation
 */
void updateAudioStateMachine() {
  if (audio_sm.state == AUDIO_IDLE) {
    return;  // Nothing to do
  }

  unsigned long now = millis();
  unsigned long elapsed = now - audio_sm.start_time;

  // Check if note duration has elapsed
  if (elapsed >= audio_sm.duration_ms) {
    // Stop playing and return to idle
    const char* old_state = audio_sm.state == AUDIO_PLAYING ? "PLAYING" :
                           audio_sm.state == AUDIO_PULSING_ON ? "PULSING_ON" :
                           audio_sm.state == AUDIO_PULSING_OFF ? "PULSING_OFF" : "UNKNOWN";
    debugAudioState(old_state, "IDLE");

    stopNote(BUZZER_PIN_2);  // Free play uses buzzer 2
    if (audio_sm.led_on) {
      turnOffAllLEDs();
      audio_sm.led_on = false;
    }
    audio_sm.state = AUDIO_IDLE;
    return;
  }

  // Handle state-specific updates
  switch (audio_sm.state) {
    case AUDIO_PLAYING:
      // Continuous tone - nothing to update
      break;

    case AUDIO_PULSING_ON:
      // Check if ON phase duration elapsed
      if (now - audio_sm.pulse_start_time >= audio_sm.pulse_on_ms) {
        // Transition to OFF phase
        stopNote(BUZZER_PIN_2);  // Free play uses buzzer 2
        audio_sm.pulse_start_time = now;
        audio_sm.state = AUDIO_PULSING_OFF;
      }
      break;

    case AUDIO_PULSING_OFF:
      // Check if OFF phase duration elapsed
      if (audio_sm.pulse_off_ms == 0 ||
          now - audio_sm.pulse_start_time >= audio_sm.pulse_off_ms) {
        // Transition back to ON phase
        playNote(audio_sm.current_note, BUZZER_PIN_2);  // Free play uses buzzer 2
        audio_sm.pulse_start_time = now;
        audio_sm.state = AUDIO_PULSING_ON;
      }
      break;

    default:
      break;
  }
}

/**
 * Stop the audio state machine immediately
 */
void stopAudioStateMachine() {
  if (audio_sm.state != AUDIO_IDLE) {
    stopNote(BUZZER_PIN_2);  // Free play uses buzzer 2
    if (audio_sm.led_on) {
      turnOffAllLEDs();
      audio_sm.led_on = false;
    }
    audio_sm.state = AUDIO_IDLE;
  }
}

/**
 * Check if audio is currently playing
 * @return true if audio is active, false if idle
 */
bool isAudioPlaying() {
  return audio_sm.state != AUDIO_IDLE;
}

/**
 * Get remaining duration of current note
 * @return Remaining milliseconds, or 0 if not playing
 */
unsigned long getAudioRemainingMs() {
  if (audio_sm.state == AUDIO_IDLE) {
    return 0;
  }

  unsigned long elapsed = millis() - audio_sm.start_time;
  if (elapsed >= audio_sm.duration_ms) {
    return 0;
  }

  return audio_sm.duration_ms - elapsed;
}

#endif // AUDIO_STATE_H
