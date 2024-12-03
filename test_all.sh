#!/bin/bash
set -euo pipefail

# Make sure to always build latest version to test
make

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
if $all_passed; then
    echo "All tests passed!"
    exit 0
else
    echo "Some tests failed!"
    exit 1
fi
