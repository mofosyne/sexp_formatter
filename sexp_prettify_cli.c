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

typedef enum styleProfile
{
    STYLE_PROFILE_NONE = 0,
    STYLE_PROFILE_KICAD_STANDARD,
    STYLE_PROFILE_KICAD_COMPACT,
} styleProfile;

const char *compact_list_prefixes_kicad[] = {"pts"};
const int compact_list_prefixes_kicad_size = sizeof(compact_list_prefixes_kicad) / sizeof(compact_list_prefixes_kicad[0]);

const char *shortform_prefixes_kicad[] = {"font", "stroke", "fill", "offset", "rotate", "scale"};
const int shortform_prefixes_kicad_size = sizeof(shortform_prefixes_kicad) / sizeof(shortform_prefixes_kicad[0]);

void putc_handler(char c, void *context_putc) { fputc(c, (FILE *)context_putc); }

void usage(const char *prog_name, bool full)
{
    if (full)
    {
        printf("S-Expression Formatter (Brian Khuu 2024)\n");
        printf("\n");
    }

    printf("Usage:\n");
    printf("  %s [OPTION]... SOURCE [DESTINATION]\n", prog_name);
    if (!full)
    {
        printf("  %s -h          Show Full Help Message\n", prog_name);
    }
    printf("  SOURCE             Source file path. If '-' then use standard stream input\n");
    printf("  DESTINATION        Destination file path. If omitted or '-' then use standard stream output\n");
    printf("\n");

    if (full)
    {
        printf("Options:\n");
        printf("  -h                 Show Help Message\n");
        printf("  -w WRAP_THRESHOLD  Set Wrap Threshold. Must be postive value. (default %d)\n", PRETTIFY_SEXPR_KICAD_DEFAULT_CONSECUTIVE_TOKEN_WRAP_THRESHOLD);
        printf("  -l COMPACT_LIST    Add To Compact List. Must be a string. \n");
        printf("  -k COLUMN_LIMIT    Add To Compact List Column Limit. Must be positive value. (default %d)\n", PRETTIFY_SEXPR_KICAD_DEFAULT_COMPACT_LIST_COLUMN_LIMIT);
        printf("  -s SHORTFORM       Add To Shortform List. Must be a string.\n");
        printf("  -p PROFILE         Predefined Style. (kicad, kicad-compact)\n");
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

    int wrap_threshold = PRETTIFY_SEXPR_KICAD_DEFAULT_CONSECUTIVE_TOKEN_WRAP_THRESHOLD;

    const char **compact_list_prefixes = NULL;
    int compact_list_prefixes_entries_count = 0;
    int compact_list_prefixes_wrap_threshold = PRETTIFY_SEXPR_KICAD_DEFAULT_COMPACT_LIST_COLUMN_LIMIT;

    const char **shortform_prefixes = NULL;
    int shortform_prefixes_entries_count = 0;

    styleProfile kicad_profile_active = STYLE_PROFILE_NONE;

    while (optind < argc)
    {
        const char c = getopt(argc, argv, "hl:s:p:k:");
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

            case 'p':
            {
                if (strcmp("kicad", optarg) == 0)
                {
                    kicad_profile_active = STYLE_PROFILE_KICAD_STANDARD;
                }
                else if (strcmp("kicad-compact", optarg) == 0)
                {
                    kicad_profile_active = STYLE_PROFILE_KICAD_COMPACT;
                }

                if (kicad_profile_active == STYLE_PROFILE_NONE)
                {
                    fprintf(stderr, "Must be either 'kicad' or 'kicad-compact'");
                    usage(prog_name, false);
                    return EXIT_FAILURE;
                }

                if (compact_list_prefixes_entries_count)
                {
                    free(compact_list_prefixes);
                    compact_list_prefixes = NULL;
                    compact_list_prefixes_entries_count = 0;
                }

                if (shortform_prefixes_entries_count)
                {
                    free(shortform_prefixes);
                    shortform_prefixes = NULL;
                    shortform_prefixes_entries_count = 0;
                }

                if (kicad_profile_active == STYLE_PROFILE_KICAD_STANDARD || kicad_profile_active == STYLE_PROFILE_KICAD_COMPACT)
                {
                    compact_list_prefixes = malloc(sizeof(compact_list_prefixes));
                    for (int i = 0; i < compact_list_prefixes_kicad_size; i++)
                    {
                        compact_list_prefixes = realloc(compact_list_prefixes, sizeof(compact_list_prefixes) * (compact_list_prefixes_entries_count + 1));
                        compact_list_prefixes[compact_list_prefixes_entries_count] = compact_list_prefixes_kicad[i];
                        compact_list_prefixes_entries_count++;
                    }
                }

                if (kicad_profile_active == STYLE_PROFILE_KICAD_COMPACT)
                {
                    shortform_prefixes = malloc(sizeof(shortform_prefixes));
                    for (int i = 0; i < shortform_prefixes_kicad_size; i++)
                    {
                        shortform_prefixes = realloc(shortform_prefixes, sizeof(shortform_prefixes) * (shortform_prefixes_entries_count + 1));
                        shortform_prefixes[shortform_prefixes_entries_count] = shortform_prefixes_kicad[i];
                        shortform_prefixes_entries_count++;
                    }
                }

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
        fprintf(stderr, "Source Path Missing\n");
        usage(prog_name, true);
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
