#!/usr/bin/env python3
"""
sysview_csv_marker_parser.py
=============================
Parse a SEGGER SystemView CSV export and validate that each task executes
exactly once within each period slot, measured from a TIME_ZERO marker.

Validation model:
  A marker with ID 0xFF defines time zero. All subsequent analysis is
  relative to that timestamp.

  For each monitored marker (task), a period is specified on the command line.
  Time is divided into slots of that period:
    slot 0 : [0,          period)
    slot 1 : [period,     2*period)
    slot 2 : [2*period,   3*period)
    ...

  Within each slot, the script checks:
    - The task marker fired exactly once          (presence + no double execution)
    - The execution time is within max_exec_us    (if --max-exec-ms specified)

  A marker interval that spans a slot boundary is counted in the slot
  where it STARTED.

  Preemption is detected from Task Run / Task Stop and ISR Enter / ISR Exit
  events present in the same CSV — no additional instrumentation needed.

Expected CSV column layout (0-based):
  0 : row index (numeric — used to identify data rows)
  1 : time "0.000 000 000" (seconds)
  2 : context  ("Engine", "Rain", "ISR 37", "Idle", ...)
  3 : event    ("Start Marker 0x000000FF", "Task Run", "Task Stop",
                "ISR Enter", "ISR Exit", ...)
  4 : detail

TIME_ZERO marker:
  Emit SEGGER_SYSVIEW_Mark(0xFF) at system start from your application.
  All timestamps are shifted so that marker is t=0.

Usage:
  python sysview_csv_marker_parser.py recording.csv --list-markers

  python sysview_csv_marker_parser.py recording.csv \\
      --marker 0x00 --task Engine --period-ms 10.0 --max-exec-ms 3.0 \\
      --marker 0x01 --task Rain   --period-ms 20.0 --max-exec-ms 5.0 \\
      --tolerance-period-start-ms 0.2

  python sysview_csv_marker_parser.py recording.csv \\
      --marker 0x00 --task Engine --period-ms 10.0 \\
      --output-csv results.csv
"""

import argparse
import csv
import os
import re
import sys
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

# ── Constants ─────────────────────────────────────────────────────────────────

TIME_ZERO_MARKER_ID = 0xFF

COL_TIME    = 1
COL_CONTEXT = 2
COL_EVENT   = 3
COL_DETAIL  = 4
MIN_COLUMNS = 4

# ── Event classification ──────────────────────────────────────────────────────

RE_START_MARKER = re.compile(r"^Start\s+Marker\s+0x([0-9a-fA-F]+)", re.IGNORECASE)
RE_STOP_MARKER  = re.compile(r"^Stop\s+Marker\s+0x([0-9a-fA-F]+)",  re.IGNORECASE)
RE_MARK_MARKER  = re.compile(r"^Mark\s+Marker\s+0x([0-9a-fA-F]+)",  re.IGNORECASE)

def classify_event(event_str: str) -> str:
    s  = event_str.strip()
    sl = s.lower()
    if RE_START_MARKER.match(s): return 'marker_start'
    if RE_STOP_MARKER.match(s):  return 'marker_stop'
    if RE_MARK_MARKER.match(s):  return 'mark_marker'
    if sl == 'task run':         return 'task_run'
    if sl == 'task stop':        return 'task_stop'
    if sl == 'isr enter':        return 'isr_enter'
    if sl == 'isr exit':         return 'isr_exit'
    return 'unknown'

def parse_time_us(time_str: str) -> float:
    """Parse '0.000 183 105' -> microseconds."""
    return float(time_str.replace(" ", "")) * 1_000_000.0


# ── Raw event ─────────────────────────────────────────────────────────────────

@dataclass
class RawEvent:
    row:       int
    time_us:   float
    context:   str
    kind:      str
    event_str: str
    detail:    str
    marker_id: Optional[int] = None


# ── Exec / ISR windows ────────────────────────────────────────────────────────

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


# ── Marker interval ───────────────────────────────────────────────────────────

@dataclass
class MarkerInterval:
    """One start->stop interval for a given marker, relative to TIME_ZERO."""
    marker_id:     int
    task_name:     str
    start_us:      float    # relative to TIME_ZERO
    stop_us:       float    # relative to TIME_ZERO

    # Filled by correlation with task/ISR events
    exec_us:       float = 0.0
    elapsed_us:    float = 0.0
    preemption_us: float = 0.0
    isr_us:        float = 0.0
    other_task_us: float = 0.0

    exec_windows:       List[ExecWindow] = field(default_factory=list)
    other_exec_windows: List[ExecWindow] = field(default_factory=list)
    isr_windows:        List[IsrWindow]  = field(default_factory=list)

    @property
    def preemption_pct(self) -> float:
        return (self.preemption_us / self.elapsed_us * 100.0
                if self.elapsed_us > 0 else 0.0)


# ── Slot validation result ────────────────────────────────────────────────────

@dataclass
class SlotResult:
    """Validation result for one period slot."""
    slot_index:    int
    slot_start_us: float
    slot_end_us:   float
    intervals:     List[MarkerInterval] = field(default_factory=list)

    @property
    def count(self) -> int:
        return len(self.intervals)

    @property
    def fired_once(self) -> bool:
        return self.count == 1

    @property
    def missing(self) -> bool:
        return self.count == 0

    @property
    def double_fired(self) -> bool:
        return self.count > 1

    def max_exec_violation(self, limit_us: float) -> bool:
        if limit_us <= 0:
            return False
        return any(iv.exec_us > limit_us for iv in self.intervals)

    def boundary_crossed(self) -> bool:
        """True if any interval's stop_us falls beyond this slot's end."""
        return any(iv.stop_us > self.slot_end_us for iv in self.intervals)

    def boundary_overshoot_us(self) -> float:
        """Maximum overshoot past the slot end across all intervals."""
        if not self.intervals:
            return 0.0
        return max(max(iv.stop_us - self.slot_end_us, 0.0)
                   for iv in self.intervals)

    def passes(self, limit_us: float = 0.0) -> bool:
        return (self.fired_once
                and not self.max_exec_violation(limit_us)
                and not self.boundary_crossed())


# ── Statistics ────────────────────────────────────────────────────────────────

@dataclass
class Stats:
    n:       int   = 0
    mean_us: float = 0.0
    min_us:  float = float('inf')
    max_us:  float = 0.0
    std_us:  float = 0.0

    @staticmethod
    def compute(values: List[float]) -> 'Stats':
        if not values:
            return Stats()
        n        = len(values)
        mean     = sum(values) / n
        variance = sum((v - mean) ** 2 for v in values) / n
        return Stats(n=n, mean_us=mean, min_us=min(values), max_us=max(values),
                     std_us=variance ** 0.5)

    def print(self, label: str) -> None:
        if self.n == 0:
            print(f"  {label}: no data")
            return
        print(f"  {label}:")
        print(f"    n={self.n}  "
              f"mean={self.mean_us/1000:.6f}ms  "
              f"min={self.min_us/1000:.6f}ms  "
              f"max={self.max_us/1000:.6f}ms  "
              f"std={self.std_us/1000:.6f}ms")


# ── CSV parser ────────────────────────────────────────────────────────────────

def parse_csv(filepath: str,
              verbose:   bool = False) -> Tuple[List[RawEvent], List[str]]:
    events:   List[RawEvent] = []
    warnings: List[str]      = []

    with open(filepath, newline='', encoding='utf-8-sig') as f:
        reader  = csv.reader(f)
        row_num = 0

        for raw_row in reader:
            row_num += 1
            row = [c.strip().strip('"') for c in raw_row]

            if len(row) < MIN_COLUMNS:
                continue

            try:
                int(row[0])
            except ValueError:
                continue   # header or metadata line

            try:
                time_us = parse_time_us(row[COL_TIME])
            except (ValueError, IndexError):
                warnings.append(
                    f"Row {row_num}: cannot parse time '{row[COL_TIME]}'")
                continue

            context   = row[COL_CONTEXT]
            event_str = row[COL_EVENT]
            detail    = row[COL_DETAIL] if len(row) > COL_DETAIL else ""
            kind      = classify_event(event_str)

            marker_id = None
            if kind == 'marker_start':
                m = RE_START_MARKER.match(event_str)
                if m: marker_id = int(m.group(1), 16)
            elif kind == 'marker_stop':
                m = RE_STOP_MARKER.match(event_str)
                if m: marker_id = int(m.group(1), 16)
            elif kind == 'mark_marker':
                m = RE_MARK_MARKER.match(event_str)
                if m: marker_id = int(m.group(1), 16)
            
            events.append(RawEvent(
                row=int(row[0]), time_us=time_us, context=context,
                kind=kind, event_str=event_str, detail=detail,
                marker_id=marker_id,
            ))

            if verbose and kind != 'unknown':
                print(f"  [{int(row[0]):5d}] {time_us:14.3f}us  "
                      f"{kind:15s}  ctx={context:20s}  "
                      f"mid={marker_id if marker_id is not None else '-':>5}  "
                      f"{detail[:40]}")

    return events, warnings


# ── Find TIME_ZERO ────────────────────────────────────────────────────────────

def find_time_zero(events: List[RawEvent]) -> Optional[float]:
    """
    Find the first Start Marker with ID 0xFF and return its timestamp.
    Returns None if not found.
    """
    for evt in events:
        if evt.kind == 'mark_marker' and evt.marker_id == TIME_ZERO_MARKER_ID:
            return evt.time_us
    return None


# ── Build exec / ISR windows ──────────────────────────────────────────────────

def build_exec_windows(events: List[RawEvent]) -> List[ExecWindow]:
    """
    Build ExecWindow list from Task Run / Task Stop pairs.
    Task Run  -> context = task that starts running
    Task Stop -> closes the current running task window
    """
    windows:       List[ExecWindow] = []
    current_task:  Optional[str]    = None
    current_start: float            = 0.0

    for evt in events:
        if evt.kind == 'task_run':
            if current_task is not None:
                windows.append(
                    ExecWindow(current_task, current_start, evt.time_us))
            current_task  = evt.context
            current_start = evt.time_us
        elif evt.kind == 'task_stop':
            if current_task is not None:
                windows.append(
                    ExecWindow(current_task, current_start, evt.time_us))
            current_task = None

    return windows


def build_isr_windows(events: List[RawEvent]) -> List[IsrWindow]:
    windows: List[IsrWindow]  = []
    pending: Dict[str, float] = {}

    for evt in events:
        if evt.kind == 'isr_enter':
            pending[evt.context] = evt.time_us
        elif evt.kind == 'isr_exit':
            if evt.context in pending:
                windows.append(
                    IsrWindow(evt.context, pending.pop(evt.context), evt.time_us))
    return windows


# ── Build marker intervals ────────────────────────────────────────────────────

def build_marker_intervals(events:       List[RawEvent],
                            exec_windows: List[ExecWindow],
                            isr_windows:  List[IsrWindow],
                            time_zero_us: float
                            ) -> Dict[int, List[MarkerInterval]]:
    """
    Build MarkerInterval list per marker ID (excluding 0xFF TIME_ZERO marker).
    All timestamps stored relative to time_zero_us.
    Correlates exec and ISR windows for preemption breakdown.
    """
    pending_start: Dict[int, RawEvent]  = {}
    result:        Dict[int, List[MarkerInterval]] = {}

    for evt in events:
        if evt.kind == 'marker_start' and evt.marker_id is not None:
            pending_start[evt.marker_id] = evt

        elif evt.kind == 'marker_stop' and evt.marker_id is not None:
            mid = evt.marker_id
            if mid not in pending_start:
                continue

            start_evt = pending_start.pop(mid)

            iv = MarkerInterval(
                marker_id = mid,
                task_name = start_evt.context,
                start_us  = start_evt.time_us - time_zero_us,
                stop_us   = evt.time_us - time_zero_us,
            )

            # Correlate: use absolute times for window overlap check
            ws = start_evt.time_us
            we = evt.time_us

            own_exec_us   = 0.0
            other_task_us = 0.0
            isr_total_us  = 0.0
            iv_exec:  List[ExecWindow] = []
            iv_other: List[ExecWindow] = []
            iv_isr:   List[IsrWindow]  = []

            for ew in exec_windows:
                ol_start = max(ew.start_us, ws)
                ol_stop  = min(ew.stop_us,  we)
                if ol_stop <= ol_start:
                    continue
                ol_us = ol_stop - ol_start
                # Store relative timestamps in the window
                rel_start = ol_start - time_zero_us
                rel_stop  = ol_stop  - time_zero_us
                if ew.task == iv.task_name:
                    own_exec_us += ol_us
                    iv_exec.append(ExecWindow(ew.task, rel_start, rel_stop))
                else:
                    other_task_us += ol_us
                    iv_other.append(ExecWindow(ew.task, rel_start, rel_stop))

            for iw in isr_windows:
                ol_start = max(iw.start_us, ws)
                ol_stop  = min(iw.stop_us,  we)
                if ol_stop <= ol_start:
                    continue
                ol_us = ol_stop - ol_start
                isr_total_us += ol_us
                iv_isr.append(IsrWindow(iw.isr_name,
                                        ol_start - time_zero_us,
                                        ol_stop  - time_zero_us))

            # Fallback: if no task exec events recorded, elapsed = exec
            if not exec_windows:
                own_exec_us = iv.stop_us - iv.start_us

            iv.exec_us            = own_exec_us
            iv.elapsed_us         = iv.stop_us - iv.start_us
            iv.isr_us             = isr_total_us
            iv.other_task_us      = other_task_us
            iv.preemption_us      = max(0.0, iv.elapsed_us - own_exec_us)
            iv.exec_windows       = iv_exec
            iv.other_exec_windows = iv_other
            iv.isr_windows        = iv_isr

            result.setdefault(mid, []).append(iv)

    return result


# ── Slot assignment ───────────────────────────────────────────────────────────

def assign_slots(intervals: List[MarkerInterval],
                 period_us: float,
                 tolerance_period_start_us: float,
                 num_slots: int) -> List[SlotResult]:
    """
    Assign each interval to the slot where its start_us falls.
    Slot N covers [N*period_us, (N+1)*period_us).
    An interval spanning a boundary is counted in the slot where it started 
    if it started before the slot start minus tolerance
    Intervals with start_us < 0 (before TIME_ZERO) are ignored.
    """
    slots = [
        SlotResult(
            slot_index    = n,
            slot_start_us = n * period_us,
            slot_end_us   = (n + 1) * period_us,
        )
        for n in range(num_slots)
    ]

    for iv in intervals:
        if iv.start_us < 0:
            continue   # before TIME_ZERO
        slot_index = int((iv.start_us + tolerance_period_start_us)/ period_us)
        if 0 <= slot_index < num_slots:
            slots[slot_index].intervals.append(iv)

    return slots


# ── Constraint ────────────────────────────────────────────────────────────────

@dataclass
class MarkerConstraint:
    marker_id:   int
    task_name:   Optional[str]   = None
    period_ms:   Optional[float] = None
    max_exec_ms: Optional[float] = None


# ── Analysis ──────────────────────────────────────────────────────────────────

def analyse_marker(mid:        int,
                   intervals:  List[MarkerInterval],
                   constraint: Optional[MarkerConstraint],
                   total_us:   float,
                   tolerance_period_start_us: float,
                   verbose:    bool = False) -> bool:

    print(f"{'=' * 68}")
    print(f"Marker {mid:#04x}  ({mid})")

    task_names = sorted(set(iv.task_name for iv in intervals))
    print(f"Task      : {', '.join(task_names)}")

    if not constraint or not constraint.period_ms:
        print("NOTE      : No period specified — reporting statistics only")
        _print_stats_only(intervals)
        return True

    period_us     = constraint.period_ms * 1000.0
    limit_exec_us = (constraint.max_exec_ms * 1000.0
                     if constraint.max_exec_ms else 0.0)

    print(f"Period    : {constraint.period_ms}ms")
    if limit_exec_us > 0:
        print(f"Max exec  : {constraint.max_exec_ms}ms")

    num_slots = max(1, int(total_us / period_us))
    print(f"Slots     : {num_slots}  "
          f"(recording covers {total_us/1000:.3f}ms after TIME_ZERO)")
    print(f"{'─' * 68}")

    slots = assign_slots(intervals, period_us, tolerance_period_start_us, num_slots)

    boundary_slots    = [s for s in slots if s.boundary_crossed()]

    # Slots whose MISSING or DOUBLE FIRE is explained by an adjacent
    # boundary crossing are suppressed — the boundary violation is the
    # root cause; cascading count anomalies are noise.
    boundary_indices  = {s.slot_index for s in boundary_slots}
    explained_indices = set()
    for bi in boundary_indices:
        explained_indices.add(bi + 1)  # next slot gets an extra interval
        explained_indices.add(bi - 1)  # prev slot may be missing one

    missing_slots     = [s for s in slots
                         if s.missing
                         and s.slot_index not in explained_indices]
    double_fire_slots = [s for s in slots
                         if s.double_fired
                         and s.slot_index not in explained_indices
                         and s.slot_index not in boundary_indices]
    exec_viol_slots   = [s for s in slots
                         if s.max_exec_violation(limit_exec_us)]
    pass_slots        = [s for s in slots if s.passes(limit_exec_us)]

    all_pass = (len(missing_slots) == 0 and
                len(double_fire_slots) == 0 and
                len(exec_viol_slots) == 0 and
                len(boundary_slots) == 0)

    # ── Execution time statistics ─────────────────────────────────────────────

    task_label     = (task_names[0] if len(task_names) == 1
                      else ', '.join(task_names))
    exec_values    = [iv.exec_us       for iv in intervals]
    elapsed_values = [iv.elapsed_us    for iv in intervals]
    preempt_values = [iv.preemption_us for iv in intervals]
    isr_values     = [iv.isr_us        for iv in intervals]
    other_values   = [iv.other_task_us for iv in intervals]

    Stats.compute(exec_values).print(
        f"Execution time  ({task_label}, excl. preemption)")
    Stats.compute(elapsed_values).print(
        f"Elapsed time    ({task_label}, incl. preemption)")
    Stats.compute(preempt_values).print(
        "Preemption      (elapsed - execution)")

    if any(v > 0 for v in isr_values):
        Stats.compute(isr_values).print("ISR time")
        isr_by_name: Dict[str, List[float]] = {}
        for iv in intervals:
            for iw in iv.isr_windows:
                isr_by_name.setdefault(iw.isr_name, []).append(iw.duration_us)
        for name, durations in sorted(isr_by_name.items()):
            s = Stats.compute(durations)
            print(f"    {name}: n={s.n}  mean={s.mean_us/1000:.6f}ms  "
                  f"max={s.max_us/1000:.6f}ms")

    if any(v > 0 for v in other_values):
        Stats.compute(other_values).print(
            f"Other tasks     (preempted {task_label})")
        other_by_name: Dict[str, List[float]] = {}
        for iv in intervals:
            for ew in iv.other_exec_windows:
                other_by_name.setdefault(ew.task, []).append(ew.duration_us)
        for name, durations in sorted(other_by_name.items()):
            s = Stats.compute(durations)
            print(f"    {name}: n={s.n}  mean={s.mean_us/1000:.6f}ms  "
                  f"max={s.max_us/1000:.6f}ms")

    if preempt_values:
        mean_pct = sum(iv.preemption_pct for iv in intervals) / len(intervals)
        max_pct  = max(iv.preemption_pct for iv in intervals)
        print(f"  Preemption %  : mean={mean_pct:.1f}%  max={max_pct:.1f}%")

    # ── Slot summary ──────────────────────────────────────────────────────────

    print(f"\n  Slot summary ({num_slots} slots x {constraint.period_ms}ms):")
    print(f"    PASS         : {len(pass_slots)}/{num_slots}")
    if boundary_slots:
        print(f"    BOUNDARY     : {len(boundary_slots)} slot(s) crossed boundary")

    if missing_slots:
        indices = [str(s.slot_index) for s in missing_slots]
        print(f"    MISSING      : slots {', '.join(indices)}")

    if double_fire_slots:
        indices = [str(s.slot_index) for s in double_fire_slots]
        print(f"    DOUBLE FIRE  : slots {', '.join(indices)}")
        for s in double_fire_slots:
            for iv in s.intervals:
                print(f"      slot {s.slot_index}: "
                      f"start={iv.start_us/1000:.4f}ms  "
                      f"exec={iv.exec_us/1000:.4f}ms")

    if exec_viol_slots:
        indices = [str(s.slot_index) for s in exec_viol_slots]
        print(f"    EXEC OVERSHOOT: slots {', '.join(indices)}  "
              f"(limit={constraint.max_exec_ms}ms)")
        for s in exec_viol_slots:
            for iv in s.intervals:
                if iv.exec_us <= limit_exec_us:
                    continue
                overshoot_us = iv.exec_us - limit_exec_us
                print(f"      slot {s.slot_index}:")
                print(f"        start      = {iv.start_us/1000:.4f}ms  "
                      f"stop = {iv.stop_us/1000:.4f}ms")
                print(f"        exec       = {iv.exec_us/1000:.4f}ms  "
                      f"limit = {constraint.max_exec_ms}ms  "
                      f"overshoot = +{overshoot_us/1000:.4f}ms")
                print(f"        elapsed    = {iv.elapsed_us/1000:.4f}ms  "
                      f"preemption = {iv.preemption_us/1000:.4f}ms "
                      f"({iv.preemption_pct:.1f}%)")
                # ISR breakdown within this interval
                if iv.isr_windows:
                    isr_by_name: Dict[str, float] = {}
                    for iw in iv.isr_windows:
                        isr_by_name[iw.isr_name] = (
                            isr_by_name.get(iw.isr_name, 0.0) + iw.duration_us)
                    isr_parts = "  ".join(
                        f"{name}={us/1000:.4f}ms"
                        for name, us in sorted(isr_by_name.items()))
                    print(f"        isr        = {iv.isr_us/1000:.4f}ms  "
                          f"({isr_parts})")
                # Other task breakdown within this interval
                if iv.other_exec_windows:
                    other_by_name: Dict[str, float] = {}
                    for ew in iv.other_exec_windows:
                        other_by_name[ew.task] = (
                            other_by_name.get(ew.task, 0.0) + ew.duration_us)
                    other_parts = "  ".join(
                        f"{name}={us/1000:.4f}ms"
                        for name, us in sorted(other_by_name.items()))
                    print(f"        other tasks= {iv.other_task_us/1000:.4f}ms  "
                          f"({other_parts})")

    if boundary_slots:
        indices = [str(s.slot_index) for s in boundary_slots]
        print(f"    BOUNDARY CROSSED: slots {', '.join(indices)}  "
              f"(task executed past slot end)")
        for s in boundary_slots:
            for iv in s.intervals:
                overshoot_us = iv.stop_us - s.slot_end_us
                if overshoot_us <= 0:
                    continue
                print(f"      slot {s.slot_index}:")
                print(f"        start      = {iv.start_us/1000:.4f}ms  "
                      f"stop = {iv.stop_us/1000:.4f}ms  "
                      f"slot end = {s.slot_end_us/1000:.4f}ms")
                print(f"        overshoot  = +{overshoot_us/1000:.4f}ms "
                      f"past slot boundary")
                print(f"        exec       = {iv.exec_us/1000:.4f}ms  "
                      f"elapsed = {iv.elapsed_us/1000:.4f}ms  "
                      f"preemption = {iv.preemption_us/1000:.4f}ms "
                      f"({iv.preemption_pct:.1f}%)")
                # ISR breakdown
                if iv.isr_windows:
                    isr_by_name_b: Dict[str, float] = {}
                    for iw in iv.isr_windows:
                        isr_by_name_b[iw.isr_name] = (
                            isr_by_name_b.get(iw.isr_name, 0.0) + iw.duration_us)
                    isr_parts = '  '.join(
                        f'{name}={us/1000:.4f}ms'
                        for name, us in sorted(isr_by_name_b.items()))
                    print(f"        isr        = {iv.isr_us/1000:.4f}ms  "
                          f"({isr_parts})")
                # Other task breakdown
                if iv.other_exec_windows:
                    other_by_name_b: Dict[str, float] = {}
                    for ew in iv.other_exec_windows:
                        other_by_name_b[ew.task] = (
                            other_by_name_b.get(ew.task, 0.0) + ew.duration_us)
                    other_parts = '  '.join(
                        f'{name}={us/1000:.4f}ms'
                        for name, us in sorted(other_by_name_b.items()))
                    print(f"        other tasks= {iv.other_task_us/1000:.4f}ms  "
                          f"({other_parts})")

    # ── Verbose per-slot table ────────────────────────────────────────────────

    if verbose:
        print()
        hdr = (f"  {'Slot':>5}  {'Slot start':>11}  {'Slot end':>11}  "
               f"{'iv start':>10}  {'exec_ms':>10}  "
               f"{'elapsed_ms':>11}  {'preempt_ms':>11}  {'Result':>12}")
        print(hdr)
        print("  " + "─" * (len(hdr) - 2))

        for s in slots:
            slot_s = f"{s.slot_start_us/1000:>11.4f}"
            slot_e = f"{s.slot_end_us/1000:>11.4f}"
            if s.intervals:
                for idx, iv in enumerate(s.intervals):
                    if s.passes(limit_exec_us):
                        tag = 'PASS'
                    elif s.boundary_crossed() and not s.max_exec_violation(limit_exec_us):
                        tag = 'FAIL BOUNDARY'
                    elif s.boundary_crossed() and s.max_exec_violation(limit_exec_us):
                        tag = 'FAIL EXEC+BNDRY'
                    else:
                        tag = 'FAIL'
                    if s.double_fired and idx > 0:
                        tag = 'FAIL +DUPE'
                    print(f"  {s.slot_index:>5}  {slot_s}  {slot_e}  "
                          f"{iv.start_us/1000:>10.4f}  "
                          f"{iv.exec_us/1000:>10.6f}  "
                          f"{iv.elapsed_us/1000:>11.6f}  "
                          f"{iv.preemption_us/1000:>11.6f}  "
                          f"{tag:>12}")
            else:
                print(f"  {s.slot_index:>5}  {slot_s}  {slot_e}  "
                      f"{'─':>10}  {'─':>10}  {'─':>11}  {'─':>11}  "
                      f"{'MISSING':>12}")
        print()

    print(f"\n  {'PASS' if all_pass else 'FAIL'}  Marker {mid:#04x}  "
          f"({len(pass_slots)}/{num_slots} slots passed)")
    print()
    return all_pass


def _print_stats_only(intervals: List[MarkerInterval]) -> None:
    exec_values    = [iv.exec_us    for iv in intervals]
    elapsed_values = [iv.elapsed_us for iv in intervals]
    Stats.compute(exec_values).print("Execution time (excl. preemption)")
    Stats.compute(elapsed_values).print("Elapsed time   (incl. preemption)")
    print()


# ── CLI ───────────────────────────────────────────────────────────────────────

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="SystemView CSV validator: slot-based period checking from TIME_ZERO.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
TIME_ZERO marker:
  Emit SEGGER_SYSVIEW_Mark(0xFF) at system start.
  All timestamps are relative to that marker.

Examples:
  List all marker IDs found:
    %(prog)s recording.csv --list-markers

  Validate Engine (10ms) and Rain (20ms) with 0.5 ms tolerance for period start
    %(prog)s recording.csv \\
        --marker 0x00 --task Engine --period-ms 10.0 --max-exec-ms 3.0 \\
        --marker 0x01 --task Rain   --period-ms 20.0 --max-exec-ms 5.0 \\
        --tolerance-period-start-ms 0.5

  Per-slot detail table:
    %(prog)s recording.csv \\
        --marker 0x00 --task Engine --period-ms 10.0 --verbose

  Export per-interval data:
    %(prog)s recording.csv \\
        --marker 0x00 --task Engine --period-ms 10.0 --output-csv results.csv
""")

    p.add_argument("csv_file")
    p.add_argument("--verbose",      "-v", action="store_true")
    p.add_argument("--list-markers",       action="store_true")

    p.add_argument("--marker",
                   type=lambda x: int(x, 0),
                   action="append", dest="markers",   metavar="ID",
                   help="Marker ID in hex (e.g. 0x00) or decimal")
    p.add_argument("--task",
                   type=str,
                   action="append", dest="tasks",     metavar="NAME")
    p.add_argument("--period-ms",
                   type=float,
                   action="append", dest="periods",   metavar="MS")
    p.add_argument("--max-exec-ms",
                   type=float,
                   action="append", dest="max_execs", metavar="MS")
    p.add_argument("--tolerance-period-start-ms", type=float, default=0)
    p.add_argument("--output-csv",
                   type=str, default=None)

    return p.parse_args()


# ── Main ──────────────────────────────────────────────────────────────────────

def main() -> None:
    args = parse_args()

    if not os.path.isfile(args.csv_file):
        print(f"ERROR: File not found: {args.csv_file}", file=sys.stderr)
        sys.exit(1)

    print(f"File: {args.csv_file}")
    print()

    tolerance_period_start_us = 500
    if args.tolerance_period_start_ms >= 0:        
        tolerance_period_start_us = args.tolerance_period_start_ms * 1000

    # ── Parse ─────────────────────────────────────────────────────────────────

    raw_events, warnings = parse_csv(args.csv_file, verbose=args.verbose)
    for w in warnings:
        print(f"WARNING: {w}")

    counts = {k: sum(1 for e in raw_events if e.kind == k)
              for k in ('marker_start', 'marker_stop',
                        'task_run', 'task_stop', 'isr_enter', 'isr_exit')}

    print(f"Events : {counts['marker_start']} marker starts  "
          f"{counts['marker_stop']} marker stops  "
          f"{counts['task_run']} task run  "
          f"{counts['task_stop']} task stop  "
          f"{counts['isr_enter']} ISR enter  "
          f"{counts['isr_exit']} ISR exit")

    # ── Find TIME_ZERO ────────────────────────────────────────────────────────

    time_zero_us = find_time_zero(raw_events)
    if time_zero_us is None:
        print(f"\nERROR: TIME_ZERO marker (0x{TIME_ZERO_MARKER_ID:02X}) "
              f"not found in recording.")
        print(f"       Add SEGGER_SYSVIEW_Mark(0xFF) at system start.")
        sys.exit(1)

    last_us  = max((e.time_us for e in raw_events), default=time_zero_us)
    total_us = last_us - time_zero_us

    print(f"TIME_ZERO: absolute t={time_zero_us:.3f}us  "
          f"recording covers {total_us/1000:.3f}ms after TIME_ZERO")
    print()

    # ── Build windows and intervals ───────────────────────────────────────────

    exec_windows    = build_exec_windows(raw_events)
    isr_windows     = build_isr_windows(raw_events)
    intervals_by_id = build_marker_intervals(raw_events, exec_windows,
                                              isr_windows, time_zero_us)

    if not intervals_by_id:
        print("No task marker intervals found.")
        print("Check that Start/Stop Marker events (other than 0xFF) "
              "are present in the CSV.")
        sys.exit(0)

    all_ids = sorted(intervals_by_id.keys())

    # ── List mode ─────────────────────────────────────────────────────────────

    if args.list_markers:
        print("Marker IDs found (TIME_ZERO 0xFF excluded):")
        print(f"  {'ID':>6}  {'intervals':>10}  task(s)")
        print(f"  {'──':>6}  {'─────────':>10}  ──────")
        for mid in all_ids:
            ivs   = intervals_by_id[mid]
            tasks = sorted(set(iv.task_name for iv in ivs))
            print(f"  {mid:#06x}  {len(ivs):>10}  {', '.join(tasks)}")
        return

    # ── Build constraints ─────────────────────────────────────────────────────

    constraints: Dict[int, MarkerConstraint] = {}
    if args.markers:
        for i, mid in enumerate(args.markers):
            c = MarkerConstraint(marker_id=mid)
            if args.tasks     and i < len(args.tasks):
                c.task_name   = args.tasks[i]
            if args.periods   and i < len(args.periods):
                c.period_ms   = args.periods[i]
            if args.max_execs and i < len(args.max_execs):
                c.max_exec_ms = args.max_execs[i]
            constraints[mid] = c

    target_ids   = sorted(constraints.keys()) if constraints else all_ids
    overall_pass = True

    # ── Analyse ───────────────────────────────────────────────────────────────

    for mid in target_ids:
        ivs    = intervals_by_id.get(mid, [])
        passed = analyse_marker(mid, ivs, constraints.get(mid),
                                total_us, tolerance_period_start_us, verbose=args.verbose)
        if not passed:
            overall_pass = False

    # ── Overall summary ───────────────────────────────────────────────────────

    print(f"{'=' * 68}")
    print(f"{'PASS' if overall_pass else 'FAIL'}  "
          f"{'All markers passed' if overall_pass else 'Violation(s) detected'}")
    print()

    # ── CSV export ────────────────────────────────────────────────────────────

    if args.output_csv:
        with open(args.output_csv, "w", newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                "marker_id", "task_name",
                "slot_index", "slot_start_ms", "slot_end_ms",
                "iv_start_ms", "iv_stop_ms",
                "exec_ms", "elapsed_ms",
                "preemption_ms", "preemption_pct",
                "isr_ms", "other_task_ms",
                "slot_result",
            ])
            for mid in sorted(intervals_by_id.keys()):
                c         = constraints.get(mid)
                period_us = c.period_ms * 1000.0 if c and c.period_ms else None
                limit_us  = (c.max_exec_ms * 1000.0
                             if c and c.max_exec_ms else 0.0)
                ivs       = intervals_by_id[mid]

                if period_us:
                    num_slots = max(1, int(total_us / period_us))
                    slots     = assign_slots(ivs, period_us, tolerance_period_start_us, num_slots)
                    for s in slots:
                        for iv in s.intervals:
                            result = 'PASS' if s.passes(limit_us) else 'FAIL'
                            if s.double_fired and iv is not s.intervals[0]:
                                result = 'FAIL_DUPE'
                            writer.writerow([
                                f"{mid:#04x}", iv.task_name,
                                s.slot_index,
                                f"{s.slot_start_us/1000:.6f}",
                                f"{s.slot_end_us/1000:.6f}",
                                f"{iv.start_us/1000:.6f}",
                                f"{iv.stop_us/1000:.6f}",
                                f"{iv.exec_us/1000:.6f}",
                                f"{iv.elapsed_us/1000:.6f}",
                                f"{iv.preemption_us/1000:.6f}",
                                f"{iv.preemption_pct:.2f}",
                                f"{iv.isr_us/1000:.6f}",
                                f"{iv.other_task_us/1000:.6f}",
                                result,
                            ])
                        if s.missing:
                            writer.writerow([
                                f"{mid:#04x}",
                                c.task_name if c else '',
                                s.slot_index,
                                f"{s.slot_start_us/1000:.6f}",
                                f"{s.slot_end_us/1000:.6f}",
                                '', '', '', '', '', '', '', '',
                                'MISSING',
                            ])
                else:
                    for iv in ivs:
                        writer.writerow([
                            f"{mid:#04x}", iv.task_name,
                            '', '', '',
                            f"{iv.start_us/1000:.6f}",
                            f"{iv.stop_us/1000:.6f}",
                            f"{iv.exec_us/1000:.6f}",
                            f"{iv.elapsed_us/1000:.6f}",
                            f"{iv.preemption_us/1000:.6f}",
                            f"{iv.preemption_pct:.2f}",
                            f"{iv.isr_us/1000:.6f}",
                            f"{iv.other_task_us/1000:.6f}",
                            '',
                        ])
        print(f"Per-interval data exported to {args.output_csv}")

    sys.exit(0 if overall_pass else 1)


if __name__ == "__main__":
    main()