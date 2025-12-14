#!/usr/bin/env bash
set -e
if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli not found; zainstaluj go najpierw."
  exit 1
fi
arduino-cli lib update-index
arduino-cli lib install "CapacitiveSensor"
arduino-cli lib install "Timers"
echo "Libraries installed."
