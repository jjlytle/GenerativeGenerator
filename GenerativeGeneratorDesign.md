# Generative Melodic Module — claude.md

## PROJECT INTENT

This project is the design and implementation of a **generative melodic instrument**.
It is NOT a traditional sequencer, arpeggiator, or random note generator.

The instrument:
- Learns a short musical gesture (4–16 notes)
- Extracts *tendencies*, not exact notes
- Continues playing indefinitely in a related style
- Is controlled through **probability bias**, not explicit programming
- Is designed to be played live

The firmware must obey the instrument concept defined in the working manual.
If there is a conflict between code convenience and musical intent, **musical intent wins**.

---

## CORE CONCEPTUAL MODEL

The module generates one note per clock pulse.

Each note is chosen through layered decisions:
1. Motion type (repeat, step, leap, octave)
2. Interval size
3. Direction
4. Register placement
5. Phrase context
6. Memory decay

No single parameter ever selects a specific note.

---

## LEARNING MODEL

### Input
- MIDI note input
- CV pitch input
- External clock / gate defines timing

### Learning window
- Minimum: 4 notes
- Maximum: 16 notes
- Learning ends when note input stops

### What is learned
- Interval size distribution
- Direction changes
- Register center
- Repetition frequency

### What is NOT learned
- Exact note order
- Exact timing
- Absolute pitch sequences

Learning may be interrupted or replaced at any time.

---

## PARAMETER MODEL

There are exactly **12 parameters** organized as **3 pages of 4 parameters**.

### Page 1: Performance — Direct Control
These must be safe to tweak live during performance.

**Pot 1: MOTION**
   Biases stepwise motion vs intervallic leaps

**Pot 2: MEMORY**
   Biases repetition and return to recent material

**Pot 3: REGISTER**
   Controls octave displacement probability

**Pot 4: DIRECTION**
   Biases ascending vs descending motion

---

### Page 2: Performance — Macro & Evolution
Broader controls that shape the feel over time.

**Pot 1: PHRASE**
   Controls expected phrase length (soft target)

**Pot 2: ENERGY**
   Macro parameter that scales:
   - interval size
   - octave motion
   - phrase looseness

**Pot 3: STABILITY**
   Biases stable vs unstable scale degrees
   (No enforced harmony or key)

**Pot 4: FORGETFULNESS**
   Controls decay rate of learned tendencies

---

### Page 3: Structural — Shape & Gravity
Long-term behavior and melodic character.

**Pot 1: LEAP SHAPE**
   Controls exponential decay of interval size

**Pot 2: DIRECTION MEMORY**
   Controls persistence of melodic direction

**Pot 3: HOME REGISTER**
   Sets center of register gravity

**Pot 4: RANGE WIDTH**
   Sets variance of register gravity

---

## MATHEMATICAL INTUITION (NON-FORMAL)

- Interval sizes follow weighted distributions
- Direction is conditionally biased based on prior state
- Register is constrained by a soft Gaussian gravity, never hard limits
- Phrase length is a soft target, not a counter
- Memory and forgetting use exponential decay
- Energy scales multiple distributions simultaneously

No parameter should produce silence, lock-ups, or unusable states.

---

## RHYTHM & TIME

- Rhythm is injected exclusively via external clock or gate
- One clock pulse = one melodic decision
- The module is rhythm-agnostic
- No internal rhythm generation unless explicitly added later

---

## INPUT / OUTPUT

### Hardware: Daisy Patch
- 4 Potentiometers (CV inputs)
- 1 Encoder (page navigation + click for learning)
- OLED Display (128x64)

### Inputs
- Gate Input 1: Note trigger (generates note during GENERATING state)
- Gate Input 2: Clock/BPM detection (measures tempo continuously)
- CV Pitch In (future expansion)
- MIDI In (for learning notes)
- Encoder (page navigation + click to reset learning)
- 4 Potentiometers (mapped to 4 parameters per page)

### Outputs
- CV Pitch Out (12-bit, 0-5V, V/OCT calibrated)
- Gate Out (synchronized with generated notes, 50% duty cycle)
- MIDI Out (mirrors note generation)

Optional future:
- Mod CV (energy / entropy / phrase state)
- CV Pitch In for hybrid MIDI+CV learning

---

## DISPLAY (OLED)

The display is **non-numerical**.

### Visual language
- Pitch = vertical position
- Motion = line curvature
- Repetition = trail persistence
- Energy = animation intensity
- Register gravity = soft bands
- Memory decay = fade rate
- Page indicator = subtle marker (not dominant)

### Page Navigation
- Encoder rotation switches between 3 pages
- Page change shows brief parameter name overlay (fades after 1 second)
- Current page indicated by small dots/markers
- Active parameters always visible through animation

The display must teach the instrument implicitly.

---

## NON-GOALS (IMPORTANT)

This module is NOT:
- A step sequencer
- A pattern recorder
- A scale quantizer with memory
- A random generator with knobs
- A preset-driven device

No complex menus.
Parameter values NOT displayed numerically.
Pages are simple and fast to navigate.

---

## DEVELOPMENT CONSTRAINTS

- Code must map cleanly to the conceptual model
- No “magic numbers” without musical justification
- Probability changes must be smooth and continuous
- State must be inspectable for debugging
- Behavior should degrade gracefully under extreme settings

---

## TODO LIST

### DESIGN
- [x] Finalize module name (GenerativeGenerator)
- [x] Lock final parameter names (12 parameters across 3 pages)
- [x] Decide OLED resolution and frame rate (128x64, 30Hz)
- [x] Finalize scale handling strategy (chromatic, no forced quantization)
- [x] Define reset behavior (encoder click clears learning buffer)

### DSP / LOGIC
- [x] Implement learning buffer and extraction (4-16 notes)
- [x] Implement interval distribution model (weighted histogram)
- [x] Implement register gravity (soft Gaussian)
- [x] Implement phrase-length bias (soft targeting)
- [x] Implement memory decay (recent notes buffer)
- [x] Implement energy macro scaling (scales interval/octave/phrase)

### INTERFACE
- [x] Encoder smoothing and hysteresis (page navigation)
- [x] Soft takeover visual feedback (dashed borders, hollow rectangles)
- [x] OLED animation mapping (pitch bars, motion trails)
- [x] Live parameter focus feedback (per-page parameter display)

### TESTING
- [ ] Musical stress test (extreme knob settings)
- [ ] Clock jitter tolerance
- [ ] Long-run stability (hours)
- [ ] MIDI + CV simultaneous input

### DOCUMENTATION
- [ ] Draft 3 manual after parameter pruning
- [ ] Performance recipes
- [ ] Panel quick-start card

---

## GUIDING QUESTION

At every stage, ask:

> “Does this make the module feel more like a collaborator?”

If the answer is no, reconsider the change.
