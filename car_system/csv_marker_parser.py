#!/usr/bin/env python3
"""
sysview_csv_marker_parser.py
=============================
Parse a SEGGER SystemView CSV export and extract performance marker
statistics (Start Marker / Stop Marker events).

For each marker ID the script computes:
  - Period  : time between consecutive Start Marker events
  - Duration: time between Start Marker and Stop Marker
  - Pass/Fail against configurable timing constraints

SystemView CSV format expected:
  - Separator      : comma
  - Time column    : seconds in "0.000 000 000" format (spaces as thousands sep)
  - Marker events  : "Start Marker 0x000..." / "Stop Marker 0x000..." in Event column

Usage:
  python sysview_csv_marker_parser.py recording.csv [options]

  python sysview_csv_marker_parser.py recording.csv \\
      --marker 0 --period-ms 10.0 --tolerance-ms 0.5 --max-duration-ms 3.0 \\
      --marker 1 --period-ms 20.0 --tolerance-ms 1.0

  python sysview_csv_marker_parser.py recording.csv --list-markers
  python sysview_csv_marker_parser.py recording.csv --output-csv stats.csv
"""

import argparse
import csv
import os
import re
import sys
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple


# ── Data structures ───────────────────────────────────────────────────────────

@dataclass
class MarkerEvent:
    kind:       str     # 'start' | 'stop'
    marker_id:  int
    time_s:     float   # absolute time in seconds


@dataclass
class MarkerInterval:
    marker_id: int
    start_s:   float
    stop_s:    float

    @property
    def duration_us(self) -> float:
        return (self.stop_s - self.start_s) * 1_000_000.0

    @property
    def duration_ms(self) -> float:
        return (self.stop_s - self.start_s) * 1_000.0


@dataclass
class MarkerConstraint:
    marker_id:       int
    period_ms:       Optional[float] = None
    tolerance_ms:    Optional[float] = None
    max_duration_ms: Optional[float] = None


# ── Time parser ───────────────────────────────────────────────────────────────

def parse_time_s(time_str: str) -> float:
    """
    Parse SystemView time string to seconds.
    SystemView exports time as "0.000 000 000" (spaces as visual separators).
    Examples:
      "0.000 123 456"  ->  0.000123456
      "1.234 567 890"  ->  1.234567890
      "0.010 000 000"  ->  0.01
    """
    # Remove all spaces then parse as float
    return float(time_str.replace(" ", ""))


# ── Marker ID parser ──────────────────────────────────────────────────────────

# Matches: "Start Marker 0x00000000" or "Stop Marker 0x00000001" etc.
# The hex value is the marker ID as set in SEGGER_SYSVIEW_MarkStart(id)
RE_START_MARKER = re.compile(r"Start\s+Marker\s+0x([0-9a-fA-F]+)", re.IGNORECASE)
RE_STOP_MARKER  = re.compile(r"Stop\s+Marker\s+0x([0-9a-fA-F]+)",  re.IGNORECASE)


def parse_marker_event(event_str: str) -> Optional[Tuple[str, int]]:
    """
    Parse the Event column string.
    Returns (kind, marker_id) or None if not a marker event.
    kind is 'start' or 'stop'.
    """
    m = RE_START_MARKER.search(event_str)
    if m:
        return 'start', int(m.group(1), 16)

    m = RE_STOP_MARKER.search(event_str)
    if m:
        return 'stop', int(m.group(1), 16)

    return None


# ── CSV parser ────────────────────────────────────────────────────────────────

def parse_csv(filepath: str,
              verbose:  bool = False) -> Tuple[List[MarkerEvent], List[str]]:
    """
    Parse a SystemView CSV export file.
    Returns (events, warnings).

    Skips header lines that do not look like data rows (SystemView prepends
    several metadata lines before the actual column headers).

    Expected column headers (case-insensitive, flexible ordering):
      Index, Time [s], Event, Detail, Context
    """
    events:   List[MarkerEvent] = []
    warnings: List[str]         = []

    with open(filepath, newline='', encoding='utf-8-sig') as f:
        raw_lines = f.readlines()

    # ── Find the header row ───────────────────────────────────────────────────
    # SystemView CSV files have metadata lines before the column header.
    # We find the header by looking for a line containing "Time" and "Event".

    header_line_idx = None
    for i, line in enumerate(raw_lines):
        line_lower = line.lower()
        if "time" in line_lower and "event" in line_lower:
            header_line_idx = i
            break

    if header_line_idx is None:
        print("ERROR: Could not find header row in CSV file.", file=sys.stderr)
        print("       Expected a row containing 'Time' and 'Event' columns.",
              file=sys.stderr)
        sys.exit(1)

    if verbose:
        print(f"[CSV] Header found at line {header_line_idx + 1}: "
              f"{raw_lines[header_line_idx].strip()}")

    # ── Parse data rows ───────────────────────────────────────────────────────

    data_lines = raw_lines[header_line_idx:]
    reader     = csv.DictReader(data_lines)

    # Normalise column names: strip whitespace, lowercase for lookup
    def find_col(fieldnames, *candidates) -> Optional[str]:
        """Find the first matching column name from candidates."""
        if fieldnames is None:
            return None
        for name in fieldnames:
            for candidate in candidates:
                if candidate.lower() in name.lower():
                    return name
        return None

    # We need these after the first next() call — defer until first row
    time_col  = None
    event_col = None
    row_count = 0

    for row in reader:
        row_count += 1

        # Discover column names from first row
        if time_col is None:
            time_col  = find_col(row.keys(), "time")
            event_col = find_col(row.keys(), "event")
            if verbose:
                print(f"[CSV] Columns detected: time='{time_col}'  "
                      f"event='{event_col}'")

        if time_col is None or event_col is None:
            warnings.append(f"Row {row_count}: could not identify Time/Event columns")
            continue

        time_str  = row.get(time_col,  "").strip()
        event_str = row.get(event_col, "").strip()

        if not time_str or not event_str:
            continue

        # Parse time
        try:
            time_s = parse_time_s(time_str)
        except ValueError:
            warnings.append(f"Row {row_count}: cannot parse time '{time_str}'")
            continue

        # Parse marker event
        result = parse_marker_event(event_str)
        if result is None:
            continue  # not a marker event -- skip silently

        kind, marker_id = result
        events.append(MarkerEvent(kind=kind, marker_id=marker_id, time_s=time_s))

        if verbose:
            print(f"  [row {row_count:6d}] {kind:5s}  "
                  f"marker_id={marker_id}  time={time_s:.9f}s")

    return events, warnings


# ── Analysis ──────────────────────────────────────────────────────────────────

def build_intervals(events: List[MarkerEvent]) -> Dict[int, List[MarkerInterval]]:
    """
    Match Start / Stop pairs per marker ID.
    Unmatched Start events (no following Stop) are silently ignored.
    """
    pending:   Dict[int, MarkerEvent]          = {}
    intervals: Dict[int, List[MarkerInterval]] = {}

    for evt in events:
        if evt.kind == 'start':
            pending[evt.marker_id] = evt
        elif evt.kind == 'stop':
            if evt.marker_id in pending:
                start_evt = pending.pop(evt.marker_id)
                intervals.setdefault(evt.marker_id, []).append(MarkerInterval(
                    marker_id = evt.marker_id,
                    start_s   = start_evt.time_s,
                    stop_s    = evt.time_s,
                ))

    return intervals


def compute_periods(events: List[MarkerEvent]) -> Dict[int, List[float]]:
    """
    Compute periods (in ms) between consecutive Start Marker events
    per marker ID.
    """
    last_start: Dict[int, float]       = {}
    periods:    Dict[int, List[float]] = {}

    for evt in events:
        if evt.kind == 'start':
            mid = evt.marker_id
            if mid in last_start:
                period_ms = (evt.time_s - last_start[mid]) * 1000.0
                if period_ms > 0:
                    periods.setdefault(mid, []).append(period_ms)
            last_start[mid] = evt.time_s

    return periods


# ── Statistics ────────────────────────────────────────────────────────────────

def compute_stats(values: List[float]) -> dict:
    if not values:
        return {}
    n        = len(values)
    mean     = sum(values) / n
    min_v    = min(values)
    max_v    = max(values)
    variance = sum((v - mean) ** 2 for v in values) / n
    std      = variance ** 0.5
    return {"n": n, "mean": mean, "min": min_v, "max": max_v, "std": std}


def print_stats(label: str, values: List[float], unit: str = "ms") -> dict:
    stats = compute_stats(values)
    if not stats:
        print(f"  {label}: no data")
        return {}
    print(f"  {label}:")
    print(f"    n={stats['n']}  "
          f"mean={stats['mean']:.6f}{unit}  "
          f"min={stats['min']:.6f}{unit}  "
          f"max={stats['max']:.6f}{unit}  "
          f"std={stats['std']:.6f}{unit}")
    return stats


# ── Validation ────────────────────────────────────────────────────────────────

def validate(periods_ms:  List[float],
             durations_ms: List[float],
             constraint:  MarkerConstraint) -> bool:
    all_pass = True

    # Period validation
    if constraint.period_ms is not None and constraint.tolerance_ms is not None:
        violations = [
            (i, p) for i, p in enumerate(periods_ms)
            if abs(p - constraint.period_ms) > constraint.tolerance_ms
        ]
        pass_count = len(periods_ms) - len(violations)
        print(f"  Period check  : {pass_count}/{len(periods_ms)} within "
              f"{constraint.period_ms:.3f}ms ± {constraint.tolerance_ms:.3f}ms")
        for i, p in violations:
            deviation = abs(p - constraint.period_ms)
            print(f"    FAIL period[{i}]: {p:.6f}ms  "
                  f"deviation={deviation:.6f}ms  "
                  f"(limit ±{constraint.tolerance_ms}ms)")
        if violations:
            all_pass = False

    # Duration validation
    if constraint.max_duration_ms is not None and durations_ms:
        violations = [
            (i, d) for i, d in enumerate(durations_ms)
            if d > constraint.max_duration_ms
        ]
        pass_count = len(durations_ms) - len(violations)
        print(f"  Duration check: {pass_count}/{len(durations_ms)} within "
              f"{constraint.max_duration_ms:.3f}ms")
        for i, d in violations:
            print(f"    FAIL duration[{i}]: {d:.6f}ms > "
                  f"max={constraint.max_duration_ms:.3f}ms")
        if violations:
            all_pass = False

    return all_pass


# ── CLI ───────────────────────────────────────────────────────────────────────

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Parse SEGGER SystemView CSV export and compute marker statistics.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  List all marker IDs found:
    %(prog)s recording.csv --list-markers

  Show statistics for all markers:
    %(prog)s recording.csv

  Validate two markers with timing constraints:
    %(prog)s recording.csv \\
        --marker 0 --period-ms 10.0 --tolerance-ms 0.5 --max-duration-ms 3.0 \\
        --marker 1 --period-ms 20.0 --tolerance-ms 1.0

  Export per-interval stats to CSV:
    %(prog)s recording.csv --output-csv stats.csv

  Show every decoded marker event:
    %(prog)s recording.csv --verbose
""")

    p.add_argument("csv_file",
                   help="Path to SystemView CSV export file")
    p.add_argument("--verbose", "-v", action="store_true",
                   help="Print every decoded marker event")
    p.add_argument("--list-markers", action="store_true",
                   help="List all marker IDs found and exit")
    p.add_argument("--marker", type=int, action="append", dest="markers",
                   metavar="ID",
                   help="Marker ID to analyze (repeatable, decimal)")
    p.add_argument("--period-ms", type=float, action="append",
                   dest="periods", metavar="MS",
                   help="Expected period in ms (one per --marker)")
    p.add_argument("--tolerance-ms", type=float, action="append",
                   dest="tolerances", metavar="MS",
                   help="Period tolerance in ms (one per --marker)")
    p.add_argument("--max-duration-ms", type=float, action="append",
                   dest="max_durations", metavar="MS",
                   help="Max Start to Stop duration in ms (one per --marker)")
    p.add_argument("--output-csv", type=str, default=None,
                   help="Export per-interval statistics to a CSV file")

    return p.parse_args()


# ── Main ──────────────────────────────────────────────────────────────────────

def main() -> None:
    args = parse_args()

    if not os.path.isfile(args.csv_file):
        print(f"ERROR: File not found: {args.csv_file}", file=sys.stderr)
        sys.exit(1)

    # ── Parse CSV ─────────────────────────────────────────────────────────────

    print(f"Parsing {args.csv_file} ...")
    events, warnings = parse_csv(args.csv_file, verbose=args.verbose)

    for w in warnings:
        print(f"WARNING: {w}")

    print(f"Found {len(events)} marker event(s)")
    print()

    if not events:
        print("No marker events found in CSV.")
        print("Check that 'Start Marker' / 'Stop Marker' events are present")
        print("in the Event column of the CSV export.")
        sys.exit(0)

    # ── List marker IDs ───────────────────────────────────────────────────────

    all_ids = sorted(set(e.marker_id for e in events))

    if args.list_markers:
        print("Marker IDs found:")
        print(f"  {'ID (dec)':>10}  {'ID (hex)':>10}  "
              f"{'starts':>8}  {'stops':>8}")
        print(f"  {'--------':>10}  {'--------':>10}  "
              f"{'------':>8}  {'-----':>8}")
        for mid in all_ids:
            starts = sum(1 for e in events if e.marker_id == mid and e.kind == 'start')
            stops  = sum(1 for e in events if e.marker_id == mid and e.kind == 'stop')
            print(f"  {mid:>10}  {mid:#010x}  {starts:>8}  {stops:>8}")
        return

    # ── Build analysis structures ─────────────────────────────────────────────

    intervals = build_intervals(events)
    periods   = compute_periods(events)

    # ── Build constraints ─────────────────────────────────────────────────────

    constraints: Dict[int, MarkerConstraint] = {}
    if args.markers:
        for i, mid in enumerate(args.markers):
            c = MarkerConstraint(marker_id=mid)
            if args.periods       and i < len(args.periods):
                c.period_ms = args.periods[i]
            if args.tolerances    and i < len(args.tolerances):
                c.tolerance_ms = args.tolerances[i]
            if args.max_durations and i < len(args.max_durations):
                c.max_duration_ms = args.max_durations[i]
            constraints[mid] = c

    target_ids   = sorted(constraints.keys()) if constraints else all_ids
    overall_pass = True

    # ── Per-marker analysis ───────────────────────────────────────────────────

    for mid in target_ids:
        print(f"{'=' * 60}")
        print(f"Marker {mid}  ({mid:#010x})")
        print(f"{'-' * 60}")

        mid_periods_ms   = periods.get(mid, [])
        mid_intervals    = intervals.get(mid, [])
        mid_durations_ms = [iv.duration_ms for iv in mid_intervals]

        if not mid_periods_ms and not mid_durations_ms:
            print("  No start/stop data found for this marker ID.")
            print()
            continue

        period_stats   = print_stats("Periods  (Start to Start)", mid_periods_ms, "ms")
        duration_stats = print_stats("Durations (Start to Stop)",  mid_durations_ms, "ms")

        if mid in constraints:
            print("  Validation:")
            passed = validate(mid_periods_ms, mid_durations_ms, constraints[mid])
            print(f"  {'PASS' if passed else 'FAIL'}  Marker {mid}")
            if not passed:
                overall_pass = False

        print()

    # ── Summary ───────────────────────────────────────────────────────────────

    if constraints:
        print(f"{'=' * 60}")
        if overall_pass:
            print("PASS  All markers within timing constraints")
        else:
            print("FAIL  One or more markers violated timing constraints")
        print()

    # ── CSV export ────────────────────────────────────────────────────────────

    if args.output_csv:
        with open(args.output_csv, "w", newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                "marker_id", "marker_id_hex",
                "interval_index",
                "start_s", "stop_s",
                "duration_ms", "period_ms"
            ])

            for mid in sorted(intervals.keys()):
                mid_intervals  = intervals[mid]
                mid_periods_ms = periods.get(mid, [])

                for i, iv in enumerate(mid_intervals):
                    period_ms = mid_periods_ms[i] if i < len(mid_periods_ms) else ""
                    writer.writerow([
                        mid,
                        f"{mid:#010x}",
                        i,
                        f"{iv.start_s:.9f}",
                        f"{iv.stop_s:.9f}",
                        f"{iv.duration_ms:.6f}",
                        f"{period_ms:.6f}" if period_ms != "" else "",
                    ])

        print(f"Per-interval statistics exported to {args.output_csv}")

    # Exit code suitable for CI pipelines
    if constraints:
        sys.exit(0 if overall_pass else 1)


if __name__ == "__main__":
    main()