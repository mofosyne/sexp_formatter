/// usr/bin/env ccache gcc -Wall -Wextra -Werror -O3 -std=gnu17 "$0" -o /tmp/sexp_formatter -lm && /tmp/sexp_formatter "$@"; exit

// KiCADv8 Style Prettify S-Expression Formatter (sexp formatter)
// By Brian Khuu, 2024
// This script reformats KiCad-like S-expressions to match a specific formatting style.
// Note: This script modifies formatting only; it does not perform linting or validation.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>


/*****************************************************************************/

#define PRETTIFY_SEXPRESSION_PREFIX_BUFFER_SIZE 256
#define PRETTIFY_SEXPRESSION_FIXED_INDENT_CHILD_ELEMENT_COUNT_PER_LINE 4

const char *fixed_indent_prefixes[] = {
    "pts",
};
const int fixed_indent_prefixes_entries_count = sizeof(fixed_indent_prefixes) / sizeof(fixed_indent_prefixes[0]);

struct PrettifySExprState
{
    unsigned int indent;
    bool in_quote;
    bool escape_next_char;
    bool singular_element;
    bool space_pending;
    char c_out_prev;

    // Prefix scanner to check if we should be in fixed indent mode
    bool scanning_for_prefix;
    int prefix_count_in_buffer;
    char prefix_buffer[PRETTIFY_SEXPRESSION_PREFIX_BUFFER_SIZE];

    // Fixed indent feature to place multiple elements in the same line for compactness
    bool fixed_indent_mode;
    unsigned int fixed_indent;
    unsigned int fixed_indent_child_element_count;
};

void prettify_sexpr(struct PrettifySExprState *state, char c, void (*output_func)(char, void *), void *context_putc)
{
    if (state->in_quote || c == '"')
    {
        // Handle quoted strings
        if (state->space_pending)
        {
            // Add space before this quoted string
            output_func(' ', context_putc);
            state->space_pending = false;
        }

        if (state->escape_next_char)
        {
            // Escaped Char
            state->escape_next_char = false;
        }
        else if (c == '\\')
        {
            // Escape Next Char
            state->escape_next_char = true;
        }
        else if (c == '"')
        {
            // End of quoted string mode
            state->in_quote = !state->in_quote;
        }

        output_func(c, context_putc);
        state->c_out_prev = c;
    }
    else if (c == '(')
    {
        // Handle opening parentheses
        if (state->fixed_indent_mode)
        {
            // Indented newline for this new parentheses unless at root
            if (state->indent >= state->fixed_indent)
            {
                if (state->fixed_indent_child_element_count == 0 || state->fixed_indent_child_element_count % PRETTIFY_SEXPRESSION_FIXED_INDENT_CHILD_ELEMENT_COUNT_PER_LINE == 0)
                {
                    // Indented newline for this new parentheses unless at root
                    state->space_pending = false;
                    if (state->fixed_indent > 0)
                    {
                        output_func('\n', context_putc);
                        for (unsigned int j = 0; j < state->fixed_indent; ++j)
                        {
                            output_func('\t', context_putc);
                        }
                    }
                }
                else
                {
                    if (state->space_pending)
                    {
                        // Add space before this quoted string
                        output_func(' ', context_putc);
                        state->space_pending = false;
                    }
                }
                state->fixed_indent_child_element_count++;
            }
        }
        else
        {
            state->space_pending = false;
            state->scanning_for_prefix = true;
            state->prefix_count_in_buffer = 0;

            // Indented newline for this new parentheses unless at root
            if (state->indent > 0)
            {
                output_func('\n', context_putc);
                for (unsigned int j = 0; j < state->indent; ++j)
                {
                    output_func('\t', context_putc);
                }
            }
        }

        state->singular_element = true;
        state->indent++;

        output_func('(', context_putc);
        state->c_out_prev = '(';
    }
    else if (c == ')')
    {
        // Handle closing parentheses
        state->space_pending = false;

        if (state->indent > 0)
        {
            state->indent--;
        }

        if (state->fixed_indent_mode)
        {
            if (!state->singular_element && state->indent <= state->fixed_indent)
            {
                state->fixed_indent_mode = false;
            }
        }

        if (state->singular_element)
        {
            // End of singular element 
            output_func(')', context_putc);
            state->singular_element = false;
        }
        else
        {
            // End of a parent element
            output_func('\n', context_putc);
            for (unsigned int j = 0; j < state->indent; ++j)
            {
                output_func('\t', context_putc);
            }
            output_func(')', context_putc);
        }

        // Add a newline if it's the end of the document
        if (state->indent <= 0)
        {
            output_func('\n', context_putc);
        }

        state->c_out_prev = ')';
    }
    else if (isspace(c))
    {
        // Handle spaces and newlines
        state->space_pending = true;

        if (state->scanning_for_prefix)
        {

            bool matched = false;
            for (int key_lookup_index = 0 ; key_lookup_index < fixed_indent_prefixes_entries_count ; key_lookup_index++)
            {
                // Check if we got a match against an expected prefix
                const char *expected_key_entry = fixed_indent_prefixes[key_lookup_index];
                bool entry_matched = true;

                for (int i = 0; i < state->prefix_count_in_buffer ; i++)
                {
                    if (state->prefix_buffer[i] != expected_key_entry[i])
                    {
                        entry_matched = false;
                        break;
                    }
                }

                if (entry_matched)
                {
                    matched = true;
                    break;
                }
            }

            if (matched)
            {
                state->fixed_indent_mode = true;
                state->fixed_indent = state->indent;
                state->fixed_indent_child_element_count = 0;
            }

            state->scanning_for_prefix = false;
        }
    }
    else if (c != '\0')
    {
        // Handle other characters
        if (state->c_out_prev == ')')
        {
            // Indented newline for this character
            output_func('\n', context_putc);
            for (unsigned int j = 0; j < state->indent; ++j)
            {
                output_func('\t', context_putc);
            }
            state->space_pending = false;
        }
        else if (state->space_pending && state->c_out_prev != '(')
        {
            // Add space before this character
            output_func(' ', context_putc);
            state->space_pending = false;
        }

        if (state->scanning_for_prefix)
        {
            if (state->prefix_count_in_buffer < (PRETTIFY_SEXPRESSION_PREFIX_BUFFER_SIZE - 1))
            {
                state->prefix_buffer[state->prefix_count_in_buffer] = c;
                state->prefix_count_in_buffer++;
            }
        }

        output_func(c, context_putc);
        state->c_out_prev = c;
    }
}

/*****************************************************************************/

void putc_handler(char c, void *context_putc)
{
    fputc(c, (FILE *) context_putc);
}

// Main function
int main(int argc, char **argv)
{
    const char *prog_name = argv[0];
    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Usage: %s <src> [dst]\n", prog_name);
        return EXIT_FAILURE;
    }

    const char *src_path = argv[1];
    const char *dst_path = argc == 3 ? argv[2] : NULL;

    // Get File Descriptor
    FILE *src_file = fopen(src_path, "r");
    if (!src_file)
    {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // Open the destination file else default to standard output
    FILE *dst_file = stdout;
    if (dst_path)
    {
        dst_file = fopen(dst_path, "wb");
        if (!dst_file)
        {
            perror("Error opening destination file");
            fclose(src_file);
            return EXIT_FAILURE;
        }
    }

    // Prettify the content
    char src_char;
    struct PrettifySExprState state = {0};
    while ((src_char = fgetc(src_file)) != EOF)
    {
        prettify_sexpr(&state, src_char, &putc_handler, dst_file);
    }

    // Wrapup and Cleanup
    fclose(src_file);
    if (dst_file != stdout)
    {
        fclose(dst_file);
    }

    return EXIT_SUCCESS;
}
