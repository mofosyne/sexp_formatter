#!/bin/bash
make sexp_prettify_cpp_cli

# Harmonized for easier comparison
rm ./minimal/formatted_via_c/Normal.kicad_sch 2> /dev/null
rm ./minimal/formatted_via_c/Test.kicad_sch 2> /dev/null
./sexp_prettify_cpp_cli -l pts ./minimal/Normal.kicad_sch ./minimal/formatted_via_c/Normal.kicad_sch
./sexp_prettify_cpp_cli -l pts ./minimal/Test.kicad_sch ./minimal/formatted_via_c/Test.kicad_sch

# CM4IOv5_example SCH
rm ./CM4IOv5_example/original/formatted_via_c/PSUs_original_harmonized.kicad_sch 2> /dev/null
rm ./CM4IOv5_example/upgraded/formatted_via_c/PSUs_upgraded_to_kicadv8_harmonized.kicad_sch 2> /dev/null
./sexp_prettify_cpp_cli -l pts ./CM4IOv5_example/original/PSUs_original.kicad_sch ./CM4IOv5_example/original/formatted_via_c/PSUs_original_harmonized.kicad_sch
./sexp_prettify_cpp_cli -l pts ./CM4IOv5_example/upgraded/PSUs_upgraded_to_kicadv8.kicad_sch ./CM4IOv5_example/upgraded/formatted_via_c/PSUs_upgraded_to_kicadv8_harmonized.kicad_sch

# CM4IOv5_example PCB
rm ./CM4IOv5_example/original/formatted_via_c/CM4IOv5_original_harmonized.kicad_pcb 2> /dev/null
rm ./CM4IOv5_example/upgraded/formatted_via_c/CM4IOv5_upgraded_to_kicadv8_harmonized.kicad_pcb 2> /dev/null
./sexp_prettify_cpp_cli -l pts ./CM4IOv5_example/original/CM4IOv5_original.kicad_pcb ./CM4IOv5_example/original/formatted_via_c/CM4IOv5_original_harmonized.kicad_pcb
./sexp_prettify_cpp_cli -l pts ./CM4IOv5_example/upgraded/CM4IOv5_upgraded_to_kicadv8.kicad_pcb ./CM4IOv5_example/upgraded/formatted_via_c/CM4IOv5_upgraded_to_kicadv8_harmonized.kicad_pcb

# CM4IOv5_example SYM
rm ./CM4IOv5_example/original/formatted_via_c/CM4IO_original_harmonized.kicad_sym 2> /dev/null
rm ./CM4IOv5_example/upgraded/formatted_via_c/CM4IO_upgraded_to_kicadv8_harmonized.kicad_sym 2> /dev/null
./sexp_prettify_cpp_cli -l pts ./CM4IOv5_example/original/CM4IO_original.kicad_sym ./CM4IOv5_example/original/formatted_via_c/CM4IO_original_harmonized.kicad_sym
./sexp_prettify_cpp_cli -l pts ./CM4IOv5_example/upgraded/CM4IO_upgraded_to_kicadv8.kicad_sym ./CM4IOv5_example/upgraded/formatted_via_c/CM4IO_upgraded_to_kicadv8_harmonized.kicad_sym

# CM4IOv5_example MOD (Footprint)
rm ./CM4IOv5_example/original/formatted_via_c/PCIex1-36_original_harmonized.kicad_mod 2> /dev/null
rm ./CM4IOv5_example/upgraded/formatted_via_c/PCIex1-36_upgraded_to_kicadv8_harmonized.kicad_mod 2> /dev/null
./sexp_prettify_cpp_cli -l pts ./CM4IOv5_example/original/PCIex1-36_original.kicad_mod ./CM4IOv5_example/original/formatted_via_c/PCIex1-36_original_harmonized.kicad_mod
./sexp_prettify_cpp_cli -l pts ./CM4IOv5_example/upgraded/PCIex1-36_upgraded_to_kicadv8.kicad_mod ./CM4IOv5_example/upgraded/formatted_via_c/PCIex1-36_upgraded_to_kicadv8_harmonized.kicad_mod
