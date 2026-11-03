"""Exercise the production graph renderer without ESP32 hardware or private tokens."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--preview-dir", type=Path, help="Optionally save example graphs as PPM files")
args = parser.parse_args()
tests = Path(__file__).resolve().parent
parent = tests.parents[1] / "main/room_environment_manager"
with tempfile.TemporaryDirectory(prefix="graph-tests-") as tmp:
    staging = Path(tmp)
    for name in ("graph.h", "graph.ino", "character.h", "command.h", "air_conditioner.h",
                 "sensors.h", "time_manager.h"):
        shutil.copy2(parent / name, staging / name)
    (staging / "token.h").write_text("#pragma once\n#define N_ADDITIONAL_SENSORS 3\n")
    executable = staging / ("test_graph.exe" if os.name == "nt" else "test_graph")
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-I", str(tests / "fakes"), "-I", str(staging),
                    str(tests / "test_graph.cpp"), "-o", str(executable)], check=True)
    command = [str(executable)]
    if args.preview_dir:
        args.preview_dir.mkdir(parents=True, exist_ok=True)
        command.append(str(args.preview_dir.resolve()))
    subprocess.run(command, check=True, timeout=30)
