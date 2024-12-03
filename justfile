default:
    just -l

init:
    make

# Run using sexp_formatter.py
test:
    ./test.sh

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
