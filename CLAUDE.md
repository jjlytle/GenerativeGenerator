# GenerativeGenerator - Claude Code Context

This file provides guidance to Claude Code when working on the GenerativeGenerator project.

## Project Overview

**GenerativeGenerator** is a generative melodic instrument for Daisy Patch that learns musical gestures and continues playing in a related style through probability bias rather than exact pattern playback.

**Hardware:** Daisy Patch (full module)
- 4 Potentiometers
- 1 Encoder (navigation + click)
- OLED Display (128x64)
- 2 Gate Inputs
- 1 Gate Output
- 4 Audio I/O
- MIDI In/Out

**Repository:** https://github.com/jjlytle/GenerativeGenerator

## Current Status

### ✅ Completed Tasks
1. **Basic Hardware Initialization** - All hardware verified working
2. **3-Page Parameter System** - Encoder navigation between 3 pages of 4 parameters

### 🚧 In Progress
3. Parameter storage for 12 parameters (4 per page)

### Design Document
See `GenerativeGeneratorDesign.md` for full conceptual model and parameter definitions.

## Parameter Layout (3 Pages × 4 Parameters)

### Page 1: Performance - Direct Control
1. **MOTION** - Biases stepwise motion vs intervallic leaps
2. **MEMORY** - Biases repetition and return to recent material
3. **REGISTER** - Controls octave displacement probability
4. **DIRECTION** - Biases ascending vs descending motion

### Page 2: Performance - Macro & Evolution
5. **PHRASE** - Controls expected phrase length (soft target)
6. **ENERGY** - Macro parameter scaling interval size, octave motion, phrase looseness
7. **STABILITY** - Biases stable vs unstable scale degrees
8. **FORGETFULNESS** - Controls decay rate of learned tendencies

### Page 3: Structural - Shape & Gravity
9. **LEAP SHAPE** - Controls exponential decay of interval size
10. **DIRECTION MEMORY** - Controls persistence of melodic direction
11. **HOME REGISTER** - Sets center of register gravity
12. **RANGE WIDTH** - Sets variance of register gravity

## Build & Development Workflow

### Quick Commands

```bash
# Build
make clean && make

# Flash via USB (put device in DFU mode first: hold BOOT, press RESET, release BOOT)
make program-dfu

# Flash via STLink
make program

# Check DFU status
dfu-util -l
```

### Git Workflow

```bash
# After completing a task
git add -A
git commit -m "Task X: Description"
git push

# View history
git log --oneline

# Revert to previous task
git checkout <commit-hash>
git checkout main  # Return to latest
```

### Current Build Stats
- **FLASH**: 90,304 bytes / 128 KB (68.90%)
- **SRAM**: 51,612 bytes / 512 KB (9.84%)

## Code Architecture

### Main Components

**Hardware Interface:**
- `DaisyPatch hw` - Hardware object
- `UpdateControls()` - Reads pots, encoder, gates (called every loop)
- `UpdateDisplay()` - Updates OLED at 30Hz
- `AudioCallback()` - Real-time audio processing

**Page System:**
- `current_page` (0-2) - Current page index
- `page_names[3][4]` - Parameter names for all pages
- Encoder rotation switches pages with wraparound
- Encoder click resets to page 0
- Page change overlay shows for 2 seconds

**Display Layout:**
```
PAGE 1                    ●○○
─────────────────────────────
MOTION      [███████     ]
MEMORY      [████        ]
REGISTER    [█████████   ]
DIRECTION   [███         ]
                        GT
```

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

## Development Constraints

From `GenerativeGeneratorDesign.md`:
- Code must map cleanly to the conceptual model
- No "magic numbers" without musical justification
- Probability changes must be smooth and continuous
- State must be inspectable for debugging
- Behavior should degrade gracefully under extreme settings

## Testing Checklist

**Hardware Test (Task 1):**
- [x] 4 Potentiometers respond
- [x] Encoder increments/decrements
- [x] OLED displays correctly
- [x] Gate input detected
- [x] LED blinks with gate
- [x] Audio passthrough works

**Page Navigation (Task 2):**
- [x] Encoder switches pages
- [x] Page wraps around (3 → 1 → 2 → 3)
- [x] Encoder click returns to page 1
- [x] Page indicator dots show current page
- [x] Parameter names display per page
- [x] Page overlay shows for 2 seconds

## Next Steps (Remaining Tasks)

3. Create parameter storage for 12 parameters
4. Implement CV/potentiometer reading with smoothing
5. Create basic OLED display with page indicator
6. Implement clock/gate input detection
7. Create note learning buffer (4-16 notes)
8. Implement interval distribution analysis
9. Build motion decision system
10. Implement register gravity
11. Create memory/repetition bias
12. Implement phrase length targeting
13. Build energy macro scaling
14. Implement CV pitch output
15. Implement gate output
16. Create visual pitch display
17. Add OLED animation
18-20. Testing phases

## Debugging Tips

**Common Issues:**
- Squiggly lines in VS Code → Reload window (c_cpp_properties.json already configured)
- Build errors → Check libDaisy and DaisySP are built: `cd ../../ && ./ci/build_libs.sh`
- Flash fails → Verify DFU mode (hold BOOT, press RESET, release BOOT)
- Display not updating → Check frame_counter logic and 30Hz timing

**Memory Monitor:**
- Keep FLASH under 128 KB (bootloader region)
- Monitor build output for memory usage
- If approaching limits, use `-Os` optimization

## Resources

- **Design Doc**: `GenerativeGeneratorDesign.md`
- **Main Daisy Guide**: `../../claude.md`
- **GitHub**: https://github.com/jjlytle/GenerativeGenerator
- **Daisy Docs**: https://daisy.audio/
- **libDaisy API**: https://electro-smith.github.io/libDaisy/
- **DaisySP API**: https://electro-smith.github.io/DaisySP/

## Guiding Principle

> "Does this make the module feel more like a collaborator?"

If the answer is no, reconsider the change.
