# Live Phrase Injection

## Feature Overview

**Live phrase injection** allows you to seamlessly transition to a new learned pattern while the module is generating. Simply send new MIDI notes during generation - the module automatically restarts learning and smoothly transitions to the new musical material.

## How It Works

### Previous Behavior (before this feature)
1. Learn pattern → Start generating
2. To learn new pattern: Press encoder button (manual reset) → Send new notes
3. Interrupts the musical flow

### New Behavior (with live injection)
1. Learn pattern → Start generating
2. To learn new pattern: Just send new MIDI notes
3. Module automatically:
   - Stops generating
   - Clears learning buffer
   - Starts learning new pattern
   - Resumes generating with new tendencies

**Result:** Smooth, interactive performance without manual resets!

---

## Visual Feedback

Watch the OLED display during live injection:

```
Before injection:
G:5             120bpm      CLK

During injection (new notes arriving):
L:1             120bpm      CLK
L:2             120bpm      CLK
L:3             120bpm      CLK
L:4             120bpm      CLK

After 2s timeout (or 16 notes):
G:4             120bpm      CLK
```

**LED behavior:**
- Generating: Pulses with clock/trigger
- Learning: Blinks rapidly
- Transitions smoothly between states

---

## Usage Examples

### Example 1: Performance Improvisation

**Scenario:** Live jamming, evolving the melody

```bash
# Learn initial pattern
python3 test_midi.py --port 0 --test pentatonic
# C4, D4, E4, G4, A4

# Wait for "G:5", start triggering notes
# Play for 30 seconds...

# Inject new pattern (just send notes directly)
python3 test_midi.py --port 0 --test chromatic
# C4, C#4, D4, D#4, E4, F4, F#4, G4

# Module instantly switches to learning chromatic pattern
# After 2s timeout, generates with chromatic tendencies
```

### Example 2: Call and Response

**Workflow:**
1. Play a melodic phrase on your MIDI keyboard
2. Module learns it (display shows L:1, L:2, L:3...)
3. Stop playing for 2 seconds
4. Module generates variations (display shows G:X)
5. While it's generating, play a NEW phrase
6. Module learns the new phrase
7. Continues with new material

**Musical flow:**
```
You play:     C D E F
Module plays: D E D F E C D...
You play:     G A B C
Module plays: A G B A C B A...
```

### Example 3: Automated Pattern Changes

**Use case:** Sequenced pattern evolution

```python
import mido
import time

port = mido.open_output('Daisy Patch')

patterns = [
    [60, 62, 64, 65],      # C major tetrachord
    [60, 63, 65, 67],      # C minor arpeggio
    [60, 64, 67, 72],      # C major 7th
    [60, 61, 62, 63, 64]   # Chromatic
]

for pattern in patterns:
    print(f"Injecting pattern: {pattern}")

    # Send notes
    for note in pattern:
        port.send(mido.Message('note_on', note=note, velocity=100))
        time.sleep(0.5)
        port.send(mido.Message('note_off', note=note))
        time.sleep(0.1)

    # Let it generate for 20 seconds
    print("Generating...")
    time.sleep(20)

    # Inject next pattern (no manual reset needed!)
```

---

## Interactive Performance Techniques

### Technique 1: Gradual Evolution
- Send notes similar to current pattern
- Small changes create smooth evolution
- Example: C D E F → D E F G

### Technique 2: Dramatic Shifts
- Send completely different pattern
- Creates sudden stylistic change
- Example: Pentatonic → Chromatic

### Technique 3: Pattern Layering
- Short injection (2-3 notes) = subtle flavor change
- Long injection (16 notes) = complete retraining

### Technique 4: Rhythmic Injection
- Use external sequencer to inject patterns rhythmically
- Creates structured evolution over time
- Example: New pattern every 4 bars

---

## Technical Details

### State Transitions

```
STATE_IDLE
    ↓ (MIDI note received)
STATE_LEARNING
    ↓ (timeout 2s OR buffer full 16 notes)
STATE_GENERATING
    ↓ (MIDI note received) ← NEW!
STATE_LEARNING (buffer cleared, starts fresh)
    ↓ (timeout 2s OR buffer full)
STATE_GENERATING (with new tendencies)
```

### Buffer Behavior

**On injection:**
- Previous learning buffer **cleared completely**
- Learned tendencies **discarded**
- New pattern starts fresh (minimum 4 notes required)

**Why clear instead of append?**
- Clean transition between musical ideas
- Prevents confusion of conflicting tendencies
- Predictable behavior

### Timing Considerations

**Learning timeout:** 2 seconds of silence
- After sending last note, wait 2s
- Or send 16 notes to trigger immediately

**Minimum notes:** 4 notes required
- If you send only 1-3 notes, module won't generate
- Keep sending notes until display shows at least "L:4"

---

## Comparison with Manual Reset

| Feature | Manual Reset | Live Injection |
|---------|-------------|----------------|
| Method | Encoder button press | Send MIDI notes |
| Speed | Stops generation | Seamless transition |
| Feel | Interrupts flow | Continuous performance |
| Use case | Deliberate reset | Live improvisation |

**Both methods are valid!**
- Manual reset: Intentional silence/pause
- Live injection: Continuous musical flow

---

## Tips & Best Practices

### ✅ Do
- Experiment with different pattern lengths (4-16 notes)
- Use similar patterns for smooth evolution
- Combine with parameter changes (MIDI CC)
- Watch display to confirm learning complete ("G:X")

### ⚠️ Avoid
- Sending < 4 notes (won't generate)
- Injecting too frequently (< 2s between injections)
- Expecting exact note sequences (it learns tendencies, not patterns)

### 🎵 Musical Suggestions
- Start simple (4-5 notes), increase complexity gradually
- Use scales from the same family for coherent evolution
- Inject contrasting patterns for dramatic effect
- Combine with ENERGY parameter for dynamic expression

---

## Troubleshooting

**Module doesn't switch to learning when I send notes:**
- Check MIDI connections
- Verify MIDI channel 1
- Try sending note velocity > 0

**Stays in learning state, never generates:**
- Need minimum 4 notes
- Wait 2 seconds of silence
- Or send 16 notes to fill buffer

**Generated notes don't reflect new pattern:**
- Wait for display to show "G:X" (X = new note count)
- Verify notes were received during LEARNING state
- Check MIDI monitor for confirmation

**Want to reset without injecting new pattern:**
- Use encoder button (manual reset)
- Or send MIDI notes then stop immediately

---

## Implementation Notes

**Code change:**
```cpp
// Before: only start learning if IDLE
if(learning_state == STATE_IDLE)
{
    start_learning();
}

// After: also restart learning if GENERATING
if(learning_state == STATE_IDLE || learning_state == STATE_GENERATING)
{
    start_learning();  // Clears buffer, resets to LEARNING
}
```

**Flash overhead:** +8 bytes

**Backwards compatible:** Existing workflows unchanged

---

## Future Enhancements

**Potential additions:**
- [ ] Blend mode: Merge new notes with existing buffer
- [ ] Append mode: Add to buffer without clearing
- [ ] Pattern queue: Store multiple patterns, rotate through
- [ ] Crossfade: Gradually transition between old/new tendencies

---

## Summary

**Live phrase injection makes the module feel like a musical collaborator:**
- No manual resets needed during performance
- Smooth transitions between musical ideas
- Encourages experimentation and improvisation
- Maintains continuous musical flow

**Try it:** Send MIDI notes while generating and experience seamless pattern evolution! 🎶
