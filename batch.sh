#!/usr/bin/env bash
set -euo pipefail

BASE_OUTDIR="results/weight/1_100_10000"
mkdir -p "$BASE_OUTDIR/0.7" "$BASE_OUTDIR/0.9"

echo "== Start NS runs =="

run() {
  local label="$1"; shift
  echo "[$(date '+%F %T')] $label"
  ns "$@"
}

for ratio in 0.7 0.9; do
  run "tcp-HRCC.tcl 1000 $ratio Topology-32hosts-NSDI21.tcl GearboxPLFL1Level"  tcp-HRCC.tcl 1000 "$ratio" Topology-32hosts-NSDI21.tcl GearboxPLFL1Level
  run "tcp-HRCC.tcl 1000 $ratio Topology-32hosts-NSDI21.tcl GearboxPLFL2Levels" tcp-HRCC.tcl 1000 "$ratio" Topology-32hosts-NSDI21.tcl GearboxPLFL2Levels
  run "tcp-HRCC.tcl 1000 $ratio Topology-32hosts-NSDI21.tcl GearboxPLFL3Levels" tcp-HRCC.tcl 1000 "$ratio" Topology-32hosts-NSDI21.tcl GearboxPLFL3Levels
  run "tcp-HRCC.tcl 1000 $ratio Topology-32hosts-NSDI21.tcl GearboxPLFL4Levels" tcp-HRCC.tcl 1000 "$ratio" Topology-32hosts-NSDI21.tcl GearboxPLFL4Levels
  run "tcp-HRCC.tcl 1000 $ratio Topology-32hosts-NSDI21.tcl GearboxPLFL5Levels" tcp-HRCC.tcl 1000 "$ratio" Topology-32hosts-NSDI21.tcl GearboxPLFL5Levels
done

echo "== Move result files =="

for ratio in 0.7 0.9; do
  for f in tcp_flow_1000_${ratio}_Topology-32hosts-NSDI21.tcl_*.tr; do
      [ -e "$f" ] || continue
      mv "$f" "$BASE_OUTDIR/$ratio/"
      echo "Moved $f -> $BASE_OUTDIR/$ratio/"
  done
done

echo "== Done. Files moved under $BASE_OUTDIR/{0.7,0.9} =="