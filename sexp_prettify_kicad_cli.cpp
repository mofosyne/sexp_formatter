// KiCADv8 Style Prettify S-Expression Formatter (sexp formatter)
// By Brian Khuu, 2024
// This script reformats KiCad-like S-expressions to match a specific formatting style.
// Note: This script modifies formatting only; it does not perform linting or validation.

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <vector>

void Prettify(std::string &aSource, bool aCompactSave = false)
{
    // Configuration
    const char quoteChar = '"';
    const char indentChar = '\t';
    const int indentSize = 1;

    // Lists exceeding this wrap threshold will be shifted to the next line
    const int compactListColumnLimit = 99;
    const std::vector<std::string> compact_list_prefixes = {"pts"};

    // Tokens exceeding this wrap threshold will be shifted to the next line
    const int consecutiveTokenWrapThreshold = 72;
    const std::vector<std::string> shortform_prefixes = {"font", "stroke", "fill", "offset", "rotate", "scale"};

    // Create and reserve formatted string
    std::string formatted;
    formatted.reserve(aSource.length());

    // Parsing Position Tracking
    unsigned int listDepth = 0;
    unsigned int column = 0;
    char previousNonSpaceOutput = '\0';

    // Parsing state
    bool inQuote = false;
    bool escapeNextChar = false;
    bool singularElement = false;
    bool spacePending = false;
    bool wrappedList = false;

    // Prefix scanner to check if a list should be specially handled
    bool scanningForPrefix = false;
    std::string prefixToken = "";

    // Fixed listDepth feature to place multiple elements in the same line for compactness
    bool compactListMode = false;
    unsigned int compactListIndent = 0;

    // Fixed listDepth feature to place multiple elements in the same line for compactness
    bool shortformMode = false;
    unsigned int shortformIndent = 0;

    for (const char c : aSource)
    {

        // Parse quoted string
        if (c == quoteChar || inQuote)
        {
            // Handle quoted strings
            if (spacePending)
            {
                // Add space before this quoted string
                formatted.push_back(' ');
                column += 1;
                spacePending = false;
            }

            if (escapeNextChar)
            {
                // Escaped Char
                escapeNextChar = false;
            }
            else if (c == '\\')
            {
                // Escape Next Char
                escapeNextChar = true;
            }
            else if (c == quoteChar)
            {
                // End of quoted string mode
                inQuote = !inQuote;
            }

            formatted.push_back(c);
            column += 1;
            previousNonSpaceOutput = c;
            continue;
        }

        // Parse space and newlines
        if (std::isspace(c))
        {
            // Handle spaces and newlines
            spacePending = true;

            if (scanningForPrefix)
            {
                // Check if we got a match against an expected prefix for fixed listDepth mode
                for (auto expectedToken : compact_list_prefixes)
                {
                    if (prefixToken == expectedToken)
                    {
                        compactListMode = true;
                        compactListIndent = listDepth;
                        break;
                    }
                }

                // Check if we got a match against an expected prefix for fixed listDepth mode
                if (aCompactSave)
                {
                    for (auto expectedToken : shortform_prefixes)
                    {
                        if (prefixToken == expectedToken)
                        {
                            shortformMode = true;
                            shortformIndent = listDepth;
                            break;
                        }
                    }
                }

                scanningForPrefix = false;
            }
            continue;
        }

        // Parse Opening parentheses
        if (c == '(')
        {
            // Handle opening parentheses
            spacePending = false;

            // Pre Opening parentheses Actions
            if (compactListMode)
            {
                // In fixed listDepth, visually compact mode
                if (column < compactListColumnLimit && previousNonSpaceOutput == ')' || compactListColumnLimit == 0)
                {
                    // Is a consecutive list and still within column limit (or column limit disabled)
                    formatted.push_back(' ');
                    column += 1;
                    spacePending = false;
                }
                else
                {
                    // List is either beyond column limit or not after another list
                    // Move this list to the next line

                    formatted.push_back('\n');
                    column = 0;

                    formatted.append(compactListIndent * indentSize, indentChar);
                    column += compactListIndent * indentSize;
                }
            }
            else if (shortformMode)
            {
                // In one liner mode
                formatted.push_back(' ');
                column += 1;
                spacePending = false;
            }
            else
            {
                // Start scanning for prefix for special list handling
                scanningForPrefix = true;
                prefixToken = "";
                if (listDepth > 0)
                {
                    // Print next line depth
                    formatted.push_back('\n');
                    column = 0;

                    formatted.append(listDepth * indentSize, indentChar);
                    column += listDepth * indentSize;
                }
            }

            singularElement = true;
            listDepth++;

            formatted.push_back('(');
            column += 1;

            previousNonSpaceOutput = '(';
            continue;
        }

        // Parse Closing Brace
        if (c == ')')
        {
            const bool curr_shortform_mode = shortformMode;

            // Handle closing parentheses
            spacePending = false;
            scanningForPrefix = false;

            if (listDepth > 0)
            {
                listDepth--;
            }

            // Check if compact list mode has completed
            if (compactListMode && listDepth < compactListIndent)
            {
                compactListMode = false;
            }

            // Check if we have finished with a shortform list
            if (shortformMode && listDepth < shortformIndent)
            {
                shortformMode = false;
            }

            if (wrappedList)
            {
                // This was a list with wrapped tokens so is already indented
                formatted.push_back('\n');
                column = 0;

                formatted.append(listDepth * indentSize, indentChar);
                column += listDepth * indentSize;

                if (singularElement)
                {
                    // End of singular element
                    singularElement = false;
                }

                wrappedList = false;
            }
            else
            {
                // Normal List
                if (singularElement)
                {
                    // End of singular element
                    singularElement = false;
                }
                else if (!curr_shortform_mode)
                {
                    // End of a parent element
                    formatted.push_back('\n');
                    column = 0;

                    formatted.append(listDepth * indentSize, indentChar);
                    column += listDepth * indentSize;
                }
            }

            formatted.push_back(')');
            column += 1;

            if (listDepth <= 0)
            {
                // Cap Root Element
                formatted.push_back('\n');
                column = 0;
            }

            previousNonSpaceOutput = ')';
            continue;
        }

        // Parse Characters
        if (c != '\0')
        {

            // Pre character actions
            if (previousNonSpaceOutput == ')' && !shortformMode)
            {
                // Is Bare token after a list that should be on next line
                // Dev Note: In KiCAD this may indicate a flag bug
                formatted.push_back('\n');
                column = 0;

                formatted.append(listDepth * indentSize, indentChar);
                column += listDepth * indentSize;

                spacePending = false;
            }
            else if (spacePending && !shortformMode && !compactListMode && column >= consecutiveTokenWrapThreshold)
            {
                // Token is above wrap threshold. Move token to next line (If token wrap threshold is zero then this feature is disabled)
                wrappedList = true;

                formatted.push_back('\n');
                column = 0;

                formatted.append(listDepth * indentSize, indentChar);
                column += listDepth * indentSize;

                spacePending = false;
            }
            else if (spacePending && previousNonSpaceOutput != '(')
            {
                // Space was pending
                formatted.push_back(' ');
                column += 1;

                spacePending = false;
            }

            // Add to prefix scanning buffer if scanning for special list handling detection
            if (scanningForPrefix)
            {
                prefixToken += c;
            }

            // Add character to list
            formatted.push_back(c);
            column += 1;

            previousNonSpaceOutput = c;
            continue;
        }
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
              << "  " << prog_name << " [OPTION]... SOURCE [DESTINATION]\n"
              << "  SOURCE             Source file path. If '-' then use standard stream input\n"
              << "  DESTINATION        Destination file path. If omitted or '-' then use standard stream output\n\n";

    if (full)
    {
        std::cout << "Options:\n"
                  << "  -h                 Show Help Message\n"
                  << "  -c                 Use Compact Mode.\n"
                  << "  -p PROFILE         Predefined Style. (kicad, kicad-compact)\n"
                  << "Example:\n"
                  << "  - Use standard input and standard output. Also use KiCAD's standard compact list and shortform setting.\n"
                  << "    " << prog_name << " - -\n";
    }
}

int main(int argc, char **argv)
{
    const std::string prog_name = argv[0];
    bool compactsave = false;

    // Parse options
    while (optind < argc)
    {
        const int c = getopt(argc, argv, "hcp:");
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
            case 'p':
            {
                if (strcmp("kicad-compact", optarg) == 0)
                {
                    compactsave = true;
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
