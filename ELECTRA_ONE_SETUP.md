# Electra One MK1 Setup for Testing

## Quick Setup

### Option 1: Load via Web Editor (Easiest)

1. **Open the Electra One Editor:**
   - Go to: https://app.electra.one/
   - Connect your Electra One via USB

2. **Load the preset:**
   - Click **"PROJECTS"** in left sidebar
   - Click **"Import Project"**
   - Select: `ElectraOne_NoteTest.eproj`
   - Click **"Upload to Electra"**

3. **Set MIDI routing:**
   - On Electra One screen, press **MENU**
   - Go to **MIDI Control**
   - Set **Port 1 Output** → **USB dev** (or **MIDI IO** if using 5-pin)

4. **Test:**
   - Press the colored pads on Electra One
   - Watch Daisy Patch display show "L:1", "L:2", "L:3", "L:4"
   - After 2 seconds, should show "G:4"

### Option 2: Manual Keyboard Mode (No preset needed)

1. **On Electra One:**
   - Press **MENU**
   - Select **MIDI Routing**
   - Enable **USB dev** → **MIDI IO** (routes USB MIDI out to 5-pin DIN)

2. **Use built-in keyboard:**
   - Electra One MK1 may have a keyboard/note mode
   - Check settings for note transmission

### Option 3: Use as MIDI Interface Only

If you just want to use the Electra One as a pass-through:

1. **Connect:**
   ```
   Computer → USB → Electra One → 5-pin MIDI → Daisy Patch
   ```

2. **Run test script:**
   ```bash
   # List ports (Electra One should appear)
   python3 test_midi.py --list

   # Send notes through Electra One
   python3 test_midi.py --port "Electra One" --test basic
   ```

## Preset Layout

### Page 1: Main Notes
```
┌─────────────────────────────────────┐
│ GENERATIVE GENERATOR TEST           │
│ Press pads to send notes            │
├────┬────┬────┬────┬────┬────┬────┬──┤
│ C4 │ D4 │ E4 │ F4 │ G4 │ A4 │ B4 │C5│
│ 60 │ 62 │ 64 │ 65 │ 67 │ 69 │ 71 │72│
└────┴────┴────┴────┴────┴────┴────┴──┘
│ D5 │ E5 │ F5 │ G5 │    │    │    │  │
│ 74 │ 76 │ 77 │ 79 │    │    │    │  │
└────┴────┴────┴────┴────┴────┴────┴──┘
```

### Page 2: Extended Range
```
┌─────────────────────────────────────┐
│ MORE NOTES                          │
├────┬────┬────┬────┬────┬────┬────┬──┤
│ A5 │ B5 │ C6 │ D6 │    │    │    │  │
│ 81 │ 83 │ 84 │ 86 │    │    │    │  │
└────┴────┴────┴────┴────┴────┴────┴──┘
```

## Testing Sequences

### Test 1: Minimum Buffer (4 notes)
Press quickly: **C4 → D4 → E4 → F4**

**Expected:**
- Display shows "L:1" → "L:2" → "L:3" → "L:4"
- Wait 2 seconds (don't press anything)
- Display switches to "G:4"
- LED stops blinking, starts pulsing with clock

### Test 2: Pentatonic (5 notes)
Press: **C4 → D4 → E4 → G4 → A4**

**Expected:**
- Display counts up to "L:5"
- After timeout → "G:5"

### Test 3: Maximum Buffer (16 notes)
Press all notes in order: **C4 through G5** (12 notes), then **A5 B5 C6 D6** (4 more)

**Expected:**
- Display counts to "L:16"
- IMMEDIATELY switches to "G:16" (no timeout, buffer full)

### Test 4: Melodic Phrase
Press: **C4 → E4 → G4 → F4 → D4 → C4**

**Expected:**
- Captures 6-note melodic phrase
- After timeout → "G:6"

## Troubleshooting

### Electra One not sending notes?
- Check **MENU → MIDI Control**
- Verify **Port 1** is set to correct output
- Make sure preset is loaded (not in default mode)

### Notes not reaching Daisy?
- Verify MIDI cable direction (Electra OUT → Daisy IN)
- Check 5-pin MIDI cable is properly seated
- Try different MIDI channel (patch uses Ch 1 by default)

### Want to use different MIDI channel?
Edit `.eproj` file:
```json
"devices": [
  {
    "id": 1,
    "name": "Daisy Patch",
    "instrumentId": "generic-controls",
    "port": 1,
    "channel": 1     ← Change this (1-16)
  }
]
```

## Debugging with Electra One

The Electra One can also **monitor** MIDI messages:

1. Press **MENU**
2. Select **MIDI Monitor**
3. Watch for **Note On** messages when pressing pads
4. Verify channel and note numbers are correct

## Python Script Through Electra One

You can also use the Python test script to send notes through the Electra One as a MIDI interface:

```bash
# Find Electra One port
python3 test_midi.py --list

# Send automated test
python3 test_midi.py --port "Electra One MIDI 1" --test basic

# Custom sequence
python3 test_midi.py --port "Electra One MIDI 1" --notes 60 64 67 72
```

## Next Steps

Once MIDI learning is verified working:
1. Use ST-Link debugger (F5 in VS Code)
2. Set breakpoint in `add_note_to_buffer()`
3. Press Electra pads
4. Watch `note_buffer[]` populate in debugger
5. Verify learning state changes

## Electra One Resources

- **Web Editor:** https://app.electra.one/
- **Documentation:** https://docs.electra.one/
- **MK1 Manual:** https://electra.one/learn-more
