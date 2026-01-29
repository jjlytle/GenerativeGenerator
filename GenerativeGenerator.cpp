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
int   current_page = 0;      // 0, 1, 2 for 3 pages
const int NUM_PAGES = 3;
const int PARAMS_PER_PAGE = 4;
const int TOTAL_PARAMS = 12;

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
    PARAM_RANGE_WIDTH = 11
};

// Parameter names for each page (4 params per page)
const char* page_names[NUM_PAGES][PARAMS_PER_PAGE] = {
    // Page 0: Performance - Direct Control
    {"MOTION", "MEMORY", "REGISTER", "DIRECTION"},
    // Page 1: Performance - Macro & Evolution
    {"PHRASE", "ENERGY", "STABILITY", "FORGET"},
    // Page 2: Structural - Shape & Gravity
    {"LEAP SHP", "DIR MEM", "HOME REG", "RANGE"}
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

// Clock/Gate detection state
bool  gate_in_state = false;
bool  gate_in_prev = false;
bool  clock_triggered = false;  // True for one frame after rising edge
uint32_t last_clock_time = 0;
uint32_t clock_interval = 0;    // Time between clock pulses (in ms)
float clock_bpm = 120.0f;        // Estimated tempo
int   clock_pulse_indicator = 0; // Visual pulse countdown (frames)

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
const uint32_t LEARNING_TIMEOUT = 2000;  // Stop learning after 2s of no input

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

// CV to MIDI conversion (for pitch CV input)
// Assumes CV input is calibrated for 1V/octave
float cv_to_midi_note(float cv_voltage)
{
    // CV range: -5V to +5V = 10 octaves = 120 semitones
    // Center at C4 (MIDI 60)
    return 60.0f + (cv_voltage * 12.0f);  // 12 semitones per volt
}

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

        // Stop learning if:
        // 1. Buffer is full (16 notes), OR
        // 2. Timeout (2 seconds) AND we have minimum notes (4)
        if(note_buffer_count >= MAX_LEARN_NOTES ||
           (time_since_note > LEARNING_TIMEOUT && note_buffer_count >= MIN_LEARN_NOTES))
        {
            learning_state = STATE_GENERATING;
            log_debug(DBG_LEARNING_STOP, note_buffer_count,
                     (time_since_note > LEARNING_TIMEOUT) ? 1 : 0);  // 1=timeout, 0=buffer full

            // Analyze learned notes and extract tendencies
            analyze_learned_notes();
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

    // Learning state indicator (bottom left)
    hw.display.SetCursor(0, 56);
    if(learning_state == STATE_LEARNING)
    {
        // Show "LEARN" and note count
        std::string learn_str = "L:" + std::to_string(note_buffer_count);
        hw.display.WriteString((char*)learn_str.c_str(), Font_6x8, true);
    }
    else if(learning_state == STATE_GENERATING)
    {
        // Show "GEN" and buffer size
        std::string gen_str = "G:" + std::to_string(note_buffer_count);
        hw.display.WriteString((char*)gen_str.c_str(), Font_6x8, true);
    }
    else
    {
        // IDLE: show BPM if clock detected
        if(last_clock_time > 0)
        {
            std::string bpm_str = std::to_string((int)clock_bpm);
            hw.display.WriteString((char*)bpm_str.c_str(), Font_6x8, true);
        }
    }

    // Clock/Gate indicator (bottom right)
    // Show filled box when gate is high
    if(gate_in_state)
    {
        hw.display.DrawRect(100, 56, 112, 63, true, true);
    }
    else
    {
        hw.display.DrawRect(100, 56, 112, 63, true, false);
    }

    // Show "CLK" label
    hw.display.SetCursor(102, 56);
    hw.display.WriteString((char*)"C", Font_6x8, !gate_in_state);

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
                // Note On: start learning if idle, or add to buffer if learning
                if(learning_state == STATE_IDLE)
                {
                    start_learning();
                }
                add_note_to_buffer(note);
                note_in_active = true;
                last_note_in = note;
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

    // Read gate/clock input
    gate_in_prev = gate_in_state;
    gate_in_state = hw.gate_input[0].State();

    // Detect rising edge (clock trigger)
    clock_triggered = false;
    if(gate_in_state && !gate_in_prev)
    {
        clock_triggered = true;
        clock_pulse_indicator = 5;  // Show pulse for 5 frames (~150ms at 30fps)
        log_debug(DBG_CLOCK_PULSE);

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
        // IDLE: show gate input
        hw.seed.SetLed(gate_in_state);
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
            // Pages 1 & 2: default to middle position
            parameters[i] = 0.5f;
            parameters_smoothed[i] = 0.5f;
            param_pickup_active[i] = false;  // Not active until picked up
            pot_last_value[i] = pot_values[i % PARAMS_PER_PAGE];  // Set to current pot position
        }
    }

    // Start audio
    hw.StartAudio(AudioCallback);

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
