#!/usr/bin/env python3
"""
sysview_csv_marker_parser.py
=============================
Parse a SEGGER SystemView CSV export and compute preemption-aware
timing statistics for performance markers.

Preemption is detected by correlating marker intervals with
Task Run / Task Stop and ISR Enter / ISR Exit events present
in the same CSV export.

Expected CSV column layout (0-based index):
  0  : row index
  1  : time in "0.000 000 000" format (seconds, spaces as visual separators)
  2  : context  -- task name or ISR name ("Engine", "ISR 37", "Idle", ...)
  3  : event    -- event type string (see below)
  4  : detail   -- human-readable description (optional, used for info only)
  5+ : ignored

Event type strings recognised (column 3):
  "Start Marker 0x<hex>"  -- marker start
  "Stop Marker 0x<hex>"   -- marker stop
  "Task Run"              -- task scheduled in (starts executing)
  "Task Stop"             -- task scheduled out (stops executing)
  "ISR Enter"             -- ISR starts executing
  "ISR Exit"              -- ISR stops executing
  "Task Info"             -- task registration (used to build name list)
  "Task Ready"            -- wakeup notification (ignored for timing)

Measurements computed per marker interval:
  elapsed_us     : stop - start (wall clock, includes preemption)
  exec_us        : sum of Task Run/Stop windows for the marked task
  preemption_us  : elapsed - exec
  isr_us         : sum of ISR Enter/Exit windows within the interval
  other_task_us  : sum of other task execution within the interval
  preemption_pct : preemption_us / elapsed_us * 100

Usage:
  python sysview_csv_marker_parser.py recording.csv [options]

  python sysview_csv_marker_parser.py recording.csv \\
      --marker 0 --period-ms 10.0 --tolerance-ms 0.5 --max-exec-ms 3.0 \\
      --marker 1 --period-ms 20.0 --tolerance-ms 1.0

  python sysview_csv_marker_parser.py recording.csv --list-markers
  python sysview_csv_marker_parser.py recording.csv --verbose
  python sysview_csv_marker_parser.py recording.csv --output-csv stats.csv
"""

import argparse
import csv
import os
import re
import sys
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple


# ── Column indices ────────────────────────────────────────────────────────────

COL_TIME    = 1
COL_CONTEXT = 2
COL_EVENT   = 3
COL_DETAIL  = 4
MIN_COLUMNS = 4   # minimum columns expected per data row


# ── Event type patterns ───────────────────────────────────────────────────────

RE_START_MARKER = re.compile(r"^Start\s+Marker\s+0x([0-9a-fA-F]+)", re.IGNORECASE)
RE_STOP_MARKER  = re.compile(r"^Stop\s+Marker\s+0x([0-9a-fA-F]+)",  re.IGNORECASE)


def classify_event(event_str: str) -> str:
    """
    Classify the event string (column 3) into an internal kind token.
    Returns one of:
      'marker_start', 'marker_stop',
      'task_run', 'task_stop',
      'isr_enter', 'isr_exit',
      'task_info', 'task_ready',
      'unknown'
    """
    s = event_str.strip()
    if RE_START_MARKER.match(s):  return 'marker_start'
    if RE_STOP_MARKER.match(s):   return 'marker_stop'
    sl = s.lower()
    if sl == 'task run':          return 'task_run'
    if sl == 'task stop':         return 'task_stop'
    if sl == 'isr enter':         return 'isr_enter'
    if sl == 'isr exit':          return 'isr_exit'
    if sl == 'task info':         return 'task_info'
    if sl == 'task ready':        return 'task_ready'
    return 'unknown'


# ── Time parser ───────────────────────────────────────────────────────────────

def parse_time_us(time_str: str) -> float:
    """
    Parse '0.000 183 105' -> 183.105 us
    Strips spaces then converts seconds to microseconds.
    """
    return float(time_str.replace(" ", "")) * 1_000_000.0


# ── Raw event ─────────────────────────────────────────────────────────────────

@dataclass
class RawEvent:
    row:       int
    time_us:   float
    context:   str             # col 2: task name or ISR name
    kind:      str             # classified event kind
    event_str: str             # original col 3 string
    detail:    str             # col 4 string
    marker_id: Optional[int]   # set for marker events only


# ── Interval data structures ──────────────────────────────────────────────────

@dataclass
class ExecWindow:
    task:     str
    start_us: float
    stop_us:  float

    @property
    def duration_us(self) -> float:
        return self.stop_us - self.start_us


@dataclass
class IsrWindow:
    isr_name: str
    start_us: float
    stop_us:  float

    @property
    def duration_us(self) -> float:
        return self.stop_us - self.start_us


@dataclass
class MarkerInterval:
    marker_id:     int
    task_name:     str        # context at the time of Start Marker
    start_us:      float
    stop_us:       float
    period_us:     Optional[float] = None

    # Filled by correlation
    exec_us:       float = 0.0
    isr_us:        float = 0.0
    other_task_us: float = 0.0
    preemption_us: float = 0.0

    exec_windows:       List[ExecWindow] = field(default_factory=list)
    other_exec_windows: List[ExecWindow] = field(default_factory=list)
    isr_windows:        List[IsrWindow]  = field(default_factory=list)

    @property
    def elapsed_us(self) -> float:
        return self.stop_us - self.start_us

    @property
    def preemption_pct(self) -> float:
        return (self.preemption_us / self.elapsed_us * 100.0
                if self.elapsed_us > 0 else 0.0)


# ── Statistics ────────────────────────────────────────────────────────────────

@dataclass
class Stats:
    n:          int   = 0
    mean_us:    float = 0.0
    min_us:     float = float('inf')
    max_us:     float = 0.0
    std_us:     float = 0.0
    violations: int   = 0

    @staticmethod
    def compute(values:       List[float],
                limit_us:     float = 0.0,
                tolerance_us: float = 0.0,
                is_period:    bool  = False) -> 'Stats':
        if not values:
            return Stats()
        n        = len(values)
        mean     = sum(values) / n
        min_v    = min(values)
        max_v    = max(values)
        variance = sum((v - mean) ** 2 for v in values) / n
        std      = variance ** 0.5
        violations = 0
        if limit_us > 0:
            for v in values:
                if is_period:
                    if abs(v - limit_us) > tolerance_us:
                        violations += 1
                else:
                    if v > limit_us:
                        violations += 1
        return Stats(n=n, mean_us=mean, min_us=min_v, max_us=max_v,
                     std_us=std, violations=violations)

    def print(self, label: str) -> None:
        if self.n == 0:
            print(f"  {label}: no data")
            return
        viol = (f"  violations={self.violations}/{self.n}"
                if self.violations > 0 else "")
        print(f"  {label}:")
        print(f"    n={self.n}  "
              f"mean={self.mean_us/1000:.6f}ms  "
              f"min={self.min_us/1000:.6f}ms  "
              f"max={self.max_us/1000:.6f}ms  "
              f"std={self.std_us/1000:.6f}ms"
              f"{viol}")


# ── CSV parser ────────────────────────────────────────────────────────────────

def parse_csv(filepath: str,
              verbose:   bool = False) -> Tuple[List[RawEvent], List[str]]:
    """
    Parse the SystemView CSV export into a flat list of RawEvents.

    Handles the multi-column format:
      index, time, context, event, detail, ...

    Skips metadata/header lines at the top of the file.
    The data rows are identified by having a numeric value in column 0.
    """
    events:   List[RawEvent] = []
    warnings: List[str]      = []

    with open(filepath, newline='', encoding='utf-8-sig') as f:
        reader = csv.reader(f)
        row_num = 0

        for raw_row in reader:
            row_num += 1

            # Strip all fields
            row = [field.strip().strip('"') for field in raw_row]

            # Skip rows that do not have enough columns
            if len(row) < MIN_COLUMNS:
                continue

            # Data rows have a numeric index in column 0
            try:
                int(row[0])
            except ValueError:
                continue   # header or metadata line

            # Parse time
            try:
                time_us = parse_time_us(row[COL_TIME])
            except (ValueError, IndexError):
                warnings.append(f"Row {row_num}: cannot parse time '{row[COL_TIME]}'")
                continue

            context   = row[COL_CONTEXT]
            event_str = row[COL_EVENT]
            detail    = row[COL_DETAIL] if len(row) > COL_DETAIL else ""

            kind = classify_event(event_str)

            # Extract marker ID for marker events
            marker_id = None
            if kind == 'marker_start':
                m = RE_START_MARKER.match(event_str)
                if m:
                    marker_id = int(m.group(1), 16)
            elif kind == 'marker_stop':
                m = RE_STOP_MARKER.match(event_str)
                if m:
                    marker_id = int(m.group(1), 16)

            evt = RawEvent(
                row       = int(row[0]),
                time_us   = time_us,
                context   = context,
                kind      = kind,
                event_str = event_str,
                detail    = detail,
                marker_id = marker_id,
            )
            events.append(evt)

            if verbose and kind != 'unknown':
                print(f"  [{int(row[0]):5d}] {time_us:14.3f}us  "
                      f"{kind:15s}  ctx={context:20s}  "
                      f"mid={marker_id if marker_id is not None else '-':>4}  "
                      f"{detail[:40]}")

    return events, warnings


# ── Correlation engine ────────────────────────────────────────────────────────

def build_exec_windows(events: List[RawEvent]) -> List[ExecWindow]:
    """
    Build ExecWindow list from Task Run / Task Stop event pairs.

    Task Run  (col 3 = "Task Run")  : context = task that starts running
    Task Stop (col 3 = "Task Stop") : context = "Idle" (or next task),
                                      the stopping task was the last running one

    We track the currently running task by watching Task Run events.
    A Task Stop ends the current task's window.
    """
    windows:        List[ExecWindow] = []
    current_task:   Optional[str]    = None
    current_start:  float            = 0.0

    for evt in events:
        if evt.kind == 'task_run':
            # Close previous window if any
            if current_task is not None:
                windows.append(ExecWindow(
                    task     = current_task,
                    start_us = current_start,
                    stop_us  = evt.time_us,
                ))
            current_task  = evt.context
            current_start = evt.time_us

        elif evt.kind == 'task_stop':
            if current_task is not None:
                windows.append(ExecWindow(
                    task     = current_task,
                    start_us = current_start,
                    stop_us  = evt.time_us,
                ))
            current_task = None

    return windows


def build_isr_windows(events: List[RawEvent]) -> List[IsrWindow]:
    """
    Build IsrWindow list from ISR Enter / ISR Exit event pairs.
    ISR name is taken from context column (e.g. "ISR 37").
    """
    windows:       List[IsrWindow]  = []
    pending:       Dict[str, float] = {}   # isr_name -> start_us

    for evt in events:
        if evt.kind == 'isr_enter':
            pending[evt.context] = evt.time_us

        elif evt.kind == 'isr_exit':
            if evt.context in pending:
                windows.append(IsrWindow(
                    isr_name = evt.context,
                    start_us = pending.pop(evt.context),
                    stop_us  = evt.time_us,
                ))

    return windows


def build_marker_intervals(events:       List[RawEvent],
                            exec_windows: List[ExecWindow],
                            isr_windows:  List[IsrWindow]
                            ) -> Dict[int, List[MarkerInterval]]:
    """
    Build MarkerInterval list per marker ID, then correlate with
    exec and ISR windows to compute execution and preemption times.
    """
    # ── Pass 1: raw marker intervals ─────────────────────────────────────────

    pending_start: Dict[int, RawEvent] = {}
    raw_intervals: List[MarkerInterval] = []
    last_start_us: Dict[int, float]     = {}

    for evt in events:
        if evt.kind == 'marker_start' and evt.marker_id is not None:
            pending_start[evt.marker_id] = evt

        elif evt.kind == 'marker_stop' and evt.marker_id is not None:
            mid = evt.marker_id
            if mid in pending_start:
                start_evt = pending_start.pop(mid)

                period_us = None
                if mid in last_start_us:
                    period_us = start_evt.time_us - last_start_us[mid]
                last_start_us[mid] = start_evt.time_us

                raw_intervals.append(MarkerInterval(
                    marker_id = mid,
                    task_name = start_evt.context,  # context at marker start
                    start_us  = start_evt.time_us,
                    stop_us   = evt.time_us,
                    period_us = period_us,
                ))

    # ── Pass 2: correlate exec/ISR windows ────────────────────────────────────

    result: Dict[int, List[MarkerInterval]] = {}

    for iv in raw_intervals:
        ws = iv.start_us
        we = iv.stop_us

        own_exec_us   = 0.0
        other_task_us = 0.0
        isr_total_us  = 0.0
        iv_exec:  List[ExecWindow] = []
        iv_other: List[ExecWindow] = []
        iv_isr:   List[IsrWindow]  = []

        # Task execution windows
        for ew in exec_windows:
            # Clamp to marker interval
            ol_start = max(ew.start_us, ws)
            ol_stop  = min(ew.stop_us,  we)
            if ol_stop <= ol_start:
                continue
            ol_us = ol_stop - ol_start

            if ew.task == iv.task_name:
                own_exec_us += ol_us
                iv_exec.append(ExecWindow(ew.task, ol_start, ol_stop))
            else:
                other_task_us += ol_us
                iv_other.append(ExecWindow(ew.task, ol_start, ol_stop))

        # ISR windows
        for iw in isr_windows:
            ol_start = max(iw.start_us, ws)
            ol_stop  = min(iw.stop_us,  we)
            if ol_stop <= ol_start:
                continue
            ol_us = ol_stop - ol_start
            isr_total_us += ol_us
            iv_isr.append(IsrWindow(iw.isr_name, ol_start, ol_stop))

        # If no task exec events at all, fall back to elapsed = exec
        # (happens when SystemView task event recording is disabled)
        if not exec_windows:
            own_exec_us = iv.elapsed_us

        iv.exec_us            = own_exec_us
        iv.isr_us             = isr_total_us
        iv.other_task_us      = other_task_us
        iv.preemption_us      = max(0.0, iv.elapsed_us - own_exec_us)
        iv.exec_windows       = iv_exec
        iv.other_exec_windows = iv_other
        iv.isr_windows        = iv_isr

        result.setdefault(iv.marker_id, []).append(iv)

    return result


# ── Constraint ────────────────────────────────────────────────────────────────

@dataclass
class MarkerConstraint:
    marker_id:       int
    period_ms:       Optional[float] = None
    tolerance_ms:    Optional[float] = None
    max_exec_ms:     Optional[float] = None
    max_elapsed_ms:  Optional[float] = None


# ── Analysis ──────────────────────────────────────────────────────────────────

def analyse_marker(mid:        int,
                   intervals:  List[MarkerInterval],
                   constraint: Optional[MarkerConstraint],
                   verbose:    bool = False) -> bool:

    print(f"{'=' * 68}")
    print(f"Marker {mid}  ({mid:#010x})")

    task_names = sorted(set(iv.task_name for iv in intervals))
    print(f"Task   : {', '.join(task_names)}")
    print(f"{'─' * 68}")

    # Collect measurement lists
    periods_us   = [iv.period_us    for iv in intervals
                    if iv.period_us is not None]
    elapsed_list = [iv.elapsed_us   for iv in intervals]
    exec_list    = [iv.exec_us      for iv in intervals]
    preempt_list = [iv.preemption_us for iv in intervals]
    isr_list     = [iv.isr_us       for iv in intervals]
    other_list   = [iv.other_task_us for iv in intervals]
    pct_list     = [iv.preemption_pct for iv in intervals]

    # Constraint limits
    lim_period_us  = constraint.period_ms    * 1000.0 \
                     if constraint and constraint.period_ms    else 0.0
    tol_us         = constraint.tolerance_ms * 1000.0 \
                     if constraint and constraint.tolerance_ms else 0.0
    lim_exec_us    = constraint.max_exec_ms  * 1000.0 \
                     if constraint and constraint.max_exec_ms  else 0.0
    lim_elapsed_us = constraint.max_elapsed_ms * 1000.0 \
                     if constraint and constraint.max_elapsed_ms else 0.0

    # Print statistics
    Stats.compute(periods_us,
                  lim_period_us, tol_us,
                  is_period=True).print(
        "Period         (start → start, wall clock)")

    # Task name in the execution label makes it immediately clear which task
    task_label = task_names[0] if len(task_names) == 1 else ', '.join(task_names)

    Stats.compute(exec_list,
                  lim_exec_us).print(
        f"Execution time  ({task_label} running, excl. preemption)")

    Stats.compute(elapsed_list,
                  lim_elapsed_us).print(
        f"Elapsed time    ({task_label} wall clock, incl. preemption)")

    Stats.compute(preempt_list).print(
        f"Preemption      (elapsed - execution for {task_label})")

    if any(v > 0 for v in isr_list):
        Stats.compute(isr_list).print(
            "ISR time        (within marker interval)")

        # Break down by ISR name
        isr_by_name: Dict[str, List[float]] = {}
        for iv in intervals:
            for iw in iv.isr_windows:
                isr_by_name.setdefault(iw.isr_name, []).append(iw.duration_us)
        for name, durations in sorted(isr_by_name.items()):
            s = Stats.compute(durations)
            print(f"    {name}: n={s.n}  "
                  f"mean={s.mean_us/1000:.6f}ms  "
                  f"max={s.max_us/1000:.6f}ms")

    if any(v > 0 for v in other_list):
        Stats.compute(other_list).print(
            f"Other tasks     (preempted {task_label} within interval)")

        # Break down by task name — who is preempting this task
        other_by_name: Dict[str, List[float]] = {}
        for iv in intervals:
            for ew in iv.other_exec_windows:
                other_by_name.setdefault(ew.task, []).append(ew.duration_us)
        for name, durations in sorted(other_by_name.items()):
            s = Stats.compute(durations)
            print(f"    {name}: n={s.n}  "
                  f"mean={s.mean_us/1000:.6f}ms  "
                  f"max={s.max_us/1000:.6f}ms")

    if pct_list:
        mean_pct = sum(pct_list) / len(pct_list)
        max_pct  = max(pct_list)
        print(f"  Preemption %  : mean={mean_pct:.1f}%  max={max_pct:.1f}%")

    # Per-interval detail table
    if verbose:
        print()
        print(f"  {'#':>4}  {'period_ms':>11}  {'exec_ms':>10}  "
              f"{'elapsed_ms':>11}  {'preempt_ms':>11}  {'preempt%':>9}")
        print(f"  {'─'*4}  {'─'*11}  {'─'*10}  "
              f"{'─'*11}  {'─'*11}  {'─'*9}")
        for i, iv in enumerate(intervals):
            period_s = f"{iv.period_us/1000:11.6f}" \
                       if iv.period_us is not None else f"{'─':>11}"
            print(f"  {i:>4}  {period_s}  "
                  f"{iv.exec_us/1000:10.6f}  "
                  f"{iv.elapsed_us/1000:11.6f}  "
                  f"{iv.preemption_us/1000:11.6f}  "
                  f"{iv.preemption_pct:8.1f}%")
        print()

    # Validation
    all_pass = True
    if constraint:
        print(f"  Validation:")

        checks: List[Tuple[str, bool, str]] = []

        ps = Stats.compute(periods_us, lim_period_us, tol_us, is_period=True)
        if periods_us and lim_period_us > 0:
            ok = ps.violations == 0
            checks.append(("Period",
                            ok,
                            f"{ps.violations}/{ps.n} violations  "
                            f"(expected {constraint.period_ms}ms "
                            f"±{constraint.tolerance_ms}ms)"))
            all_pass &= ok

        es = Stats.compute(exec_list, lim_exec_us)
        if exec_list and lim_exec_us > 0:
            ok = es.violations == 0
            checks.append(("Exec time",
                            ok,
                            f"{es.violations}/{es.n} violations  "
                            f"max={es.max_us/1000:.4f}ms  "
                            f"limit={constraint.max_exec_ms}ms"))
            all_pass &= ok

        ls = Stats.compute(elapsed_list, lim_elapsed_us)
        if elapsed_list and lim_elapsed_us > 0:
            ok = ls.violations == 0
            checks.append(("Elapsed",
                            ok,
                            f"{ls.violations}/{ls.n} violations  "
                            f"max={ls.max_us/1000:.4f}ms  "
                            f"limit={constraint.max_elapsed_ms}ms"))
            all_pass &= ok

        for label, ok, detail in checks:
            print(f"    {'PASS' if ok else 'FAIL'}  {label}: {detail}")

        print(f"  {'PASS' if all_pass else 'FAIL'}  Marker {mid}")

    print()
    return all_pass


# ── CLI ───────────────────────────────────────────────────────────────────────

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="SystemView CSV marker analyser with preemption detection.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Preemption is detected from Task Run/Stop and ISR Enter/Exit events
present in the same SystemView CSV export. No additional instrumentation
is needed beyond a standard SystemView recording.

Examples:
  List all marker IDs found:
    %(prog)s recording.csv --list-markers

  Analyse all markers with statistics:
    %(prog)s recording.csv

  Per-interval detail table:
    %(prog)s recording.csv --verbose

  Validate specific markers with timing constraints:
    %(prog)s recording.csv \\
        --marker 0 --period-ms 10.0 --tolerance-ms 0.5 --max-exec-ms 3.0 \\
        --marker 3 --period-ms 20.0 --tolerance-ms 1.0

  Export per-interval data to CSV:
    %(prog)s recording.csv --output-csv stats.csv
""")

    p.add_argument("csv_file",
                   help="SystemView CSV export file")
    p.add_argument("--verbose", "-v", action="store_true",
                   help="Show per-interval breakdown table")
    p.add_argument("--list-markers", action="store_true",
                   help="List marker IDs found and exit")

    p.add_argument("--marker",         type=int,   action="append",
                   dest="markers",     metavar="ID",
                   help="Marker ID to analyse (repeatable)")
    p.add_argument("--period-ms",      type=float, action="append",
                   dest="periods",     metavar="MS")
    p.add_argument("--tolerance-ms",   type=float, action="append",
                   dest="tolerances",  metavar="MS")
    p.add_argument("--max-exec-ms",    type=float, action="append",
                   dest="max_execs",   metavar="MS",
                   help="Max execution time excl. preemption")
    p.add_argument("--max-elapsed-ms", type=float, action="append",
                   dest="max_elapsed", metavar="MS",
                   help="Max elapsed time incl. preemption")
    p.add_argument("--output-csv",     type=str,   default=None,
                   help="Export per-interval statistics to CSV")

    return p.parse_args()


# ── Main ──────────────────────────────────────────────────────────────────────

def main() -> None:
    args = parse_args()

    if not os.path.isfile(args.csv_file):
        print(f"ERROR: File not found: {args.csv_file}", file=sys.stderr)
        sys.exit(1)

    print(f"File: {args.csv_file}")
    print()

    # ── Parse ─────────────────────────────────────────────────────────────────

    raw_events, warnings = parse_csv(args.csv_file, verbose=args.verbose)
    for w in warnings:
        print(f"WARNING: {w}")

    counts = {k: sum(1 for e in raw_events if e.kind == k)
              for k in ('marker_start', 'marker_stop',
                        'task_run',     'task_stop',
                        'isr_enter',    'isr_exit')}

    print(f"Events parsed:")
    print(f"  marker : {counts['marker_start']} starts  "
          f"{counts['marker_stop']} stops")
    print(f"  task   : {counts['task_run']} run  "
          f"{counts['task_stop']} stop")
    print(f"  ISR    : {counts['isr_enter']} enter  "
          f"{counts['isr_exit']} exit")
    print()

    if counts['task_run'] == 0:
        print("WARNING: No Task Run events found.")
        print("         Preemption cannot be computed — "
              "exec time will equal elapsed time.")
        print("         Ensure SystemView records task execution events.")
        print()

    if counts['isr_enter'] == 0:
        print("NOTE: No ISR events found — ISR time will be 0.")
        print()

    # ── Build windows and intervals ───────────────────────────────────────────

    exec_windows = build_exec_windows(raw_events)
    isr_windows  = build_isr_windows(raw_events)
    intervals_by_id = build_marker_intervals(raw_events,
                                             exec_windows,
                                             isr_windows)

    if not intervals_by_id:
        print("No marker intervals found.")
        print("Check that Start/Stop Marker events are present in the CSV.")
        sys.exit(0)

    all_ids = sorted(intervals_by_id.keys())

    # ── List mode ─────────────────────────────────────────────────────────────

    if args.list_markers:
        print("Marker IDs found:")
        print(f"  {'ID':>4}  {'ID (hex)':>10}  {'intervals':>10}  "
              f"{'task(s)'}")
        print(f"  {'──':>4}  {'────────':>10}  {'─────────':>10}  "
              f"{'───────'}")
        for mid in all_ids:
            ivs   = intervals_by_id[mid]
            tasks = sorted(set(iv.task_name for iv in ivs))
            print(f"  {mid:>4}  {mid:#010x}  {len(ivs):>10}  "
                  f"{', '.join(tasks)}")
        return

    # ── Build constraints ─────────────────────────────────────────────────────

    constraints: Dict[int, MarkerConstraint] = {}
    if args.markers:
        for i, mid in enumerate(args.markers):
            c = MarkerConstraint(marker_id=mid)
            if args.periods      and i < len(args.periods):
                c.period_ms      = args.periods[i]
            if args.tolerances   and i < len(args.tolerances):
                c.tolerance_ms   = args.tolerances[i]
            if args.max_execs    and i < len(args.max_execs):
                c.max_exec_ms    = args.max_execs[i]
            if args.max_elapsed  and i < len(args.max_elapsed):
                c.max_elapsed_ms = args.max_elapsed[i]
            constraints[mid] = c

    target_ids   = sorted(constraints.keys()) if constraints else all_ids
    overall_pass = True

    # ── Analyse ───────────────────────────────────────────────────────────────

    for mid in target_ids:
        ivs    = intervals_by_id.get(mid, [])
        passed = analyse_marker(mid, ivs, constraints.get(mid),
                                verbose=args.verbose)
        if not passed:
            overall_pass = False

    # ── Summary ───────────────────────────────────────────────────────────────

    if constraints:
        print(f"{'=' * 68}")
        print(f"{'PASS' if overall_pass else 'FAIL'}  "
              f"{'All markers within constraints' if overall_pass else 'Constraint violation(s) detected'}")
        print()

    # ── CSV export ────────────────────────────────────────────────────────────

    if args.output_csv:
        with open(args.output_csv, "w", newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                "marker_id", "marker_id_hex", "task_name",
                "interval_index",
                "period_ms",
                "start_us", "stop_us",
                "elapsed_ms", "exec_ms",
                "preemption_ms", "preemption_pct",
                "isr_ms", "other_task_ms",
            ])
            for mid in sorted(intervals_by_id.keys()):
                for i, iv in enumerate(intervals_by_id[mid]):
                    period_str = (f"{iv.period_us/1000:.6f}"
                                  if iv.period_us is not None else "")
                    writer.writerow([
                        mid,
                        f"{mid:#010x}",
                        iv.task_name,
                        i,
                        period_str,
                        f"{iv.start_us:.3f}",
                        f"{iv.stop_us:.3f}",
                        f"{iv.elapsed_us/1000:.6f}",
                        f"{iv.exec_us/1000:.6f}",
                        f"{iv.preemption_us/1000:.6f}",
                        f"{iv.preemption_pct:.2f}",
                        f"{iv.isr_us/1000:.6f}",
                        f"{iv.other_task_us/1000:.6f}",
                    ])
        print(f"Per-interval data exported to {args.output_csv}")

    if constraints:
        sys.exit(0 if overall_pass else 1)


if __name__ == "__main__":
    main()
