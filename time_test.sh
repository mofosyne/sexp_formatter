#!/bin/bash

# Define the files to test
file_pairs_path_dir=./testcases/
file_pairs=(
    "group_and_image.kicad_pcb group_and_image_formatted.kicad_pcb"
    "Reverb_BTDR-1V.kicad_mod Reverb_BTDR-1V_formatted.kicad_mod"
    "Samtec_HLE-133-02-xx-DV-PE-LC_2x33_P2.54mm_Horizontal.kicad_mod Samtec_HLE-133-02-xx-DV-PE-LC_2x33_P2.54mm_Horizontal_formatted.kicad_mod"
)

executable=$1

# Test each file pair
# Make sure the executable exists and is executable
if [[ ! -x "$executable" ]]; then
    echo "ERROR: Executable '$executable' not found or not executable!"
    exit 1
fi

total_time=0

for pair in "${file_pairs[@]}"; do
    # Split pair into unformatted and formatted file
    read -r unformatted formatted <<< "$pair"

    real_time=$( { time $executable "$file_pairs_path_dir$unformatted" - ; } 2>&1 | grep real | awk '{print $2}' | sed 's/m/*60+/;s/s//' | bc)
    total_time=$(echo "$total_time + $real_time" | bc)

    real_time=$( { time $executable "$file_pairs_path_dir$formatted" - ; } 2>&1 | grep real | awk '{print $2}' | sed 's/m/*60+/;s/s//' | bc)
    total_time=$(echo "$total_time + $real_time" | bc)
done

#echo "Time taken for $executable to run though test suites is $total_time seconds"
printf "Time taken for %-40s to run though test suites is %s seconds\n" $executable $total_time