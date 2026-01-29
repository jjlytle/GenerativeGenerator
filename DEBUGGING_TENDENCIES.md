# Debugging Learned Tendencies

## Task 9 Complete: Interval Distribution Analysis

The firmware now analyzes learned notes and extracts musical tendencies:

- **Interval distribution** - Histogram of interval sizes (0-12 semitones)
- **Direction tendencies** - Ascending vs descending motion counts
- **Register analysis** - Min, max, center, and range
- **Most common intervals** - First and second most frequent

## Inspecting Analysis Results

### Via VS Code Debugger

1. **Start debugging:**
   ```
   Press F5 in VS Code
   ```

2. **Send MIDI notes:**
   ```bash
   python3 test_midi.py --port 0 --test pentatonic
   # Sends: C4 (60) → D4 (62) → E4 (64) → G4 (67) → A4 (69)
   ```

3. **Wait for "G:5" on display** (learning complete)

4. **In VS Code Debug Console, inspect:**
   ```
   -exec p tendencies
   -exec p tendencies.interval_counts[0..12]
   -exec p tendencies.register_center
   -exec p tendencies.register_range
   -exec p tendencies.most_common_interval
   ```

### Watch Panel Variables

Add these to the **Watch** panel (right side in VS Code debug view):

```
note_buffer[0..15]           // Captured notes
note_buffer_count            // How many notes
learning_state               // 0=IDLE, 1=LEARNING, 2=GENERATING

tendencies.interval_counts   // Histogram of interval sizes
tendencies.ascending_count   // How many ascending intervals
tendencies.descending_count  // How many descending intervals
tendencies.repeat_count      // How many repeated notes
tendencies.register_center   // Average pitch (MIDI note)
tendencies.register_min      // Lowest note
tendencies.register_max      // Highest note
tendencies.register_range    // Max - min
tendencies.most_common_interval      // Most frequent interval size
tendencies.second_common_interval    // Second most frequent
```

## Example Analysis Results

### Test: Pentatonic Scale
**Input:** C4 (60) → D4 (62) → E4 (64) → G4 (67) → A4 (69)

**Expected Analysis:**
```
note_buffer = [60, 62, 64, 67, 69, ...]
note_buffer_count = 5

Intervals between notes:
  62-60 = 2 (ascending whole step)
  64-62 = 2 (ascending whole step)
  67-64 = 3 (ascending minor third)
  69-67 = 2 (ascending whole step)

tendencies.interval_counts:
  [0] = 0  (unison/repeat)
  [1] = 0  (semitone)
  [2] = 3  (whole step) ← most common!
  [3] = 1  (minor third)
  [4..12] = 0

tendencies.ascending_count = 4
tendencies.descending_count = 0
tendencies.repeat_count = 0

tendencies.register_center = 64.4  (average of 60,62,64,67,69)
tendencies.register_min = 60 (C4)
tendencies.register_max = 69 (A4)
tendencies.register_range = 9 (semitones)

tendencies.most_common_interval = 2 (whole step)
tendencies.second_common_interval = 3 (minor third)
```

### Test: Octave Leap
**Input:** C3 (48) → C4 (60) → C5 (72) → C4 (60) → C3 (48)

**Expected Analysis:**
```
Intervals:
  60-48 = 12 (octave up)
  72-60 = 12 (octave up)
  60-72 = -12 (octave down)
  48-60 = -12 (octave down)

tendencies.interval_counts:
  [12] = 4  (all octave leaps!)

tendencies.ascending_count = 2
tendencies.descending_count = 2
tendencies.repeat_count = 0

tendencies.register_center = 57.6
tendencies.register_min = 48
tendencies.register_max = 72
tendencies.register_range = 24 (2 octaves)

tendencies.most_common_interval = 12 (octave)
```

### Test: Chromatic
**Input:** C4 (60) → C#4 (61) → D4 (62) → D#4 (63) → E4 (64) → ...

**Expected Analysis:**
```
tendencies.interval_counts:
  [1] = 7  (all semitones!)

tendencies.ascending_count = 7
tendencies.descending_count = 0

tendencies.most_common_interval = 1 (semitone)
```

## Automated Testing

Run different sequences and inspect results:

```bash
# Test different interval patterns
for test in basic pentatonic chromatic octave_leap max_buffer; do
    echo "=== Testing: $test ==="
    python3 test_midi.py --port 0 --test $test
    sleep 3
    # Inspect in debugger after each test
done
```

## Breakpoint Locations

Set breakpoints at these lines to step through analysis:

1. **Line ~290** - `analyze_learned_notes()` start
2. **Line ~300** - After register analysis complete
3. **Line ~315** - After interval analysis complete
4. **Line ~330** - After finding most common intervals

## Common Patterns to Verify

### Pattern 1: Stepwise Motion (Small Intervals)
- `interval_counts[1]` and `[2]` should be high
- `most_common_interval` should be 1 or 2

### Pattern 2: Arpeggios (Larger Intervals)
- `interval_counts[3..5]` should dominate (thirds/fourths)
- `most_common_interval` likely 3, 4, or 5

### Pattern 3: Octave Leaps
- `interval_counts[12]` should be highest
- `register_range` should be >= 12

### Pattern 4: Repetitive
- `repeat_count` should be > 0
- `interval_counts[0]` should be high

## Next Steps

This analysis data will be used in **Task 10** to:
- Weight interval choices based on learned distribution
- Bias direction based on ascending/descending counts
- Apply register gravity using min/max/center
- Favor most common intervals

The generative algorithm will use **probability** rather than rules - it will tend toward the learned style without exactly repeating it.

## Debugging Tips

**If all counts are zero:**
- Check `note_buffer_count` - need at least 2 notes
- Verify `analyze_learned_notes()` is being called
- Set breakpoint in analysis function

**If intervals seem wrong:**
- Print `note_buffer[i+1] - note_buffer[i]` for each pair
- Remember intervals can be negative (descending)
- Absolute value is used for histogram

**If register values unexpected:**
- Check MIDI note range (0-127)
- Verify `register_center` calculation (should be average)
- Make sure min/max are updating correctly
