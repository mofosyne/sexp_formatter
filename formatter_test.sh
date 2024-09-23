#!/bin/bash
python3 -m venv .venv
source ./.venv/bin/activate

# Parse/Save Test
rm ./minimal/Test.kicad_sch
cp ./minimal/Normal.kicad_sch ./minimal/Test.kicad_sch
python3 ./kiutils_copy_save_test.py ./minimal/Test.kicad_sch

# Harmonized for easier comparison
rm ./minimal/formatted/Normal.kicad_sch
rm ./minimal/formatted/Test.kicad_sch
python3 ./sexp_formatter.py ./minimal/Normal.kicad_sch --dst ./minimal/formatted/Normal.kicad_sch
python3 ./sexp_formatter.py ./minimal/Test.kicad_sch --dst ./minimal/formatted/Test.kicad_sch

# CM4IOv5_example SCH
rm ./CM4IOv5_example/original/formatted/PSUs_original_harmonized.kicad_sch
rm ./CM4IOv5_example/upgraded/formatted/PSUs_upgraded_to_kicadv8_harmonized.kicad_sch
python3 ./sexp_formatter.py ./CM4IOv5_example/original/PSUs_original.kicad_sch --dst ./CM4IOv5_example/original/formatted/PSUs_original_harmonized.kicad_sch
python3 ./sexp_formatter.py ./CM4IOv5_example/upgraded/PSUs_upgraded_to_kicadv8.kicad_sch --dst ./CM4IOv5_example/upgraded/formatted/PSUs_upgraded_to_kicadv8_harmonized.kicad_sch

# CM4IOv5_example PCB
rm ./CM4IOv5_example/original/formatted/CM4IOv5_original_harmonized.kicad_pcb
rm ./CM4IOv5_example/upgraded/formatted/CM4IOv5_upgraded_to_kicadv8_harmonized.kicad_pcb
python3 ./sexp_formatter.py ./CM4IOv5_example/original/CM4IOv5_original.kicad_pcb --dst ./CM4IOv5_example/original/formatted/CM4IOv5_original_harmonized.kicad_pcb
python3 ./sexp_formatter.py ./CM4IOv5_example/upgraded/CM4IOv5_upgraded_to_kicadv8.kicad_pcb --dst ./CM4IOv5_example/upgraded/formatted/CM4IOv5_upgraded_to_kicadv8_harmonized.kicad_pcb

# CM4IOv5_example SYM
rm ./CM4IOv5_example/original/formatted/CM4IO_original_harmonized.kicad_sym
rm ./CM4IOv5_example/upgraded/formatted/CM4IO_upgraded_to_kicadv8_harmonized.kicad_sym
python3 ./sexp_formatter.py ./CM4IOv5_example/original/CM4IO_original.kicad_sym --dst ./CM4IOv5_example/original/formatted/CM4IO_original_harmonized.kicad_sym
python3 ./sexp_formatter.py ./CM4IOv5_example/upgraded/CM4IO_upgraded_to_kicadv8.kicad_sym --dst ./CM4IOv5_example/upgraded/formatted/CM4IO_upgraded_to_kicadv8_harmonized.kicad_sym

# CM4IOv5_example MOD (Footprint)
rm ./CM4IOv5_example/original/formatted/PCIex1-36_original_harmonized.kicad_mod
rm ./CM4IOv5_example/upgraded/formatted/PCIex1-36_upgraded_to_kicadv8_harmonized.kicad_mod
python3 ./sexp_formatter.py ./CM4IOv5_example/original/PCIex1-36_original.kicad_mod --dst ./CM4IOv5_example/original/formatted/PCIex1-36_original_harmonized.kicad_mod
python3 ./sexp_formatter.py ./CM4IOv5_example/upgraded/PCIex1-36_upgraded_to_kicadv8.kicad_mod --dst ./CM4IOv5_example/upgraded/formatted/PCIex1-36_upgraded_to_kicadv8_harmonized.kicad_mod
