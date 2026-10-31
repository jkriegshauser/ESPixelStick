# Get CI version number
Import('env')
import os
import re
import subprocess

version = "dev"

if "CI" in os.environ:
    with open("firmware/VERSION", "r") as file:
        version = file.readline().rstrip()
    env.Append(BUILD_FLAGS=[f"-DESPS_VERSION=\"{version}\""])
else:
    # Local build: stamp the commit so a board reports which build it is running
    project_dir = env.subst("$PROJECT_DIR")
    base = "4.0"
    try:
        with open(os.path.join(project_dir, ".github", "workflows", "build.yml"), "r") as file:
            match = re.search(r'VERSION_BASE:\s*"([^"]+)"', file.read())
            if match:
                base = match.group(1)
    except OSError:
        pass
    try:
        commit = subprocess.check_output(["git", "rev-parse", "--short=8", "HEAD"], cwd=project_dir, text=True).strip()
        changes = subprocess.check_output(["git", "status", "--porcelain", "--", "src", "include", "html", "platformio.ini"], cwd=project_dir, text=True).strip()
        version = f"{base}-{commit}" + ("-dirty" if changes else "")
    except Exception:
        version = f"{base}-dev"
    env.Append(BUILD_FLAGS=[f"-DESPS_VERSION=\"{version}\""])
