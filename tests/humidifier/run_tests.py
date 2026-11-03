"""Run the production humidifier code against fake hardware, without private token.h files."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
tests = Path(__file__).resolve().parent
parent = root / "main/room_environment_manager"
child = root / "main/humidifier"
assert (parent / "humidifier_protocol.h").read_bytes() == (child / "humidifier_protocol.h").read_bytes()
compiler = os.environ.get("CXX", "g++")
with tempfile.TemporaryDirectory(prefix="humidifier-tests-") as tmp:
    staging = Path(tmp)
    for name in ("parent", "child"):
        (staging / name).mkdir()
    for name in ("humidifier.h", "humidifier.ino", "humidifier_pid.h", "humidifier_protocol.h",
                 "command.h", "sensors.h", "time_manager.h", "memory.h", "memory.ino",
                 "air_conditioner.h", "additional_sensors.h"):
        shutil.copy2(parent / name, staging / "parent" / name)
    for name in ("humidifier.ino", "humidifier_protocol.h"):
        shutil.copy2(child / name, staging / "child" / name)
    (staging / "parent/token.h").write_text("#pragma once\n#define N_ADDITIONAL_SENSORS 3\n")
    (staging / "child/token.h").write_text(
        "#pragma once\n#define HUMIDIFIER_PARENT_MAC_ADDR {2,0,0,0,0,2}\n")
    cases = [
        ("pid", "parent", "test_pid.cpp", []),
        ("parent", "parent", "test_parent.cpp", ["-DHUMIDIFIER_MAC_ADDR={2,0,0,0,0,1}"]),
        ("unconfigured", "parent", "test_parent.cpp", []),
        ("node", "child", "test_node.cpp", []),
        ("node_active_low", "child", "test_node.cpp", ["-DHUMIDIFIER_BUTTON_ACTIVE_LEVEL=LOW"]),
    ]
    for name, sketch, source, defines in cases:
        executable = staging / (name + (".exe" if os.name == "nt" else ""))
        subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                        "-include", str(tests / "fakes/Arduino.h"),
                        "-I", str(tests / "fakes"), "-I", str(staging / sketch), *defines,
                        str(tests / source), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
print("All humidifier tests passed.")
