# Generative Generator

A generative Eurorack oscillator module based on the Daisy Patch SM.

## Description

This project started from the SimpleOscillator example and will be developed into a generative sound module. The module features voltage-controlled oscillator capabilities with extensive CV control.

## Hardware Setup

### Daisy Patch SM Connections

**CV Inputs:**
- CV_1: Coarse frequency control (MIDI note 36-96)
- CV_2: Fine tuning (±1 semitone)
- CV_5: V/OCT input (standard 1V/octave)

**Audio Outputs:**
- OUT_L / OUT_R: Main oscillator output (stereo)

## Controls

- **Coarse Tuning**: CV_1 controls the base frequency from MIDI note 36 to 96
- **Fine Tuning**: CV_2 provides ±1 semitone fine tuning
- **V/OCT Input**: CV_5 accepts standard 1V/octave control voltage (0-5V = 60 semitones)

## Building

```bash
# Build the project
make

# Clean build
make clean

# Build and program via USB (DFU)
make program-dfu

# Build and program via STLink
make program
```

## VS Code Workflow

- **Build**: Press `Cmd+Shift+B`
- **Build & Flash (DFU)**: Press `Cmd+Shift+D`
- **Build & Flash (STLink)**: Press `Cmd+Shift+U`
- **Debug**: Press `F5` (requires STLink)

## Development Roadmap

- [x] Basic oscillator functionality
- [ ] Add generative pattern generation
- [ ] Implement multiple waveforms
- [ ] Add modulation capabilities
- [ ] Create generative algorithms

## Technical Details

- **Platform**: Daisy Patch SM
- **MCU**: STM32H750 (ARM Cortex-M7 @ 480MHz)
- **Sample Rate**: 48kHz
- **Audio**: 24-bit stereo

## Resources

- [Daisy Documentation](https://daisy.audio/)
- [libDaisy API](https://electro-smith.github.io/libDaisy/)
- [DaisySP API](https://electro-smith.github.io/DaisySP/)

---

Created with the Daisy Examples VS Code development environment.
