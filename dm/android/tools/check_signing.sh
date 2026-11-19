#!/bin/bash
set -eu
mode=$(cat "$1")
case "$mode" in
  release) ;;
  debug-fallback)
    echo "WARNING: Using the existing Android debug key. This APK is for local testing only; do not publish it." >&2
    if [ "$2" = release ] && [ "${3:-}" != true ] && [ "${3:-}" != 1 ]; then
      echo "Release signing is missing. Opt in only for local testing with -PdmAllowDebugSigning=true or DM_ALLOW_DEBUG_SIGNING=1." >&2
      exit 1
    fi ;;
  *) echo "Unknown signing mode: $mode" >&2; exit 1 ;;
esac
