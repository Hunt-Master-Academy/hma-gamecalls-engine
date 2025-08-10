#!/usr/bin/env bash
# verify_normalized_calls.sh
# Verifies that data/processed_calls/normalized mirrors directory/file coverage of data/master_calls.
# For each non-hidden file under master_calls that has an audio extension, ensures a corresponding .wav exists in normalized.
# Exit codes:
#   0 = perfect mirror
#   1 = mismatch (missing or extra) without auto-populate
#   4 = auto-populate attempted but some items still missing (error)
# Options:
#   --auto-populate : For any missing normalized WAV whose source exists, invoke convert_to_wav.sh with --include-wav for those parents.
#   --quiet         : Suppress normal success output.

set -euo pipefail

AUTO_POPULATE=0
QUIET=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --auto-populate) AUTO_POPULATE=1; shift;;
    --quiet) QUIET=1; shift;;
    *) echo "Unknown option: $1" >&2; exit 99;;
  esac
done

MASTER_ROOT="data/master_calls"
NORMALIZED_ROOT="data/processed_calls/normalized"

if [[ ! -d "$MASTER_ROOT" ]]; then
  echo "ERROR: Missing $MASTER_ROOT" >&2
  exit 2
fi
if [[ ! -d "$NORMALIZED_ROOT" ]]; then
  echo "ERROR: Missing $NORMALIZED_ROOT" >&2
  exit 3
fi

shopt -s nullglob globstar nocaseglob

# Collect source files (any supported audio ext)
mapfile -t master_files < <(find "$MASTER_ROOT" -type f \( -iname '*.mp3' -o -iname '*.wav' -o -iname '*.m4a' -o -iname '*.aac' -o -iname '*.flac' -o -iname '*.ogg' -o -iname '*.opus' -o -iname '*.aif' -o -iname '*.aiff' -o -iname '*.wma' \) -print | sort)

# Build expected set of relative normalized wav paths
declare -A expected_rel
for src in "${master_files[@]}"; do
  rel="${src#$MASTER_ROOT/}"; rel_no_ext="${rel%.*}"; expected_rel["${rel_no_ext}.wav"]=1
done

missing=()
extra=()

# For each master file, derive expected normalized wav path
for src in "${master_files[@]}"; do
  rel="${src#$MASTER_ROOT/}"
  base_no_ext="${rel%.*}"
  expected="$NORMALIZED_ROOT/${base_no_ext}.wav"
  if [[ ! -f "$expected" ]]; then
    missing+=("$expected")
  fi
done

# Detect extra wavs whose relative path not among expected set
mapfile -t normalized_wavs < <(find "$NORMALIZED_ROOT" -type f -iname '*.wav' -print | sort)
for wav in "${normalized_wavs[@]}"; do
  rel="${wav#$NORMALIZED_ROOT/}"
  if [[ -z "${expected_rel[$rel]:-}" ]]; then
    extra+=("$wav")
  fi
done

if [[ ${#missing[@]} -eq 0 && ${#extra[@]} -eq 0 ]]; then
  if [[ $QUIET -eq 0 ]]; then
    echo "✅ Normalized directory mirrors master_calls (all expected WAVs present; no extras)."
  fi
  exit 0
fi

if [[ ${#missing[@]} -gt 0 ]]; then
  echo "Missing normalized WAVs (count=${#missing[@]}):" >&2
  printf '  %s\n' "${missing[@]}" >&2
fi
if [[ ${#extra[@]} -gt 0 ]]; then
  echo "Extra normalized WAVs without source (count=${#extra[@]}):" >&2
  printf '  %s\n' "${extra[@]}" >&2
fi

if [[ $AUTO_POPULATE -eq 1 ]]; then
  echo "Attempting auto-populate of missing WAVs..." >&2
  # Derive unique parent directories among missing sources
  declare -A parents=()
  for exp in "${missing[@]}"; do
    rel="${exp#$NORMALIZED_ROOT/}"
    base_no_ext="${rel%.wav}"
    # locate original source (first match among allowed extensions)
    for ext in mp3 wav m4a aac flac ogg opus aif aiff wma; do
      cand="$MASTER_ROOT/${base_no_ext}.${ext}"
      if [[ -f "$cand" ]]; then
        parents["$(dirname "$cand")"]=1
        break
      fi
    done
  done
  # Prune nested parents (keep highest-level only)
  for p in "${!parents[@]}"; do
    for q in "${!parents[@]}"; do
      if [[ "$p" != "$q" && "$p" == "$q"/* ]]; then
        unset 'parents[$p]' # p is descendant of q
      fi
    done
  done
  for p in "${!parents[@]}"; do
    echo "  Normalizing parent directory: $p" >&2
    scripts/convert_to_wav.sh "$p" --out "$NORMALIZED_ROOT" --include-wav --rate 44100 --channels 1 --format pcm_f32le >/dev/null
  done

  # Recompute after populate
  exec "$0" "${QUIET:+--quiet}" # Recursively re-run without auto-populate to reassess
fi

exit 1
