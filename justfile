default:
    just -l

init:
    make

# Run using sexp_formatter.py
test:
    make
    ./test_all.sh

kicad_test:
    make
    ./test_standard_single.sh ./sexp_prettify_kicad_cli

cpp_test:
    make
    ./test_standard_single.sh ./sexp_prettify_cpp_cli

c_test:
    make
    ./test_standard_single.sh ./sexp_prettify_cli

python_test:
    make
    ./test_standard_single.sh ./sexp_prettify_cli.py


# Personal Kiutils Bringup
dev-bringup-mofosyne:
    #!/usr/bin/env bash
    # Get Custom KiUtils Implementation (Development)
    if [ ! -d "kiutils" ]; then
        git clone https://github.com/mofosyne/kiutils;
    else
        echo "Kiutils directory already exists.";
    fi

    # Setup python virtual enviroment and link kiutils to the custom kiutil library
    python3 -m venv .venv
    source ./.venv/bin/activate
    pip install -e kiutils


format:
    # pip install clang-format
    clang-format -i *.cpp
    clang-format -i *.c
    clang-format -i *.h
