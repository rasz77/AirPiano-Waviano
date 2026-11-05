#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include "config.h"
#include "note_mapping.h"

// ============================================
// ULTRASONIC SENSOR STATE
// ============================================

// Timing variables for ultrasonic sensor
unsigned long last_time_ultrasonic_trigger = 0;

// Pulse timing for distance calculation
volatile unsigned long pulse_in_begin = 0;
volatile unsigned long pulse_in_end = 0;
volatile bool new_distance_available = false;

// ============================================
// ULTRASONIC SENSOR FUNCTIONS
// ============================================

/**
 * Trigger the ultrasonic sensor to send a pulse
 */
void triggerUltrasonicSensor() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
}

/**
 * Interrupt handler for echo pin
 * Measures pulse duration for distance calculation
 */
void echo_pin_interrupt() {
  if (digitalRead(ECHO_PIN) == HIGH) {
    pulse_in_begin = micros();
  } else {
    pulse_in_end = micros();
    new_distance_available = true;
  }
}

/**
 * Calculate distance from ultrasonic pulse timing
 * @return Distance in centimeters
 */
float getDistance() {
  unsigned long duration_micros = pulse_in_end - pulse_in_begin;
  return duration_micros / 58.0;  // Convert to cm
}

/**
 * Check if new distance measurement is available
 * @return true if new measurement ready
 */
bool isNewDistanceAvailable() {
  return new_distance_available;
}

/**
 * Clear the new distance flag
 */
void clearDistanceFlag() {
  new_distance_available = false;
}

/**
 * Update ultrasonic sensor (call in main loop)
 * Triggers sensor at regular intervals
 */
void updateUltrasonicSensor() {
  unsigned long time_now = millis();
  if (time_now - last_time_ultrasonic_trigger > ULTRASONIC_TRIGGER_DELAY) {
    last_time_ultrasonic_trigger = time_now;
    triggerUltrasonicSensor();
  }
}

// ============================================
// LED CONTROL FUNCTIONS (Shift Register)
// ============================================

// Current LED state (bit pattern for 8 LEDs)
// Bit 0 = Do, Bit 1 = Re, ..., Bit 7 = Do (High)
byte current_led_state = 0;

/**
 * Update shift register with current LED pattern
 * @param pattern 8-bit pattern where each bit controls one LED
 */
void updateShiftRegister(byte pattern) {
  // Pull latch low to begin data transfer
  digitalWrite(SHIFT_LATCH_PIN, LOW);

  // Shift out the data (MSB first)
  shiftOut(SHIFT_DATA_PIN, SHIFT_CLOCK_PIN, MSBFIRST, pattern);

  // Pull latch high to update outputs
  digitalWrite(SHIFT_LATCH_PIN, HIGH);

  // Update current state
  current_led_state = pattern;
}

/**
 * Turn off all LEDs
 */
void turnOffAllLEDs() {
  updateShiftRegister(0x00);
}

/**
 * Light up LED for specific note
 * @param note_index Note index (0-7)
 */
void lightUpNoteLED(int note_index) {
  if (note_index >= 0 && note_index < 8) {
    // Set the bit for this note
    byte new_state = current_led_state | (1 << note_index);
    updateShiftRegister(new_state);
  }
}

/**
 * Light up LED for specific note (turn off others first)
 * @param note_index Note index (0-7)
 */
void setNoteLED(int note_index) {
  if (note_index >= 0 && note_index < 8) {
    // Only this LED should be on
    updateShiftRegister(1 << note_index);
  }
}

/**
 * Turn off LED for specific note
 * @param note_index Note index (0-7)
 */
void turnOffNoteLED(int note_index) {
  if (note_index >= 0 && note_index < 8) {
    // Clear the bit for this note
    byte new_state = current_led_state & ~(1 << note_index);
    updateShiftRegister(new_state);
  }
}

// ============================================
// POTENTIOMETER FUNCTIONS
// ============================================

/**
 * Read volume potentiometer and return volume percentage (0-100)
 * @return Volume percentage (0-100)
 */
int getVolumePercent() {
  int pot_value = analogRead(VOLUME_POT_PIN);  // 0-1023
  return map(pot_value, 0, 1023, MIN_VOLUME_PERCENT, MAX_VOLUME_PERCENT);
}

/**
 * Read pitch potentiometer and return pitch shift multiplier
 * @return Pitch multiplier (e.g., 1.0 = no shift, 1.122 = +2 semitones, 0.891 = -2 semitones)
 */
float getPitchMultiplier() {
  int pot_value = analogRead(PITCH_POT_PIN);  // 0-1023

  // Map to range: -PITCH_SHIFT_SEMITONES to +PITCH_SHIFT_SEMITONES
  float semitones = map(pot_value, 0, 1023, -PITCH_SHIFT_SEMITONES * 100, PITCH_SHIFT_SEMITONES * 100) / 100.0;

  // Convert semitones to frequency multiplier
  // Formula: multiplier = 2^(semitones/12)
  return pow(2.0, semitones / 12.0);
}

// ============================================
// TIMER1 TONE GENERATION (Buzzer 2)
// ============================================
// Timer1 is used for buzzer 2 to enable true simultaneous playback
// with Timer2 (used by standard tone() function for buzzer 1)

// Forward declaration
void stopToneTimer1();

/**
 * Initialize Timer1 for tone generation on pin 9 (OC1A)
 * Must be called in setup()
 */
void setupTimer1() {
  // Set pin 9 as output
  pinMode(BUZZER_PIN_2, OUTPUT);

  // Stop Timer1
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // Configure Timer1 for CTC mode (Clear Timer on Compare Match)
  // WGM13:0 = 0100 (CTC mode, TOP = OCR1A)
  TCCR1B |= (1 << WGM12);

  // Toggle OC1A (pin 9) on compare match
  // COM1A1:0 = 01 (toggle OC1A on compare match)
  TCCR1A |= (1 << COM1A0);
}

/**
 * Play a tone on Timer1 (buzzer 2)
 * @param frequency Frequency in Hz
 */
void playToneTimer1(uint16_t frequency) {
  if (frequency == 0) {
    stopToneTimer1();
    return;
  }

  // Calculate OCR1A value for desired frequency
  // Formula: frequency = F_CPU / (2 * prescaler * (OCR1A + 1))
  // Rearranged: OCR1A = (F_CPU / (2 * prescaler * frequency)) - 1

  // Choose prescaler based on frequency range
  // For musical notes (500-2000 Hz), prescaler = 8 works well
  uint8_t prescaler_bits;
  uint16_t prescaler_value;

  if (frequency >= 2000) {
    // High frequency: prescaler = 1
    prescaler_bits = (1 << CS10);
    prescaler_value = 1;
  } else if (frequency >= 250) {
    // Medium frequency: prescaler = 8
    prescaler_bits = (1 << CS11);
    prescaler_value = 8;
  } else {
    // Low frequency: prescaler = 64
    prescaler_bits = (1 << CS11) | (1 << CS10);
    prescaler_value = 64;
  }

  // Calculate compare match value
  uint32_t ocr_value = (F_CPU / (2UL * prescaler_value * (uint32_t)frequency)) - 1;

  // Limit to 16-bit range
  if (ocr_value > 65535) ocr_value = 65535;

  // Set compare match value
  OCR1A = (uint16_t)ocr_value;

  // Start timer with selected prescaler
  TCCR1B = (TCCR1B & 0xF8) | prescaler_bits | (1 << WGM12);
}

/**
 * Stop Timer1 tone
 */
void stopToneTimer1() {
  // Stop timer (no clock source)
  TCCR1B &= 0xF8;

  // Set pin low
  digitalWrite(BUZZER_PIN_2, LOW);
}

// ============================================
// BUZZER CONTROL FUNCTIONS
// ============================================

/**
 * Play a note on the buzzer with pitch adjustment
 * @param note_index Note index (0-7)
 * @param buzzer_pin Pin number for the buzzer (defaults to BUZZER_PIN for playback)
 */
void playNote(int note_index, int buzzer_pin = BUZZER_PIN) {
  int base_frequency = getNoteFrequency(note_index);
  if (base_frequency > 0) {
    // Apply pitch shift from potentiometer
    float pitch_multiplier = getPitchMultiplier();
    int adjusted_frequency = (int)(base_frequency * pitch_multiplier);

    // Route to correct timer based on pin
    if (buzzer_pin == BUZZER_PIN_2) {
      // Use Timer1 for buzzer 2
      playToneTimer1(adjusted_frequency);
    } else {
      // Use Timer2 (standard tone()) for buzzer 1
      tone(buzzer_pin, adjusted_frequency);
    }
  }
}

/**
 * Stop playing note on buzzer
 * @param buzzer_pin Pin number for the buzzer (defaults to BUZZER_PIN for playback)
 */
void stopNote(int buzzer_pin = BUZZER_PIN) {
  // Route to correct timer based on pin
  if (buzzer_pin == BUZZER_PIN_2) {
    // Stop Timer1 for buzzer 2
    stopToneTimer1();
  } else {
    // Stop Timer2 (standard noTone()) for buzzer 1
    noTone(buzzer_pin);
  }
}

/**
 * LEGACY: Play a note for a specific duration with volume control
 * WARNING: This function is BLOCKING and should be avoided!
 * Use requestPlayNote() from audio_state.h instead for non-blocking operation
 *
 * @param note_index Note index (0-7)
 * @param duration_ms Duration in milliseconds
 * @deprecated Use requestPlayNote() for non-blocking audio
 */
void playNoteWithDuration(int note_index, unsigned int duration_ms) {
  // CRITICAL FIX: Redirect to non-blocking audio system
  // This prevents IR remote lockup issues caused by delay()

  // Forward declaration of non-blocking function from audio_state.h
  extern void requestPlayNote(int note_index, unsigned int duration_ms);

  // Use non-blocking audio instead of blocking delays
  requestPlayNote(note_index, duration_ms);

  // Note: The audio will play in the background while main loop continues
  // This eliminates the IR signal processing failures!
}

/**
 * BLOCKING VERSION - Only use if absolutely necessary
 * This is the original blocking implementation
 */
void playNoteWithDurationBlocking(int note_index, unsigned int duration_ms) {
  int volume = getVolumePercent();

  if (volume == 0) {
    // Muted - just show LED
    setNoteLED(note_index);
    delay(duration_ms);
    return;
  }

  // For volume control with passive buzzer, we use pulsing
  // Higher volume = longer pulse, lower volume = shorter pulse
  playNote(note_index);
  setNoteLED(note_index);

  if (volume < 100) {
    // Pulse the tone for volume effect
    unsigned long start_time = millis();
    int pulse_on_ms = map(volume, 0, 100, 5, 50);   // On time
    int pulse_off_ms = map(volume, 0, 100, 45, 0);  // Off time

    while (millis() - start_time < duration_ms) {
      playNote(note_index);
      delay(pulse_on_ms);

      if (pulse_off_ms > 0) {
        stopNote();
        delay(pulse_off_ms);
      }
    }
    stopNote();
  } else {
    // Full volume - continuous tone
    delay(duration_ms);
    stopNote();
  }
}

// ============================================
// INITIALIZATION FUNCTIONS
// ============================================

/**
 * Initialize all hardware pins
 */
void initializeHardware() {
  // Initialize ultrasonic sensor pins
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIGGER_PIN, OUTPUT);

  // Initialize shift register pins
  pinMode(SHIFT_DATA_PIN, OUTPUT);
  pinMode(SHIFT_LATCH_PIN, OUTPUT);
  pinMode(SHIFT_CLOCK_PIN, OUTPUT);

  // Initialize buzzer pins
  pinMode(BUZZER_PIN, OUTPUT);    // Buzzer 1: Playback
  pinMode(BUZZER_PIN_2, OUTPUT);  // Buzzer 2: Free play

  // Turn off all LEDs initially
  turnOffAllLEDs();

  // Attach interrupt for echo pin
  attachInterrupt(digitalPinToInterrupt(ECHO_PIN), echo_pin_interrupt, CHANGE);
}

#endif // UTILS_H
