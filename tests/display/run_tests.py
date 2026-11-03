"""Exercise the production LCD formatting and rendering without hardware or tokens."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

tests = Path(__file__).resolve().parent
parent = tests.parents[1] / "main/room_environment_manager"
with tempfile.TemporaryDirectory(prefix="display-tests-") as tmp:
    staging = Path(tmp)
    for name in ("display.h", "display.ino", "display_format.h", "sensors.h",
                 "command.h", "air_conditioner.h", "time_manager.h"):
        shutil.copy2(parent / name, staging / name)
    (staging / "token.h").write_text("#pragma once\n#define N_ADDITIONAL_SENSORS 3\n")
    executable = staging / ("test_display.exe" if os.name == "nt" else "test_display")
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-I", str(tests / "fakes"), "-I", str(tests.parent / "humidifier/fakes"),
                    "-I", str(staging), str(tests / "test_display.cpp"), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True, timeout=30)
