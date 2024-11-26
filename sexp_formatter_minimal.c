/// usr/bin/env ccache gcc -Wall -Wextra -Werror -O3 -std=gnu17 "$0" -o /tmp/sexp_formatter -lm && /tmp/sexp_formatter "$@"; exit

// TODO: Transfer the main compacting logic across

// KiCADv8 Style Prettify S-Expression Formatter (sexp formatter) (minimal logic version)
// By Brian Khuu, 2024
// This script reformats KiCad-like S-expressions to match a specific formatting style.
// Note: This script modifies formatting only; it does not perform linting or validation.
//       Also in addition this is a minimal version which does not support compact element handling
//       (e.g. pts element will compact xy sub elements into one line)

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

/*****************************************************************************/

struct PrettifySExprState
{
    int indent;
    bool in_quote;
    bool escape_next_char;
    bool singular_element;
    bool space_pending;
    char c_out_prev;
};

void prettify_sexpr_minimal(struct PrettifySExprState *state, char c, void (*output_func)(char, void *), void *context_putc)
{
    if (state->in_quote || c == '"')
    {
        // Handle quoted strings
        if (state->space_pending)
        {
            output_func(' ', context_putc);
            state->space_pending = false;
        }

        if (state->escape_next_char)
        {
            state->escape_next_char = false;
        }
        else if (c == '\\')
        {
            state->escape_next_char = true;
        }
        else if (c == '"')
        {
            state->in_quote = !state->in_quote;
        }

        output_func(c, context_putc);
        state->c_out_prev = c;
    }
    else if (c == '(')
    {
        // Handle opening parentheses
        state->space_pending = false;
        if (state->indent > 0)
        {
            output_func('\n', context_putc);
            for (int j = 0; j < state->indent; ++j)
            {
                output_func('\t', context_putc);
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
        state->indent--;
        if (state->singular_element)
        {
            output_func(')', context_putc);
            state->singular_element = false;
        }
        else
        {
            output_func('\n', context_putc);
            for (int j = 0; j < state->indent; ++j)
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
        bool prev_is_space = isspace(state->c_out_prev);
        bool prev_is_open_brace = state->c_out_prev == '(';
        if (!prev_is_space && !prev_is_open_brace)
        {
            state->space_pending = true;
        }
    }
    else
    {
        // Handle other characters
        if (state->c_out_prev == ')')
        {
            output_func('\n', context_putc);
            for (int j = 0; j < state->indent; ++j)
            {
                output_func('\t', context_putc);
            }
            state->space_pending = false;
        }
        else if (state->space_pending)
        {
            output_func(' ', context_putc);
            state->space_pending = false;
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
    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Usage: %s <src> [dst]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src_path = argv[1];
    const char *dst_path = argc == 3 ? argv[2] : NULL;

    /*
     * Map the source file
     */

    // Get File Descriptor
    int fd = open(src_path, O_RDONLY);
    if (fd < 0)
    {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // Check File Statistics
    struct stat st;
    if (fstat(fd, &st) < 0)
    {
        perror("Error retrieving file information");
        close(fd);
        return EXIT_FAILURE;
    }

    // Check if File is empty
    size_t source_file_size = st.st_size;
    if (source_file_size == 0)
    {
        fprintf(stderr, "File is empty: %s\n", src_path);
        close(fd);
        return EXIT_FAILURE;
    }

    char *src_data = mmap(NULL, source_file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (src_data == MAP_FAILED)
    {
        perror("Error mapping file");
        close(fd);
        return EXIT_FAILURE;
    }

    // File successfully memory mapped,
    // can now safely close file descriptor
    close(fd);

    /*
     * Open File Destination
     */

    // Open the destination file
    FILE *dst_file = stdout;
    if (dst_path)
    {
        dst_file = fopen(dst_path, "wb");
        if (!dst_file)
        {
            perror("Error opening destination file");
            munmap(src_data, source_file_size);
            return EXIT_FAILURE;
        }
    }

    /*
     * Prettify the content
     */
    struct PrettifySExprState state = {0};
    for (size_t i = 0; i < source_file_size; ++i)
    {
        prettify_sexpr_minimal(&state, src_data[i], &putc_handler, dst_file);
    }

    /*
     * Wrapup and Cleanup
     */
    munmap(src_data, source_file_size);
    if (dst_file != stdout)
    {
        fclose(dst_file);
    }

    return EXIT_SUCCESS;
}
