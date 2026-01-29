# Quick Testing Setup

## Hardware Hookup

```
┌──────────────┐
│   Computer   │
└──────┬───────┘
       │
       ├─── USB ──→ [ST-Link V3 Mini] ──→ SWD ──→ [Daisy Patch]
       │                                            (Program/Debug)
       │
       └─── USB ──→ [USB-MIDI Interface] ──→ MIDI IN ──→ [Daisy Patch]
                    (5-pin DIN)                          (MIDI Learn)
```

## Quick Start

### 1. Install MIDI Tools
```bash
pip3 install mido python-rtmidi
```

### 2. Build with Debug
```bash
make clean
DEBUG=1 make
```

### 3. Program via ST-Link
```bash
make program
```

### 4. Test MIDI Learning
```bash
# List available MIDI ports
python3 test_midi.py --list

# Send test sequence
python3 test_midi.py --test basic

# Expected output on Daisy display:
# L:1 → L:2 → L:3 → L:4 → (wait 2s) → G:4
```

## VS Code Debugging

### Start Debug
1. **F5** - Start debugging
2. Set breakpoint in `add_note_to_buffer()`
3. Send MIDI notes with test script
4. Inspect variables in Watch panel

### View Debug Log
**Add to Watch panel:**
```
debug_log[0..15]
debug_log_index
note_buffer[0..15]
note_buffer_count
learning_state
```

## Test Scripts

### All Tests
```bash
# Basic 4-note sequence
python3 test_midi.py --test basic

# Pentatonic scale (5 notes)
python3 test_midi.py --test pentatonic

# Maximum buffer (16 notes)
python3 test_midi.py --test max_buffer

# Custom notes
python3 test_midi.py --notes 60 64 67 72
```

### Interactive Mode
```bash
python3 test_midi.py --interactive
Notes> 60 62 64 65
Notes> C4 D4 E4 F4
```

## Debug Log Events

View in debugger to see:
- **DBG_STARTUP** - Firmware boot
- **DBG_NOTE_RECEIVED** - MIDI note (shows note# and count)
- **DBG_LEARNING_START** - Learning began
- **DBG_LEARNING_STOP** - Learning ended (shows count and reason)
- **DBG_PAGE_CHANGE** - Page switched
- **DBG_CLOCK_PULSE** - Clock received

## Automated Test Loop

```bash
# Run all tests in sequence
for test in basic pentatonic chromatic max_buffer; do
    echo "=== Testing: $test ==="
    python3 test_midi.py --test $test
    sleep 3
done
```

## Troubleshooting

**MIDI not working?**
```bash
# Check ports
python3 test_midi.py --list

# Try different port
python3 test_midi.py --port 1 --test basic
```

**ST-Link not found?**
```bash
# Check connection
st-info --probe

# Use DFU instead
make program-dfu
```

**Want to see what's happening?**
- Start VS Code debugger (F5)
- Add watches for `note_buffer` and `learning_state`
- Run test script while debugging

## See Full Guide
For comprehensive testing documentation, see **TESTING.md**
