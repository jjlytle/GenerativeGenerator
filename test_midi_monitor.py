#!/usr/bin/env python3
"""
MIDI Monitor for GenerativeGenerator
Monitors MIDI output from Daisy Patch to verify note generation
"""

import mido
import time
import argparse
from collections import Counter

def monitor_midi(port_name, duration=None):
    """
    Monitor MIDI messages from a port

    Args:
        port_name: Name of MIDI input port
        duration: Monitor duration in seconds (None = infinite)
    """
    try:
        with mido.open_input(port_name) as port:
            print(f"Monitoring MIDI from: {port_name}")
            print("Press Ctrl+C to stop\n")

            start_time = time.time()
            note_counter = Counter()
            interval_list = []
            last_note = None

            try:
                while True:
                    msg = port.poll()

                    if msg:
                        timestamp = time.time() - start_time

                        if msg.type == 'note_on':
                            note = msg.note
                            velocity = msg.velocity

                            # Calculate interval from last note
                            interval_str = ""
                            if last_note is not None:
                                interval = note - last_note
                                interval_list.append(interval)
                                direction = "↑" if interval > 0 else "↓" if interval < 0 else "="
                                interval_str = f" [{direction}{abs(interval)}st]"

                            note_name = mido.note_number_to_name(note) if hasattr(mido, 'note_number_to_name') else f"Note {note}"
                            print(f"[{timestamp:6.2f}s] Note On:  {note:3d} ({note_name:>4s}) vel={velocity:3d}{interval_str}")

                            note_counter[note] += 1
                            last_note = note

                        elif msg.type == 'note_off':
                            note = msg.note
                            note_name = mido.note_number_to_name(note) if hasattr(mido, 'note_number_to_name') else f"Note {note}"
                            print(f"[{timestamp:6.2f}s] Note Off: {note:3d} ({note_name:>4s})")

                        elif msg.type == 'clock':
                            pass  # Don't spam with clock messages
                        else:
                            print(f"[{timestamp:6.2f}s] {msg.type}: {msg}")

                    # Check duration
                    if duration and (time.time() - start_time) > duration:
                        break

                    time.sleep(0.001)  # Small sleep to prevent CPU spinning

            except KeyboardInterrupt:
                print("\n\nMonitoring stopped by user")

            # Print statistics
            if note_counter:
                print("\n" + "="*60)
                print("STATISTICS")
                print("="*60)

                print(f"\nTotal notes received: {sum(note_counter.values())}")

                print("\nNote frequency:")
                for note, count in note_counter.most_common():
                    note_name = mido.note_number_to_name(note) if hasattr(mido, 'note_number_to_name') else f"Note {note}"
                    bar = "█" * min(count, 50)
                    print(f"  {note:3d} ({note_name:>4s}): {count:3d} {bar}")

                if interval_list:
                    print("\nInterval statistics:")
                    interval_counter = Counter(interval_list)
                    for interval, count in sorted(interval_counter.items(), key=lambda x: abs(x[0])):
                        direction = "↑" if interval > 0 else "↓" if interval < 0 else "="
                        bar = "█" * min(count, 30)
                        print(f"  {direction}{abs(interval):2d}st: {count:3d} {bar}")

                    # Direction stats
                    ascending = sum(1 for i in interval_list if i > 0)
                    descending = sum(1 for i in interval_list if i < 0)
                    repeats = sum(1 for i in interval_list if i == 0)

                    print(f"\nDirection:")
                    print(f"  Ascending:  {ascending:3d} ({100*ascending/len(interval_list):.1f}%)")
                    print(f"  Descending: {descending:3d} ({100*descending/len(interval_list):.1f}%)")
                    print(f"  Repeats:    {repeats:3d} ({100*repeats/len(interval_list):.1f}%)")

                    # Range
                    print(f"\nPitch range: {min(note_counter.keys())} to {max(note_counter.keys())} ({max(note_counter.keys()) - min(note_counter.keys())} semitones)")

    except IOError as e:
        print(f"Error: Could not open MIDI port '{port_name}'")
        print(f"Details: {e}")
        return False

    return True

def list_midi_ports():
    """List available MIDI input ports"""
    print("Available MIDI input ports:")
    for i, port in enumerate(mido.get_input_names()):
        print(f"  {i}: {port}")

def main():
    parser = argparse.ArgumentParser(description='Monitor MIDI output from GenerativeGenerator')
    parser.add_argument('-l', '--list', action='store_true', help='List available MIDI ports')
    parser.add_argument('-p', '--port', type=str, help='MIDI port name (or index)')
    parser.add_argument('-d', '--duration', type=float, help='Monitor duration in seconds (default: infinite)')

    args = parser.parse_args()

    # List ports
    if args.list:
        list_midi_ports()
        return

    # Determine port
    if not args.port:
        ports = mido.get_input_names()
        if not ports:
            print("Error: No MIDI input ports found!")
            return

        print("No port specified. Available ports:")
        list_midi_ports()
        print("\nUsing first port by default...")
        port_name = ports[0]
    else:
        # Check if port is an index or name
        try:
            port_idx = int(args.port)
            ports = mido.get_input_names()
            if 0 <= port_idx < len(ports):
                port_name = ports[port_idx]
            else:
                print(f"Error: Port index {port_idx} out of range")
                return
        except ValueError:
            port_name = args.port

    # Monitor
    monitor_midi(port_name, args.duration)

if __name__ == '__main__':
    main()
