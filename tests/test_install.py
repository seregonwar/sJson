"""Build consumers against installed headers and exported CMake targets."""
from pathlib import Path
import subprocess
import sys

cmake, ctest, source, build, config, generator, cc, cxx = sys.argv[1:]
prefix = Path(build) / "install-test"
consumer = Path(build) / "consumer-test"
config = config or "Release"


def run(*args):
    subprocess.run([str(argument) for argument in args], check=True)


run(cmake, "--install", build, "--prefix", prefix, "--config", config)
run(cmake, "-S", Path(source) / "tests/install_consumer", "-B", consumer,
    "-G", generator, f"-DCMAKE_PREFIX_PATH={prefix}", f"-DCMAKE_BUILD_TYPE={config}",
    f"-DCMAKE_C_COMPILER={cc}", f"-DCMAKE_CXX_COMPILER={cxx}")
run(cmake, "--build", consumer, "--config", config, "--parallel", "2")
run(ctest, "--test-dir", consumer, "-C", config, "--output-on-failure")
