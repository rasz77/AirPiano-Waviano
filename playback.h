#ifndef PLAYBACK_H
#define PLAYBACK_H

#include <Arduino.h>
#include "config.h"
#include "note_mapping.h"
#include "recording.h"
#include "utils.h"

// ============================================
// PLAYBACK STATE
// ============================================

// Merged timeline for multi-track playback
struct TimelineEvent {
  unsigned long timestamp_ms;  // Time from start of playback
  uint8_t note_index;          // Note to play (0-7)
  uint16_t duration_ms;        // Duration of note

  TimelineEvent() : timestamp_ms(0), note_index(0), duration_ms(0) {}

  TimelineEvent(unsigned long time, uint8_t note, uint16_t dur)
    : timestamp_ms(time), note_index(note), duration_ms(dur) {}
};

// Maximum events in merged timeline
// This depends on how many slots we merge
#define MAX_TIMELINE_EVENTS (MAX_NOTES_PER_SLOT * NUM_RECORDING_SLOTS)

// Playback state
bool is_playing = false;
TimelineEvent timeline[MAX_TIMELINE_EVENTS];
int timeline_event_count = 0;
int current_timeline_index = 0;
unsigned long playback_start_time = 0;
unsigned long next_event_time = 0;

// Active slots for playback
bool playback_slots[NUM_RECORDING_SLOTS];

// ============================================
// TIMELINE BUILDING FUNCTIONS
// ============================================

// Forward declaration
void resolveOverlaps(OverlapStrategy strategy);

/**
 * Comparison function for sorting timeline events
 */
int compareTimelineEvents(const void* a, const void* b) {
  TimelineEvent* event_a = (TimelineEvent*)a;
  TimelineEvent* event_b = (TimelineEvent*)b;

  if (event_a->timestamp_ms < event_b->timestamp_ms) return -1;
  if (event_a->timestamp_ms > event_b->timestamp_ms) return 1;
  return 0;
}

/**
 * Build timeline from a single recording slot
 * @param slot_num Slot number
 * @return true if successful
 */
bool buildTimelineFromSlot(int slot_num) {
  RecordingSlot* slot = getRecordingSlot(slot_num);
  if (slot == NULL || !slot->is_active) {
    return false;
  }

  timeline_event_count = 0;
  unsigned long current_time = 0;

  for (int i = 0; i < slot->note_count && timeline_event_count < MAX_TIMELINE_EVENTS; i++) {
    NoteEvent* note = &slot->events[i];
    uint16_t duration_ms = note->duration_units * DURATION_UNIT_MS;

    timeline[timeline_event_count++] = TimelineEvent(current_time, note->note_index, duration_ms);
    current_time += duration_ms;
  }

  return true;
}

/**
 * Merge multiple recording slots into timeline
 * @param slots Array of slot numbers to merge
 * @param num_slots Number of slots in array
 * @param strategy Overlap resolution strategy
 * @return true if successful
 */
bool buildTimelineFromMultipleSlots(int* slots, int num_slots, OverlapStrategy strategy) {
  if (num_slots == 0 || slots == NULL) {
    return false;
  }

  // First, collect all events from all slots
  timeline_event_count = 0;

  for (int s = 0; s < num_slots; s++) {
    int slot_num = slots[s];
    RecordingSlot* slot = getRecordingSlot(slot_num);

    if (slot == NULL || !slot->is_active) {
      continue;  // Skip invalid or empty slots
    }

    unsigned long current_time = 0;

    for (int i = 0; i < slot->note_count && timeline_event_count < MAX_TIMELINE_EVENTS; i++) {
      NoteEvent* note = &slot->events[i];
      uint16_t duration_ms = note->duration_units * DURATION_UNIT_MS;

      timeline[timeline_event_count++] = TimelineEvent(current_time, note->note_index, duration_ms);
      current_time += duration_ms;
    }
  }

  if (timeline_event_count == 0) {
    return false;
  }

  // Sort events by timestamp
  qsort(timeline, timeline_event_count, sizeof(TimelineEvent), compareTimelineEvents);

  // Resolve overlaps based on strategy
  resolveOverlaps(strategy);

  return true;
}

/**
 * Resolve overlapping notes in timeline based on strategy
 * @param strategy Overlap resolution strategy
 */
void resolveOverlaps(OverlapStrategy strategy) {
  if (timeline_event_count <= 1) {
    return;  // No overlaps possible
  }

  // Create a new timeline with overlaps resolved
  TimelineEvent resolved[MAX_TIMELINE_EVENTS];
  int resolved_count = 0;

  for (int i = 0; i < timeline_event_count; i++) {
    TimelineEvent current = timeline[i];
    unsigned long current_end = current.timestamp_ms + current.duration_ms;

    // Check for overlaps with next events
    bool has_overlap = false;

    for (int j = i + 1; j < timeline_event_count; j++) {
      TimelineEvent next = timeline[j];

      // If next event starts before current ends, we have an overlap
      if (next.timestamp_ms < current_end) {
        has_overlap = true;

        // Apply overlap strategy
        switch (strategy) {
          case OVERLAP_PRIORITY_HIGH:
            // Keep the higher note
            if (next.note_index > current.note_index) {
              current = next;
              current_end = current.timestamp_ms + current.duration_ms;
            }
            break;

          case OVERLAP_PRIORITY_LOW:
            // Keep the lower note
            if (next.note_index < current.note_index) {
              current = next;
              current_end = current.timestamp_ms + current.duration_ms;
            }
            break;

          case OVERLAP_DROP:
            // Keep first note, skip others
            // (current is already the first note)
            break;

          case OVERLAP_ALTERNATE:
            // For alternate mode, we'll split the duration
            // This creates a rapid alternation effect
            // Add current note with shortened duration
            if (resolved_count < MAX_TIMELINE_EVENTS) {
              TimelineEvent split_current = current;
              split_current.duration_ms = ALTERNATE_SWITCH_INTERVAL_MS;
              resolved[resolved_count++] = split_current;
            }

            // Add next note starting after the switch interval
            if (resolved_count < MAX_TIMELINE_EVENTS) {
              TimelineEvent split_next = next;
              split_next.timestamp_ms = current.timestamp_ms + ALTERNATE_SWITCH_INTERVAL_MS;
              split_next.duration_ms = ALTERNATE_SWITCH_INTERVAL_MS;
              resolved[resolved_count++] = split_next;

              // Update current end time
              current.timestamp_ms = split_next.timestamp_ms + ALTERNATE_SWITCH_INTERVAL_MS;
              current_end = current.timestamp_ms;
            }
            break;
        }
      } else {
        break;  // No more overlaps for this event
      }
    }

    // Add the resolved event (unless we used ALTERNATE mode, which already added events)
    if (strategy != OVERLAP_ALTERNATE && resolved_count < MAX_TIMELINE_EVENTS) {
      resolved[resolved_count++] = current;
    }
  }

  // Copy resolved timeline back
  if (strategy != OVERLAP_ALTERNATE) {
    for (int i = 0; i < resolved_count; i++) {
      timeline[i] = resolved[i];
    }
    timeline_event_count = resolved_count;
  }
}

// ============================================
// PLAYBACK CONTROL FUNCTIONS
// ============================================

/**
 * Start playback of a single slot
 * @param slot_num Slot number
 * @return true if playback started
 */
bool playSingleSlot(int slot_num) {
  if (is_playing) {
    return false;  // Already playing
  }

  if (!buildTimelineFromSlot(slot_num)) {
    return false;  // Failed to build timeline
  }

  is_playing = true;
  current_timeline_index = 0;
  playback_start_time = millis();
  next_event_time = playback_start_time;

  // Mark only this slot as active for playback
  for (int i = 0; i < NUM_RECORDING_SLOTS; i++) {
    playback_slots[i] = (i == slot_num);
  }

  return true;
}

/**
 * Start playback of multiple slots (merged)
 * @param slots Array of slot numbers
 * @param num_slots Number of slots
 * @param strategy Overlap resolution strategy
 * @return true if playback started
 */
bool playMultipleSlots(int* slots, int num_slots, OverlapStrategy strategy) {
  if (is_playing) {
    return false;  // Already playing
  }

  if (!buildTimelineFromMultipleSlots(slots, num_slots, strategy)) {
    return false;  // Failed to build timeline
  }

  is_playing = true;
  current_timeline_index = 0;
  playback_start_time = millis();
  next_event_time = playback_start_time;

  // Mark selected slots as active for playback
  for (int i = 0; i < NUM_RECORDING_SLOTS; i++) {
    playback_slots[i] = false;
  }
  for (int i = 0; i < num_slots; i++) {
    if (slots[i] >= 0 && slots[i] < NUM_RECORDING_SLOTS) {
      playback_slots[slots[i]] = true;
    }
  }

  return true;
}

/**
 * Start playback of all active slots
 * @param strategy Overlap resolution strategy
 * @return true if playback started
 */
bool playAllSlots(OverlapStrategy strategy) {
  int active_slots[NUM_RECORDING_SLOTS];
  int num_active = 0;

  for (int i = 0; i < NUM_RECORDING_SLOTS; i++) {
    if (isSlotActive(i)) {
      active_slots[num_active++] = i;
    }
  }

  if (num_active == 0) {
    return false;  // No active slots
  }

  return playMultipleSlots(active_slots, num_active, strategy);
}

/**
 * Stop playback
 */
void stopPlayback() {
  is_playing = false;
  stopNote();  // Uses BUZZER_PIN (buzzer 1) by default for playback
  turnOffAllLEDs();
  current_timeline_index = 0;
}

// Track the last event start time for volume pulsing
unsigned long last_note_start_time = 0;
int current_playing_note = -1;

/**
 * Update playback (call in main loop)
 * @return true if still playing, false if finished
 */
bool updatePlayback() {
  if (!is_playing) {
    return false;
  }

  unsigned long current_time = millis();
  unsigned long elapsed = current_time - playback_start_time;

  // Get current volume setting
  int volume = getVolumePercent();

  // Check if current event should end
  if (current_time >= next_event_time && current_timeline_index < timeline_event_count) {
    stopNote();  // Stop previous note on buzzer 1 (playback)
    current_playing_note = -1;

    // Get next event
    TimelineEvent* event = &timeline[current_timeline_index];

    // Wait until it's time for this event
    if (elapsed >= event->timestamp_ms) {
      // Play the note (if not muted) on buzzer 1 (playback)
      if (volume > 0) {
        playNote(event->note_index);  // Uses BUZZER_PIN (buzzer 1) by default
        current_playing_note = event->note_index;
        last_note_start_time = current_time;
      }
      setNoteLED(event->note_index);

      // Calculate when this note should end
      next_event_time = playback_start_time + event->timestamp_ms + event->duration_ms;

      current_timeline_index++;
    }
  }

  // Apply volume pulsing effect if a note is currently playing
  if (current_playing_note != -1 && volume < 100 && volume > 0) {
    int pulse_on_ms = map(volume, 0, 100, 5, 50);
    int pulse_off_ms = map(volume, 0, 100, 45, 0);

    unsigned long note_elapsed = current_time - last_note_start_time;
    unsigned long pulse_cycle = pulse_on_ms + pulse_off_ms;

    if (pulse_cycle > 0) {
      unsigned long position_in_cycle = note_elapsed % pulse_cycle;

      if (position_in_cycle < pulse_on_ms) {
        // Should be playing on buzzer 1 (playback)
        if (digitalRead(BUZZER_PIN) == LOW) {
          playNote(current_playing_note);  // Uses BUZZER_PIN (buzzer 1) by default
        }
      } else {
        // Should be silent
        stopNote();  // Uses BUZZER_PIN (buzzer 1) by default
      }
    }
  }

  // Check if playback is finished
  if (current_timeline_index >= timeline_event_count && current_time >= next_event_time) {
    stopPlayback();
    return false;
  }

  return true;
}

/**
 * Check if playback is active
 * @return true if playing
 */
bool isPlaying() {
  return is_playing;
}

/**
 * Get playback progress
 * @param out_current Output: current event index
 * @param out_total Output: total events
 * @return true if playing
 */
bool getPlaybackProgress(int* out_current, int* out_total) {
  if (!is_playing) {
    return false;
  }

  *out_current = current_timeline_index;
  *out_total = timeline_event_count;
  return true;
}

#endif // PLAYBACK_H
