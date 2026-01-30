#!/usr/bin/env python3
"""
MIDI CC Test Script for GenerativeGenerator
Demonstrates parameter control via MIDI Control Change messages
"""

import mido
import time
import argparse

def list_midi_ports():
    """List available MIDI output ports"""
    print("Available MIDI output ports:")
    for i, port in enumerate(mido.get_output_names()):
        print(f"  {i}: {port}")

def send_cc(port, cc_number, value):
    """Send a MIDI CC message"""
    msg = mido.Message('control_change', control=cc_number, value=value)
    port.send(msg)
    print(f"  CC {cc_number:3d} = {value:3d} ({value/127.0*100:.1f}%)")

def test_all_parameters(port_name):
    """Test all 12 parameters by sweeping through values"""
    cc_map = {
        3:  "MOTION",
        9:  "MEMORY",
        14: "REGISTER",
        15: "DIRECTION",
        20: "PHRASE",
        21: "ENERGY",
        22: "STABILITY",
        23: "FORGETFULNESS",
        24: "LEAP SHAPE",
        25: "DIRECTION MEMORY",
        26: "HOME REGISTER",
        27: "RANGE WIDTH"
    }

    with mido.open_output(port_name) as port:
        print(f"Connected to: {port_name}")
        print("\nTesting all 12 parameters:")
        print("=" * 60)

        for cc_num, param_name in cc_map.items():
            print(f"\n{param_name} (CC {cc_num}):")
            # Set to 0%
            send_cc(port, cc_num, 0)
            time.sleep(0.3)
            # Set to 50%
            send_cc(port, cc_num, 64)
            time.sleep(0.3)
            # Set to 100%
            send_cc(port, cc_num, 127)
            time.sleep(0.5)

        print("\n" + "=" * 60)
        print("Test complete!")

def sweep_parameter(port_name, cc_number, duration=2.0):
    """Sweep a single parameter from 0% to 100%"""
    steps = 128
    delay = duration / steps

    with mido.open_output(port_name) as port:
        print(f"Connected to: {port_name}")
        print(f"Sweeping CC {cc_number} from 0 to 127 over {duration:.1f} seconds...")

        for value in range(steps):
            send_cc(port, cc_number, value)
            time.sleep(delay)

        print("Sweep complete!")

def set_parameters(port_name, cc_values):
    """Set multiple parameters to specific values"""
    with mido.open_output(port_name) as port:
        print(f"Connected to: {port_name}")
        print("Setting parameters:")

        for cc_num, value in cc_values.items():
            send_cc(port, cc_num, value)
            time.sleep(0.1)

        print("Done!")

def performance_preset(port_name, preset_name):
    """Load a performance preset (predefined parameter sets)"""
    presets = {
        "default": {
            3: 64, 9: 64, 14: 64, 15: 64,
            20: 64, 21: 64, 22: 64, 23: 64,
            24: 64, 25: 64, 26: 64, 27: 64
        },
        "smooth": {
            3: 10,   # Low MOTION (stepwise)
            9: 90,   # High MEMORY (repetitive)
            14: 20,  # Low REGISTER (stay in range)
            15: 64,  # Neutral DIRECTION
            20: 90,  # Long PHRASE
            21: 30,  # Low ENERGY (calm)
            22: 80,  # High STABILITY
            23: 20,  # Low FORGETFULNESS
            24: 40, 25: 80, 26: 64, 27: 50
        },
        "chaotic": {
            3: 110,  # High MOTION (leaps)
            9: 20,   # Low MEMORY (novelty)
            14: 100, # High REGISTER (octave jumps)
            15: 64,  # Neutral DIRECTION
            20: 30,  # Short PHRASE
            21: 110, # High ENERGY (intense)
            22: 20,  # Low STABILITY
            23: 100, # High FORGETFULNESS
            24: 80, 25: 30, 26: 64, 27: 100
        },
        "ascending": {
            3: 50, 9: 50, 14: 50,
            15: 127,  # Max DIRECTION (ascending)
            20: 80, 21: 70, 22: 64, 23: 40,
            24: 50, 25: 90, 26: 64, 27: 60
        },
        "descending": {
            3: 50, 9: 50, 14: 50,
            15: 0,    # Min DIRECTION (descending)
            20: 80, 21: 70, 22: 64, 23: 40,
            24: 50, 25: 90, 26: 64, 27: 60
        },
        "energetic": {
            3: 90, 9: 40, 14: 80, 15: 64,
            20: 40,
            21: 127,  # Max ENERGY
            22: 50, 23: 60,
            24: 70, 25: 50, 26: 64, 27: 90
        }
    }

    if preset_name not in presets:
        print(f"Error: Unknown preset '{preset_name}'")
        print(f"Available presets: {', '.join(presets.keys())}")
        return

    print(f"Loading preset: {preset_name}")
    set_parameters(port_name, presets[preset_name])

def interactive_mode(port_name):
    """Interactive mode for manual CC control"""
    print("\nInteractive MIDI CC mode")
    print("Enter: CC_NUMBER VALUE (e.g., '3 64' sets MOTION to 50%)")
    print("Or: 'sweep CC_NUMBER' to sweep parameter")
    print("Or: 'preset NAME' to load preset")
    print("Type 'help' for CC mapping, 'quit' to exit\n")

    cc_map = {
        3: "MOTION", 9: "MEMORY", 14: "REGISTER", 15: "DIRECTION",
        20: "PHRASE", 21: "ENERGY", 22: "STABILITY", 23: "FORGETFULNESS",
        24: "LEAP SHAPE", 25: "DIRECTION MEMORY", 26: "HOME REGISTER", 27: "RANGE WIDTH"
    }

    with mido.open_output(port_name) as port:
        while True:
            try:
                user_input = input("CC> ").strip().lower()

                if user_input in ['quit', 'exit', 'q']:
                    break
                elif user_input == 'help':
                    print("\nCC Mapping:")
                    for cc, name in cc_map.items():
                        print(f"  CC {cc:2d}: {name}")
                    print()
                elif user_input.startswith('sweep '):
                    parts = user_input.split()
                    if len(parts) == 2:
                        cc_num = int(parts[1])
                        sweep_parameter(port_name, cc_num, duration=2.0)
                elif user_input.startswith('preset '):
                    preset = user_input.split(' ', 1)[1]
                    performance_preset(port_name, preset)
                else:
                    parts = user_input.split()
                    if len(parts) == 2:
                        cc_num = int(parts[0])
                        value = int(parts[1])
                        if 0 <= value <= 127:
                            name = cc_map.get(cc_num, f"CC {cc_num}")
                            print(f"Setting {name}:")
                            send_cc(port, cc_num, value)
                        else:
                            print("Error: Value must be 0-127")
                    else:
                        print("Invalid input. Use: CC_NUMBER VALUE")

            except (ValueError, KeyError) as e:
                print(f"Error: {e}")
            except KeyboardInterrupt:
                print("\nExiting...")
                break

def main():
    parser = argparse.ArgumentParser(description='MIDI CC test for GenerativeGenerator')
    parser.add_argument('-l', '--list', action='store_true', help='List available MIDI ports')
    parser.add_argument('-p', '--port', type=str, help='MIDI port name (or index)')
    parser.add_argument('-t', '--test-all', action='store_true', help='Test all 12 parameters')
    parser.add_argument('-s', '--sweep', type=int, metavar='CC', help='Sweep specified CC number')
    parser.add_argument('-d', '--duration', type=float, default=2.0, help='Sweep duration in seconds')
    parser.add_argument('-i', '--interactive', action='store_true', help='Interactive mode')
    parser.add_argument('--preset', type=str, help='Load performance preset')
    parser.add_argument('--set', nargs=2, action='append', metavar=('CC', 'VALUE'),
                       help='Set CC to value (can be used multiple times)')

    args = parser.parse_args()

    # List ports
    if args.list:
        list_midi_ports()
        return

    # Determine port
    if not args.port:
        ports = mido.get_output_names()
        if not ports:
            print("Error: No MIDI output ports found!")
            return

        print("No port specified. Available ports:")
        list_midi_ports()
        print("\nUsing first port by default...")
        port_name = ports[0]
    else:
        # Check if port is an index or name
        try:
            port_idx = int(args.port)
            ports = mido.get_output_names()
            if 0 <= port_idx < len(ports):
                port_name = ports[port_idx]
            else:
                print(f"Error: Port index {port_idx} out of range")
                return
        except ValueError:
            port_name = args.port

    # Execute command
    if args.test_all:
        test_all_parameters(port_name)
    elif args.sweep is not None:
        sweep_parameter(port_name, args.sweep, args.duration)
    elif args.preset:
        performance_preset(port_name, args.preset)
    elif args.set:
        cc_values = {int(cc): int(val) for cc, val in args.set}
        set_parameters(port_name, cc_values)
    elif args.interactive:
        interactive_mode(port_name)
    else:
        print("No action specified. Use -h for help.")
        print("Quick start: --test-all, --interactive, or --preset smooth")

if __name__ == '__main__':
    main()
