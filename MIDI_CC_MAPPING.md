# MIDI CC Parameter Mapping

GenerativeGenerator responds to MIDI Control Change (CC) messages for all 12 parameters.

## CC Assignments

These CC numbers were chosen from the "undefined" range to avoid conflicts with standard MIDI controllers:

### Page 0: Performance - Direct Control

| CC# | Parameter | Description |
|-----|-----------|-------------|
| 3   | MOTION    | Biases stepwise motion vs intervallic leaps |
| 9   | MEMORY    | Biases repetition and return to recent material |
| 14  | REGISTER  | Controls octave displacement probability |
| 15  | DIRECTION | Biases ascending vs descending motion |

### Page 1: Performance - Macro & Evolution

| CC# | Parameter      | Description |
|-----|----------------|-------------|
| 20  | PHRASE         | Controls expected phrase length (soft target) |
| 21  | ENERGY         | Macro parameter scaling interval size, octave motion, phrase looseness |
| 22  | STABILITY      | Biases stable vs unstable scale degrees |
| 23  | FORGETFULNESS  | Controls decay rate of learned tendencies |

### Page 2: Structural - Shape & Gravity

| CC# | Parameter          | Description |
|-----|--------------------|-------------|
| 24  | LEAP SHAPE         | Controls exponential decay of interval size |
| 25  | DIRECTION MEMORY   | Controls persistence of melodic direction |
| 26  | HOME REGISTER      | Sets center of register gravity |
| 27  | RANGE WIDTH        | Sets variance of register gravity |

## CC Value Range

- **MIDI CC Values**: 0-127
- **Internal Parameter Values**: 0.0-1.0 (linear mapping)

## Behavior

**Interaction with Hardware Pots:**
- When a MIDI CC is received, the corresponding parameter is **immediately updated**
- The hardware pot is **deactivated** (soft takeover disabled)
- The pot must **catch up** to the CC-set value before it can control the parameter again
- Visual feedback: parameter bar shows CC-set value, hollow rectangle shows pot position

**MIDI Channel:**
- All CC messages are received on **MIDI Channel 1** (default)

## Testing MIDI CC

### Using Python mido Library

```python
import mido
import time

# Open MIDI output to Daisy Patch
port = mido.open_output('Daisy Patch')  # or your MIDI port name

# Set MOTION (CC 3) to 50%
port.send(mido.Message('control_change', control=3, value=64))

# Set ENERGY (CC 21) to 100%
port.send(mido.Message('control_change', control=21, value=127))

# Set MEMORY (CC 9) to 0%
port.send(mido.Message('control_change', control=9, value=0))

port.close()
```

### Using DAW / MIDI Controller

1. Map your MIDI controller knobs/faders to CC 3, 9, 14, 15, 20, 21, 22, 23, 24, 25, 26, 27
2. Set MIDI output to channel 1
3. Adjust controls - parameters update in real-time
4. Hardware pots will show dashed borders until they catch up to CC values

### Using Command Line (sendmidi)

```bash
# Install sendmidi: https://github.com/gbevin/SendMIDI

# Set MOTION to 75%
sendmidi dev "Daisy Patch" cc 3 96

# Set PHRASE to 25%
sendmidi dev "Daisy Patch" cc 20 32

# Sweep ENERGY from 0 to 100%
for i in {0..127}; do
    sendmidi dev "Daisy Patch" cc 21 $i
    sleep 0.01
done
```

## Use Cases

**Live Performance:**
- Use hardware pots for primary control
- Use MIDI CC for parameter automation from DAW
- Use foot controllers mapped to ENERGY/DIRECTION for expressive control

**Studio Recording:**
- Automate all 12 parameters via DAW automation lanes
- Record parameter changes as MIDI CC in real-time
- Edit CC curves in DAW for precise control over time

**External Sequencing:**
- Modular sequencers with MIDI output can control parameters
- Step sequencers can create rhythmic parameter changes
- LFOs/envelopes from other modules via MIDI can modulate parameters

## Standard MIDI CC Reference (Avoided)

For reference, these standard MIDI CCs were **NOT used** to avoid conflicts:

- CC 1: Modulation Wheel
- CC 7: Volume
- CC 10: Pan
- CC 11: Expression
- CC 64: Sustain Pedal
- CC 71-74: Filter controls
- CC 91-93: Effect depth

## Implementation Notes

- CC messages are processed in the same loop as Note On/Off messages
- Parameter changes are **immediate** (no smoothing on CC input)
- Smoothed parameter values are updated directly to match CC value
- Pickup threshold is **not applied** to CC changes (only to pot movement)
- CC values outside the range 0-127 are clamped automatically by MIDI spec
