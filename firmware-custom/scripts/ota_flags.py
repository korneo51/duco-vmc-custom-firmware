from pathlib import Path
import re

from SCons.Script import Import

Import("env")

config_path = Path(env["PROJECT_SRC_DIR"]) / "config.h"
config = config_path.read_text(encoding="utf-8")

match = re.search(r'^\s*#define\s+OTA_PASSWORD\s+"([^"]*)"', config, re.MULTILINE)
if match and match.group(1):
    env.Append(UPLOAD_FLAGS=[f"--auth={match.group(1)}"])
