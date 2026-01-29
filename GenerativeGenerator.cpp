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

        // Bar graph (0-70 pixels) - showing smoothed parameter value
        int bar_width = (int)(param_value * 70.0f);
        for(int x = 0; x < bar_width; x++)
        {
            hw.display.DrawLine(56 + x, y, 56 + x, y + 6, true);
        }

        // Border
        hw.display.DrawRect(56, y, 126, y + 7, true, false);
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

    // Show BPM (if clock has been detected)
    if(last_clock_time > 0)
    {
        hw.display.SetCursor(0, 56);
        std::string bpm_str = std::to_string((int)clock_bpm);
        hw.display.WriteString((char*)bpm_str.c_str(), Font_6x8, true);
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

    // Read 4 potentiometers and update parameters
    for(int i = 0; i < PARAMS_PER_PAGE; i++)
    {
        pot_values[i] = hw.controls[i].Process();

        // Map pot to correct parameter based on current page
        int param_index = (current_page * PARAMS_PER_PAGE) + i;

        // Store raw pot value
        parameters[param_index] = pot_values[i];
    }

    // Apply smoothing to ALL parameters (not just current page)
    for(int i = 0; i < TOTAL_PARAMS; i++)
    {
        // One-pole lowpass filter (exponential smoothing)
        // smoothed = smoothed + coeff * (target - smoothed)
        parameters_smoothed[i] += SMOOTHING_COEFF * (parameters[i] - parameters_smoothed[i]);
    }

    // Read encoder for page navigation
    int encoder_change = hw.encoder.Increment();
    if(encoder_change != 0)
    {
        current_page += encoder_change;

        // Wrap around pages (0, 1, 2)
        if(current_page < 0)
            current_page = NUM_PAGES - 1;
        else if(current_page >= NUM_PAGES)
            current_page = 0;

        // Show page change overlay for 2 seconds
        page_change_timer = 60;  // 60 frames at 30fps = 2 seconds
    }

    // Encoder click resets to page 0
    if(hw.encoder.RisingEdge())
    {
        current_page = 0;
        page_change_timer = 60;
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

    // Blink LED with gate
    hw.seed.SetLed(gate_in_state);
}

int main(void)
{
    // Initialize hardware
    hw.Init();

    // Initialize all parameters to 0.5 (middle position)
    for(int i = 0; i < TOTAL_PARAMS; i++)
    {
        parameters[i] = 0.5f;
        parameters_smoothed[i] = 0.5f;  // Start smoothed values at same position
    }

    // Initialize pot values
    for(int i = 0; i < PARAMS_PER_PAGE; i++)
    {
        pot_values[i] = 0.0f;
    }

    // Start ADC for CV inputs
    hw.StartAdc();

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
