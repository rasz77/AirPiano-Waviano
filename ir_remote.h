#ifndef IR_REMOTE_H
#define IR_REMOTE_H

#include <Arduino.h>
#include <IRremote.h>
#include "debug.h"  // Include debug system

// IR Configuration
#define IR_RECEIVE_PIN 11  // Digital pin for IR receiver data pin
                           // Connect IR receiver: GND->GND, VCC->5V, DATA->Pin 11

// IR Button Code Definitions
// These codes are mapped to your specific remote control
// Mapping based on physical button layout (left to right, top to bottom)

// Row 1: Power, VOL-, FUNC/STOP
#define IR_BTN_POWER       0x45  // Stop playback/recording (context aware)
#define IR_BTN_VOL_MINUS   0x15  // Reserved/unused
#define IR_BTN_FUNC_STOP   0x47  // Alternative stop button

// Row 2: <<, Play/Pause, >>
#define IR_BTN_PREV        0x44  // Cycle overlap mode backward
#define IR_BTN_PLAY_PAUSE  0x40  // Play all slots (PA)
#define IR_BTN_NEXT        0x43  // Cycle overlap mode forward

// Row 3: Down, VOL+, Up
#define IR_BTN_DOWN        0x07  // Stop recording (S)
#define IR_BTN_VOL_PLUS    0x46  // Reserved/unused
#define IR_BTN_UP          0x09  // Help menu (H)

// Row 4: 0, EQ, ST/REPT
#define IR_BTN_0           0x16  // Toggle free play mode
#define IR_BTN_EQ          0x19  // Clear last played slot
#define IR_BTN_ST_REPT     0x0D  // Clear all recordings (CA)

// Row 5: 1, 2, 3
#define IR_BTN_1           0x0C  // Play slot 1 (P1)
#define IR_BTN_2           0x18  // Play slot 2 (P2)
#define IR_BTN_3           0x5E  // Play slot 3 (P3)

// Row 6: 4, 5, 6
#define IR_BTN_4           0x08  // Reserved/unused (was P4)
#define IR_BTN_5           0x1C  // Record to slot 1 (R1)
#define IR_BTN_6           0x5A  // Record to slot 2 (R2)

// Row 7: 7, 8, 9
#define IR_BTN_7           0x42  // Record to slot 3 (R3)
#define IR_BTN_8           0x52  // Reserved/unused (was R4)
#define IR_BTN_9           0x4A  // List recordings (L)

// ============================================
// IR COMMAND QUEUE SYSTEM
// ============================================

// Command queue for decoupling IR decode from execution
#ifndef IR_COMMAND_QUEUE_SIZE
  #define IR_COMMAND_QUEUE_SIZE 4  // Reduced to save RAM (was 8)
#endif

struct IRCommand {
  uint16_t command;     // IR command code
  bool valid;           // Valid command flag
  unsigned long timestamp;  // When command was received
};

// IR command queue
IRCommand ir_command_queue[IR_COMMAND_QUEUE_SIZE];
int ir_queue_head = 0;  // Write position
int ir_queue_tail = 0;  // Read position

// IR processing statistics
// Note: IRStats structure is forward declared in debug.h to avoid circular dependency
// If debug.h is not included, we define it here
#ifndef DEBUG_H
struct IRStats {
  unsigned long total_decoded;
  unsigned long total_queued;
  unsigned long total_processed;
  unsigned long queue_overflows;
  unsigned long invalid_commands;
  unsigned long last_decode_time;
};
#endif

IRStats ir_stats = {0, 0, 0, 0, 0, 0};

// Track last played slot for "clear last" functionality
int last_played_slot = -1;

// Initialize IR receiver
void initIRReceiver() {
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  Serial.println(F("IR Receiver initialized on pin 11"));
}

// ============================================
// NEW: QUEUE-BASED IR PROCESSING
// ============================================

/**
 * Fast IR decode task - runs frequently (10ms)
 * Only decodes and queues commands, doesn't execute them
 */
void processIRTask() {
  if (IrReceiver.decode()) {
    // Extract data immediately
    // The command field contains our button codes!
    uint16_t command = IrReceiver.decodedIRData.command;
    uint8_t flags = IrReceiver.decodedIRData.flags;
    uint8_t protocol = IrReceiver.decodedIRData.protocol;

    // Resume IMMEDIATELY to prepare for next signal
    IrReceiver.resume();

    // Debug: Log the raw decode
    debugIRDecode(command, flags, protocol);

    // Extended debug to troubleshoot 0x0 issue
    #if DEBUG_VERBOSE
    debugIRDecodeExtended(
      IrReceiver.decodedIRData.decodedRawData,
      IrReceiver.decodedIRData.address,
      IrReceiver.decodedIRData.command,
      flags
    );
    #endif

    ir_stats.total_decoded++;
    ir_stats.last_decode_time = millis();

    // Queue valid commands for processing
    if (flags == 0 && command != 0) {  // Non-repeat, valid command
      // Calculate next queue position
      int next_head = (ir_queue_head + 1) % IR_COMMAND_QUEUE_SIZE;
      int queue_depth = (ir_queue_head - ir_queue_tail + IR_COMMAND_QUEUE_SIZE) % IR_COMMAND_QUEUE_SIZE;

      if (next_head != ir_queue_tail) {
        // Queue has space
        ir_command_queue[ir_queue_head].command = command;
        ir_command_queue[ir_queue_head].valid = true;
        ir_command_queue[ir_queue_head].timestamp = millis();

        // Debug: Log enqueue operation
        debugIRQueue("Enqueue", next_head, ir_queue_tail, queue_depth + 1, ir_queue_head);

        ir_queue_head = next_head;
        ir_stats.total_queued++;
      } else {
        // Queue overflow - drop oldest command
        debugPrint("QUEUE", "OVERFLOW - dropping oldest");
        debugPrintHex("QUEUE", "Dropped cmd", ir_command_queue[ir_queue_tail].command);

        ir_stats.queue_overflows++;
        ir_queue_tail = (ir_queue_tail + 1) % IR_COMMAND_QUEUE_SIZE;

        // Now add new command
        ir_command_queue[ir_queue_head].command = command;
        ir_command_queue[ir_queue_head].valid = true;
        ir_command_queue[ir_queue_head].timestamp = millis();
        ir_queue_head = next_head;
        ir_stats.total_queued++;
      }
    } else if (command == 0) {
      ir_stats.invalid_commands++;
      debugPrint("IR", "Invalid command (0x0)");
    }
  }
}

/**
 * Process queued IR commands - executes one command per call
 * Returns updated SystemMode
 */
SystemMode processQueuedIRCommands() {
  // Declare extern to access global variables
  extern SystemMode current_mode;
  extern OverlapStrategy overlap_strategy;

  // Check if queue is empty
  if (ir_queue_tail == ir_queue_head) {
    return current_mode;  // Nothing to process
  }

  // Get next command from queue
  IRCommand cmd = ir_command_queue[ir_queue_tail];
  int old_tail = ir_queue_tail;
  ir_queue_tail = (ir_queue_tail + 1) % IR_COMMAND_QUEUE_SIZE;

  // Debug: Log dequeue operation
  int queue_depth = (ir_queue_head - ir_queue_tail + IR_COMMAND_QUEUE_SIZE) % IR_COMMAND_QUEUE_SIZE;
  debugIRQueue("Dequeue", ir_queue_head, ir_queue_tail, queue_depth, old_tail);

  if (!cmd.valid) {
    debugPrint("QUEUE", "Invalid command in queue");
    return current_mode;
  }

  ir_stats.total_processed++;

  // Debug: Show command age and processing
  if (debug_enabled) {
    printTimestamp();
    Serial.print(F("[IR] Processing: 0x"));
    Serial.print(cmd.command, HEX);
    Serial.print(F(" (age: "));
    Serial.print(millis() - cmd.timestamp);
    Serial.println(F("ms)"));
  }

  // Map IR codes to commands (existing switch logic)
  char cmd_buffer[4] = "";
  bool valid_command = true;

  switch (cmd.command) {
    // Number keys for primary functions
    case IR_BTN_0:
      strcpy(cmd_buffer, "0");
      Serial.println(F("[IR:0]"));
      break;

    case IR_BTN_1:
      strcpy(cmd_buffer, "P1");
      last_played_slot = 1;
      Serial.println(F("[IR:P1]"));
      break;

    case IR_BTN_2:
      strcpy(cmd_buffer, "P2");
      last_played_slot = 2;
      Serial.println(F("[IR:P2]"));
      break;

    case IR_BTN_3:
      strcpy(cmd_buffer, "P3");
      last_played_slot = 3;
      Serial.println(F("[IR:P3]"));
      break;

    case IR_BTN_4:
      // Button 4 is now unused (was P4)
      valid_command = false;
      Serial.println(F("[IR:4]"));
      break;

    case IR_BTN_5:
      strcpy(cmd_buffer, "R1");
      Serial.println(F("[IR:R1]"));
      break;

    case IR_BTN_6:
      strcpy(cmd_buffer, "R2");
      Serial.println(F("[IR:R2]"));
      break;

    case IR_BTN_7:
      strcpy(cmd_buffer, "R3");
      Serial.println(F("[IR:R3]"));
      break;

    case IR_BTN_8:
      // Button 8 is now unused (was R4)
      valid_command = false;
      Serial.println(F("[IR:8]"));
      break;

    case IR_BTN_9:
      strcpy(cmd_buffer, "L");
      Serial.println(F("[IR:L]"));
      break;

    // Media control buttons
    case IR_BTN_PLAY_PAUSE:
      strcpy(cmd_buffer, "PA");
      Serial.println(F("[IR:PA]"));
      break;

    case IR_BTN_PREV:
      // Cycle overlap mode backward
      if (overlap_strategy == OVERLAP_PRIORITY_HIGH) {
        strcpy(cmd_buffer, "M4");
      } else {
        sprintf(cmd_buffer, "M%d", (int)overlap_strategy);
      }
      Serial.println(F("[IR:<<]"));
      break;

    case IR_BTN_NEXT:
      // Cycle overlap mode forward
      if (overlap_strategy == OVERLAP_DROP) {
        strcpy(cmd_buffer, "M1");
      } else {
        sprintf(cmd_buffer, "M%d", (int)overlap_strategy + 2);
      }
      Serial.println(F("[IR:>>]"));
      break;

    // Navigation buttons
    case IR_BTN_UP:
      strcpy(cmd_buffer, "H");
      Serial.println(F("[IR:UP]"));
      break;

    case IR_BTN_DOWN:
      strcpy(cmd_buffer, "S");
      Serial.println(F("[IR:DN]"));
      break;

    // Function buttons
    case IR_BTN_POWER:
    case IR_BTN_FUNC_STOP:
      // Context-aware stop
      if (current_mode == MODE_RECORDING) {
        strcpy(cmd_buffer, "S");
      } else if (current_mode == MODE_PLAYBACK) {
        strcpy(cmd_buffer, "X");
      } else {
        valid_command = false;
      }
      Serial.println(F("[IR:PWR]"));
      break;

    case IR_BTN_EQ:
      // Clear last played slot
      if (last_played_slot >= 1 && last_played_slot <= 3) {
        sprintf(cmd_buffer, "C%d", last_played_slot);
      } else {
        valid_command = false;
      }
      Serial.println(F("[IR:EQ]"));
      break;

    case IR_BTN_ST_REPT:
      strcpy(cmd_buffer, "CA");
      Serial.println(F("[IR:CA]"));
      break;

    case IR_BTN_VOL_PLUS:
      // Toggle debug mode
      #if DEBUG_VERBOSE
      toggleDebugMode();
      #endif
      valid_command = false;
      Serial.println(F("[IR:V+]"));
      break;

    case IR_BTN_VOL_MINUS:
      // Show status
      #if DEBUG_VERBOSE
      printSystemStatus();
      #endif
      valid_command = false;
      Serial.println(F("[IR:V-]"));
      break;

    default:
      // Unknown button
      Serial.print(F("[IR:?"));
      Serial.print(cmd.command, HEX);
      Serial.println(F("]"));
      valid_command = false;
      break;
  }

  // Execute the command if valid
  SystemMode new_mode = current_mode;
  if (valid_command && strlen(cmd_buffer) > 0) {
    // Debug: Log command execution
    debugIRExecute(cmd.command, cmd_buffer);

    new_mode = handleSerialCommand(cmd_buffer);

    // Debug: Log mode change if occurred
    if (new_mode != current_mode) {
      debugModeChange(current_mode, new_mode);
    }
  }

  return new_mode;
}

/**
 * Print IR statistics (for debugging)
 */
void printIRStats() {
  Serial.println(F("\n=== IR STATISTICS ==="));
  Serial.print(F("Total decoded: "));
  Serial.println(ir_stats.total_decoded);
  Serial.print(F("Total queued: "));
  Serial.println(ir_stats.total_queued);
  Serial.print(F("Total processed: "));
  Serial.println(ir_stats.total_processed);
  Serial.print(F("Queue overflows: "));
  Serial.println(ir_stats.queue_overflows);
  Serial.print(F("Invalid (0x0): "));
  Serial.println(ir_stats.invalid_commands);
  Serial.print(F("Last decode: "));
  Serial.print(millis() - ir_stats.last_decode_time);
  Serial.println(F("ms ago"));
  Serial.println(F("=====================\n"));
}

// LEGACY processIRInput() removed - using queue-based system instead

// Print IR button reference card
void printIRButtonReference() {
  Serial.println(F("\n========================================"));
  Serial.println(F("       IR REMOTE BUTTON REFERENCE"));
  Serial.println(F("========================================"));
  Serial.println(F("NUMBERS:"));
  Serial.println(F("  0     - Toggle free play mode"));
  Serial.println(F("  1-3   - Play slots (P1-P3)"));
  Serial.println(F("  5-7   - Record slots (R1-R3)"));
  Serial.println(F("  9     - List recordings"));
  Serial.println(F(""));
  Serial.println(F("MEDIA CONTROLS:"));
  Serial.println(F("  Play  - Play all slots"));
  Serial.println(F("  <<    - Previous overlap mode"));
  Serial.println(F("  >>    - Next overlap mode"));
  Serial.println(F(""));
  Serial.println(F("NAVIGATION:"));
  Serial.println(F("  Up    - Help menu"));
  Serial.println(F("  Down  - Stop recording"));
  Serial.println(F(""));
  Serial.println(F("SPECIAL:"));
  Serial.println(F("  Power - Stop (context aware)"));
  Serial.println(F("  EQ    - Clear last played slot"));
  Serial.println(F("  ST/R  - Clear all recordings"));
  Serial.println(F("========================================\n"));
}

#endif // IR_REMOTE_H
