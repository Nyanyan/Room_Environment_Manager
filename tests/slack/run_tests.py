"""Exercise the production Slack receive path without Wi-Fi or private token.h files."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--arduinojson-dir", default=os.environ.get("ARDUINOJSON_DIR"),
                    help="ArduinoJson 7 library directory (or set ARDUINOJSON_DIR)")
args = parser.parse_args()
if not args.arduinojson_dir:
    parser.error("Specify --arduinojson-dir or ARDUINOJSON_DIR")
json_src = Path(args.arduinojson_dir) / "src"
if not (json_src / "ArduinoJson.h").is_file():
    parser.error("ArduinoJson.h not found under the library's src directory")
tests = Path(__file__).resolve().parent
parent = tests.parents[1] / "main/room_environment_manager"
source = (parent / "slack.ino").read_text(encoding="utf-8")

# Compile the actual queue, transport, ACK and dispatch functions. The omitted
# portions handle HTTP uploads/connection setup and require real ESP32 libraries.
def section(start, end):
    return source[source.index(start):source.index(end)]

implementation = section("#define SLACK_COMMAND_QUEUE_SIZE", "static bool slack_app_token_configured")
implementation += section("static uint32_t g_slack_message_started_ms", "static bool read_websocket_handshake")
implementation += section("static bool websocket_send_frame", "static bool slack_socket_open")
with tempfile.TemporaryDirectory(prefix="slack-tests-") as tmp:
    staging = Path(tmp)
    (staging / "socket_under_test.inc").write_text(implementation, encoding="utf-8")
    message_implementation = section("static bool parse_slack_ok", "static void log_network_diagnostics")
    message_implementation += section("static char *slack_post_message", "String slack_upload_img")
    (staging / "message_under_test.inc").write_text(message_implementation, encoding="utf-8")
    for name in ("slack_report.h", "sensors.h", "time_manager.h"):
        shutil.copy2(parent / name, staging / name)
    (staging / "token.h").write_text("#pragma once\n#define N_ADDITIONAL_SENSORS 3\n")
    for name in ("test_socket", "test_report"):
        executable = staging / (name + (".exe" if os.name == "nt" else ""))
        subprocess.run([os.environ.get("CXX", "g++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                        "-DARDUINOJSON_ENABLE_ARDUINO_STRING=1",
                        "-I", str(tests / "fakes"), "-I", str(staging),
                        "-I", str(parent), "-I", str(json_src),
                        str(tests / (name + ".cpp")), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True, timeout=30)
