#!/bin/bash

# Define the files to test
file_pairs_path_dir=./testcases/
file_pairs=(
    "group_and_image.kicad_pcb group_and_image_formatted.kicad_pcb"
    "Reverb_BTDR-1V.kicad_mod Reverb_BTDR-1V_formatted.kicad_mod"
    "Samtec_HLE-133-02-xx-DV-PE-LC_2x33_P2.54mm_Horizontal.kicad_mod Samtec_HLE-133-02-xx-DV-PE-LC_2x33_P2.54mm_Horizontal_formatted.kicad_mod"
)

executable=$1

# Temporary directory in /tmp
temp_dir="kicad_formatter_test"
mkdir -p "$temp_dir"

# Test each file pair
# Make sure the executable exists and is executable
if [[ ! -x "$executable" ]]; then
    echo "ERROR: Executable '$executable' not found or not executable!"
    exit 1
fi

all_passed=true
for pair in "${file_pairs[@]}"; do
    # Split pair into unformatted and formatted file
    read -r unformatted formatted <<< "$pair"

    # Run the formatter and save the output to a temp file
    temp_output="$temp_dir/$(basename "$unformatted")"
    $executable "$file_pairs_path_dir$unformatted" "$temp_output"

    # Compare the output with the corresponding formatted file
    if diff -q "$file_pairs_path_dir$formatted" "$temp_output" > /dev/null; then
        : #echo "PASS: $executable - $unformatted matches $formatted"
    else
        echo ""
        echo "========================================================================"
        echo " FAILED ARGUMENT : $executable"
        echo " PROCESSED INPUT : $unformatted"
        echo " EXPECTED OUTPUT : $unformatted"
        echo " DIFF SNIPPET:"
        diff --color=always -u "$file_pairs_path_dir$formatted" "$temp_output" | head -n 10 # Show the first 20 lines of the diff
        all_passed=false
        echo "========================================================================"
        echo ""
    fi
done

# Clean up
rm -rf "$temp_dir"

# Final result
if $all_passed; then
    echo "All tests passed for $executable"
    exit 0
else
    echo "Some tests failed"
    exit 1
fi
