import subprocess
Import("env")

try:
    version = subprocess.check_output(
        ["git", "describe", "--tags", "--abbrev=0"],
        stderr=subprocess.DEVNULL,
    ).decode().strip() or "dev"
except Exception:
    version = "dev"

env.Append(CPPDEFINES=[("APP_VERSION", '\\"' + version + '\\"')])
