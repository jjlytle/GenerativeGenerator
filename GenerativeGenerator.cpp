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

// Test variables
float pot_values[4];
int   encoder_position = 0;
bool  gate_in_state = false;
int   frame_counter = 0;

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

    // Title
    hw.display.SetCursor(0, 0);
    std::string title = "GEN HARDWARE TEST";
    hw.display.WriteString((char*)title.c_str(), Font_6x8, true);

    // Draw line separator
    for(int x = 0; x < 128; x++)
    {
        hw.display.DrawPixel(x, 12, true);
    }

    // Pot values (as bar graphs)
    for(int i = 0; i < 4; i++)
    {
        int y = 16 + (i * 11);

        // Label
        std::string label = "P" + std::to_string(i + 1);
        hw.display.SetCursor(0, y);
        hw.display.WriteString((char*)label.c_str(), Font_6x8, true);

        // Bar graph (0-100 pixels)
        int bar_width = (int)(pot_values[i] * 100.0f);
        for(int x = 0; x < bar_width; x++)
        {
            hw.display.DrawLine(16 + x, y, 16 + x, y + 6, true);
        }

        // Border
        hw.display.DrawRect(16, y, 116, y + 7, true, false);
    }

    // Encoder position
    hw.display.SetCursor(0, 56);
    std::string enc = "ENC:" + std::to_string(encoder_position);
    hw.display.WriteString((char*)enc.c_str(), Font_6x8, true);

    // Gate indicator
    if(gate_in_state)
    {
        hw.display.DrawRect(100, 56, 115, 63, true, true);
    }
    else
    {
        hw.display.DrawRect(100, 56, 115, 63, true, false);
    }
    hw.display.SetCursor(104, 56);
    hw.display.WriteString((char*)"GT", Font_6x8, !gate_in_state);

    hw.display.Update();
}

void UpdateControls()
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    // Read 4 potentiometers
    for(int i = 0; i < 4; i++)
    {
        pot_values[i] = hw.controls[i].Process();
    }

    // Read encoder
    encoder_position += hw.encoder.Increment();

    // Read gate input
    gate_in_state = hw.gate_input[0].State();

    // Blink LED with gate
    hw.seed.SetLed(gate_in_state);
}

int main(void)
{
    // Initialize hardware
    hw.Init();

    // Initialize test variables
    for(int i = 0; i < 4; i++)
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
