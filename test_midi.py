#!/usr/bin/env python3
"""
MIDI Test Script for GenerativeGenerator
Sends test MIDI sequences to verify learning buffer functionality
"""

import mido
import time
import argparse

def list_midi_ports():
    """List available MIDI output ports"""
    print("Available MIDI output ports:")
    for i, port in enumerate(mido.get_output_names()):
        print(f"  {i}: {port}")

def send_note_sequence(port_name, notes, note_duration=0.5, gap_duration=0.1):
    """
    Send a sequence of MIDI notes

    Args:
        port_name: Name of MIDI output port
        notes: List of MIDI note numbers (0-127)
        note_duration: Duration of each note in seconds
        gap_duration: Gap between notes in seconds
    """
    try:
        with mido.open_output(port_name) as port:
            print(f"Connected to: {port_name}")
            print(f"Sending {len(notes)} notes: {notes}")

            for i, note in enumerate(notes):
                # Send Note On
                msg = mido.Message('note_on', note=note, velocity=100)
                port.send(msg)
                print(f"  {i+1}. Note On: {note} (MIDI name: {mido.note_name(note)})")

                time.sleep(note_duration)

                # Send Note Off
                msg = mido.Message('note_off', note=note, velocity=0)
                port.send(msg)
                print(f"     Note Off: {note}")

                time.sleep(gap_duration)

            print(f"\nSequence complete! Sent {len(notes)} notes.")
            print("Waiting 2.5s for learning timeout...")
            time.sleep(2.5)
            print("Learning should have stopped. Module should now be in GENERATING mode.")

    except IOError as e:
        print(f"Error: Could not open MIDI port '{port_name}'")
        print(f"Details: {e}")
        return False

    return True

def run_test_sequence(port_name, test_name="basic"):
    """Run predefined test sequences"""

    sequences = {
        "basic": {
            "notes": [60, 62, 64, 65],  # C, D, E, F (4 notes minimum)
            "description": "Basic 4-note ascending scale"
        },
        "pentatonic": {
            "notes": [60, 62, 64, 67, 69],  # C, D, E, G, A (5 notes)
            "description": "Pentatonic scale"
        },
        "octave_leap": {
            "notes": [48, 60, 72, 60, 48],  # C2, C3, C4, C3, C2 (5 notes with octave leaps)
            "description": "Octave leaps"
        },
        "chromatic": {
            "notes": [60, 61, 62, 63, 64, 65, 66, 67],  # Chromatic scale (8 notes)
            "description": "Chromatic scale"
        },
        "max_buffer": {
            "notes": [60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84, 86],  # 16 notes (max)
            "description": "Maximum buffer size (16 notes)"
        },
        "melodic": {
            "notes": [60, 64, 67, 65, 62, 60],  # C, E, G, F, D, C (6 notes melodic)
            "description": "Melodic phrase"
        }
    }

    if test_name not in sequences:
        print(f"Error: Unknown test '{test_name}'")
        print(f"Available tests: {', '.join(sequences.keys())}")
        return False

    seq = sequences[test_name]
    print(f"\n{'='*60}")
    print(f"Running test: {test_name}")
    print(f"Description: {seq['description']}")
    print(f"{'='*60}\n")

    return send_note_sequence(port_name, seq['notes'])

def interactive_mode(port_name):
    """Interactive mode for manual note entry"""
    print("\nInteractive MIDI mode")
    print("Enter MIDI note numbers (0-127) separated by spaces")
    print("Or use note names: C4, D#5, etc.")
    print("Type 'quit' to exit\n")

    with mido.open_output(port_name) as port:
        while True:
            try:
                user_input = input("Notes> ").strip()
                if user_input.lower() in ['quit', 'exit', 'q']:
                    break

                # Parse input (numbers or note names)
                notes = []
                for item in user_input.split():
                    try:
                        # Try parsing as number first
                        note = int(item)
                        if 0 <= note <= 127:
                            notes.append(note)
                        else:
                            print(f"Warning: Note {note} out of range (0-127)")
                    except ValueError:
                        # Try parsing as note name
                        try:
                            note = mido.note_name_to_number(item)
                            notes.append(note)
                        except:
                            print(f"Warning: Could not parse '{item}'")

                if notes:
                    send_note_sequence(port_name, notes)

            except KeyboardInterrupt:
                print("\nExiting...")
                break

def main():
    parser = argparse.ArgumentParser(description='MIDI test script for GenerativeGenerator')
    parser.add_argument('-l', '--list', action='store_true', help='List available MIDI ports')
    parser.add_argument('-p', '--port', type=str, help='MIDI port name (or index)')
    parser.add_argument('-t', '--test', type=str, default='basic',
                       help='Test sequence: basic, pentatonic, octave_leap, chromatic, max_buffer, melodic')
    parser.add_argument('-i', '--interactive', action='store_true', help='Interactive mode')
    parser.add_argument('-n', '--notes', type=int, nargs='+', help='Custom note sequence (space-separated)')

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

    # Interactive mode
    if args.interactive:
        interactive_mode(port_name)
        return

    # Custom notes
    if args.notes:
        print(f"Sending custom sequence: {args.notes}")
        send_note_sequence(port_name, args.notes)
        return

    # Run test sequence
    run_test_sequence(port_name, args.test)

if __name__ == '__main__':
    main()
