"""Pass --coverage to the linker for the native coverage envs.

PlatformIO only forwards build_flags to the compiler (CCFLAGS).
This script appends the flag to LINKFLAGS so libgcov is linked.
"""
Import("env")  # noqa: F821
env.Append(LINKFLAGS=["--coverage"])  # noqa: F821
