# Page 3: Utility Parameters

## Overview

Page 3 provides utility parameters for learning behavior and I/O configuration.

**Access:** Turn encoder to navigate: Page 0 → 1 → 2 → 3 → 0

---

## Parameters

### Parameter 1: LRN TIME (Learning Timeout)
**CC 28** | Range: 0.5s - 10s | Default: 2.0s

Controls how long to wait after the last MIDI note before automatically stopping learning and entering GENERATING mode.

**Mapping:**
```
Parameter Value    Timeout
0.0 (0%)          0.5s  (very fast)
0.158 (16%)       2.0s  (default)
0.5 (50%)         5.0s  (medium)
1.0 (100%)        10.0s (slow)
```

**Formula:** `timeout_ms = 500 + (parameter * 9500)`

**Usage:**
- **Short timeout (0.5-1s):** Quick patterns, rhythmic input, sequenced learning
- **Medium timeout (2-3s):** Default for manual keyboard input
- **Long timeout (5-10s):** Complex phrases, slow playing, contemplative input

**MIDI CC Example:**
```bash
# Set to 0.5s (fast)
python3 test_midi_cc.py --port 0 --set 28 0

# Set to 2s (default)
python3 test_midi_cc.py --port 0 --set 28 20

# Set to 10s (slow)
python3 test_midi_cc.py --port 0 --set 28 127
```

---

### Parameter 2: ECHO (MIDI Echo)
**CC 29** | Range: OFF/ON | Default: OFF

When enabled, notes received during LEARNING are immediately echoed to:
1. **MIDI output** (with original velocity)
2. **Gate output** (100ms pulse)

**Behavior:**
```
Parameter < 0.5  →  Echo OFF (no output during learning)
Parameter ≥ 0.5  →  Echo ON (hear notes as you play)
```

**Use Cases:**

**Echo OFF (default):**
- Silent learning
- Pre-recorded sequences
- Avoid double-triggering external synths

**Echo ON:**
- Hear notes as you play them
- Practice/performance mode
- Immediate auditory feedback
- Monitor what's being learned

**MIDI CC Example:**
```bash
# Turn echo OFF
python3 test_midi_cc.py --port 0 --set 29 0

# Turn echo ON
python3 test_midi_cc.py --port 0 --set 29 127
```

---

### Parameter 3-4: Reserved
**CC 30-31** | Future use

Currently inactive. Reserved for future features (microtonal settings, scale loading, etc.).

---

## Workflow Examples

### Example 1: Quick Pattern Learning
**Goal:** Learn short patterns quickly without waiting

```bash
# Set fast timeout (0.5s)
python3 test_midi_cc.py --port 0 --set 28 0

# Enable echo to hear what you're playing
python3 test_midi_cc.py --port 0 --set 29 127

# Send quick pattern
python3 test_midi.py --port 0 --test basic
# Switches to GENERATING after just 0.5s!
```

### Example 2: Contemplative Input
**Goal:** Play slowly, think between notes

```bash
# Set long timeout (10s)
python3 test_midi_cc.py --port 0 --set 28 127

# Enable echo for feedback
python3 test_midi_cc.py --port 0 --set 29 127

# Play notes with pauses... module waits 10s before generating
```

### Example 3: Silent Sequenced Learning
**Goal:** Pre-recorded sequences without audio feedback

```bash
# Default timeout (2s)
python3 test_midi_cc.py --port 0 --set 28 20

# Disable echo (silent learning)
python3 test_midi_cc.py --port 0 --set 29 0

# Send automated patterns
for test in basic pentatonic chromatic; do
    python3 test_midi.py --port 0 --test $test
    sleep 5
done
```

---

## Page Navigation

**Encoder behavior:**
```
Turn CW:  Page 0 → 1 → 2 → 3 → 0 (wrap)
Turn CCW: Page 0 → 3 → 2 → 1 → 0 (wrap)
Click:    Return to Page 0 (or clear learning buffer when generating)
```

**Visual feedback:**
```
PAGE 3                    ○○○●  ○
─────────────────────────────────
LRN TIME    [███          ]  16%  (2.0s)
ECHO        [             ]  OFF
---         [             ]
---         [             ]
                     120  CLK
```

---

## Technical Details

### Learning Timeout Implementation

**Previous behavior:**
- Hardcoded 2 second timeout
- No user control

**New behavior:**
- Parameter-controlled (Page 3, Pot 1)
- Calculated dynamically each frame
- Range: 500ms - 10000ms

**Code:**
```cpp
float timeout_param = parameters_smoothed[PARAM_LEARN_TIMEOUT];
uint32_t learning_timeout_ms = 500 + (uint32_t)(timeout_param * 9500.0f);
```

### MIDI Echo Implementation

**Trigger condition:**
```cpp
if(learning_state == STATE_LEARNING &&
   parameters_smoothed[PARAM_ECHO_NOTES] > 0.5f)
{
    send_midi_note(note, velocity);
    // Gate pulse: 100ms
}
```

**Gate timing:**
- Echo gate: 100ms (fixed, short)
- Generated note gate: 50% of clock interval (variable)

### Default Values

Set during initialization:
```cpp
parameters[PARAM_LEARN_TIMEOUT] = 0.158f;  // 2.0s
parameters[PARAM_ECHO_NOTES] = 0.0f;        // OFF
```

---

## MIDI CC Summary (Full Module)

| Page | Param | Name       | CC# | Default | Range        |
|------|-------|------------|-----|---------|--------------|
| 0    | 1     | MOTION     | 3   | 50%     | stepwise↔leap|
| 0    | 2     | MEMORY     | 9   | 50%     | novelty↔repeat|
| 0    | 3     | REGISTER   | 14  | 50%     | stay↔octave  |
| 0    | 4     | DIRECTION  | 15  | 50%     | down↔up      |
| 1    | 1     | PHRASE     | 20  | 50%     | short↔long   |
| 1    | 2     | ENERGY     | 21  | 50%     | calm↔intense |
| 1    | 3     | STABILITY  | 22  | 50%     | tense↔stable |
| 1    | 4     | FORGET     | 23  | 50%     | remember↔forget|
| 2    | 1     | LEAP SHP   | 24  | 50%     | flat↔decay   |
| 2    | 2     | DIR MEM    | 25  | 50%     | zigzag↔runs  |
| 2    | 3     | HOME REG   | 26  | 50%     | bass↔treble  |
| 2    | 4     | RANGE      | 27  | 50%     | tight↔wide   |
| **3**| **1** | **LRN TIME**|**28**| **16%**| **0.5s-10s** |
| **3**| **2** | **ECHO**   |**29**| **0%** | **OFF/ON**   |
| 3    | 3     | ---        | 30  | 50%     | reserved     |
| 3    | 4     | ---        | 31  | 50%     | reserved     |

---

## Tips & Tricks

**Fast workflow:**
- LRN TIME = 0.5s
- ECHO = ON
- Send patterns quickly, minimal wait

**Patient exploration:**
- LRN TIME = 10s
- ECHO = ON
- Take your time between notes

**Silent automation:**
- LRN TIME = 2s (default)
- ECHO = OFF
- Script-driven pattern injection

**Live performance:**
- LRN TIME = 1-2s (responsive but not rushed)
- ECHO = ON (hear what you're playing)
- Manual keyboard input with immediate feedback

---

## Future Enhancements (Reserved Parameters)

Parameters 3-4 on Page 3 are reserved for:
- Scala .scl file selection
- Microtonal output mode (MTS/MPE/CV)
- Quantization settings
- I/O configuration
- CV calibration

Stay tuned! 🎶
