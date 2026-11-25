import os
from pathlib import Path
import shutil
import subprocess

android = Path(__file__).resolve().parents[1]
args = [str(android / "tests/SigningTest.groovy"), str(android / "release-signing.gradle")]
if shutil.which("groovy"):
    command = ["groovy", *args]
else:
    gradle = shutil.which("gradle")
    home = os.environ.get("GRADLE_HOME")
    if not home and gradle:
        home = str(Path(gradle).resolve().parents[1])
    if not home or not list((Path(home) / "lib").glob("groovy-*.jar")):
        raise SystemExit("Signing tests require Groovy or a Gradle installation on PATH / GRADLE_HOME")
    command = ["java", "-cp", str(Path(home) / "lib/*"), "groovy.ui.GroovyMain", *args]
subprocess.run(command, check=True)
