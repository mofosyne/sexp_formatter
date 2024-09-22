#!/usr/bin/env python3
# KiCADv8 Style Prettify S-Expression Formatter (sexp formatter)
# By Brian Khuu, 2024
# This script reformats KiCad-like S-expressions to match a specific formatting style.
# Note: This script modifies formatting only; it does not perform linting or validation.
# Context: Compact element settings are added to support KiCAD-specific handling for readability, e.g., PCB_PLUGIN::formatPolyPts.

import os
import argparse
from pathlib import Path

def prettify_sexpr(sexpr_str, compact_element_settings):
    """
    Prettifies KiCad-like S-expressions according to a KiCADv8-style formatting.

    Args:
        sexpr_str (str): The input S-expression string to be formatted.
        compact_element_settings (list of dict): A list of dictionaries containing element-specific settings.
            Each dictionary should contain:
                - "prefix" (str): The prefix of elements that should be handled specially.
                - "elements per line" (int): The number of elements per line in compact mode (optional).

    Returns:
        str: The prettified S-expression string.

    Example:
        # Input S-expression string
        sexpr = "(module (fp_text \"example\"))"

        # Settings for compact element handling
        compact_settings = [{"prefix": "pts", "elements per line": 4}]

        # Prettify the S-expression
        formatted_sexpr = prettify_sexpr(sexpr, compact_settings)
        print(formatted_sexpr)
    """

    indent = 0
    result = []

    in_quote = False
    escape_next_char = False
    singular_element = False

    in_prefix_scan = False
    prefix_keyword_buffer = ""
    prefix_stack = []

    element_count_stack = [0]

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
            in_compact_mode = False
            elements_per_line = 0

            if compact_element_settings:
                parent_prefix = prefix_stack[-1] if (len(prefix_stack) > 0) else None
                for setting in compact_element_settings:
                    if setting.get("prefix") in prefix_stack:
                        in_compact_mode = True
                    if setting.get("prefix") == parent_prefix:
                        elements_per_line = setting.get("elements per line", 0)

            if in_compact_mode:
                if elements_per_line > 0:
                    parent_element_count = element_count_stack[-1]
                    if parent_element_count != 0 and ((parent_element_count % elements_per_line) == 0):
                        result.append('\n' + '\t' * indent)

                result.append('(')

            else:
                # New line and add an opening parenthesis with the current indentation
                result.append('\n' + '\t' * indent + '(')

            # Start Prefix Keyword Scanning
            in_prefix_scan = True
            prefix_keyword_buffer = ""

            # Start tracking if element is singular
            singular_element = True

            # Element Count Tracking
            element_count_stack[-1] += 1
            element_count_stack.append(0)

            indent += 1

        elif char == ')':
            # Handle closing elements
            indent -= 1
            element_count_stack.pop()

            if singular_element:
                result.append(')')
                singular_element = False
            else:
                result.append('\n' + '\t' * indent + ')')

            if in_prefix_scan:
                prefix_stack.append(prefix_keyword_buffer)
                in_prefix_scan = False

            prefix_stack.pop()

        elif char.isspace():
            # Handling spaces
            if result and not result[-1].isspace() and result[-1] != '(':
                result.append(' ')

                if in_prefix_scan:
                    # Capture Prefix Keyword
                    prefix_stack.append(prefix_keyword_buffer)

                    # Handle special compact elements
                    if compact_element_settings:
                            for setting in compact_element_settings:
                                if setting.get("prefix") == prefix_keyword_buffer:
                                    result.append('\n' + '\t' * indent)
                                    break

                    in_prefix_scan = False

        else:
            # Handle any other characters
            result.append(char)

            # Capture Prefix Keyword
            if in_prefix_scan:
                prefix_keyword_buffer += char

            # Dev Note: In my opinion, this shouldn't be here... but is here so that we can match KiCADv8's behavior when a ')' is following a non ')'
            singular_element = True

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

# Compact element settings for special handling
compact_element_settings = []
src_basename = os.path.basename(src_file)

if src_basename.endswith(".kicad_sym"):
    compact_element_settings.append({"prefix":"pts", "elements per line": 6})
elif src_basename.endswith(".kicad_mod"):
    pass
elif src_basename.endswith(".kicad_sch"):
    compact_element_settings.append({"prefix":"pts", "elements per line": 6})
elif src_basename.endswith(".kicad_pcb"):
    compact_element_settings.append({"prefix":"pts", "elements per line": 4})

# Format the S-expression
pretty_sexpr = prettify_sexpr(sexp_data, compact_element_settings)

# Save or output the result
if dst_file:
    with open(dst_file, "w") as file:
        file.write(pretty_sexpr)
else:
    print(pretty_sexpr)
