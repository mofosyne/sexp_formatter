// KiCADv8 Style Prettify S-Expression Formatter (sexp formatter)
// By Brian Khuu, 2024
// This script reformats KiCad-like S-expressions to match a specific formatting style.
// Note: This script modifies formatting only; it does not perform linting or validation.

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sexp_prettify.h"

void putc_handler(char c, void *context_putc) { fputc(c, (FILE *)context_putc); }

void usage(const char *prog_name, bool full)
{
    if (full)
    {
        printf("S-Expression Formatter (Brian Khuu 2024)\n");
        printf("\n");
    }

    printf("Usage:\n");
    printf("  %s [OPTION]... SRC [DST]\n", prog_name);
    printf("  SRC                Source file path. If '-' then use standard stream input\n");
    printf("  DST                Destination file path. If omitted or '-' then use standard stream output\n");
    printf("\n");

    if (full)
    {
        printf("Options:\n");
        printf("  -h                 Show Help Message\n");
        printf("  -w WRAP_THRESHOLD  Set Wrap Threshold. Must be postive value. (default %d)\n", PRETTIFY_SEXPR_KICAD_DEFAULT_CONSECUTIVE_TOKEN_WRAP_THRESHOLD);
        printf("  -l COMPACT_LIST    Add To Compact List. Must be a string. \n");
        printf("  -k COLUMN_LIMIT    Add To Compact List Column Limit. Must be positive value. (default %d)\n", PRETTIFY_SEXPR_KICAD_DEFAULT_COMPACT_LIST_COLUMN_LIMIT);
        printf("  -s SHORTFORM       Add To Shortform List. Must be a string.\n");
        printf("  -d                 Dryrun\n");
        printf("\n");
        printf("Example:\n");
        printf("  - Use standard input and standard output. Also use KiCAD's standard compact list and shortform setting.\n");
        printf("    %s -l pts -s font -s stroke -s fill -s offset -s rotate -s scale - -\n", prog_name);
    }
}

// Main function
int main(int argc, char **argv)
{
    const char *prog_name = argv[0];
    bool dryrun = false;

    int wrap_threshold = PRETTIFY_SEXPR_KICAD_DEFAULT_CONSECUTIVE_TOKEN_WRAP_THRESHOLD;

    const char **compact_list_prefixes = NULL;
    int compact_list_prefixes_entries_count = 0;
    int compact_list_prefixes_wrap_threshold = PRETTIFY_SEXPR_KICAD_DEFAULT_COMPACT_LIST_COLUMN_LIMIT;

    const char **shortform_prefixes = NULL;
    int shortform_prefixes_entries_count = 0;

    while (optind < argc)
    {
        const char c = getopt(argc, argv, "hl:s:d");
        if (c == -1)
        {
            break;
        }

        switch (c)
        {
            case 'h':
            {
                usage(prog_name, true);
                return EXIT_SUCCESS;
            }

            case 'l':
            {
                if (!compact_list_prefixes)
                {
                    compact_list_prefixes = malloc(sizeof(compact_list_prefixes));
                }
                else
                {
                    compact_list_prefixes = realloc(compact_list_prefixes, sizeof(compact_list_prefixes) * (compact_list_prefixes_entries_count + 1));
                }
                compact_list_prefixes[compact_list_prefixes_entries_count] = optarg;
                compact_list_prefixes_entries_count++;
                break;
            }

            case 's':
            {
                if (!shortform_prefixes)
                {
                    shortform_prefixes = malloc(sizeof(shortform_prefixes));
                }
                else
                {
                    shortform_prefixes = realloc(shortform_prefixes, sizeof(shortform_prefixes) * (shortform_prefixes_entries_count + 1));
                }
                shortform_prefixes[shortform_prefixes_entries_count] = optarg;
                shortform_prefixes_entries_count++;
                break;
            }

            case 'w':
            {
                const int value = atoi(optarg);

                if (value < 0)
                {
                    usage(prog_name, false);
                    return EXIT_FAILURE;
                }

                wrap_threshold = value;
                break;
            }

            case 'k':
            {
                const int value = atoi(optarg);

                if (value < 0)
                {
                    usage(prog_name, false);
                    return EXIT_FAILURE;
                }

                compact_list_prefixes_wrap_threshold = value;
                break;
            }

            case 'd':
            {
                dryrun = true;
                break;
            }

            case '?':
            {
                usage(prog_name, false);
                return EXIT_FAILURE;
            }

            default:
            {
                usage(prog_name, false);
                return EXIT_FAILURE;
            }
        }
    }

    // Get fixed arguments
    const char *src_path = NULL;
    const char *dst_path = NULL;

    if (optind < argc)
    {
        src_path = argv[optind++];
    }

    if (optind < argc)
    {
        dst_path = argv[optind++];
    }

    if (!src_path)
    {
        usage(prog_name, true);
        return EXIT_SUCCESS;
    }

    // Dryrun Output
    if (dryrun)
    {
        printf("src = %s\n", src_path ? src_path : "stdin");
        printf("dst = %s\n", dst_path ? dst_path : "stdout");
        printf("wrap threshold: %d\n", wrap_threshold);
        printf("compact wrap threshold: %d\n", compact_list_prefixes_wrap_threshold);
        printf("compact list (%d):\n", compact_list_prefixes_entries_count);
        for (int i = 0; i < compact_list_prefixes_entries_count; i++)
        {
            printf(" - %d : %s\n", i, compact_list_prefixes[i]);
        }
        printf("shortform list (%d):\n", shortform_prefixes_entries_count);
        for (int i = 0; i < shortform_prefixes_entries_count; i++)
        {
            printf(" - %d : %s\n", i, shortform_prefixes[i]);
        }
        return EXIT_SUCCESS;
    }

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

    // Initialise and sanity check
    struct PrettifySExprState state = {0};

    assert(sexp_prettify_init(&state, PRETTIFY_SEXPR_KICAD_DEFAULT_INDENT_CHAR, PRETTIFY_SEXPR_KICAD_DEFAULT_INDENT_SIZE, wrap_threshold));

    if (compact_list_prefixes_entries_count > 0)
    {
        assert(sexp_prettify_compact_list_set(&state, compact_list_prefixes, compact_list_prefixes_entries_count, compact_list_prefixes_wrap_threshold));
    }

    if (shortform_prefixes_entries_count > 0)
    {
        assert(sexp_prettify_shortform_set(&state, shortform_prefixes, shortform_prefixes_entries_count));
    }

    // Process Source Files
    char src_char;
    while ((src_char = fgetc(src_file)) != EOF)
    {
        sexp_prettify(&state, src_char, &putc_handler, dst_file);
    }

    // Wrapup and Cleanup
    fclose(src_file);
    if (dst_file != stdout)
    {
        fclose(dst_file);
    }

    return EXIT_SUCCESS;
}
