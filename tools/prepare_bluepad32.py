Import("env")

from pathlib import Path
import subprocess

project = Path(env.subst("$PROJECT_DIR"))
target = project / "third_party" / "bluepad32"
commit = "6888717"  # Bluepad32 v4.0-beta2; includes Bluetooth keyboard support.

def run(args, cwd=None):
    subprocess.check_call(args, cwd=str(cwd) if cwd else None)

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

# Bluepad32 v4.0-beta2 carries its BTstack dependency in the repository.
# No global ESP-IDF / PlatformIO installation is modified.
print("[BLUEPAD32] Using project-local source at", target)
print("[BLUEPAD32] Pinned commit:", commit)
