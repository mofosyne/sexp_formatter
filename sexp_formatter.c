/// usr/bin/env ccache gcc -Wall -Wextra -Werror -O3 -std=gnu17 "$0" -o /tmp/sexp_formatter -lm && /tmp/sexp_formatter "$@"; exit

// KiCADv8 Style Prettify S-Expression Formatter (sexp formatter)
// By Brian Khuu, 2024
// This script reformats KiCad-like S-expressions to match a specific formatting style.
// Note: This script modifies formatting only; it does not perform linting or validation.

#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/*****************************************************************************/

// The size of this should be larger than the largest prefixes in fixed_indent_prefixes
#define PRETTIFY_SEXPR_PREFIX_BUFFER_SIZE 256

// This is to account for files where a list may contain long list of sub lists that we would prefer
// to be presented as a single line until we reach a column limit
// (e.g. In kicad there is xy lists that takes up lots and lots of lines within pts list if not condensed)
#define PRETTIFY_SEXPR_FIXED_INDENT_COLUMN_LIMIT 99

// If a list has tokens inside that exceeds this wrap threshold then we would prefer to shift it to the next line
#define PRETTIFY_SEXPR_CONSECUTIVE_TOKEN_WRAP_THRESHOLD 72

// Indentation character used
#define PRETTIFY_SEXPR_INDENT_CHAR '\t'
#define PRETTIFY_SEXPR_INDENT_SIZE 1

// Lookup table of lists that require special handling as fixed indent lists
const char *fixed_indent_prefixes[] = {"pts"};
const int fixed_indent_prefixes_entries_count = sizeof(fixed_indent_prefixes) / sizeof(fixed_indent_prefixes[0]);

// Prettify S-Expr State
struct PrettifySExprState
{
    unsigned int indent;
    unsigned int column;
    char c_out_prev;

    // Parsing state
    bool in_quote;
    bool escape_next_char;
    bool singular_element;
    bool space_pending;

    // Prefix scanner to check if we should be in fixed indent mode
    bool scanning_for_prefix;
    int prefix_count_in_buffer;
    char prefix_buffer[PRETTIFY_SEXPR_PREFIX_BUFFER_SIZE];

    // Fixed indent feature to place multiple elements in the same line for compactness
    bool fixed_indent_mode;
    unsigned int fixed_indent;
};

/*
 * Formatting rules (Based on KiCAD S-Expression Style Guide):
 * - All extra (non-indentation) whitespace is trimmed.
 * - Indentation is one tab.
 * - Starting a new list (open paren) starts a new line with one deeper indentation.
 * - Lists with no inner lists go on a single line.
 * - End of multi-line lists (close paren) goes on a single line at the same indentation as its start.
 * - If fixed-indent mode is active and within column limits, parentheses will stay on the same line.
 * - Closing parentheses align with the indentation of the corresponding opening parenthesis.
 * - Quoted strings are treated as a single token.
 * - Tokens exceeding the column threshold are moved to the next line.
 * - Singular elements are inlined (e.g., `()`).
 * - Output ends with a newline to ensure POSIX compliance.
 *
 * For example:
 * (first
 *     (second
 *         (third list)
 *         (another list)
 *     )
 *     (fifth)
 *     (sixth thing with lots of tokens
 *         (first sub list)
 *         (second sub list)
 *         and another series of tokens
 *     )
 * )
 */
void prettify_sexpr(struct PrettifySExprState *state, char c, void (*output_func)(char, void *), void *context_putc)
{
    // Parse quoted string
    if (state->in_quote || c == '"')
    {
        // Handle quoted strings
        if (state->space_pending)
        {
            // Add space before this quoted string
            output_func(' ', context_putc);
            state->column += 1;
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
        state->column += 1;
        state->c_out_prev = c;
        return;
    }

    // Parse space
    if (isspace(c))
    {
        // Handle spaces and newlines
        state->space_pending = true;

        if (state->scanning_for_prefix)
        {
            bool matched = false;
            for (int key_lookup_index = 0; key_lookup_index < fixed_indent_prefixes_entries_count; key_lookup_index++)
            {
                // Check if we got a match against an expected prefix
                const char *expected_key_entry = fixed_indent_prefixes[key_lookup_index];
                bool entry_matched = true;

                for (int i = 0; i < state->prefix_count_in_buffer; i++)
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
            }

            state->scanning_for_prefix = false;
        }
        return;
    }

    // Parse Opening parentheses
    if (c == '(')
    {
        // Handle opening parentheses
        state->space_pending = false;

        // Pre Opening parentheses Actions
        if (state->fixed_indent_mode)
        {
            // In fixed indent, visually compact mode
            if (state->column < PRETTIFY_SEXPR_FIXED_INDENT_COLUMN_LIMIT && state->c_out_prev == ')')
            {
                // Is a consecutive list and still within column limit
                output_func(' ', context_putc);
                state->column += 1;
                state->space_pending = false;
            }
            else
            {
                // List is either beyond column limit or not after another list
                // Move this list to the next line

                output_func('\n', context_putc);
                state->column = 0;

                for (unsigned int j = 0; j < state->fixed_indent; ++j)
                {
                    output_func(PRETTIFY_SEXPR_INDENT_CHAR, context_putc);
                }
                state->column += state->fixed_indent * PRETTIFY_SEXPR_INDENT_SIZE;
            }
        }
        else
        {
            // Start scanning for prefix for special list handling
            state->scanning_for_prefix = true;
            state->prefix_count_in_buffer = 0;

            if (state->indent > 0)
            {
                output_func('\n', context_putc);
                state->column = 0;

                for (unsigned int j = 0; j < state->indent; ++j)
                {
                    output_func(PRETTIFY_SEXPR_INDENT_CHAR, context_putc);
                }
                state->column += state->fixed_indent * PRETTIFY_SEXPR_INDENT_SIZE;
            }
        }

        state->singular_element = true;
        state->indent++;

        output_func('(', context_putc);
        state->column += 1;

        state->c_out_prev = '(';
        return;
    }

    // Parse Closing Brace
    if (c == ')')
    {
        // Handle closing parentheses
        state->space_pending = false;
        state->scanning_for_prefix = false;

        if (state->indent > 0)
        {
            state->indent--;
        }

        // Check if in fixed indent mode and if it's already finished
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
            state->column += 1;

            state->singular_element = false;
        }
        else
        {
            // End of a parent element
            output_func('\n', context_putc);
            state->column = 0;

            for (unsigned int j = 0; j < state->indent; ++j)
            {
                output_func(PRETTIFY_SEXPR_INDENT_CHAR, context_putc);
            }
            state->column += state->fixed_indent * PRETTIFY_SEXPR_INDENT_SIZE;

            output_func(')', context_putc);
            state->column += 1;
        }

        // Add a newline if it's the end of the document
        if (state->indent <= 0)
        {
            output_func('\n', context_putc);
            state->column = 0;
        }

        state->c_out_prev = ')';
        return;
    }

    // Parse Characters
    if (c != '\0')
    {
        // Handle other characters

        // Pre character actions
        if (state->c_out_prev == ')')
        {
            // Indented newline for this character
            output_func('\n', context_putc);
            state->column = 0;

            for (unsigned int j = 0; j < state->indent; ++j)
            {
                output_func(PRETTIFY_SEXPR_INDENT_CHAR, context_putc);
            }
            state->column += state->fixed_indent * PRETTIFY_SEXPR_INDENT_SIZE;

            state->space_pending = false;
        }
        else if (!state->fixed_indent_mode && state->column >= PRETTIFY_SEXPR_CONSECUTIVE_TOKEN_WRAP_THRESHOLD)
        {
            // Indented newline for this character
            output_func('\n', context_putc);
            state->column = 0;

            for (unsigned int j = 0; j < state->indent; ++j)
            {
                output_func(PRETTIFY_SEXPR_INDENT_CHAR, context_putc);
            }
            state->column += state->fixed_indent * PRETTIFY_SEXPR_INDENT_SIZE;

            state->space_pending = false;
        }
        else if (state->space_pending && state->c_out_prev != '(')
        {
            // Space was pending
            output_func(' ', context_putc);
            state->column += 1;

            state->space_pending = false;
        }

        // Add to prefix scanning buffer if scanning for special list handling detection
        if (state->scanning_for_prefix)
        {
            if (state->prefix_count_in_buffer < (PRETTIFY_SEXPR_PREFIX_BUFFER_SIZE - 1))
            {
                state->prefix_buffer[state->prefix_count_in_buffer] = c;
                state->prefix_count_in_buffer++;
            }
        }

        // Add character to list
        output_func(c, context_putc);
        state->column += 1;

        state->c_out_prev = c;
        return;
    }
}

/*****************************************************************************/

void putc_handler(char c, void *context_putc) { fputc(c, (FILE *)context_putc); }

// Main function
int main(int argc, char **argv)
{
    const char *prog_name = argv[0];

    if (argc == 1 || (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)))
    {
        printf("S-Expression Formatter (Brian Khuu 2024)\n");
        printf("\n");
        printf("Usage:\n");
        printf("  %s -     -      Standard Input To Standard Output\n", prog_name);
        printf("  %s -     [dst]  Standard Input To File Output\n", prog_name);
        printf("  %s [src] -      File Input To Standard Output\n", prog_name);
        printf("  %s [src]        File Input To Standard Output\n", prog_name);
        printf("  %s [src] [dst]  File Input To File Output\n", prog_name);
        printf("\n");
        printf("Options:\n");
        printf("  -h --help       Show Help Message\n");
        return EXIT_SUCCESS;
    }

    const char *src_path = argc >= 2 ? argv[1] : NULL;
    const char *dst_path = argc >= 3 ? argv[2] : NULL;

    // Get File Descriptor
    FILE *src_file = stdin;
    if (src_path && strcmp(src_path, "-") != 0)
    {
        src_file = fopen(src_path, "r");
        if (!src_file)
        {
            perror("Error opening source file");
            return EXIT_FAILURE;
        }
    }

    // Open the destination file else default to standard output
    FILE *dst_file = stdout;
    if (dst_path && strcmp(dst_path, "-") != 0)
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
