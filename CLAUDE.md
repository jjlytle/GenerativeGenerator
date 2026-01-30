# GenerativeGenerator - Claude Code Context

This file provides guidance to Claude Code when working on the GenerativeGenerator project.

## Project Overview

**GenerativeGenerator** is a generative melodic instrument for Daisy Patch that learns musical gestures and continues playing in a related style through probability bias rather than exact pattern playback.

**Hardware:** Daisy Patch (full module)
- 4 Potentiometers
- 1 Encoder (navigation + click)
- OLED Display (128x64)
- 2 Gate Inputs:
  - Gate Input 1: Note trigger (generates note during GENERATING state)
  - Gate Input 2: Clock/BPM detection (measures tempo continuously)
- 1 Gate Output (synchronized with note generation)
- 4 Audio I/O
- MIDI In/Out

**Repository:** https://github.com/jjlytle/GenerativeGenerator

## Current Status (18 of 21 Tasks Complete - 86%)

### ✅ Completed Tasks (Tasks 1-18)

1. **Basic Hardware Initialization** - All peripherals verified working
2. **3-Page Parameter System** - Encoder navigation with visual feedback
3. **Parameter Storage** - 12 parameters (3 pages × 4 params) with persistence
4. **CV/Pot Reading with Smoothing** - One-pole lowpass filter (coeff 0.15)
5. **OLED Display** - Page indicators, parameter bars, visual feedback
6. **Clock/Gate Detection** - Rising edge detection with BPM calculation
7. **Soft Takeover (Parameter Pickup)** - Prevents jumps when changing pages
8. **Note Learning Buffer** - Captures 4-16 MIDI notes with auto-timeout
9. **Interval Distribution Analysis** - Extracts tendencies from learned notes
10. **Motion Decision System** - Probability-based repeat/step/leap/octave selection
11. **Register Gravity & Octave Displacement** - Soft Gaussian pull toward center pitch
12. **Memory/Repetition Bias** - Weighted return to recent notes
13. **Phrase Length Soft Targeting** - Probabilistic phrase boundary detection
14. **Energy Macro Parameter** - Scales interval size, octave motion, phrase looseness
15. **CV Pitch Output** - 12-bit CV generation with V/OCT calibration
16. **Gate Output** - Synchronized with clock, 50% duty cycle
17. **Visual Pitch Display** - Vertical bar showing current note on OLED
18. **OLED Animation** - Motion trails, energy indicators, memory feedback

### 🚧 Next Up (Task 19-21)

**Testing Phase** - Verify system behavior under various conditions

### 📋 Remaining Tasks (19-21)

19. Test basic note generation with single parameter
20. Test full 12-parameter interaction
21. Musical stress test with extreme settings

### Design Document

See `GenerativeGeneratorDesign.md` for full conceptual model and parameter definitions.

## Parameter Layout (3 Pages × 4 Parameters)

### Page 0: Performance - Direct Control
1. **MOTION** - Biases stepwise motion vs intervallic leaps
2. **MEMORY** - Biases repetition and return to recent material
3. **REGISTER** - Controls octave displacement probability
4. **DIRECTION** - Biases ascending vs descending motion

### Page 1: Performance - Macro & Evolution
5. **PHRASE** - Controls expected phrase length (soft target)
6. **ENERGY** - Macro parameter scaling interval size, octave motion, phrase looseness
7. **STABILITY** - Biases stable vs unstable scale degrees
8. **FORGET** - Controls decay rate of learned tendencies

### Page 2: Structural - Shape & Gravity
9. **LEAP SHP** - Controls exponential decay of interval size
10. **DIR MEM** - Controls persistence of melodic direction
11. **HOME REG** - Sets center of register gravity
12. **RANGE** - Sets variance of register gravity

## Build & Development Workflow

### Quick Commands

```bash
# Build with debug symbols
make clean && DEBUG=1 make

# Flash via ST-Link (recommended - no bootloader needed)
make program

# Flash via USB DFU (hold BOOT, press RESET, release BOOT)
make program-dfu

# Check DFU status
dfu-util -l
```

### Git Workflow

```bash
# After completing a task
git add -A
git commit -m "Task X Complete: Description"
git push

# View history
git log --oneline

# Revert to previous task
git checkout <commit-hash>
git checkout main  # Return to latest
```

### Current Build Stats
- **FLASH**: 99,460 bytes / 128 KB (75.88%)
- **SRAM**: 52,484 bytes / 512 KB (10.01%)
- **Debug Log**: 384 bytes (64 entries × 6 bytes)
- **Learning Buffer**: 16 bytes (16 notes × 1 byte)
- **Tendencies**: ~88 bytes
- **Generation State**: Recent notes buffer (8 notes × 1 byte)

## Code Architecture

### Main Components

**Hardware Interface:**
- `DaisyPatch hw` - Hardware object
- `UpdateControls()` - Reads pots, encoder, gates, MIDI (called every loop)
- `UpdateDisplay()` - Updates OLED at 30Hz
- `AudioCallback()` - Real-time audio processing (currently passthrough)

**Page System:**
- `current_page` (0-2) - Current page index
- `page_names[3][4]` - Parameter names for all pages
- Encoder rotation switches pages with wraparound
- Encoder click resets to page 0 (or clears learning buffer when generating)
- Page change overlay shows for 2 seconds

**Parameter System:**
- `parameters[12]` - Raw parameter values (0.0-1.0)
- `parameters_smoothed[12]` - Smoothed for display/use (exponential filter)
- `param_pickup_active[12]` - Soft takeover state per parameter
- `pot_last_value[12]` - Previous pot positions for pickup detection

**Learning System:**
- `note_buffer[16]` - Captured MIDI notes (4-16 notes)
- `learning_state` - STATE_IDLE, STATE_LEARNING, or STATE_GENERATING
- `analyze_learned_notes()` - Extracts tendencies when learning completes

**Tendency Analysis:**
- `LearnedTendencies tendencies` - Global struct containing:
  - `interval_counts[13]` - Histogram of interval sizes (0-12 semitones)
  - `ascending_count / descending_count / repeat_count` - Direction stats
  - `register_center / min / max / range` - Pitch range analysis
  - `most_common_interval / second_common_interval` - Dominant intervals

**Generative System:**
- `generate_next_note()` - Main generation function triggered by Gate Input 1
- Motion decision: repeat/step/leap/octave (weighted by MOTION parameter)
- Interval selection: weighted by learned interval distribution
- Direction bias: blends learned tendencies with DIRECTION parameter
- Register gravity: soft Gaussian pull toward center (REGISTER parameter)
- Memory bias: weighted return to recent 8 notes (MEMORY parameter)
- Phrase tracking: soft boundary detection (PHRASE parameter)
- Energy scaling: macro control over interval size and motion (ENERGY parameter)
- Octave displacement: ±1-2 octave jumps based on probability

**Debug System:**
- `debug_log[64]` - Circular buffer of events (inspectable via debugger)
- Events: startup, page change, MIDI notes, learning state, clock pulses
- Each entry: timestamp, event type, 3 data bytes

**Display Layout:**
```
PAGE 1                    ●○○  ○
─────────────────────────────────
MOTION      [███████     ]
MEMORY      [████▯       ]  ← hollow rect = pot position
REGISTER    [█████████   ]     (dashed = waiting for pickup)
DIRECTION   [███         ]
                     120  CLK
```
Top left circle = clock pulse indicator
Bottom left = learning state or BPM
Bottom right = gate indicator

### Critical Rules

**Audio Callback:**
- NO blocking operations
- NO dynamic memory allocation
- Must complete in < 1ms
- Pre-allocate all DSP objects in main()

**Display Refresh:**
- Target 30Hz (33ms frame time)
- Use frame counter to throttle updates
- Keep display updates out of audio callback

**Soft Takeover:**
- Parameters persist when changing pages
- Visual feedback: dashed border = waiting for pickup
- Hollow rectangle shows current pot position
- Filled bar shows stored parameter value
- Pickup activates when pot is within 5% or crosses stored value

**MIDI Learning:**
- Minimum 4 notes, maximum 16 notes
- Auto-starts on first note input
- Auto-stops after 2 second timeout (minimum 4 notes required)
- Buffer full (16 notes) stops immediately
- Display shows "L:X" during learning, "G:X" when generating
- LED blinks during learning, pulses with clock when generating

## Development Constraints

From `GenerativeGeneratorDesign.md`:
- Code must map cleanly to the conceptual model
- No "magic numbers" without musical justification
- Probability changes must be smooth and continuous
- State must be inspectable for debugging
- Behavior should degrade gracefully under extreme settings

## Testing Infrastructure

### Hardware Setup
- **ST-Link V3 Mini** connected for programming/debugging
- **Electra One MK1** or USB-MIDI interface for note input
- **VS Code** with Cortex-Debug extension

### Automated MIDI Testing

```bash
# Install dependencies
pip3 install mido python-rtmidi

# List available MIDI ports
python3 test_midi.py --list

# Run predefined test sequences
python3 test_midi.py --port 0 --test basic          # 4 notes (C-D-E-F)
python3 test_midi.py --port 0 --test pentatonic     # 5 notes
python3 test_midi.py --port 0 --test octave_leap    # Large intervals
python3 test_midi.py --port 0 --test chromatic      # Semitones
python3 test_midi.py --port 0 --test max_buffer     # 16 notes (fills buffer)

# Custom note sequence
python3 test_midi.py --port 0 --notes 60 64 67 72

# Interactive mode
python3 test_midi.py --port 0 --interactive
```

### VS Code Debugging

**Start debugging:**
```
Press F5 in VS Code
```

**Watch panel variables:**
```
note_buffer[0..15]               // Captured notes
note_buffer_count                // Number captured
learning_state                   // 0=IDLE, 1=LEARNING, 2=GENERATING

tendencies.interval_counts[0..12]  // Interval histogram
tendencies.ascending_count       // Upward motion count
tendencies.descending_count      // Downward motion count
tendencies.register_center       // Average pitch
tendencies.most_common_interval  // Most frequent interval

debug_log[0..63]                 // Event log (circular buffer)
debug_log_index                  // Current write position
```

**Breakpoint locations:**
- `add_note_to_buffer()` - Line ~268 - Triggers on MIDI note
- `analyze_learned_notes()` - Line ~198 - After learning completes
- `UpdateControls()` - Line ~480 - Main control loop
- `UpdateDisplay()` - Line ~338 - Display refresh

See `TESTING.md` and `DEBUGGING_TENDENCIES.md` for comprehensive guides.

## Testing Checklist

### Hardware (Task 1) ✅
- [x] 4 Potentiometers respond
- [x] Encoder increments/decrements
- [x] OLED displays correctly
- [x] Gate input detected
- [x] LED blinks with gate
- [x] Audio passthrough works

### Page Navigation (Task 2) ✅
- [x] Encoder switches pages (0 → 1 → 2 → 0)
- [x] Encoder click returns to page 0
- [x] Page indicator dots show current page
- [x] Parameter names display per page
- [x] Page overlay shows for 2 seconds

### Parameter System (Tasks 3-4) ✅
- [x] 12 parameters stored across 3 pages
- [x] Parameters persist when changing pages
- [x] Smoothing applied (one-pole filter)
- [x] Page 0 parameters match pot positions on startup

### Soft Takeover (Task 7) ✅
- [x] Parameters don't jump when switching pages
- [x] Dashed border shows waiting for pickup
- [x] Hollow rectangle shows current pot position
- [x] Filled bar shows stored parameter value
- [x] Solid border when pot catches stored value

### MIDI Learning (Task 8) ✅
- [x] Captures 4-16 MIDI notes
- [x] Display shows "L:X" during learning
- [x] Auto-stops after 2s timeout (≥4 notes)
- [x] Auto-stops at 16 notes (buffer full)
- [x] Display shows "G:X" when generating
- [x] LED blinks during learning
- [x] Encoder click resets buffer

### Interval Analysis (Task 9) ✅
- [x] Extracts interval size distribution
- [x] Counts ascending/descending/repeat
- [x] Calculates register statistics
- [x] Finds most common intervals
- [x] Data inspectable via debugger

## Common Issues & Solutions

**VS Code IntelliSense errors:**
- Reload window (Cmd+Shift+P → Reload Window)
- `.vscode/c_cpp_properties.json` already configured

**Build errors - "cannot find -ldaisy":**
```bash
cd ../..
./ci/build_libs.sh
```

**Flash fails - "target voltage 0V":**
- Ensure Daisy Patch is powered (USB or Eurorack)
- Check ST-Link connections (SWDIO, SWCLK, GND)

**Display not updating:**
- Check `frame_counter` logic (updates every 33ms)
- Verify `UpdateDisplay()` is called in main loop

**MIDI notes not received:**
- Check MIDI cable direction (OUT → IN)
- Verify USB-MIDI interface routing
- Use `test_midi.py --list` to check ports
- Check debug log via debugger: `p debug_log`

**Soft takeover not working:**
- Page 0 should pickup immediately (pot positions initialized on startup)
- Other pages show dashed border until pot catches up
- Move pot toward filled bar until they meet

**Learning not triggering:**
- Send at least 4 notes
- Check MIDI channel (default: channel 1)
- Inspect `note_buffer[]` in debugger

## Memory Budget

**Flash (128 KB limit):**
- Current: 95,956 bytes (73.21%)
- Headroom: 34,292 bytes (26.79%)

**SRAM (512 KB):**
- Current: 52,436 bytes (10.00%)
- Headroom: ~460 KB

**Key Data Structures:**
- Learning buffer: 16 bytes
- Debug log: 384 bytes
- Parameters: 144 bytes (12 × 3 × 4 bytes)
- Tendencies: ~88 bytes
- Page names: ~144 bytes

## File Structure

```
GenerativeGenerator/
├── GenerativeGenerator.cpp     # Main source (18.5 KB)
├── Makefile                     # Build configuration
├── CLAUDE.md                    # This file (project context)
├── GenerativeGeneratorDesign.md # Design specification
├── README.md                    # Project overview
│
├── test_midi.py                 # Automated MIDI testing
├── TESTING.md                   # Testing guide
├── README_TESTING.md            # Quick testing reference
├── DEBUGGING_TENDENCIES.md      # Analysis debugging guide
├── ELECTRA_ONE_SETUP.md         # Electra One integration
├── ElectraOne_NoteTest.eproj    # Electra One preset
│
└── .vscode/
    ├── c_cpp_properties.json    # IntelliSense config
    ├── launch.json              # Debug config (ST-Link)
    └── tasks.json               # Build tasks
```

## Resources

- **Design Doc**: `GenerativeGeneratorDesign.md`
- **Testing Guides**: `TESTING.md`, `README_TESTING.md`, `DEBUGGING_TENDENCIES.md`
- **Main Daisy Guide**: `../../CLAUDE.md` (parent directory)
- **GitHub**: https://github.com/jjlytle/GenerativeGenerator
- **Daisy Docs**: https://daisy.audio/
- **libDaisy API**: https://electro-smith.github.io/libDaisy/
- **DaisySP API**: https://electro-smith.github.io/DaisySP/

## Guiding Principle

> "Does this make the module feel more like a collaborator?"

If the answer is no, reconsider the change.

## Next Session Context

**What works:**
- Complete UI with 3 pages, 4 parameters per page, soft takeover
- MIDI note learning (4-16 notes) with visual feedback
- Full generative system with probability-based note generation
- 12-parameter control over motion, memory, register, direction, phrase, energy
- Gate Input 1: Note triggering (during GENERATING state)
- Gate Input 2: BPM/clock detection (continuous)
- Gate Output: Synchronized with generated notes
- Visual pitch display on OLED
- Debugging infrastructure with ST-Link and Python test scripts

**What's next (Tasks 19-21):**
- Test basic note generation with single parameter variations
- Test full 12-parameter interaction and musical expressiveness
- Musical stress test with extreme parameter settings
- Verify system stability during long performances

**Known behavior:**
- Music generation sounds "amazing" (user feedback)
- All core generative features implemented and functional
- Gate inputs properly separated (Gate 1 = trigger, Gate 2 = BPM)
- Flash usage: 75.88% (24KB headroom remaining)
