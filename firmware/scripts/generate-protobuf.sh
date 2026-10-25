#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python=${PROTO_PYTHON:-python3}
"$python" -c 'from importlib.metadata import version; from pathlib import Path; pins=Path("scripts/protobuf-requirements.txt").read_text().splitlines(); assert all(version(p.split("==")[0]) == p.split("==")[1] for p in pins), "Install pinned protobuf requirements"'
output=$(mktemp -d "${TMPDIR:-/tmp}/aqm-proto.XXXXXX")
trap 'rm -rf "$output"' EXIT HUP INT TERM
"$python" -m nanopb.generator.nanopb_generator --no-timestamp -I ../protocol -D "$output" ../protocol/measurements.proto
case "${1:-}" in
    --check)
        diff -u components/ble_measurements/generated/measurements.pb.h "$output/measurements.pb.h"
        diff -u components/ble_measurements/generated/measurements.pb.c "$output/measurements.pb.c"
        ;;
    '') cp "$output/measurements.pb.h" "$output/measurements.pb.c" components/ble_measurements/generated/ ;;
    *) echo "Usage: sh scripts/generate-protobuf.sh [--check]" >&2; exit 2 ;;
esac
