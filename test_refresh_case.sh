#!/bin/bash

# Define the files to test
file_pairs_path_dir=./testcases/
file_pairs=(
    "pts_list_test_symbol.kicad_sym pts_list_test_symbol_formatted.kicad_sym"
    "ad620.kicad_sym ad620_formatted.kicad_sym"
    "complex_hierarchy_schlib.kicad_sym complex_hierarchy_schlib_formatted.kicad_sym"
    "flat_hierarchy_schlib.kicad_sym flat_hierarchy_schlib_formatted.kicad_sym"
    "group_and_image.kicad_pcb group_and_image_formatted.kicad_pcb"
    "Reverb_BTDR-1V.kicad_mod Reverb_BTDR-1V_formatted.kicad_mod"
    "Samtec_HLE-133-02-xx-DV-PE-LC_2x33_P2.54mm_Horizontal.kicad_mod Samtec_HLE-133-02-xx-DV-PE-LC_2x33_P2.54mm_Horizontal_formatted.kicad_mod"
)

executable=$1

all_passed=true

# Test each file pair
# Make sure the executable exists and is executable
if [[ ! -x "$executable" ]]; then
    echo "ERROR: Executable '$executable' not found or not executable!"
    exit 1
fi

function create_test ()
{
    local executable=$1
    local unformatted=$2
    local formatted=$3
    $executable "$file_pairs_path_dir$unformatted" "$file_pairs_path_dir$formatted"
}

for pair in "${file_pairs[@]}"; do
    # Split pair into unformatted and formatted file
    read -r unformatted formatted <<< "$pair"
    create_test "$executable" "$unformatted" "zerostyle/$formatted"
    create_test "$executable -p kicad" "$unformatted" "standard/$formatted"
    create_test "$executable -p kicad-compact" "$unformatted" "compact/$formatted"
done

echo "Test Case Generated"
