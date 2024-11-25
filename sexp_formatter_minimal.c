/// usr/bin/env ccache gcc -Wall -Wextra -Werror -O3 -std=gnu17 "$0" -o /tmp/sexp_formatter -lm && /tmp/sexp_formatter "$@"; exit
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

struct PrettifySExprState 
{
    int indent;
    bool in_quote;
    bool escape_next_char;
    bool singular_element;
    bool space_pending;
    char c_out_prev;
};

// Function to prettify KiCad-like S-expressions
void prettify_sexpr_minimal_r(struct PrettifySExprState *state, char c, FILE *output)
{
    if (state->in_quote || c == '"')
    {
        // Handle quoted strings
        if (state->space_pending)
        {
            fputc(' ', output);
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

        fputc(c, output);
        state->c_out_prev = c;
    }
    else if (c == '(')
    {
        // Handle opening parentheses
        state->space_pending = false;
        if (state->indent > 0)
        {
            fputc('\n', output);
            for (int j = 0; j < state->indent; ++j)
            {
                fputc('\t', output);
            }
        }

        state->singular_element = true;
        state->indent++;

        fputc('(', output);
        state->c_out_prev = '(';
    }
    else if (c == ')')
    {
        // Handle closing parentheses
        state->space_pending = false;
        state->indent--;
        if (state->singular_element)
        {
            fputc(')', output);
            state->singular_element = false;
        }
        else
        {
            fputc('\n', output);
            for (int j = 0; j < state->indent; ++j)
            {
                fputc('\t', output);
            }
            fputc(')', output);
        }

        // Add a newline if it's the end of the document
        if (state->indent <= 0)
        {
            fputc('\n', output);
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
            fputc('\n', output);
            for (int j = 0; j < state->indent; ++j)
            {
                fputc('\t', output);
            }
            state->space_pending = false;
        }
        else if (state->space_pending)
        {
            fputc(' ', output);
            state->space_pending = false;
        }

        fputc(c, output);
        state->c_out_prev = c;
    }
}

// Function to prettify KiCad-like S-expressions
void prettify_sexpr_minimal(const char *sexpr_str, size_t length, FILE *output)
{
    struct PrettifySExprState state = {0};

    for (size_t i = 0; i < length; ++i)
    {
        prettify_sexpr_minimal_r(&state, sexpr_str[i], output);
    }
}

// Function to memory-map a file
char *map_file(const char *filepath, size_t *size_out)
{
    int fd = open(filepath, O_RDONLY);
    if (fd < 0)
    {
        perror("Error opening file");
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) < 0)
    {
        perror("Error retrieving file information");
        close(fd);
        return NULL;
    }

    if (st.st_size == 0)
    {
        fprintf(stderr, "File is empty: %s\n", filepath);
        close(fd);
        return NULL;
    }

    char *data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED)
    {
        perror("Error mapping file");
        close(fd);
        return NULL;
    }

    *size_out = st.st_size;
    close(fd);
    return data;
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

    // Map the source file
    size_t src_size;
    char *src_data = map_file(src_path, &src_size);
    if (!src_data)
    {
        return EXIT_FAILURE;
    }

    // Open the destination file
    FILE *dst_file = stdout;
    if (dst_path)
    {
        dst_file = fopen(dst_path, "wb");
        if (!dst_file)
        {
            perror("Error opening destination file");
            munmap(src_data, src_size);
            return EXIT_FAILURE;
        }
    }

    // Prettify the content
    prettify_sexpr_minimal(src_data, src_size, dst_file);

    // Cleanup
    munmap(src_data, src_size);
    if (dst_file != stdout)
    {
        fclose(dst_file);
    }

    return EXIT_SUCCESS;
}
