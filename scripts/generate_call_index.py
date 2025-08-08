#!/usr/bin/env python3
# filepath: scripts/generate_call_index.py

import json
import os
import sys
from pathlib import Path
from datetime import datetime


def generate_index(processed_dir):
    """Generate a comprehensive index of all processed master calls."""

    index = {
        "version": "1.0",
        "generated": datetime.now().isoformat(),
        "species": {},
        "calls": []
    }

    metadata_dir = Path(processed_dir) / "metadata"

    for metadata_file in metadata_dir.glob("*.json"):
        with open(metadata_file, 'r') as f:
            metadata = json.load(f)

        base_name = metadata_file.stem

        call_entry = {
            "id": base_name,
            "species": metadata.get("species", "unknown"),
            "callType": metadata.get("callType", "unknown"),
            "season": metadata.get("season", "all"),
            "difficulty": metadata.get("difficulty", 1),
            "duration": metadata.get("duration", 0),
            "files": {
                # Adjust extension as needed
                "audio": f"../master_calls/{base_name}.wav",
                "mfc": f"mfc/{base_name}.mfc",
                "waveform": f"waveforms/{base_name}.json",
                "metadata": f"metadata/{base_name}.json"
            }
        }

        index["calls"].append(call_entry)

        # Group by species
        species = metadata.get("species", "unknown")
        if species not in index["species"]:
            index["species"][species] = []
        index["species"][species].append(base_name)

    # Save index
    index_path = Path(processed_dir) / "index.json"
    with open(index_path, 'w') as f:
        json.dump(index, f, indent=2)

    print(f"Generated index with {len(index['calls'])} calls")
    print(f"Species: {', '.join(index['species'].keys())}")

    return index_path


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <processed_dir>")
        sys.exit(1)

    generate_index(sys.argv[1])
