/**
 * Generative Generator - Basic Hardware Test
 *
 * Hardware: Daisy Patch (full module with OLED)
 * - 4 Potentiometers (CV inputs)
 * - 1 Encoder (navigation + click)
 * - OLED Display (128x64)
 * - 2 Gate Inputs
 * - 1 Gate Output
 * - 4 Audio I/O
 */

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>

using namespace daisy;
using namespace daisysp;

// Hardware
DaisyPatch hw;

// Page system
int   current_page = 0;      // 0, 1, 2, 3 for 4 pages
const int NUM_PAGES = 4;
const int PARAMS_PER_PAGE = 4;
const int TOTAL_PARAMS = 16;

// Parameter indices (for clarity)
enum ParamIndex {
    // Page 0: Performance - Direct Control
    PARAM_MOTION = 0,
    PARAM_MEMORY = 1,
    PARAM_REGISTER = 2,
    PARAM_DIRECTION = 3,
    // Page 1: Performance - Macro & Evolution
    PARAM_PHRASE = 4,
    PARAM_ENERGY = 5,
    PARAM_STABILITY = 6,
    PARAM_FORGETFULNESS = 7,
    // Page 2: Structural - Shape & Gravity
    PARAM_LEAP_SHAPE = 8,
    PARAM_DIRECTION_MEMORY = 9,
    PARAM_HOME_REGISTER = 10,
    PARAM_RANGE_WIDTH = 11,
    // Page 3: Utility - Learning & I/O
    PARAM_LEARN_TIMEOUT = 12,
    PARAM_ECHO_NOTES = 13,
    PARAM_RESERVED_1 = 14,
    PARAM_RESERVED_2 = 15
};

// Parameter names for each page (4 params per page)
const char* page_names[NUM_PAGES][PARAMS_PER_PAGE] = {
    // Page 0: Performance - Direct Control
    {"MOTION", "MEMORY", "REGISTER", "DIRECTION"},
    // Page 1: Performance - Macro & Evolution
    {"PHRASE", "ENERGY", "STABILITY", "FORGET"},
    // Page 2: Structural - Shape & Gravity
    {"LEAP SHP", "DIR MEM", "HOME REG", "RANGE"},
    // Page 3: Utility - Learning & I/O
    {"LRN TIME", "ECHO", "---", "---"}
};

// MIDI CC mapping (CC number to parameter index)
// Using undefined CCs to avoid conflicts with standard MIDI controllers
const uint8_t MIDI_CC_COUNT = 16;
const uint8_t midi_cc_numbers[MIDI_CC_COUNT] = {
    3,  // MOTION (Page 0, Param 0)
    9,  // MEMORY (Page 0, Param 1)
    14, // REGISTER (Page 0, Param 2)
    15, // DIRECTION (Page 0, Param 3)
    20, // PHRASE (Page 1, Param 0)
    21, // ENERGY (Page 1, Param 1)
    22, // STABILITY (Page 1, Param 2)
    23, // FORGETFULNESS (Page 1, Param 3)
    24, // LEAP SHAPE (Page 2, Param 0)
    25, // DIRECTION MEMORY (Page 2, Param 1)
    26, // HOME REGISTER (Page 2, Param 2)
    27, // RANGE WIDTH (Page 2, Param 3)
    28, // LEARN TIMEOUT (Page 3, Param 0)
    29, // ECHO NOTES (Page 3, Param 1)
    30, // RESERVED (Page 3, Param 2)
    31  // RESERVED (Page 3, Param 3)
};

// Parameter storage (all 12 parameters, 0.0 to 1.0)
float parameters[TOTAL_PARAMS];
float parameters_smoothed[TOTAL_PARAMS];  // Smoothed versions for display/use

// Soft takeover (parameter pickup)
bool  param_pickup_active[TOTAL_PARAMS];  // True when pot has "caught" the stored value
float pot_last_value[TOTAL_PARAMS];       // Last pot value for each parameter
const float PICKUP_THRESHOLD = 0.05f;     // How close pot must be to pickup (5%)

// Current pot readings (4 pots)
float pot_values[4];

// Smoothing coefficient (0.0 = no change, 1.0 = instant change)
// Lower values = more smoothing, better for slow gestures
// Higher values = less smoothing, better for fast tweaking
const float SMOOTHING_COEFF = 0.15f;

// Gate inputs
// Gate Input 1: Note trigger (generates new note when HIGH during GENERATING)
bool  gate1_state = false;
bool  gate1_prev = false;
bool  note_triggered = false;  // True for one frame after rising edge

// Gate Input 2: Clock/BPM detection
bool  gate2_state = false;
bool  gate2_prev = false;
uint32_t last_clock_time = 0;
uint32_t clock_interval = 0;    // Time between clock pulses (in ms)
float clock_bpm = 120.0f;        // Estimated tempo
int   clock_pulse_indicator = 0; // Visual pulse countdown (frames)

// Gate output state
bool  gate_out_state = false;    // Current gate output state
uint32_t gate_out_start_time = 0; // When gate went high (ms)
float gate_length_ms = 50.0f;    // Gate length in milliseconds

// Other state
int   frame_counter = 0;
int   page_change_timer = 0;  // For showing page name overlay

// ============================================================================
// DEBUG LOGGING (inspectable via debugger)
// ============================================================================

#define DEBUG_LOG_SIZE 64
struct DebugLogEntry {
    uint32_t timestamp;
    uint8_t event_type;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
};

enum DebugEventType {
    DBG_STARTUP = 0,
    DBG_PAGE_CHANGE = 1,
    DBG_NOTE_RECEIVED = 2,
    DBG_LEARNING_START = 3,
    DBG_LEARNING_STOP = 4,
    DBG_PICKUP_ACTIVE = 5,
    DBG_PICKUP_WAITING = 6,
    DBG_CLOCK_PULSE = 7
};

DebugLogEntry debug_log[DEBUG_LOG_SIZE];
int debug_log_index = 0;

void log_debug(uint8_t event_type, uint8_t d1 = 0, uint8_t d2 = 0, uint8_t d3 = 0)
{
    debug_log[debug_log_index].timestamp = System::GetNow();
    debug_log[debug_log_index].event_type = event_type;
    debug_log[debug_log_index].data1 = d1;
    debug_log[debug_log_index].data2 = d2;
    debug_log[debug_log_index].data3 = d3;
    debug_log_index = (debug_log_index + 1) % DEBUG_LOG_SIZE;
}

// ============================================================================
// NOTE LEARNING SYSTEM
// ============================================================================

// Learning states
enum LearningState {
    STATE_IDLE,        // Waiting for input
    STATE_LEARNING,    // Recording notes
    STATE_GENERATING   // Playing back variations
};

LearningState learning_state = STATE_IDLE;

// Note buffer (stores MIDI note numbers 0-127)
const int MIN_LEARN_NOTES = 4;
const int MAX_LEARN_NOTES = 16;
uint8_t note_buffer[MAX_LEARN_NOTES];
int note_buffer_count = 0;

// Learning input detection
uint8_t last_note_in = 0;          // Last received note
bool note_in_active = false;       // True when a note is being held
uint32_t last_note_time = 0;       // Time of last note input (ms)
const uint32_t DEFAULT_LEARNING_TIMEOUT = 2000;  // Default: 2s (adjustable via parameter)

// ============================================================================
// TENDENCY ANALYSIS (extracted from learned notes)
// ============================================================================

struct LearnedTendencies {
    // Interval distribution (histogram of interval sizes)
    int interval_counts[13];  // 0=unison, 1=semitone, ... 12=octave
    int total_intervals;

    // Direction tendencies
    int ascending_count;
    int descending_count;
    int repeat_count;        // Same note twice in a row

    // Register analysis
    float register_center;   // Average MIDI note number
    float register_range;    // Max - min note
    uint8_t register_min;
    uint8_t register_max;

    // Most common intervals (for weighted generation)
    int most_common_interval;
    int second_common_interval;
};

LearnedTendencies tendencies;

// Analyze learned notes and extract tendencies
void analyze_learned_notes()
{
    // Clear previous analysis
    tendencies = LearnedTendencies();

    if(note_buffer_count < 2)
        return;  // Need at least 2 notes to analyze

    // Find register min/max and calculate center
    tendencies.register_min = 127;
    tendencies.register_max = 0;
    float note_sum = 0.0f;

    for(int i = 0; i < note_buffer_count; i++)
    {
        uint8_t note = note_buffer[i];
        if(note < tendencies.register_min) tendencies.register_min = note;
        if(note > tendencies.register_max) tendencies.register_max = note;
        note_sum += note;
    }

    tendencies.register_center = note_sum / (float)note_buffer_count;
    tendencies.register_range = tendencies.register_max - tendencies.register_min;

    // Analyze intervals between consecutive notes
    for(int i = 0; i < note_buffer_count - 1; i++)
    {
        int interval = note_buffer[i + 1] - note_buffer[i];

        // Count direction
        if(interval > 0)
            tendencies.ascending_count++;
        else if(interval < 0)
            tendencies.descending_count++;
        else
            tendencies.repeat_count++;

        // Count interval size (use absolute value, cap at octave)
        int interval_size = abs(interval);
        if(interval_size > 12) interval_size = 12;  // Cap at octave

        tendencies.interval_counts[interval_size]++;
        tendencies.total_intervals++;
    }

    // Find most common intervals
    int max_count = 0;
    int second_max_count = 0;

    for(int i = 0; i <= 12; i++)
    {
        if(tendencies.interval_counts[i] > max_count)
        {
            second_max_count = max_count;
            tendencies.second_common_interval = tendencies.most_common_interval;
            max_count = tendencies.interval_counts[i];
            tendencies.most_common_interval = i;
        }
        else if(tendencies.interval_counts[i] > second_max_count)
        {
            second_max_count = tendencies.interval_counts[i];
            tendencies.second_common_interval = i;
        }
    }
}

// ============================================================================
// NOTE GENERATION SYSTEM
// ============================================================================

// Generation state
uint8_t current_note = 60;       // Current generated note (MIDI)
uint8_t previous_note = 60;      // Previous note for direction memory
int last_interval = 0;           // Last interval taken
bool last_direction_up = true;   // Last direction was ascending

// Note history for memory/repetition bias
#define NOTE_HISTORY_SIZE 8
uint8_t note_history[NOTE_HISTORY_SIZE];
int note_history_count = 0;
int note_history_index = 0;

// Phrase length tracking
int phrase_note_count = 0;       // Notes generated in current phrase
int phrase_target_length = 12;   // Target phrase length (updated from PHRASE parameter)

// Simple random number generator (XORshift)
uint32_t rng_state = 12345;
uint32_t xorshift32()
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

// Random float 0.0 to 1.0
float random_float()
{
    return (float)xorshift32() / 4294967296.0f;
}

// Add note to history buffer (circular buffer)
void add_note_to_history(uint8_t note)
{
    note_history[note_history_index] = note;
    note_history_index = (note_history_index + 1) % NOTE_HISTORY_SIZE;
    if(note_history_count < NOTE_HISTORY_SIZE)
        note_history_count++;
}

// Check if note appears in recent history
// Returns count of how many times it appears (0-8)
int count_in_history(uint8_t note)
{
    int count = 0;
    for(int i = 0; i < note_history_count; i++)
    {
        if(note_history[i] == note)
            count++;
    }
    return count;
}

// Apply memory bias to note acceptance
// Returns true if note should be accepted, false if it should be rejected
bool apply_memory_bias(uint8_t candidate_note)
{
    // Get MEMORY parameter (0.0 = avoid repeats, 0.5 = neutral, 1.0 = favor repeats)
    float memory_param = parameters_smoothed[PARAM_MEMORY];

    // Apply energy scaling to memory
    // High energy = seek more novelty (reduce memory toward 0.0)
    float energy = parameters_smoothed[PARAM_ENERGY];
    float energy_deviation = (energy - 0.5f) * 2.0f;  // -1.0 to +1.0
    memory_param = memory_param - (energy_deviation * 0.3f);  // Energy biases toward novelty
    if(memory_param < 0.0f) memory_param = 0.0f;
    if(memory_param > 1.0f) memory_param = 1.0f;

    // Check if note is in recent history
    int history_count = count_in_history(candidate_note);

    // If note not in history, always accept
    if(history_count == 0)
        return true;

    // Calculate acceptance probability based on memory setting
    float acceptance_probability = 1.0f;

    if(memory_param < 0.4f)
    {
        // Low memory: Avoid repeats (seek novelty)
        // The more the note appears in history, the lower the acceptance
        float avoidance = (0.4f - memory_param) / 0.4f;  // 0.0 to 1.0
        acceptance_probability = 1.0f - (avoidance * history_count / NOTE_HISTORY_SIZE);
    }
    else if(memory_param > 0.6f)
    {
        // High memory: Favor repeats
        // The more the note appears in history, the higher the acceptance
        float favoritism = (memory_param - 0.6f) / 0.4f;  // 0.0 to 1.0
        acceptance_probability = 1.0f + (favoritism * history_count / NOTE_HISTORY_SIZE);
    }
    // else: neutral range (0.4-0.6), acceptance = 1.0 (ignore history)

    // Clamp probability
    if(acceptance_probability < 0.0f) acceptance_probability = 0.0f;
    if(acceptance_probability > 1.0f) acceptance_probability = 1.0f;

    // Accept or reject based on probability
    return random_float() < acceptance_probability;
}

// Weighted random selection from interval histogram
// Returns interval size (0-12 semitones)
int select_interval_from_distribution()
{
    if(tendencies.total_intervals == 0)
        return 2;  // Default to whole step if no data

    // Build cumulative weights
    int cumulative[13];
    int total = 0;
    for(int i = 0; i <= 12; i++)
    {
        total += tendencies.interval_counts[i];
        cumulative[i] = total;
    }

    // Select random point in distribution
    int rand_val = (int)(random_float() * total);

    // Find which interval this corresponds to
    for(int i = 0; i <= 12; i++)
    {
        if(rand_val < cumulative[i])
            return i;
    }

    return 2;  // Fallback
}

// Select direction based on learned tendencies, DIRECTION parameter, and register gravity
// Returns true for ascending, false for descending
bool select_direction()
{
    // Get direction parameter (0.0 = all down, 0.5 = neutral, 1.0 = all up)
    float direction_bias = parameters_smoothed[PARAM_DIRECTION];

    // Get register gravity parameter (0.0 = no gravity, 1.0 = strong pull to center)
    float register_gravity = parameters_smoothed[PARAM_REGISTER];

    // Calculate base probability from learned tendencies
    float learned_up_probability = 0.5f;
    int total_directional = tendencies.ascending_count + tendencies.descending_count;
    if(total_directional > 0)
    {
        learned_up_probability = (float)tendencies.ascending_count / (float)total_directional;
    }

    // Blend learned tendency with direction parameter
    float blend_factor = fabs(direction_bias - 0.5f) * 2.0f;  // 0.0 to 1.0
    float target_probability = (direction_bias > 0.5f) ? 1.0f : 0.0f;
    float base_probability = learned_up_probability * (1.0f - blend_factor) +
                             target_probability * blend_factor;

    // Apply register gravity - bias direction toward center pitch
    // Gravity increases as we approach phrase target length, decreases with high energy
    float gravity_influence = 0.0f;
    float effective_gravity = register_gravity;

    // Apply energy scaling to gravity
    // High energy = less gravity (more exploration)
    float energy = parameters_smoothed[PARAM_ENERGY];
    float energy_deviation = (energy - 0.5f) * 2.0f;  // -1.0 to +1.0
    effective_gravity = register_gravity - (energy_deviation * 0.3f);  // Energy reduces gravity
    if(effective_gravity < 0.0f) effective_gravity = 0.0f;
    if(effective_gravity > 1.0f) effective_gravity = 1.0f;

    // Boost gravity near phrase boundaries
    if(phrase_target_length > 0)
    {
        float phrase_progress = (float)phrase_note_count / (float)phrase_target_length;
        if(phrase_progress > 0.7f)  // In last 30% of phrase
        {
            float phrase_boost = (phrase_progress - 0.7f) / 0.3f;  // 0.0 to 1.0
            effective_gravity = effective_gravity + (phrase_boost * 0.3f);  // Add up to 0.3
            if(effective_gravity > 1.0f) effective_gravity = 1.0f;
        }
    }

    if(effective_gravity > 0.05f)  // Only apply if gravity is meaningful
    {
        // Calculate distance from learned center (in semitones)
        float distance_from_center = current_note - tendencies.register_center;

        // Normalize to roughly -1.0 to +1.0 (assuming ±24 semitone typical range)
        float normalized_distance = distance_from_center / 24.0f;
        if(normalized_distance < -1.0f) normalized_distance = -1.0f;
        if(normalized_distance > 1.0f) normalized_distance = 1.0f;

        // Gravity pulls toward center:
        // If above center (positive distance), bias downward (negative influence)
        // If below center (negative distance), bias upward (positive influence)
        gravity_influence = -normalized_distance * effective_gravity;
    }

    // Apply gravity as probability shift (-0.5 to +0.5 max)
    float final_probability = base_probability + (gravity_influence * 0.5f);
    if(final_probability < 0.0f) final_probability = 0.0f;
    if(final_probability > 1.0f) final_probability = 1.0f;

    return random_float() < final_probability;
}

// Apply octave displacement based on RANGE_WIDTH parameter
// Occasionally transposes notes by ±1 or ±2 octaves for variety
uint8_t apply_octave_displacement(uint8_t note)
{
    // Get RANGE_WIDTH parameter (0.0 = no displacement, 1.0 = frequent/large displacements)
    float range_param = parameters_smoothed[PARAM_RANGE_WIDTH];

    // Apply energy scaling to range
    // High energy = more octave displacements
    float energy = parameters_smoothed[PARAM_ENERGY];
    float energy_deviation = (energy - 0.5f) * 2.0f;  // -1.0 to +1.0
    range_param = range_param + (energy_deviation * 0.3f);  // Energy adds up to ±0.3
    if(range_param < 0.0f) range_param = 0.0f;
    if(range_param > 1.0f) range_param = 1.0f;

    // No displacement if parameter very low
    if(range_param < 0.1f)
        return note;

    // Calculate displacement probability (0% at 0.0, ~20% at 1.0)
    float displacement_probability = range_param * 0.2f;

    // Most of the time, no displacement
    if(random_float() > displacement_probability)
        return note;

    // Decide displacement amount based on RANGE setting
    int octave_shift = 0;
    if(range_param < 0.5f)
    {
        // Low range: only ±1 octave
        octave_shift = (random_float() > 0.5f) ? 12 : -12;
    }
    else
    {
        // High range: can do ±1 or ±2 octaves
        float roll = random_float();
        if(roll < 0.5f)
            octave_shift = 12;      // +1 octave
        else if(roll < 0.75f)
            octave_shift = -12;     // -1 octave
        else if(roll < 0.875f)
            octave_shift = 24;      // +2 octaves
        else
            octave_shift = -24;     // -2 octaves
    }

    // Apply displacement with MIDI range clamping
    int displaced = note + octave_shift;
    displaced = fmax(0, fmin(127, displaced));

    return (uint8_t)displaced;
}

// Generate next note based on learned tendencies and parameters
uint8_t generate_next_note()
{
    uint8_t candidate_note = 0;
    int attempts = 0;
    const int MAX_ATTEMPTS = 4;  // Try up to 4 times to find acceptable note

    // Get ENERGY macro parameter (0.0 = low energy/calm, 1.0 = high energy/wild)
    float energy = parameters_smoothed[PARAM_ENERGY];

    // Apply energy scaling to other parameters
    // Energy modulates: motion (more leaps), memory (less repetition), phrase length
    // Energy centered at 0.5 = neutral, deviations scale effects

    float energy_deviation = (energy - 0.5f) * 2.0f;  // -1.0 to +1.0

    // Get MOTION parameter and apply energy boost
    float motion_bias = parameters_smoothed[PARAM_MOTION];
    motion_bias = motion_bias + (energy_deviation * 0.3f);  // Energy adds up to ±0.3
    if(motion_bias < 0.0f) motion_bias = 0.0f;
    if(motion_bias > 1.0f) motion_bias = 1.0f;

    // Update phrase target length from PHRASE parameter, scaled by energy
    float phrase_param = parameters_smoothed[PARAM_PHRASE];
    phrase_param = phrase_param + (energy_deviation * 0.2f);  // Energy adds up to ±0.2
    if(phrase_param < 0.0f) phrase_param = 0.0f;
    if(phrase_param > 1.0f) phrase_param = 1.0f;
    phrase_target_length = (int)(4.0f + phrase_param * 28.0f);  // 4 to 32 range

    // Try generating notes until memory bias accepts one (or max attempts reached)
    while(attempts < MAX_ATTEMPTS)
    {
        // Select interval size from learned distribution
        int interval_size = select_interval_from_distribution();

        // Bias toward smaller or larger intervals based on MOTION parameter
        if(motion_bias < 0.5f)
        {
            // Bias toward smaller intervals
            float scale = motion_bias * 2.0f;  // 0.0 to 1.0
            interval_size = (int)(interval_size * scale + 0.5f);
            if(interval_size == 0 && random_float() > 0.5f)
                interval_size = 1;  // Prefer steps over repeats when going small
        }
        else
        {
            // Bias toward larger intervals
            float scale = (motion_bias - 0.5f) * 2.0f;  // 0.0 to 1.0
            int boost = (int)(scale * 4.0f);  // Add up to 4 semitones
            interval_size = fmin(interval_size + boost, 12);
        }

        // Select direction (includes register gravity influence)
        bool go_up = select_direction();

        // Apply interval with direction
        int signed_interval = go_up ? interval_size : -interval_size;
        int new_note = current_note + signed_interval;

        // Clamp to MIDI range
        new_note = fmax(0, fmin(127, new_note));

        // Apply octave displacement for variety
        new_note = apply_octave_displacement(new_note);

        // Store candidate
        candidate_note = (uint8_t)new_note;

        // Apply memory bias - accept or reject based on recent history
        if(apply_memory_bias(candidate_note))
        {
            // Note accepted!
            break;
        }

        attempts++;
    }

    // Add accepted note to history
    add_note_to_history(candidate_note);

    // Update phrase tracking
    phrase_note_count++;

    // Check if we should reset phrase (soft boundary)
    if(phrase_note_count >= phrase_target_length)
    {
        // Probabilistic reset - higher chance as we go past target
        float overrun = (float)(phrase_note_count - phrase_target_length);
        float reset_probability = 0.5f + (overrun / (float)phrase_target_length) * 0.5f;
        if(reset_probability > 1.0f) reset_probability = 1.0f;

        if(random_float() < reset_probability)
        {
            phrase_note_count = 0;
            // Optionally reseed RNG for variation
            rng_state ^= System::GetNow();
        }
    }

    // Store for next iteration
    previous_note = current_note;
    last_interval = candidate_note - current_note;
    last_direction_up = (candidate_note > current_note);

    return candidate_note;
}

// Send MIDI note output
void send_midi_note(uint8_t note, uint8_t velocity)
{
    // Hard clamp MIDI note to valid range: 0-127 (C0 to G9)
    if(note > 127) note = 127;
    // note is uint8_t so it can't be < 0

    // Send Note On message (status byte + 2 data bytes)
    uint8_t midi_data[3];
    midi_data[0] = 0x90;  // Note On, channel 1
    midi_data[1] = note;
    midi_data[2] = velocity;
    hw.midi.SendMessage(midi_data, 3);
}

// MIDI to CV conversion for pitch output (1V/octave)
// Maps MIDI note to 0-5V DAC range
// C1 (MIDI 36) = 0V, C2 (48) = 1V, C3 (60) = 2V, C4 (72) = 3V, C5 (84) = 4V, C6 (96) = 5V
float midi_note_to_cv(uint8_t midi_note)
{
    // 1V/octave standard: each octave (12 semitones) = 1 volt
    // Reference: MIDI 36 (C1) = 0V
    float cv = (float)(midi_note - 36) / 12.0f;

    // Clamp to DAC range (0-5V)
    if(cv < 0.0f) cv = 0.0f;
    if(cv > 5.0f) cv = 5.0f;

    return cv;
}

// CV to MIDI conversion (for pitch CV input)
// Assumes CV input is calibrated for 1V/octave
float cv_to_midi_note(float cv_voltage)
{
    // CV range: -5V to +5V = 10 octaves = 120 semitones
    // Center at C4 (MIDI 60)
    return 60.0f + (cv_voltage * 12.0f);  // 12 semitones per volt
}

// DAC callback for CV pitch output (only for Daisy Patch SM)
// Full Daisy Patch doesn't have CV outputs - uses MIDI instead
// Keeping this code commented for reference/future PatchSM port
/*
void CvCallback(uint16_t **output, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        // Convert current MIDI note to CV voltage (0-5V)
        float cv_voltage = midi_note_to_cv(current_note);

        // Convert to 12-bit DAC value (0-4095)
        // DAC range: 0 = 0V, 4095 = ~5V
        uint16_t dac_value = (uint16_t)(cv_voltage * 4095.0f / 5.0f);

        // Output to both CV outputs
        output[0][i] = dac_value;  // CV OUT 1
        output[1][i] = dac_value;  // CV OUT 2
    }
}
*/

// Start learning from user input
void start_learning()
{
    learning_state = STATE_LEARNING;
    note_buffer_count = 0;
    last_note_time = System::GetNow();
    log_debug(DBG_LEARNING_START);
}

// Add note to learning buffer
void add_note_to_buffer(uint8_t midi_note)
{
    if(learning_state == STATE_LEARNING && note_buffer_count < MAX_LEARN_NOTES)
    {
        note_buffer[note_buffer_count] = midi_note;
        note_buffer_count++;
        last_note_time = System::GetNow();
        log_debug(DBG_NOTE_RECEIVED, midi_note, note_buffer_count);

        // Visual feedback: blink LED
        hw.seed.SetLed(true);
    }
}

// Check if learning should stop (timeout or buffer full)
void update_learning_state()
{
    if(learning_state == STATE_LEARNING)
    {
        uint32_t current_time = System::GetNow();
        uint32_t time_since_note = current_time - last_note_time;

        // Calculate learning timeout from parameter (0.5s - 10s)
        // Parameter 0.0 = 500ms, 0.158 ≈ 2000ms (default), 1.0 = 10000ms
        float timeout_param = parameters_smoothed[PARAM_LEARN_TIMEOUT];
        uint32_t learning_timeout_ms = 500 + (uint32_t)(timeout_param * 9500.0f);

        // Stop learning if:
        // 1. Buffer is full (16 notes), OR
        // 2. Timeout (parameter-controlled) AND we have minimum notes (4)
        if(note_buffer_count >= MAX_LEARN_NOTES ||
           (time_since_note > learning_timeout_ms && note_buffer_count >= MIN_LEARN_NOTES))
        {
            learning_state = STATE_GENERATING;
            log_debug(DBG_LEARNING_STOP, note_buffer_count,
                     (time_since_note > learning_timeout_ms) ? 1 : 0);  // 1=timeout, 0=buffer full

            // Analyze learned notes and extract tendencies
            analyze_learned_notes();

            // Initialize generation state from learned notes
            // Start at the register center
            current_note = (uint8_t)tendencies.register_center;
            previous_note = current_note;
            last_interval = 0;
            last_direction_up = (tendencies.ascending_count >= tendencies.descending_count);

            // Clear note history for memory bias system
            note_history_count = 0;
            note_history_index = 0;
            for(int i = 0; i < NOTE_HISTORY_SIZE; i++)
                note_history[i] = 0;

            // Initialize phrase tracking
            phrase_note_count = 0;
            phrase_target_length = 12;  // Default medium length

            // Seed RNG with current time for variety
            rng_state = System::GetNow();
        }
    }
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    // Simple passthrough for now
    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
        out[2][i] = in[2][i];
        out[3][i] = in[3][i];
    }
}

void UpdateDisplay()
{
    hw.display.Fill(false);

    // Page indicator dots (top right)
    for(int i = 0; i < NUM_PAGES; i++)
    {
        int x = 110 + (i * 6);
        int y = 2;
        if(i == current_page)
        {
            // Filled circle for current page
            hw.display.DrawCircle(x, y, 2, true);
        }
        else
        {
            // Empty circle for other pages
            hw.display.DrawCircle(x, y, 2, false);
        }
    }

    // Clock pulse indicator (top left, next to page name)
    if(clock_pulse_indicator > 0)
    {
        hw.display.DrawCircle(60, 2, 2, true);
    }
    else
    {
        hw.display.DrawCircle(60, 2, 2, false);
    }

    // Page name (top left)
    hw.display.SetCursor(0, 0);
    std::string page_title = "PAGE " + std::to_string(current_page + 1);
    hw.display.WriteString((char*)page_title.c_str(), Font_6x8, true);

    // Draw line separator
    for(int x = 0; x < 128; x++)
    {
        hw.display.DrawPixel(x, 12, true);
    }

    // Parameter names and values (4 params per page)
    for(int i = 0; i < PARAMS_PER_PAGE; i++)
    {
        int y = 16 + (i * 11);

        // Parameter name from current page
        hw.display.SetCursor(0, y);
        hw.display.WriteString((char*)page_names[current_page][i], Font_6x8, true);

        // Get smoothed parameter value for this page/slot
        int param_index = (current_page * PARAMS_PER_PAGE) + i;
        float param_value = parameters_smoothed[param_index];
        bool is_active = param_pickup_active[param_index];

        // Get current pot position for this parameter
        float pot_position = pot_values[i];
        int pot_x = 56 + (int)(pot_position * 70.0f);

        // Border (solid if active, dashed if waiting for pickup)
        if(is_active)
        {
            // Active: show filled bar at stored value
            int bar_width = (int)(param_value * 70.0f);
            for(int x = 0; x < bar_width; x++)
            {
                hw.display.DrawLine(56 + x, y, 56 + x, y + 6, true);
            }
            hw.display.DrawRect(56, y, 126, y + 7, true, false);
        }
        else
        {
            // Not active: show stored value as filled bar, pot position as hollow indicator
            int bar_width = (int)(param_value * 70.0f);
            for(int x = 0; x < bar_width; x++)
            {
                hw.display.DrawLine(56 + x, y, 56 + x, y + 6, true);
            }

            // Dashed border to show waiting for pickup
            for(int x = 56; x < 126; x += 4)
            {
                hw.display.DrawPixel(x, y, true);
                hw.display.DrawPixel(x, y + 7, true);
            }
            hw.display.DrawLine(56, y, 56, y + 7, true);
            hw.display.DrawLine(126, y, 126, y + 7, true);

            // Show current pot position as hollow rectangle (3 pixels wide)
            if(pot_x > 56 && pot_x < 126)
            {
                hw.display.DrawRect(pot_x - 1, y + 1, pot_x + 1, y + 5, true, false);
            }
        }
    }

    // Learning state indicator (bottom left) - always show state
    hw.display.SetCursor(0, 56);
    if(learning_state == STATE_LEARNING)
    {
        // Show "L:" and note count
        std::string learn_str = "L:" + std::to_string(note_buffer_count);
        hw.display.WriteString((char*)learn_str.c_str(), Font_6x8, true);
    }
    else if(learning_state == STATE_GENERATING)
    {
        // Show "G:" and buffer size
        std::string gen_str = "G:" + std::to_string(note_buffer_count);
        hw.display.WriteString((char*)gen_str.c_str(), Font_6x8, true);
    }
    else
    {
        // IDLE: show dash
        hw.display.WriteString((char*)"-", Font_6x8, true);
    }

    // BPM display (bottom center-left) - always show if clock detected
    if(last_clock_time > 0)
    {
        hw.display.SetCursor(30, 56);
        std::string bpm_str = std::to_string((int)clock_bpm) + "bpm";
        hw.display.WriteString((char*)bpm_str.c_str(), Font_6x8, true);
    }

    // Clock/Gate indicator (bottom right)
    // Show filled box when gate is high
    if(gate2_state)
    {
        hw.display.DrawRect(100, 56, 112, 63, true, true);
    }
    else
    {
        hw.display.DrawRect(100, 56, 112, 63, true, false);
    }

    // Show "CLK" label
    hw.display.SetCursor(102, 56);
    hw.display.WriteString((char*)"C", Font_6x8, !gate2_state);

    // Pitch visualization (right side, vertical bar showing current note)
    if(learning_state == STATE_GENERATING)
    {
        // Map MIDI note (0-127) to vertical position (12-55)
        // Higher notes = higher on screen (lower y value)
        // Display area: y=12 (top) to y=55 (bottom) = 43 pixels
        float note_normalized = (float)current_note / 127.0f;  // 0.0 to 1.0
        int note_y = 55 - (int)(note_normalized * 43.0f);       // 55 (low) to 12 (high)

        // Draw vertical pitch reference bar (right edge)
        hw.display.DrawLine(126, 12, 126, 55, true);

        // Draw current note position as filled circle/dot
        hw.display.DrawCircle(126, note_y, 2, true);

        // Optional: show register center as reference line
        if(note_buffer_count >= MIN_LEARN_NOTES)
        {
            float center_normalized = tendencies.register_center / 127.0f;
            int center_y = 55 - (int)(center_normalized * 43.0f);
            // Draw small horizontal tick at center
            hw.display.DrawLine(123, center_y, 125, center_y, true);
        }
    }

    // Page change overlay (shows for 2 seconds after page change)
    if(page_change_timer > 0)
    {
        // Semi-transparent overlay box (just draw a box, can't do true transparency)
        hw.display.DrawRect(10, 22, 118, 42, true, true);
        hw.display.DrawRect(11, 23, 117, 41, false, false);

        // Show page name in center
        hw.display.SetCursor(30, 28);
        std::string overlay_text;
        if(current_page == 0)
            overlay_text = "PERFORMANCE";
        else if(current_page == 1)
            overlay_text = "MACRO";
        else
            overlay_text = "STRUCTURAL";
        hw.display.WriteString((char*)overlay_text.c_str(), Font_7x10, false);
    }

    hw.display.Update();
}

void UpdateControls()
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    // Process MIDI input for note learning
    hw.midi.Listen();
    while(hw.midi.HasEvents())
    {
        MidiEvent midi_event = hw.midi.PopEvent();

        // Only process Note On messages (and Note On with velocity 0 = Note Off)
        if(midi_event.type == NoteOn)
        {
            uint8_t note = midi_event.data[0];
            uint8_t velocity = midi_event.data[1];

            if(velocity > 0)
            {
                // Note On: start learning if idle/generating, or add to buffer if learning
                if(learning_state == STATE_IDLE || learning_state == STATE_GENERATING)
                {
                    // Start fresh learning (resets buffer)
                    // This allows live phrase injection while generating
                    start_learning();
                }
                add_note_to_buffer(note);
                note_in_active = true;
                last_note_in = note;

                // Echo notes during learning if ECHO parameter enabled (> 0.5)
                if(learning_state == STATE_LEARNING && parameters_smoothed[PARAM_ECHO_NOTES] > 0.5f)
                {
                    send_midi_note(note, velocity);

                    // Also trigger gate output for immediate feedback
                    gate_out_state = true;
                    gate_out_start_time = System::GetNow();
                    dsy_gpio_write(&hw.gate_output, 1);
                    gate_length_ms = 100.0f;  // Short 100ms gate for echo
                }
            }
            else
            {
                // Note Off (velocity 0)
                note_in_active = false;
            }
        }
        else if(midi_event.type == NoteOff)
        {
            note_in_active = false;
        }
        else if(midi_event.type == ControlChange)
        {
            // Handle MIDI CC for parameter control
            uint8_t cc_number = midi_event.data[0];
            uint8_t cc_value = midi_event.data[1];  // 0-127

            // Check if this CC number matches one of our defined parameters
            for(int i = 0; i < MIDI_CC_COUNT; i++)
            {
                if(cc_number == midi_cc_numbers[i])
                {
                    // Convert CC value (0-127) to parameter value (0.0-1.0)
                    float param_value = (float)cc_value / 127.0f;

                    // Update parameter directly
                    parameters[i] = param_value;
                    parameters_smoothed[i] = param_value;  // Set smoothed to match immediately

                    // Deactivate pickup for this parameter so pot must catch up
                    // This prevents pot from immediately overriding MIDI control
                    param_pickup_active[i] = false;

                    break;  // Found matching CC, stop searching
                }
            }
        }
    }

    // Update learning state (check for timeout)
    update_learning_state();

    // Read encoder for page navigation (do this first to detect page changes)
    int encoder_change = hw.encoder.Increment();
    if(encoder_change != 0)
    {
        current_page += encoder_change;

        // Wrap around pages (0, 1, 2)
        if(current_page < 0)
            current_page = NUM_PAGES - 1;
        else if(current_page >= NUM_PAGES)
            current_page = 0;

        log_debug(DBG_PAGE_CHANGE, current_page);

        // Show page change overlay for 2 seconds
        page_change_timer = 60;  // 60 frames at 30fps = 2 seconds

        // On page change, deactivate pickup for new page's parameters
        // and read current pot positions to prevent spurious crosses
        hw.ProcessAnalogControls();
        for(int i = 0; i < PARAMS_PER_PAGE; i++)
        {
            int param_index = (current_page * PARAMS_PER_PAGE) + i;
            param_pickup_active[param_index] = false;

            // Update pot_last_value to current position to establish baseline
            pot_last_value[param_index] = hw.controls[i].Process();
        }
    }

    // Read 4 potentiometers and update parameters with soft takeover
    for(int i = 0; i < PARAMS_PER_PAGE; i++)
    {
        pot_values[i] = hw.controls[i].Process();

        // Map pot to correct parameter based on current page
        int param_index = (current_page * PARAMS_PER_PAGE) + i;

        // Check if pickup is active for this parameter
        if(!param_pickup_active[param_index])
        {
            // Not active yet - check if pot has "caught up" to stored value
            float stored_value = parameters[param_index];
            float pot_value = pot_values[i];

            // Check if pot is within threshold of stored value
            if(fabs(pot_value - stored_value) < PICKUP_THRESHOLD)
            {
                // Close enough - activate pickup
                param_pickup_active[param_index] = true;
            }
            // Also check if pot has crossed the stored value
            // (moving from one side to the other)
            else
            {
                float last_pot = pot_last_value[param_index];
                bool crossed = (last_pot <= stored_value && pot_value >= stored_value) ||
                               (last_pot >= stored_value && pot_value <= stored_value);
                if(crossed)
                {
                    param_pickup_active[param_index] = true;
                }
            }
        }

        // Only update parameter if pickup is active
        if(param_pickup_active[param_index])
        {
            parameters[param_index] = pot_values[i];
        }

        // Store pot value for next comparison
        pot_last_value[param_index] = pot_values[i];
    }

    // Apply smoothing to ALL parameters (not just current page)
    for(int i = 0; i < TOTAL_PARAMS; i++)
    {
        // One-pole lowpass filter (exponential smoothing)
        // smoothed = smoothed + coeff * (target - smoothed)
        parameters_smoothed[i] += SMOOTHING_COEFF * (parameters[i] - parameters_smoothed[i]);
    }

    // Encoder click behavior depends on learning state
    if(hw.encoder.RisingEdge())
    {
        if(learning_state == STATE_GENERATING)
        {
            // Reset learning: go back to IDLE, clear buffer
            learning_state = STATE_IDLE;
            note_buffer_count = 0;
            page_change_timer = 30;  // Brief flash
        }
        else
        {
            // Normal behavior: reset to page 0
            current_page = 0;
            page_change_timer = 60;
        }
    }

    // Decrement page change timer
    if(page_change_timer > 0)
    {
        page_change_timer--;
    }

    // Read Gate Input 1 (Note Trigger)
    // Used to trigger note generation during GENERATING state
    gate1_prev = gate1_state;
    gate1_state = hw.gate_input[0].State();

    // Detect rising edge on Gate 1 (note trigger)
    note_triggered = false;
    if(gate1_state && !gate1_prev)
    {
        note_triggered = true;
        log_debug(DBG_CLOCK_PULSE);

        // Generate new note if in generating mode
        if(learning_state == STATE_GENERATING && note_buffer_count >= MIN_LEARN_NOTES)
        {
            current_note = generate_next_note();
            send_midi_note(current_note, 100);  // Velocity 100

            // Trigger gate output
            gate_out_state = true;
            gate_out_start_time = System::GetNow();
            dsy_gpio_write(&hw.gate_output, 1);  // Set gate HIGH

            // Calculate gate length as 50% of clock interval (or 50ms minimum)
            if(clock_interval > 0)
            {
                gate_length_ms = (float)clock_interval * 0.5f;
                if(gate_length_ms < 20.0f) gate_length_ms = 20.0f;    // Min 20ms
                if(gate_length_ms > 500.0f) gate_length_ms = 500.0f;  // Max 500ms
            }
            else
            {
                gate_length_ms = 50.0f;  // Default 50ms if no clock yet
            }
        }
    }

    // Read Gate Input 2 (Clock/BPM Detection)
    // Used to measure tempo continuously
    gate2_prev = gate2_state;
    gate2_state = hw.gate_input[1].State();

    // Detect rising edge on Gate 2 (BPM clock)
    if(gate2_state && !gate2_prev)
    {
        clock_pulse_indicator = 5;  // Show pulse for 5 frames (~150ms at 30fps)

        // Measure time since last clock pulse
        uint32_t current_time = System::GetNow();
        if(last_clock_time > 0)
        {
            clock_interval = current_time - last_clock_time;

            // Calculate BPM (assuming quarter notes)
            // BPM = 60000 / interval_ms
            if(clock_interval > 0)
            {
                clock_bpm = 60000.0f / (float)clock_interval;
                // Clamp to reasonable range
                if(clock_bpm < 20.0f) clock_bpm = 20.0f;
                if(clock_bpm > 300.0f) clock_bpm = 300.0f;
            }
        }
        last_clock_time = current_time;
    }

    // Decrement pulse indicator
    if(clock_pulse_indicator > 0)
    {
        clock_pulse_indicator--;
    }

    // Update gate output timing
    if(gate_out_state)
    {
        uint32_t current_time = System::GetNow();
        uint32_t gate_elapsed = current_time - gate_out_start_time;

        // Check if gate should go low
        if(gate_elapsed >= (uint32_t)gate_length_ms)
        {
            gate_out_state = false;
            dsy_gpio_write(&hw.gate_output, 0);  // Set gate LOW
        }
    }

    // LED indicates learning state
    if(learning_state == STATE_LEARNING)
    {
        // Blink LED while learning (on when note active)
        hw.seed.SetLed(note_in_active);
    }
    else if(learning_state == STATE_GENERATING)
    {
        // Pulse LED with clock when generating
        hw.seed.SetLed(clock_pulse_indicator > 0);
    }
    else
    {
        // IDLE: show gate input (clock on Gate 2)
        hw.seed.SetLed(gate2_state);
    }
}

int main(void)
{
    // Initialize hardware
    hw.Init();
    log_debug(DBG_STARTUP);

    // Start ADC for CV inputs (need this before reading pots)
    hw.StartAdc();
    System::Delay(100);  // Give ADC time to stabilize

    // Read actual pot positions before initializing parameters
    hw.ProcessAnalogControls();
    for(int i = 0; i < PARAMS_PER_PAGE; i++)
    {
        pot_values[i] = hw.controls[i].Process();
    }

    // Initialize parameters based on actual pot positions for page 0
    // Other pages default to 0.5
    for(int i = 0; i < TOTAL_PARAMS; i++)
    {
        if(i < PARAMS_PER_PAGE)
        {
            // Page 0: use actual pot positions
            parameters[i] = pot_values[i];
            parameters_smoothed[i] = pot_values[i];
            param_pickup_active[i] = true;  // Active since they match
            pot_last_value[i] = pot_values[i];
        }
        else
        {
            // Pages 1, 2, 3: default to middle position
            parameters[i] = 0.5f;
            parameters_smoothed[i] = 0.5f;
            param_pickup_active[i] = false;  // Not active until picked up
            pot_last_value[i] = pot_values[i % PARAMS_PER_PAGE];  // Set to current pot position
        }
    }

    // Special defaults for Page 3 (Utility) parameters
    // LEARN_TIMEOUT: default to 2 seconds (param value ≈ 0.158)
    // Formula: timeout_ms = 500 + (param * 9500), so 2000ms requires param = 0.158
    parameters[PARAM_LEARN_TIMEOUT] = 0.158f;
    parameters_smoothed[PARAM_LEARN_TIMEOUT] = 0.158f;

    // ECHO_NOTES: default to OFF (0.0)
    parameters[PARAM_ECHO_NOTES] = 0.0f;
    parameters_smoothed[PARAM_ECHO_NOTES] = 0.0f;

    // Start audio
    hw.StartAudio(AudioCallback);

    // Note: Full Daisy Patch doesn't have CV DAC outputs
    // CV output is only available on Daisy Patch SM
    // This module uses MIDI output instead (already implemented)

    // Display startup message
    hw.display.Fill(false);
    hw.display.SetCursor(20, 28);
    std::string msg = "GENERATIVE";
    hw.display.WriteString((char*)msg.c_str(), Font_7x10, true);
    hw.display.Update();
    System::Delay(1000);

    // Main loop
    while(1)
    {
        UpdateControls();

        // Update display at ~30Hz (every 33ms)
        if(frame_counter++ > 33)
        {
            UpdateDisplay();
            frame_counter = 0;
        }

        System::Delay(1);
    }
}
