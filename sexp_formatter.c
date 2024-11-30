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

// The size of this should be larger than the largest prefixes in compact_list_prefixes
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
const char *compact_list_prefixes[] = {"pts"};
const int compact_list_prefixes_entries_count = sizeof(compact_list_prefixes) / sizeof(compact_list_prefixes[0]);

// Lookup table of lists whose internal content should be a one liner
const char *shortform_prefixes[] = {"font", "stroke", "fill", "offset", "rotate", "scale"};
const int shortform_prefixes_entries_count = sizeof(shortform_prefixes) / sizeof(shortform_prefixes[0]);

// Prettify S-Expr State
struct PrettifySExprState
{
    // Settings
    char **compact_list_prefixes;
    int compact_list_prefixes_entries_count;
    char **shortform_prefixes;
    int shortform_prefixes_entries_count;

    // Parsing Position Tracking
    unsigned int indent;
    unsigned int column;
    char c_out_prev;

    // Parsing state
    bool in_quote;
    bool escape_next_char;
    bool singular_element;
    bool space_pending;

    // Prefix scanner to check if a list should be specially handled
    bool scanning_for_prefix;
    int prefix_count_in_buffer;
    char prefix_buffer[PRETTIFY_SEXPR_PREFIX_BUFFER_SIZE];

    // Fixed indent feature to place multiple elements in the same line for compactness
    bool compact_list_mode;
    unsigned int compact_list_indent;
    
    // Fixed indent feature to place multiple elements in the same line for compactness
    bool shortform_mode;
    unsigned int shortform_indent;
    
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
            state->prefix_buffer[state->prefix_count_in_buffer] = '\0';

            // Check if we got a match against an expected prefix for fixed indent mode
            if (state->compact_list_prefixes)
            {
                for (int key_lookup_index = 0; key_lookup_index < state->compact_list_prefixes_entries_count; key_lookup_index++)
                {
                    if (strcmp(state->prefix_buffer, state->compact_list_prefixes[key_lookup_index]) == 0)
                    {
                        state->compact_list_mode = true;
                        state->compact_list_indent = state->indent;
                        break;
                    }
                }
            }

            // Check if we got a match against an expected prefix for fixed indent mode
            if (state->shortform_prefixes)
            {
                for (int key_lookup_index = 0; key_lookup_index < state->shortform_prefixes_entries_count; key_lookup_index++)
                {
                    if (strcmp(state->prefix_buffer, state->shortform_prefixes[key_lookup_index]) == 0)
                    {
                        state->shortform_mode = true;
                        state->shortform_indent = state->indent;
                        break;
                    }
                }
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
        if (state->compact_list_mode)
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

                for (unsigned int j = 0; j < state->compact_list_indent; ++j)
                {
                    output_func(PRETTIFY_SEXPR_INDENT_CHAR, context_putc);
                }
                state->column += state->compact_list_indent * PRETTIFY_SEXPR_INDENT_SIZE;
            }
        }
        else if (state->shortform_mode)
        {
            // In one liner mode
            output_func(' ', context_putc);
            state->column += 1;
            state->space_pending = false;
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
                state->column += state->compact_list_indent * PRETTIFY_SEXPR_INDENT_SIZE;
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
        const bool curr_shortform_mode = state->shortform_mode;

        // Handle closing parentheses
        state->space_pending = false;
        state->scanning_for_prefix = false;

        if (state->indent > 0)
        {
            state->indent--;
        }

        // Check if compact list mode has completed
        if (state->compact_list_mode && state->indent < state->compact_list_indent)
        {
            state->compact_list_mode = false;
        }

        // Check if we have finished with a shortform list
        if (state->shortform_mode && state->indent < state->shortform_indent)
        {
            state->shortform_mode = false;
        }

        if (state->singular_element)
        {
            // End of singular element
            state->singular_element = false;
        }
        else if (!curr_shortform_mode)
        {
            // End of a parent element
            output_func('\n', context_putc);
            state->column = 0;

            for (unsigned int j = 0; j < state->indent; ++j)
            {
                output_func(PRETTIFY_SEXPR_INDENT_CHAR, context_putc);
            }
            state->column += state->compact_list_indent * PRETTIFY_SEXPR_INDENT_SIZE;
        }

        output_func(')', context_putc);
        state->column += 1;

        if (state->indent <= 0)
        {
            // Cap Root Element
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
        if (!state->shortform_mode && state->c_out_prev == ')')
        {
            // Is Bare token after a list that should be on next line
            // Dev Note: In KiCAD this may indicate a flag bug
            output_func('\n', context_putc);
            state->column = 0;

            for (unsigned int j = 0; j < state->indent; ++j)
            {
                output_func(PRETTIFY_SEXPR_INDENT_CHAR, context_putc);
            }
            state->column += state->indent * PRETTIFY_SEXPR_INDENT_SIZE;

            state->space_pending = false;
        }
        else if (!state->shortform_mode && !state->compact_list_mode && state->column >= PRETTIFY_SEXPR_CONSECUTIVE_TOKEN_WRAP_THRESHOLD)
        {
            // Token is above wrap threshold. Move token to next line
            output_func('\n', context_putc);
            state->column = 0;

            for (unsigned int j = 0; j < state->indent; ++j)
            {
                output_func(PRETTIFY_SEXPR_INDENT_CHAR, context_putc);
            }
            state->column += state->indent * PRETTIFY_SEXPR_INDENT_SIZE;

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

    state.compact_list_prefixes = (char**) compact_list_prefixes;
    state.compact_list_prefixes_entries_count = compact_list_prefixes_entries_count;

    //state.shortform_prefixes = (char**) shortform_prefixes;
    //state.shortform_prefixes_entries_count = shortform_prefixes_entries_count;

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
