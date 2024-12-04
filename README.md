# sexp_formatter

![CI/CD Status Badge](https://github.com/mofosyne/sexp_formatter/actions/workflows/ci.yml/badge.svg)

Prettifies KiCad-like S-expressions according to a KiCADv8-style formatting Via Python

```
========================================================================
 Timing Test Report 
========================================================================
Time taken for ./sexp_prettify_cli.py                   to run though test suites is .327 seconds
Time taken for ./sexp_prettify_kicad_original_cli       to run though test suites is .014 seconds
Time taken for ./sexp_prettify_kicad_cli                to run though test suites is .012 seconds
Time taken for ./sexp_prettify_cpp_cli                  to run though test suites is .009 seconds
Time taken for ./sexp_prettify_cli                      to run though test suites is .006 seconds
```

## Developer

* Run `make` to build all the c and cpp binaries shown above.
* Run `make check` to test all the executable (except for sexp_prettify_kicad_original_cli, I am trying to replace)
* Run `make time` to generate a timing test report

### About Files

* sexp_prettify_cli.py             : This is a python implementation 
* sexp_prettify_kicad_original_cli : This is a cpp original logic from the KiCAD repository as of 2024-12-03
* sexp_prettify_kicad_cli          : This is the new cpp logic from the KiCAD repository being proposed for KiCAD
* sexp_prettify_cpp_cli            : This is a cpp cli wrapper around the c function `sexp_prettify()` in `sexp_prettify.c/h`
* sexp_prettify_cli                : This is a c cli wrapper around the c function `sexp_prettify()` in `sexp_prettify.c/h`

## Usage

### sexp_prettify_cli.py (python) (KiCad fixed styling)
```
usage: sexp_prettify_cli.py [-h] [-c] [-p P] src [dst]

KiCad S-Expression Formatter

positional arguments:
  src         Source file path ('-' for stdin)
  dst         Destination file path ('-' for stdout)

options:
  -h, --help  show this help message and exit
  -c          Use compact mode
  -p P        Predefined Style. (kicad, kicad-compact)
```

### sexp_prettify_cpp_cli (cpp)
```
S-Expression Formatter (Brian Khuu 2024)

Usage:
  ./sexp_prettify_cpp_cli [OPTION]... SOURCE [DESTINATION]
  SOURCE                Source file path. If '-' then use standard stream input
  DESTINATION           Destination file path. If omitted or '-' then use standard stream output

Options:
  -h                 Show Help Message
  -w WRAP_THRESHOLD  Set Wrap Threshold. Must be positive value. (default 72)
  -l COMPACT_LIST    Add To Compact List. Must be a string.
  -k COLUMN_LIMIT    Set Compact List Column Limit. Must be positive value. (default 99)
  -s SHORTFORM       Add To Shortform List. Must be a string.
  -p PROFILE         Predefined Style. (kicad, kicad-compact)
Example:
  - Use standard input and standard output. Also use KiCAD's standard compact list and shortform setting.
    ./sexp_prettify_cpp_cli -l pts -s font -s stroke -s fill -s offset -s rotate -s scale - -
```

### sexp_prettify_cli (c)
```
S-Expression Formatter (Brian Khuu 2024)

Usage:
  ./sexp_prettify_cli [OPTION]... SOURCE [DESTINATION]
  SOURCE             Source file path. If '-' then use standard stream input
  DESTINATION        Destination file path. If omitted or '-' then use standard stream output

Options:
  -h                 Show Help Message
  -w WRAP_THRESHOLD  Set Wrap Threshold. Must be postive value. (default 72)
  -l COMPACT_LIST    Add To Compact List. Must be a string. 
  -k COLUMN_LIMIT    Add To Compact List Column Limit. Must be positive value. (default 99)
  -s SHORTFORM       Add To Shortform List. Must be a string.
  -p PROFILE         Predefined Style. (kicad, kicad-compact)

Example:
  - Use standard input and standard output. Also use KiCAD's standard compact list and shortform setting.
    ./sexp_prettify_cli -l pts -s font -s stroke -s fill -s offset -s rotate -s scale - -
```

## History

This repo was originally created to run a test to open and save a file using kiutils to investigate a [file corruption issue](https://github.com/mvnmgrx/kiutils/issues/120) and part of the effort is to create a formatter as the output s-expression differs greatly from the input source... so it was hard to determine exactly what was failing.

The formatter ended up being quite a project, but is now complete enough and may become useful for kiutils and other 3rd party kicad python based tools to have an output that relatively matches the output of kicadv8.

This will aim to match as closely as possible to kicad output (which was v8.0.5 when this was written). Just note that this will still differ as there is some inconsistencies in how kicad is outputting their file. The development team for KiCAD is aware of this issue and is tracking it under <https://gitlab.com/kicad/code/kicad/-/issues/15232> however according to craftyjon there is no current big effort to change how the formatting works as of 2024-09-23. So in the meantime, you are recommended to run both files though this sexp_formatter.py if you want to diff both files.

On 2024-12-03 I found out about the kicad official test cases so refactored the codebase to match against it (with some slight adjustment to tokens after list).
