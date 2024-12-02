// KiCADv8 Style Prettify S-Expression Formatter (sexp formatter)
// By Brian Khuu, 2024
// This script reformats KiCad-like S-expressions to match a specific formatting style.
// Note: This script modifies formatting only; it does not perform linting or validation.

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

void Prettify(std::string &aSource, bool aCompactSave)
{
    // Configuration
    const char quoteChar = '"';
    const char indentChar = '\t';
    const int indentSize = 1;

    // In order to visually compress PCB files, it is helpful to special-case long lists of (xy ...)
    // lists, which we allow to exist on a single line until we reach column 99.
    const int xySpecialCaseColumnLimit = 99;

    // If whitespace occurs inside a list after this threshold, it will be converted into a newline
    // and the indentation will be increased.  This is mainly used for image and group objects,
    // which contain potentially long sets of string tokens within a single list.
    const int consecutiveTokenWrapThreshold = 72;

    std::string formatted;
    formatted.reserve(aSource.length());

    auto cursor = aSource.begin();
    auto seek = cursor;

    int listDepth = 0;
    char lastNonWhitespace = 0;
    bool inQuote = false;
    bool hasInsertedSpace = false;
    bool inMultiLineList = false;
    bool inXY = false;
    bool inShortForm = false;
    int shortFormDepth = 0;
    int column = 0;
    int backslashCount = 0; // Count of successive backslash read since any other char

    auto isWhitespace = [](const char aChar) { return (aChar == ' ' || aChar == '\t' || aChar == '\n' || aChar == '\r'); };

    auto nextNonWhitespace = [&](std::string::iterator aIt)
    {
        seek = aIt;

        while (seek != aSource.end() && isWhitespace(*seek))
        {
            seek++;
        }

        if (seek == aSource.end())
        {
            return (char)0;
        }

        return *seek;
    };

    auto isXY = [&](std::string::iterator aIt)
    {
        seek = aIt;

        if (++seek == aSource.end() || *seek != 'x')
        {
            return false;
        }

        if (++seek == aSource.end() || *seek != 'y')
        {
            return false;
        }

        if (++seek == aSource.end() || *seek != ' ')
        {
            return false;
        }

        return true;
    };

    auto isShortForm = [&](std::string::iterator aIt)
    {
        seek = aIt;
        std::string token;

        while (++seek != aSource.end() && isalpha(*seek))
        {
            token += *seek;
        }

        return token == "font" || token == "stroke" || token == "fill" || token == "offset" || token == "rotate" || token == "scale";
    };

    while (cursor != aSource.end())
    {
        char next = nextNonWhitespace(cursor);

        if (isWhitespace(*cursor) && !inQuote)
        {
            if (!hasInsertedSpace           // Only permit one space between chars
                && listDepth > 0            // Do not permit spaces in outer list
                && lastNonWhitespace != '(' // Remove extra space after start of list
                && next != ')'              // Remove extra space before end of list
                && next != '(')             // Remove extra space before newline
            {
                if (inXY || column < consecutiveTokenWrapThreshold)
                {
                    // Note that we only insert spaces here, no matter what kind of whitespace is
                    // in the input.  Newlines will be inserted as needed by the logic below.
                    formatted.push_back(' ');
                    column++;
                }
                else if (inShortForm)
                {
                    formatted.push_back(' ');
                }
                else
                {
                    formatted.push_back('\n');
                    formatted.append(listDepth * indentSize, indentChar);
                    column = listDepth * indentSize;
                    inMultiLineList = true;
                }

                hasInsertedSpace = true;
            }
        }
        else
        {
            hasInsertedSpace = false;

            if (*cursor == '(' && !inQuote)
            {
                bool currentIsXY = isXY(cursor);
                bool currentIsShortForm = aCompactSave && isShortForm(cursor);

                if (formatted.empty())
                {
                    formatted.push_back('(');
                    column++;
                }
                else if (inXY && currentIsXY && column < xySpecialCaseColumnLimit)
                {
                    // List-of-points special case
                    formatted += " (";
                    column += 2;
                }
                else if (inShortForm)
                {
                    formatted += " (";
                    column += 2;
                }
                else
                {
                    formatted.push_back('\n');
                    formatted.append(listDepth * indentSize, indentChar);
                    formatted.push_back('(');
                    column = listDepth * indentSize + 1;
                }

                inXY = currentIsXY;

                if (currentIsShortForm)
                {
                    inShortForm = true;
                    shortFormDepth = listDepth;
                }

                listDepth++;
            }
            else if (*cursor == ')' && !inQuote)
            {
                if (listDepth > 0)
                {
                    listDepth--;
                }

                if (inShortForm)
                {
                    formatted.push_back(')');
                    column++;
                }
                else if (lastNonWhitespace == ')' || inMultiLineList)
                {
                    formatted.push_back('\n');
                    formatted.append(listDepth * indentSize, indentChar);
                    formatted.push_back(')');
                    column = listDepth * indentSize + 1;
                    inMultiLineList = false;
                }
                else
                {
                    formatted.push_back(')');
                    column++;
                }

                if (shortFormDepth == listDepth)
                {
                    inShortForm = false;
                    shortFormDepth = 0;
                }
            }
            else
            {
                // The output formatter escapes double-quotes (like \")
                // But a corner case is a sequence like \\"
                // therefore a '\' is attached to a '"' if a odd number of '\' is detected
                if (*cursor == '\\')
                {
                    backslashCount++;
                }
                else if (*cursor == quoteChar && (backslashCount & 1) == 0)
                {
                    inQuote = !inQuote;
                }

                if (*cursor != '\\')
                {
                    backslashCount = 0;
                }

                formatted.push_back(*cursor);
                column++;
            }

            lastNonWhitespace = *cursor;
        }

        ++cursor;
    }

    // newline required at end of line / file for POSIX compliance. Keeps git diffs clean.
    formatted += '\n';

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
