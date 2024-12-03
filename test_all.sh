#!/bin/bash
set -euo pipefail

# List of formatter executables
executables=(
    "./sexp_prettify_kicad_cli"
    "./sexp_prettify_cpp_cli"
    "./sexp_prettify_cli"
    "./sexp_prettify_cli.py"
)

# Temporary directory in /tmp
temp_dir="/tmp/kicad_formatter_test"
mkdir -p "$temp_dir"

echo ""
echo "========================================================================"
echo " TEST REPORT "
echo "========================================================================"

# Test each file pair
all_passed=true
# Run each formatter executable on the unformatted file
for executable in "${executables[@]}"; do
    # Make sure the executable exists and is executable
    if [[ ! -x "$executable" ]]; then
        echo "ERROR: Executable '$executable' not found or not executable!"
        exit 1
    fi
    # Run test
    ./test_standard_single.sh "$executable"
done

# Clean up
rm -rf "$temp_dir"

# Final result
echo ""
if $all_passed; then
    echo "All tests passed in all executables!"
    exit 0
else
    echo "Some tests failed in some executables!"
    exit 1
fi