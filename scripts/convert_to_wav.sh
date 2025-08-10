#!/usr/bin/env bash

# convert_to_wav.sh
#
# Batch-convert audio files to high-quality WAV for Huntmaster master calls.
# - Defaults: 44.1kHz sample rate, mono, 32-bit float PCM
# - Preserves filenames; can output in-place or to a separate output directory
# - Requires: ffmpeg
#
# Usage:
#   scripts/convert_to_wav.sh <input_file_or_dir> [--out <output_dir>] \
#       [--rate 44100] [--channels 1] [--format pcm_f32le] [--include-wav] [--force] [--dry-run]
#
# Examples:
#   scripts/convert_to_wav.sh data/master_calls/turkey
#   scripts/convert_to_wav.sh data/master_calls --out data/processed_calls/normalized --rate 44100 --channels 1
#   scripts/convert_to_wav.sh data/master_calls --include-wav   # Also (re)encode existing WAV masters into normalized mirror
#   # If --out is omitted, defaults to data/processed_calls/normalized
#
# Notes:
# - Recommended format for the engine is 44.1kHz, mono, 32-bit float WAV
# - If ffmpeg is missing, install it (Ubuntu): sudo apt-get update && sudo apt-get install -y ffmpeg

set -euo pipefail

if ! command -v ffmpeg >/dev/null 2>&1; then
  echo "Error: ffmpeg not found in PATH. Please install ffmpeg." >&2
  exit 1
fi

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <input_file_or_dir> [--out <output_dir>] [--rate 44100] [--channels 1] [--format pcm_f32le] [--dry-run]" >&2
  echo "       Default --out: data/processed_calls/normalized" >&2
  exit 1
fi

INPUT_PATH=""
OUT_DIR=""
RATE=44100
CHANNELS=1
## Defaults
# Audio codec for WAV output. Prefer explicit PCM codecs: pcm_f32le, pcm_s24le, pcm_s16le
FORMAT="pcm_f32le"
DRY_RUN=0
INCLUDE_WAV=0   # When set, existing WAV masters are also normalized into OUT_DIR
FORCE=0         # When set, overwrite existing destination WAVs

INPUT_PATH="$1"; shift || true
while [[ $# -gt 0 ]]; do
  case "$1" in
    --out)
      OUT_DIR="$2"; shift 2;;
    --rate)
      RATE="$2"; shift 2;;
    --channels)
      CHANNELS="$2"; shift 2;;
    --format)
      FORMAT="$2"; shift 2;;
    --dry-run)
      DRY_RUN=1; shift;;
    --include-wav)
      INCLUDE_WAV=1; shift;;
    --force)
      FORCE=1; shift;;
    *)
      echo "Unknown option: $1" >&2; exit 1;;
  esac
done

# Default output to normalized derivatives if not specified
if [[ -z "$OUT_DIR" ]]; then
  OUT_DIR="data/processed_calls/normalized"
fi

if [[ ! -e "$INPUT_PATH" ]]; then
  echo "Error: input path not found: $INPUT_PATH" >&2
  exit 1
fi

mapfile -t EXTENSIONS < <(printf "%s\n" mp3 m4a aac ogg opus flac wma aiff aif webm wav)

should_convert() {
  local f="$1"
  local ext="${f##*.}"; ext="${ext,,}"
  if [[ "$ext" == "wav" ]]; then
    # WAV handled separately if INCLUDE_WAV is enabled
    return 1
  fi
  for e in "${EXTENSIONS[@]}"; do
    if [[ "$ext" == "$e" ]]; then return 0; fi
  done
  return 1
}

rel_path() {
  # Print path of $1 relative to $2
  python3 - "$1" "$2" <<'PY'
import os, sys
path, base = sys.argv[1], sys.argv[2]
print(os.path.relpath(path, base))
PY
}

convert_one() {
  local src="$1"; shift
  local out_dir="$1"; shift
  local do_force="$1"; shift

  local base_name name_no_ext dest_dir dst
  base_name=$(basename "$src")
  name_no_ext="${base_name%.*}"
  dest_dir="$out_dir"
  if [[ -z "$dest_dir" ]]; then
    dest_dir="$(dirname "$src")"
  fi
  mkdir -p "$dest_dir"
  dst="$dest_dir/$name_no_ext.wav"

  if [[ -f "$dst" && $do_force -eq 0 ]]; then
    echo "Skip (exists): $dst" >&2
    return 0
  fi

  # High quality SoX Resampler with high precision, clamp sample fmt
  local af="aresample=resampler=soxr:precision=28:cutoff=0.97"
  local ac="-ac $CHANNELS"
  local ar="-ar $RATE"
  local codec="-c:a $FORMAT"

  if [[ $DRY_RUN -eq 1 ]]; then
    echo "[DRY RUN] ffmpeg -y -hide_banner -loglevel error -i '$src' -vn $af $ac $ar $codec '$dst'"
    return 0
  fi
  ffmpeg -y -hide_banner -loglevel error -i "$src" -vn -af "$af" $ac $ar $codec "$dst"
  echo "Converted: $src -> $dst"
}

export -f should_convert convert_one rel_path

process_dir() {
  local dir="$1"; local outdir="$2"
  shopt -s globstar nullglob nocaseglob
  local files=()
  while IFS= read -r -d '' f; do files+=("$f"); done < <(find "$dir" -type f -print0)
  for f in "${files[@]}"; do
    local ext="${f##*.}"; ext="${ext,,}"
    local rel="$(rel_path "$f" "$dir")"
    local sub="$(dirname "$rel")"
    # If file is at root, dirname returns '.', avoid trailing '/.'
    if [[ "$sub" == "." ]]; then sub=""; fi
    local target_dir="$outdir"
    if [[ -n "$outdir" && -n "$sub" ]]; then
      target_dir="$outdir/$sub"
    fi

    if should_convert "$f"; then
      convert_one "$f" "$target_dir" $FORCE
    elif [[ "$ext" == "wav" && $INCLUDE_WAV -eq 1 ]]; then
      convert_one "$f" "$target_dir" $FORCE
    fi
  done
}

if [[ -d "$INPUT_PATH" ]]; then
  process_dir "$INPUT_PATH" "$OUT_DIR"
else
  ext="${INPUT_PATH##*.}"; ext="${ext,,}"
  if should_convert "$INPUT_PATH"; then
    convert_one "$INPUT_PATH" "$OUT_DIR" $FORCE
  elif [[ "$ext" == "wav" && $INCLUDE_WAV -eq 1 ]]; then
    convert_one "$INPUT_PATH" "$OUT_DIR" $FORCE
  else
    echo "Input is already WAV or unsupported type: $INPUT_PATH (use --include-wav to normalize/copy)" >&2
  fi
fi

echo "Done."
