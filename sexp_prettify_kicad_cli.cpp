// KiCADv8 Style Prettify S-Expression Formatter (sexp formatter)
// By Brian Khuu, 2024
// This script reformats KiCad-like S-expressions to match a specific formatting style.
// Note: This script modifies formatting only; it does not perform linting or validation.

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

extern "C"
{
#include "sexp_prettify.h"
}

// Dev Note: This is intended to replace Prettify() in kicad/kicad/common/io/kicad/kicad_io_utils.cpp
void Prettify(std::string &aSource, bool aCompactSave)
{
    std::string formatted;
    formatted.reserve(aSource.length());

    PrettifySExprState state = {0};

    assert(sexp_prettify_init(&state, PRETTIFY_SEXPR_KICAD_DEFAULT_INDENT_CHAR, PRETTIFY_SEXPR_KICAD_DEFAULT_INDENT_SIZE, PRETTIFY_SEXPR_KICAD_DEFAULT_CONSECUTIVE_TOKEN_WRAP_THRESHOLD));

    std::vector<const char *> compact_list_ptrs;
    compact_list_ptrs.push_back("pts");

    std::vector<const char *> shortform_ptrs;
    shortform_ptrs.push_back("ptr");
    shortform_ptrs.push_back("font");
    shortform_ptrs.push_back("stroke");
    shortform_ptrs.push_back("fill");
    shortform_ptrs.push_back("offset");
    shortform_ptrs.push_back("rotate");
    shortform_ptrs.push_back("scale");

    assert(sexp_prettify_compact_list_set(&state, compact_list_ptrs.data(), compact_list_ptrs.size(), PRETTIFY_SEXPR_KICAD_DEFAULT_COMPACT_LIST_COLUMN_LIMIT));
    if (aCompactSave)
    {
        assert(sexp_prettify_shortform_set(&state, shortform_ptrs.data(), shortform_ptrs.size()));
    }

    // Define the lambda closer to usage
    auto putc_handler = [](char ch, void *context_putc)
    {
        auto &out_stream = *static_cast<std::string *>(context_putc);
        out_stream.push_back(ch);
    };

    // Process the input
    auto cursor = aSource.begin();
    while (cursor != aSource.end())
    {
        char c = *cursor;
        sexp_prettify(&state, c, putc_handler, &formatted);
        cursor++;
    }

    aSource = std::move(formatted);
}

// Display usage instructions
void usage(const std::string &prog_name, bool full)
{
    if (full)
    {
        std::cout << "S-Expression Formatter KiCAD Specific (Brian Khuu 2024)\n\n";
    }

    std::cout << "Usage:\n"
              << "  " << prog_name << " [OPTION]... SRC [DST]\n"
              << "  SRC                Source file path. If '-' then use standard stream input\n"
              << "  DST                Destination file path. If omitted or '-' then use standard stream output\n\n";

    if (full)
    {
        std::cout << "Options:\n"
                  << "  -h                 Show Help Message\n"
                  << "  -c                 Use Compact Mode.\n"
                  << "  -d                 Dryrun\n\n"
                  << "Example:\n"
                  << "  - Use standard input and standard output. Also use KiCAD's standard compact list and shortform setting.\n"
                  << "    " << prog_name << " - -\n";
    }
}

int main(int argc, char **argv)
{
    const std::string prog_name = argv[0];
    bool dryrun = false;
    bool compactsave = false;

    // Parse options
    while (optind < argc)
    {
        const int c = getopt(argc, argv, "hcd");
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
            case 'c':
            {
                compactsave = true;
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
    const char *src_path = nullptr;
    const char *dst_path = nullptr;

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
        std::cout << "src = " << (src_path ? src_path : "stdin") << "\n"
                  << "dst = " << (dst_path ? dst_path : "stdout") << "\n";
        return EXIT_SUCCESS;
    }

    // Set up file streams
    std::unique_ptr<std::ifstream> src_file;
    std::istream *src_stream = &std::cin;
    if (src_path && strcmp(src_path, "-") != 0)
    {
        src_file = std::make_unique<std::ifstream>(src_path);
        if (!src_file->is_open())
        {
            std::cerr << "Error opening source file: " << src_path << "\n";
            return EXIT_FAILURE;
        }
        src_stream = src_file.get();
    }

    std::unique_ptr<std::ofstream> dst_file;
    std::ostream *dst_stream = &std::cout;
    if (dst_path && strcmp(dst_path, "-") != 0)
    {
        dst_file = std::make_unique<std::ofstream>(dst_path);
        if (!dst_file->is_open())
        {
            std::cerr << "Error opening destination file: " << dst_path << "\n";
            return EXIT_FAILURE;
        }
        dst_stream = dst_file.get();
    }

    // Read input into a string
    std::string aSource((std::istreambuf_iterator<char>(*src_stream)), std::istreambuf_iterator<char>());

    // Process the input
    Prettify(aSource, compactsave);

    // Write the result to the output stream
    *dst_stream << aSource;

    return EXIT_SUCCESS;
}
