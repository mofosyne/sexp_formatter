default:
    just -l

init:
    init.sh

test:
    formatter_test.sh

meld-minimal:
    meld ./minimal/formatted/Normal.kicad_sch ./minimal/formatted/Test.kicad_sch

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
