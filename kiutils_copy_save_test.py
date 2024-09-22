#!/usr/bin/env python3
# Was created to investigate https://github.com/mvnmgrx/kiutils/issues/120
from kiutils.schematic import Schematic

from os import path

import argparse
from pathlib import Path

# Arg Parsing
parser=argparse.ArgumentParser(description="Open Then Save and Close To Test Bug Replication")
parser.add_argument("src", type=Path)
p = parser.parse_args()
src_file = p.src.resolve()
base_path = path.dirname(path.realpath(__file__))

# Offending Lines
kicad_container = Schematic().from_file(src_file)
kicad_container.to_file(src_file)