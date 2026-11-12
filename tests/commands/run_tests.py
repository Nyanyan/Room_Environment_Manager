"""Exercise real command dispatch, humidifier control, graphs and EEPROM with fake I/O."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

tests = Path(__file__).resolve().parent
source = tests.parents[1] / "main/room_environment_manager"
with tempfile.TemporaryDirectory(prefix="command-tests-") as tmp:
    staging = Path(tmp)
    for pattern in ("*.h", "*.ino"):
        for path in source.glob(pattern):
            if path.name != "token.h":
                shutil.copy2(path, staging / path.name)
    (staging / "token.h").write_text("#pragma once\n#define N_ADDITIONAL_SENSORS 3\n")
    executable = staging / ("test_commands.exe" if os.name == "nt" else "test_commands")
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-DHUMIDIFIER_MAC_ADDR={2,0,0,0,0,1}",
                    "-include", str(tests.parent / "humidifier/fakes/Arduino.h"),
                    "-I", str(tests.parent / "humidifier/fakes"),
                    "-I", str(tests.parent / "graph/fakes"), "-I", str(staging),
                    str(tests / "test_commands.cpp"), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True, timeout=30)
