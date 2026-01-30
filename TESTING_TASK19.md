# Task 19: Single Parameter Testing Guide

## Overview

This guide systematically tests each of the 12 parameters to verify they affect note generation as designed. We'll use a simple learned pattern and vary one parameter at a time.

## Setup

### Hardware Connections
1. **Gate Input 1**: Connect trigger/clock (generates notes)
2. **Gate Input 2**: Connect clock for BPM detection (optional)
3. **MIDI Out**: Connect to synth or MIDI monitor

### Software Setup
```bash
# Terminal 1: Monitor MIDI output
python3 test_midi_monitor.py --port 0 --duration 60

# Terminal 2: Send learning pattern (one time)
python3 test_midi.py --port 0 --test pentatonic
# Sends: C4 (60) → D4 (62) → E4 (64) → G4 (67) → A4 (69)
# Wait for "G:5" on display

# Terminal 3: Control parameters (use this for testing)
python3 test_midi_cc.py --port 0 --interactive
```

### Test Pattern Analysis
**Pentatonic scale input:**
- Notes: C4, D4, E4, G4, A4 (60, 62, 64, 67, 69)
- Intervals: +2, +2, +3, +2 semitones
- Direction: 100% ascending
- Register center: 64.4 (around E4)
- Range: 9 semitones

---

## Page 0: Performance - Direct Control

### Test 1: MOTION (CC 3)
**What it controls:** Biases stepwise motion vs intervallic leaps

**Test procedure:**
```python
# Set all others to default (64)
CC> 9 64    # MEMORY = 50%
CC> 14 64   # REGISTER = 50%
CC> 15 64   # DIRECTION = 50%

# Test MOTION at different values
CC> 3 0     # 0% = Maximum stepwise (small intervals)
# Listen for 30 seconds, observe intervals

CC> 3 64    # 50% = Balanced
# Listen for 30 seconds

CC> 3 127   # 100% = Maximum leaps (large intervals)
# Listen for 30 seconds
```

**Expected behavior:**
- **0%**: Mostly 1-2 semitone steps, smooth melodic motion
- **50%**: Mix of steps and small leaps (2-4 semitones)
- **100%**: Frequent large leaps (5-12 semitones), angular motion

**Pass criteria:**
- [ ] 0% produces mostly stepwise motion
- [ ] 100% produces frequent leaps
- [ ] Transition is smooth/gradual
- [ ] No silence or stuck notes

---

### Test 2: MEMORY (CC 9)
**What it controls:** Biases repetition and return to recent material

**Test procedure:**
```python
# Reset MOTION, keep others default
CC> 3 64    # MOTION = 50%

# Test MEMORY at different values
CC> 9 0     # 0% = Maximum novelty (avoid recent notes)
# Listen for 30 seconds

CC> 9 64    # 50% = Balanced
# Listen for 30 seconds

CC> 9 127   # 100% = Maximum repetition (favor recent notes)
# Listen for 30 seconds
```

**Expected behavior:**
- **0%**: Constantly explores new notes, rarely repeats
- **50%**: Occasional returns to recent material
- **100%**: Frequent repetitions, cycles through recent notes

**Pass criteria:**
- [ ] 0% shows high variety, few immediate repeats
- [ ] 100% shows clear repetition patterns
- [ ] Can hear difference between settings
- [ ] No stuck on single note

---

### Test 3: REGISTER (CC 14)
**What it controls:** Controls octave displacement probability

**Test procedure:**
```python
# Reset MEMORY, keep others default
CC> 9 64    # MEMORY = 50%

# Test REGISTER at different values
CC> 14 0    # 0% = No octave jumps (stay in learned range)
# Listen for 30 seconds, watch pitch display

CC> 14 64   # 50% = Occasional octave jumps
# Listen for 30 seconds

CC> 14 127  # 100% = Frequent octave displacement
# Listen for 30 seconds
```

**Expected behavior:**
- **0%**: All notes stay within ±1 octave of learned range
- **50%**: Occasional ±1 octave jumps
- **100%**: Frequent ±1-2 octave displacements

**Pass criteria:**
- [ ] 0% stays in learned register (around C4-A4)
- [ ] 100% shows clear octave jumps
- [ ] Pitch display shows vertical jumps at 100%
- [ ] No notes outside MIDI range (0-127)

---

### Test 4: DIRECTION (CC 15)
**What it controls:** Biases ascending vs descending motion

**Test procedure:**
```python
# Reset REGISTER, keep others default
CC> 14 64   # REGISTER = 50%

# Test DIRECTION at different values
CC> 15 0    # 0% = Bias descending
# Listen for 30 seconds

CC> 15 64   # 50% = Follow learned tendency (ascending)
# Listen for 30 seconds

CC> 15 127  # 100% = Bias ascending
# Listen for 30 seconds
```

**Expected behavior:**
- **0%**: More downward motion, descending phrases
- **50%**: Follows learned pattern (mostly ascending for pentatonic)
- **100%**: More upward motion, ascending phrases

**Pass criteria:**
- [ ] 0% shows noticeable downward drift
- [ ] 100% shows noticeable upward drift
- [ ] 50% reflects learned ascending tendency
- [ ] Doesn't get stuck at register limits

---

## Page 1: Performance - Macro & Evolution

### Test 5: PHRASE (CC 20)
**What it controls:** Expected phrase length (soft target)

**Test procedure:**
```python
# Set Page 0 to default
CC> 3 64
CC> 9 64
CC> 14 64
CC> 15 64

# Test PHRASE at different values
CC> 20 0    # 0% = Very short phrases (2-3 notes)
# Listen for 30 seconds, feel phrase boundaries

CC> 20 64   # 50% = Medium phrases
# Listen for 30 seconds

CC> 20 127  # 100% = Long phrases (12+ notes)
# Listen for 30 seconds
```

**Expected behavior:**
- **0%**: Short musical gestures, frequent direction changes
- **50%**: Moderate phrase lengths (5-8 notes)
- **100%**: Long continuous phrases, rare direction changes

**Pass criteria:**
- [ ] 0% feels more fragmented
- [ ] 100% feels more continuous/flowing
- [ ] Changes are musical, not mechanical
- [ ] No silence or stuck notes

---

### Test 6: ENERGY (CC 21)
**What it controls:** Macro scaling of interval size, octave motion, phrase looseness

**Test procedure:**
```python
# Reset PHRASE
CC> 20 64   # PHRASE = 50%

# Test ENERGY at different values
CC> 21 0    # 0% = Minimal energy (calm, small intervals)
# Listen for 30 seconds

CC> 21 64   # 50% = Moderate energy
# Listen for 30 seconds

CC> 21 127  # 100% = Maximum energy (intense, large intervals)
# Listen for 30 seconds
```

**Expected behavior:**
- **0%**: Calm, small intervals, stable, predictable
- **50%**: Balanced intensity
- **100%**: Intense, large intervals, more unpredictable

**Pass criteria:**
- [ ] 0% sounds calm and controlled
- [ ] 100% sounds energetic and dynamic
- [ ] Clear difference in character
- [ ] Both extremes remain musical

---

### Test 7: STABILITY (CC 22)
**What it controls:** Biases stable vs unstable scale degrees

**Test procedure:**
```python
# Reset ENERGY
CC> 21 64   # ENERGY = 50%

# Test STABILITY at different values
CC> 22 0    # 0% = Favor unstable notes (tensions)
# Listen for 30 seconds

CC> 22 64   # 50% = Balanced
# Listen for 30 seconds

CC> 22 127  # 100% = Favor stable notes (resolutions)
# Listen for 30 seconds
```

**Expected behavior:**
- **0%**: More chromatic/tense notes (half-steps from learned notes)
- **50%**: Mix of stable and unstable
- **100%**: More notes from learned set (C, D, E, G, A)

**Pass criteria:**
- [ ] 0% introduces more "outside" notes
- [ ] 100% sticks closer to learned notes
- [ ] Both settings remain musical
- [ ] Learned notes still appear at 0%

---

### Test 8: FORGETFULNESS (CC 23)
**What it controls:** Decay rate of learned tendencies

**Test procedure:**
```python
# Reset STABILITY
CC> 22 64   # STABILITY = 50%

# Test FORGETFULNESS at different values
CC> 23 0    # 0% = Remember learned pattern strongly
# Listen for 60 seconds

CC> 23 127  # 100% = Forget quickly, more random
# Listen for 60 seconds (need longer to hear effect)
```

**Expected behavior:**
- **0%**: Stays close to learned interval distribution and tendencies
- **100%**: Gradually diverges, becomes more uniform/random

**Pass criteria:**
- [ ] 0% maintains learned character over time
- [ ] 100% shows gradual drift away from learned style
- [ ] Effect is cumulative (needs time to observe)
- [ ] Doesn't become completely random at 100%

---

## Page 2: Structural - Shape & Gravity

### Test 9: LEAP SHAPE (CC 24)
**What it controls:** Exponential decay of interval size probability

**Test procedure:**
```python
# Set Page 1 to default
CC> 20 64
CC> 21 64
CC> 22 64
CC> 23 64

# Test LEAP SHAPE at different values
CC> 24 0    # 0% = Flat distribution (all intervals equal)
# Listen for 30 seconds

CC> 24 64   # 50% = Moderate decay
# Listen for 30 seconds

CC> 24 127  # 100% = Strong decay (heavily favor small intervals)
# Listen for 30 seconds
```

**Expected behavior:**
- **0%**: Large leaps as common as small steps
- **50%**: Preference for smaller intervals
- **100%**: Strong preference for small intervals, rare large leaps

**Pass criteria:**
- [ ] 0% shows more large leaps than default
- [ ] 100% shows mostly small steps
- [ ] Works with MOTION parameter
- [ ] Smooth transition between values

---

### Test 10: DIRECTION MEMORY (CC 25)
**What it controls:** Persistence of melodic direction

**Test procedure:**
```python
# Reset LEAP SHAPE
CC> 24 64   # LEAP SHAPE = 50%

# Test DIRECTION MEMORY at different values
CC> 25 0    # 0% = Frequent direction changes (zigzag)
# Listen for 30 seconds

CC> 25 64   # 50% = Moderate persistence
# Listen for 30 seconds

CC> 25 127  # 100% = Strong persistence (long runs)
# Listen for 30 seconds
```

**Expected behavior:**
- **0%**: Frequent up/down changes, angular melodic contour
- **50%**: Balanced, natural phrase contours
- **100%**: Long ascending or descending runs

**Pass criteria:**
- [ ] 0% changes direction frequently
- [ ] 100% maintains direction for multiple notes
- [ ] Works with DIRECTION parameter
- [ ] Melodic contours feel natural

---

### Test 11: HOME REGISTER (CC 26)
**What it controls:** Center of register gravity

**Test procedure:**
```python
# Reset DIRECTION MEMORY
CC> 25 64   # DIRECTION MEMORY = 50%

# Test HOME REGISTER at different values
CC> 26 0    # 0% = Low register center (pull toward bass)
# Listen for 30 seconds, watch pitch display

CC> 26 64   # 50% = Mid register (around learned center)
# Listen for 30 seconds

CC> 26 127  # 100% = High register center (pull toward treble)
# Listen for 30 seconds
```

**Expected behavior:**
- **0%**: Notes gravitate toward lower octaves
- **50%**: Stays near learned register center (E4)
- **100%**: Notes gravitate toward higher octaves

**Pass criteria:**
- [ ] 0% shows lower average pitch
- [ ] 100% shows higher average pitch
- [ ] Pitch display shows register shift
- [ ] Still uses full MIDI range

---

### Test 12: RANGE WIDTH (CC 27)
**What it controls:** Variance of register gravity (tightness)

**Test procedure:**
```python
# Reset HOME REGISTER
CC> 26 64   # HOME REGISTER = 50%

# Test RANGE WIDTH at different values
CC> 27 0    # 0% = Tight range (strong gravity)
# Listen for 30 seconds, watch pitch display

CC> 27 64   # 50% = Moderate range
# Listen for 30 seconds

CC> 27 127  # 100% = Wide range (weak gravity)
# Listen for 30 seconds
```

**Expected behavior:**
- **0%**: Notes cluster tightly around register center
- **50%**: Natural spread (similar to learned range)
- **100%**: Wide pitch exploration, full keyboard usage

**Pass criteria:**
- [ ] 0% shows narrow pitch range
- [ ] 100% uses much wider pitch range
- [ ] Works with HOME REGISTER
- [ ] Doesn't violate MIDI limits

---

## Test Results Summary

After completing all 12 tests, fill out:

### Parameter Functionality
- [ ] All 12 parameters affect generation
- [ ] Effects match design specifications
- [ ] No parameters cause crashes or silence
- [ ] Smooth transitions between values

### Musical Quality
- [ ] 0% settings produce musical results
- [ ] 50% settings produce musical results
- [ ] 100% settings produce musical results
- [ ] Parameters interact well (no conflicts)

### Technical Stability
- [ ] No MIDI note errors (all 0-127)
- [ ] No stuck notes
- [ ] Display updates correctly
- [ ] Gate output synchronized

### Visual Feedback
- [ ] Parameter bars update with CC changes
- [ ] Pitch display tracks generated notes
- [ ] BPM display accurate
- [ ] No display glitches

---

## Quick Test Commands

**Reset to defaults (all 50%):**
```bash
python3 test_midi_cc.py --port 0 --preset default
```

**Test single parameter (example: MOTION):**
```bash
# Set to 0%
python3 test_midi_cc.py --port 0 --set 3 0

# Set to 100%
python3 test_midi_cc.py --port 0 --set 3 127
```

**Load test preset:**
```bash
# Smooth (stepwise, repetitive)
python3 test_midi_cc.py --port 0 --preset smooth

# Chaotic (leaps, novelty)
python3 test_midi_cc.py --port 0 --preset chaotic
```

---

## Notes

- Each test should run for at least 30 seconds
- Use headphones or good monitoring
- Watch pitch display for visual confirmation
- Listen for musical character, not just technical correctness
- Note any unexpected behaviors for investigation
- Compare against design specifications in GenerativeGeneratorDesign.md

---

## Completion

When all 12 parameters pass their tests:
- [ ] Mark Task 19 complete
- [ ] Document any unexpected behaviors
- [ ] Move to Task 20 (full parameter interaction)
