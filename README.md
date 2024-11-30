# sexp_formatter
Prettifies KiCad-like S-expressions according to a KiCADv8-style formatting Via Python

This repo was originally created to run a test to open and save a file using kiutils to investigate a [file corruption issue](https://github.com/mvnmgrx/kiutils/issues/120) and part of the effort is to create a formatter as the output s-expression differs greatly from the input source... so it was hard to determine exactly what was failing.

The formatter ended up being quite a project, but is now complete enough and may become useful for kiutils and other 3rd party kicad python based tools to have an output that relatively matches the output of kicadv8.

This will aim to match as closely as possible to kicad output (which was v8.0.5 when this was written). Just note that this will still differ as there is some inconsistencies in how kicad is outputting their file. The development team for KiCAD is aware of this issue and is tracking it under <https://gitlab.com/kicad/code/kicad/-/issues/15232> however according to craftyjon there is no current big effort to change how the formatting works as of 2024-09-23. So in the meantime, you are recommended to run both files though this sexp_formatter.py if you want to diff both files.

## Usage

You can use `sexp_formatter.py` as a cli program via this usage instruction below:

```
usage: sexp_formatter.py [-h] [--dst DST] src

Prettify S-expression files

positional arguments:
  src         Source file path

options:
  -h, --help  show this help message and exit
  --dst DST   Destination file path
```

## Integration Usage for prettify_sexpr() in other programs

`sexp_formatter.py` was written in such a way to minimise dependencies so you only need to copy `prettify_sexpr()` to your project and supply below arguments to function: 

- Args:
    - sexpr_str (str): The input S-expression string to be formatted.
    - compact_element_settings (list of dict): A list of dictionaries containing element-specific settings.
        - Each dictionary should contain:
            - "prefix" (str): The prefix of elements that should be handled specially.
            - "elements per line" (int): The number of elements per line in compact mode (optional).

- Returns:
    - str: The prettified S-expression string.

Example:

```python
    # Input S-expression string
    sexpr = "(module (fp_text \"example\"))"

    # Settings for compact element handling
    compact_settings = [{"prefix": "pts", "elements per line": 4}]

    # Prettify the S-expression
    formatted_sexpr = prettify_sexpr(sexpr, compact_settings)
    print(formatted_sexpr)
```

## Run test

Running `formatter_test.sh` will run `sexp_formatter.py` against `CM4IOv5_example` which contains both the original file from raspberry pi from an old version of KiCAD as well as the file that KiCADv8 outputs when the original file is saved again and upgraded to the new KiCADv8 format. These files are passed though the formatter to the corresponding `./formatted/` folder for easier comparison.

This let's you spot changes between the pre KiCADv8 release, e.g. That there is some clear difference where `hide` was converted to `(hide yes)` in the new file format version... which was what tripped up kiutils as shown in the minimal folder. You can run `just meld-minimal` to check the difference if you got meld installed (You will need <https://just.systems/man/en/> installed to use `just` however)


## C example

```bash
./sexp_prettify_cli -l pts -s font -s stroke -s fill -s offset -s rotate -s scale ./minimal/Normal.kicad_sch ./minimal/formatted_via_c/Normal.kicad_sch
```