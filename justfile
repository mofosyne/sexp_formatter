default:
    just -l

test:
    formatter_test.sh

meld-minimal:
    meld ./minimal/formatted/Normal.kicad_sch ./minimal/formatted/Test.kicad_sch