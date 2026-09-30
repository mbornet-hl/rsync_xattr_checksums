#!/usr/bin/bash

set -euo pipefail

usage()
{
    echo "Usage: $0 <config_file> <output_directory>" >&2
    exit 1
}

normalize_unit()
{
    local raw="$1"
    echo "$raw" | tr '[:lower:]' '[:upper:]'
}

if [[ $# -lt 2 ]]; then
    usage
fi

CONFIG_FILE="$1"
OUTPUT_DIR="$2"

if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "Error: configuration file '$CONFIG_FILE' not found." >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

echo "=== Starting dataset generation: $OUTPUT_DIR ==="

while IFS= read -r line || [[ -n "$line" ]]; do
    # Strip leading/trailing whitespaces
    trimmed=$(echo "$line" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')

    # Ignore comments and empty lines
    if [[ -z "$trimmed" ]] || [[ "$trimmed" =~ ^# ]]; then
        continue
    fi

    # Parse key-value structure (<size> : <count>)
    if [[ "$trimmed" =~ ^([0-9]+[a-zA-Z]+)[[:space:]]*:[[:space:]]*([0-9]+)$ ]]; then
        size_label="${BASH_REMATCH[1]}"
        count="${BASH_REMATCH[2]}"
    else
        echo "Warning: skipping invalid line -> $line" >&2
        continue
    fi

    fallocate_size=$(normalize_unit "$size_label")
    dir_name="d_${size_label}"
    target_dir="${OUTPUT_DIR}/${dir_name}"

    mkdir -p "$target_dir"
    echo "[+] Creating $count file(s) of size $size_label in $target_dir"

    for ((i=1; i<=count; i++)); do
        seq_num=$(printf "%04d" "$i")
        file_path="${target_dir}/f_${size_label}_${seq_num}"

        # Write unique payload bytes to guarantee distinct cryptographic hash
        printf "%s_%s_%s_%s\n" "$(date +%s%N)" "$RANDOM" "$i" "$file_path" > "$file_path"

        # Extend file size instantly without full payload I/O bottleneck
        if ! fallocate -l "$fallocate_size" "$file_path" 2>/dev/null; then
            truncate -s "$fallocate_size" "$file_path"
        fi
    done

done < "$CONFIG_FILE"

echo "=== Dataset generation completed ==="
