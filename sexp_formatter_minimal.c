/// usr/bin/env ccache gcc -Wall -Wextra -Werror -O3 -std=gnu17 "$0" -o /tmp/sexp_formatter -lm && /tmp/sexp_formatter "$@"; exit

// KiCADv8 Style Prettify S-Expression Formatter (sexp formatter) (minimal logic version)
// By Brian Khuu, 2024
// This script reformats KiCad-like S-expressions to match a specific formatting style.
// Note: This script modifies formatting only; it does not perform linting or validation.
//       Also in addition this is a minimal version which does not support compact element handling
//       (e.g. pts element will compact xy sub elements into one line)

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

struct PrettifySExprState
{
    unsigned int indent;
    bool in_quote;
    bool escape_next_char;
    bool singular_element;
    bool space_pending;
    char c_out_prev;
};

void prettify_sexpr_minimal(struct PrettifySExprState *state, char c, void (*output_func)(char, void *), void *context_putc)
{
    // Parse quoted string
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
        return;
    }

    // Parse space
    if (isspace(c))
    {
        // Handle spaces and newlines
        state->space_pending = true;
    }

    // Parse Opening parentheses
    if (c == '(')
    {
        // Handle opening parentheses
        state->space_pending = false;

        // Indented newline for this new parentheses unless at root
        if (state->indent > 0)
        {
            output_func('\n', context_putc);
            for (unsigned int j = 0; j < state->indent; ++j)
            {
                output_func('\t', context_putc);
            }
        }

        state->singular_element = true;
        state->indent++;

        output_func('(', context_putc);
        state->c_out_prev = '(';
        return;
    }

    // Parse Closing Brace
    if (c == ')')
    {
        // Handle closing parentheses
        state->space_pending = false;

        if (state->indent > 0)
        {
            state->indent--;
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
        return;
    }

    // Parse Characters
    if (c != '\0')
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
        else if (state->space_pending)
        {
            // Add space before this character
            output_func(' ', context_putc);
            state->space_pending = false;
        }

        output_func(c, context_putc);
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
        printf("S-Expression Formatter Minimal (Brian Khuu 2024)\n");
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
        prettify_sexpr_minimal(&state, src_char, &putc_handler, dst_file);
    }

    // Wrapup and Cleanup
    fclose(src_file);
    if (dst_file != stdout)
    {
        fclose(dst_file);
    }

    return EXIT_SUCCESS;
}
