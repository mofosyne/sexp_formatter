#!/bin/bash

# Define the files to test
file_pairs_path_dir=./testcases/
file_pairs=(
    "pts_list_test_symbol.kicad_sym pts_list_test_symbol_formatted.kicad_sym"
    "ad620.kicad_sym ad620_formatted.kicad_pcb"
    "group_and_image.kicad_pcb group_and_image_formatted.kicad_pcb"
    "Reverb_BTDR-1V.kicad_mod Reverb_BTDR-1V_formatted.kicad_mod"
    "Samtec_HLE-133-02-xx-DV-PE-LC_2x33_P2.54mm_Horizontal.kicad_mod Samtec_HLE-133-02-xx-DV-PE-LC_2x33_P2.54mm_Horizontal_formatted.kicad_mod"
)

executable=$1
nozerostyle=${2:-false}

all_passed=true

# Temporary directory in /tmp
temp_dir="temp"
mkdir -p "$temp_dir/zerostyle"
mkdir -p "$temp_dir/standard"
mkdir -p "$temp_dir/compact"

# Test each file pair
# Make sure the executable exists and is executable
if [[ ! -x "$executable" ]]; then
    echo "ERROR: Executable '$executable' not found or not executable!"
    exit 1
fi

function execute_test ()
{
    # Run the formatter and save the output to a temp file
    local executable=$1
    local unformatted=$2
    local formatted=$3

    local temp_output="$temp_dir/$formatted"
    $executable "$file_pairs_path_dir$unformatted" "$temp_output"

    # Compare the output with the corresponding formatted file
    if diff -q "$file_pairs_path_dir$formatted" "$temp_output" > /dev/null; then
        : #echo "PASS: $executable - $unformatted matches $formatted"
        return 0
    else
        echo ""
        echo "========================================================================"
        echo "nozerostyle = $nozerostyle"
        echo " FAILED ARGUMENT : $executable $file_pairs_path_dir$unformatted $temp_output"
        echo " PROCESSED INPUT : $unformatted"
        echo " EXPECTED OUTPUT : $formatted"
        echo " DIFF SNIPPET:"
        diff --color=always -u "$file_pairs_path_dir$formatted" "$temp_output" | head -n 20 # Show the first 20 lines of the diff
        echo "========================================================================"
        echo " meld $file_pairs_path_dir$formatted $temp_output"
        echo "========================================================================"
        echo ""
        all_passed=false
        return -1
    fi
}


for pair in "${file_pairs[@]}"; do
    # Split pair into unformatted and formatted file
    read -r unformatted formatted <<< "$pair"
    if [ ! "$nozerostyle" = true ]; then
        execute_test "$executable" "$unformatted" "zerostyle/$formatted"
    fi

    

    execute_test "$executable -p kicad" "$unformatted" "standard/$formatted"
    execute_test "$executable -p kicad-compact" "$unformatted" "compact/$formatted"
done

# Final result
if $all_passed; then
    rm -rf "$temp_dir"
    echo "All tests passed for $executable"
    exit 0
else
    echo "Some tests failed"
    exit 1
fi
