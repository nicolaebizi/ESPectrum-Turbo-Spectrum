Import("env")

from pathlib import Path
import subprocess
import os
import sys

project = Path(env.subst("$PROJECT_DIR"))
target = project / "third_party" / "bluepad32"
commit = "7e13707"  # Bluepad32 4.0.3; includes Bluetooth keyboard support and updated BTstack.

def run(args, cwd=None, env=None):
    subprocess.check_call(args, cwd=str(cwd) if cwd else None, env=env)

if not (target / ".git").exists():
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists():
        import shutil
        shutil.rmtree(target)
    run(["git", "clone", "--recurse-submodules", "https://github.com/ricardoquesada/bluepad32.git", str(target)])
    run(["git", "checkout", commit], cwd=target)
    run(["git", "submodule", "update", "--init", "--recursive"], cwd=target)
else:
    run(["git", "checkout", commit], cwd=target)
    run(["git", "submodule", "update", "--init", "--recursive"], cwd=target)

btstack = target / "external" / "btstack"
btstack_port = btstack / "port" / "esp32"
patches = sorted((target / "external" / "patches").glob("*.patch"))

# Bluepad32 requires its BTstack patches to be applied before integration.
# Keep this entirely inside the project-owned third_party tree.
for patch in patches:
    check = subprocess.run(
        ["git", "apply", "--check", str(patch)],
        cwd=str(btstack),
        capture_output=True,
        text=True,
    )
    if check.returncode == 0:
        run(["git", "apply", str(patch)], cwd=btstack)
    else:
        reverse = subprocess.run(
            ["git", "apply", "--reverse", "--check", str(patch)],
            cwd=str(btstack),
            capture_output=True,
            text=True,
        )
        if reverse.returncode != 0:
            raise RuntimeError(
                "Cannot apply BTstack patch %s:\n%s"
                % (patch.name, check.stderr)
            )

bt_env = os.environ.copy()
bt_env["IDF_PATH"] = str(target / "src")
run([sys.executable, "integrate_btstack.py"], cwd=btstack_port, env=bt_env)

# No global ESP-IDF / PlatformIO installation is modified.
print("[BLUEPAD32] Using project-local source at", target)
print("[BLUEPAD32] Pinned commit:", commit)
