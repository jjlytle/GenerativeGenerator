# GenerativeGenerator - Testing Guide

## Hardware Setup

### Required Equipment
1. **Daisy Patch** - Full module with OLED display
2. **ST-Link V3 Mini** - For programming and debugging
3. **USB MIDI Interface** - Class-compliant USB to 5-pin DIN MIDI
4. **MIDI cables** - 5-pin DIN cables

### Connections
```
[Computer USB] → [USB MIDI Interface] → [5-pin MIDI cable] → [Daisy Patch MIDI IN]
[Computer USB] → [ST-Link V3 Mini] → [SWD/JTAG] → [Daisy Patch]
```

## Software Setup

### Install Dependencies
```bash
# Install Python MIDI library
pip3 install mido python-rtmidi

# Verify MIDI ports
python3 test_midi.py --list
```

### Build with Debug Symbols
```bash
# Clean build with debug symbols
make clean
DEBUG=1 make
```

### Program via ST-Link
```bash
# Flash via ST-Link (no DFU bootloader needed)
make program
```

## Debugging with VS Code

### Start Debug Session
1. Connect ST-Link V3 Mini to Daisy Patch
2. Open VS Code in project directory
3. Press **F5** or select "Run → Start Debugging"
4. Select **"Cortex Debug"** configuration

### Inspect Debug Log
The firmware maintains a circular debug log buffer that can be inspected via the debugger.

**In VS Code Debug Console:**
```
-exec p debug_log
-exec p debug_log_index
```

Or add a **Watch** expression:
- `debug_log[0..63]` - View entire log buffer
- `debug_log_index` - Current write position

### Debug Log Events
```c
DBG_STARTUP = 0         // Firmware started
DBG_PAGE_CHANGE = 1     // Page changed (data1 = new page)
DBG_NOTE_RECEIVED = 2   // MIDI note received (data1 = note, data2 = buffer count)
DBG_LEARNING_START = 3  // Learning started
DBG_LEARNING_STOP = 4   // Learning stopped (data1 = count, data2 = 1=timeout/0=full)
DBG_PICKUP_ACTIVE = 5   // Parameter pickup activated
DBG_PICKUP_WAITING = 6  // Parameter waiting for pickup
DBG_CLOCK_PULSE = 7     // Clock pulse received
```

### Set Breakpoints
Useful locations:
- `add_note_to_buffer()` - Line ~175 - Triggers on MIDI note
- `update_learning_state()` - Line ~185 - Triggers on state change
- `UpdateControls()` - Line ~380 - Main control loop
- `UpdateDisplay()` - Line ~240 - Display refresh

## Automated MIDI Testing

### Basic Test Sequences

**List available tests:**
```bash
python3 test_midi.py --help
```

**Run basic 4-note sequence:**
```bash
python3 test_midi.py --test basic
```

**Run pentatonic scale (5 notes):**
```bash
python3 test_midi.py --test pentatonic
```

**Run maximum buffer test (16 notes):**
```bash
python3 test_midi.py --test max_buffer
```

**Custom note sequence:**
```bash
python3 test_midi.py --notes 60 64 67 72
```

### Test Scenarios

#### Test 1: Minimum Buffer (4 notes)
```bash
python3 test_midi.py --test basic
```
**Expected:**
- Display shows "L:1", "L:2", "L:3", "L:4"
- After 2s timeout, shows "G:4"
- LED blinks during learning, pulses with clock after

#### Test 2: Maximum Buffer (16 notes)
```bash
python3 test_midi.py --test max_buffer
```
**Expected:**
- Display counts up to "L:16"
- Immediately switches to "G:16" (no timeout, buffer full)

#### Test 3: Octave Leaps
```bash
python3 test_midi.py --test octave_leap
```
**Expected:**
- Large interval jumps (C2 → C3 → C4)
- Tests interval distribution analysis (Task 9)

#### Test 4: Interactive Mode
```bash
python3 test_midi.py --interactive
```
Type notes at prompt:
```
Notes> 60 62 64 65
Notes> C4 D4 E4 F4
Notes> quit
```

## Manual Test Checklist

### Hardware Interface
- [ ] Encoder rotation changes pages (0 → 1 → 2 → 0)
- [ ] Page dots show current page (top right)
- [ ] Encoder click resets to page 0 when idle
- [ ] Encoder click clears learning buffer when generating

### Soft Takeover
- [ ] Page 0 parameters respond immediately on startup
- [ ] Page 1/2 show dashed borders after page change
- [ ] Hollow rectangle shows current pot position
- [ ] Filled bar shows stored parameter value
- [ ] Border becomes solid when pot catches stored value
- [ ] Parameters don't jump when switching pages

### MIDI Learning
- [ ] Display shows "L:X" when receiving MIDI notes
- [ ] LED blinks when notes are received
- [ ] Learning stops after 2s timeout (minimum 4 notes)
- [ ] Learning stops immediately at 16 notes (buffer full)
- [ ] Display shows "G:X" after learning completes
- [ ] Encoder click resets buffer (goes back to idle)

### Clock/Gate
- [ ] Clock pulse indicator (top left circle) lights on gate high
- [ ] BPM calculated and displayed (bottom left)
- [ ] LED pulses with clock when in generating mode

### Display
- [ ] OLED shows startup message "GENERATIVE"
- [ ] All 4 parameter names visible per page
- [ ] Parameter bars update smoothly
- [ ] Page change overlay shows page name
- [ ] No flickering or visual artifacts

## Regression Tests

Run after each firmware update:

```bash
# Test all sequences
for test in basic pentatonic octave_leap chromatic max_buffer melodic; do
    echo "Testing: $test"
    python3 test_midi.py --test $test
    sleep 3
done
```

## Common Issues

### MIDI Not Working
```bash
# List MIDI ports
python3 test_midi.py --list

# Specify port explicitly
python3 test_midi.py --port 0 --test basic
```

### ST-Link Not Found
```bash
# Check connection
st-info --probe

# Try programming again
make program
```

### Debug Symbols Missing
```bash
# Rebuild with debug symbols
make clean
DEBUG=1 make
make program
```

### Learning Not Triggering
- Check MIDI cable direction (OUT → IN)
- Verify MIDI port in use: `test_midi.py --list`
- Check debug log in debugger: `p debug_log`

## Performance Metrics

### Current Build Stats
- **Flash:** ~95 KB / 128 KB (74%)
- **SRAM:** ~52 KB / 512 KB (10%)
- **Audio Latency:** ~1ms
- **Display Refresh:** 30 Hz

### Memory Budget
- Learning buffer: 16 bytes (16 notes × 1 byte)
- Debug log: 384 bytes (64 entries × 6 bytes)
- Parameter storage: 144 bytes (12 params × 3 × 4 bytes)

## Next Steps

### Task 9: Interval Distribution Analysis
Once MIDI learning is verified working:
1. Analyze captured notes in `note_buffer[]`
2. Extract interval sizes (semitone distances)
3. Calculate direction changes (up/down)
4. Find register center (average pitch)
5. Measure repetition frequency

**Debug in VS Code:**
```
-exec p note_buffer[0..15]
-exec p note_buffer_count
```

### Task 10-14: Generative Algorithm
Implement probability-based note generation using learned tendencies.

### Task 15-16: CV/Gate Output
Generate CV pitch and gate signals based on clock input.

### Task 17-18: Visual Feedback
Animate OLED to show pitch motion and energy.
