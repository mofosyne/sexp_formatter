#!/usr/bin/env python3
# KiCADv8 Style Prettify S-Expression Formatter (sexp formatter) (minimal logic version)
# By Brian Khuu, 2024
# This script reformats KiCad-like S-expressions to match a specific formatting style.
# Note: This script modifies formatting only; it does not perform linting or validation.

import argparse
from pathlib import Path

def prettify_sexpr_minimal(sexpr_str):
    """
    Prettifies KiCad-like S-expressions according to a KiCADv8-style formatting (minimal logic version)

    Args:
        sexpr_str (str): The input S-expression string to be formatted.

    Returns:
        str: The prettified S-expression string.

    Example:
        # Input S-expression string
        sexpr = "(module (fp_text \"example\"))"

        # Settings for compact element handling
        compact_settings = [{"prefix": "pts", "elements per line": 4}]

        # Prettify the S-expression
        formatted_sexpr = prettify_sexpr_minimal(sexpr, compact_settings)
        print(formatted_sexpr)
    """

    indent = 0
    result = []

    in_quote = False
    escape_next_char = False
    singular_element = False

    # Iterate through the s-expression and apply formatting
    for char in sexpr_str:

        if char == '"' or in_quote:
            # Handle quoted strings, preserving their internal formatting
            result.append(char)
            if escape_next_char:
                escape_next_char = False
            elif char == '\\':
                escape_next_char = True
            elif char == '"':
                in_quote = not in_quote

        elif char == '(':
            # Check for compact element handling
            result.append('\n' + '\t' * indent + '(')
            singular_element = True
            indent += 1

        elif char == ')':
            # Handle closing elements
            indent -= 1
            if singular_element:
                result.append(')')
                singular_element = False
            else:
                result.append('\n' + '\t' * indent + ')')

        elif char.isspace():
            # Handling spaces
            if result and not result[-1].isspace() and result[-1] != '(':
                result.append(' ')

        else:
            # Handle any other characters
            result.append(char)

    # Join results list and strip out any spaces in the beginning and end of the document
    formatted_sexp = ''.join(result).strip()

    # Strip out any extra space on the right hand side of each line
    formatted_sexp = '\n'.join(line.rstrip() for line in formatted_sexp.split('\n')) + '\n'

    # Formatting of s-expression completed
    return formatted_sexp

# Argument Parsing
parser = argparse.ArgumentParser(description="Prettify S-expression files")
parser.add_argument("src", type=Path, help="Source file path")
parser.add_argument("--dst", type=Path, help="Destination file path")
args = parser.parse_args()

src_file = args.src.resolve()
dst_file = args.dst.resolve() if args.dst else None

# Open source file
with open(src_file, "r") as file:
    sexp_data = file.read()

# Format the S-expression
pretty_sexpr = prettify_sexpr_minimal(sexp_data)

# Save or output the result
if dst_file:
    with open(dst_file, "w") as file:
        file.write(pretty_sexpr)
else:
    print(pretty_sexpr)
