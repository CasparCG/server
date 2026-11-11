#!/usr/bin/env python3
"""Make CasparCG.app self-contained: copy all non-system dylibs (Homebrew) into
Contents/Frameworks and rewrite references to @rpath.

usage: bundle_dylibs.py <CasparCG.app> [extra dylib ...]

Extra dylibs are libraries loaded at runtime with dlopen() (Vulkan loader,
MoltenVK) which otool cannot discover from the executable.

The main executable gets two rpaths:
  @executable_path/../Frameworks        (CasparCG.app/Contents/MacOS/casparcg)
  @executable_path/../../..             (Contents/Frameworks/casparcg Helper*.app/Contents/MacOS/...)
"""

import os
import shutil
import subprocess
import sys

SYSTEM_PREFIXES = ("/usr/lib/", "/System/")


def run(*args):
    return subprocess.run(args, check=True, capture_output=True, text=True).stdout


def dependencies(binary):
    """Install names this binary links against (excluding its own id)."""
    out = run("otool", "-L", binary).splitlines()[1:]
    deps = [line.strip().split(" (")[0] for line in out if line.strip()]
    own_id = install_id(binary)
    return [d for d in deps if d != own_id]


def install_id(binary):
    out = run("otool", "-D", binary).splitlines()
    return out[1].strip() if len(out) > 1 else None


def rpaths(binary):
    out = run("otool", "-l", binary).splitlines()
    result = []
    for i, line in enumerate(out):
        if "cmd LC_RPATH" in line:
            for follow in out[i + 1 : i + 4]:
                follow = follow.strip()
                if follow.startswith("path "):
                    result.append(follow.split(" ")[1])
    return result


def resolve(dep, binary, binary_origin, search):
    """Resolve an install name to a real file path."""
    if dep.startswith("@rpath/"):
        leaf = dep[len("@rpath/") :]
        for rp in rpaths(binary_origin) + search:
            rp = rp.replace("@loader_path", os.path.dirname(binary_origin))
            candidate = os.path.join(rp, leaf)
            if os.path.exists(candidate):
                return os.path.realpath(candidate)
        return None
    if dep.startswith("@loader_path/"):
        return os.path.realpath(os.path.join(os.path.dirname(binary_origin), dep[len("@loader_path/") :]))
    if dep.startswith("@executable_path/"):
        return None
    return os.path.realpath(dep) if os.path.exists(dep) else None


def main():
    app = os.path.abspath(sys.argv[1])
    extras = sys.argv[2:]
    exe = os.path.join(app, "Contents", "MacOS", "casparcg")
    frameworks = os.path.join(app, "Contents", "Frameworks")
    os.makedirs(frameworks, exist_ok=True)

    search = ["/opt/homebrew/lib", "/usr/local/lib"]

    # binary in bundle -> where it came from (to resolve @loader_path / its own rpaths)
    origin = {exe: os.path.realpath(exe)}
    queue = [exe]
    bundled = {}  # leaf name -> path in bundle

    def bundle_lib(src, leaf):
        if leaf in bundled:
            return bundled[leaf]
        dst = os.path.join(frameworks, leaf)
        if os.path.realpath(src) != os.path.realpath(dst):
            shutil.copy2(src, dst)
        os.chmod(dst, 0o755)
        bundled[leaf] = dst
        origin[dst] = src
        queue.append(dst)
        return dst

    for extra in extras:
        real = os.path.realpath(extra)
        bundle_lib(real, os.path.basename(extra))

    processed = set()
    modified = set()
    while queue:
        binary = queue.pop()
        if binary in processed:
            continue
        processed.add(binary)

        changes = []
        for dep in dependencies(binary):
            if dep.startswith(SYSTEM_PREFIXES):
                continue
            if dep.startswith("@rpath/") and os.path.exists(os.path.join(frameworks, dep[len("@rpath/") :])):
                # Already bundled (e.g. libndi.dylib) or will be
                leaf = dep[len("@rpath/") :]
                if leaf not in bundled:
                    bundle_lib(os.path.join(frameworks, leaf), leaf)
                continue
            real = resolve(dep, binary, origin[binary], search)
            if not real:
                print(f"warning: cannot resolve {dep} (from {binary})", file=sys.stderr)
                continue
            leaf = os.path.basename(dep)
            bundle_lib(real, leaf)
            changes += ["-change", dep, f"@rpath/{leaf}"]

        args = ["install_name_tool"]
        if binary != exe and install_id(binary) != f"@rpath/{os.path.basename(binary)}":
            args += ["-id", f"@rpath/{os.path.basename(binary)}"]
        args += changes
        # Drop absolute rpaths (e.g. /opt/homebrew/...), so nothing outside the bundle is used
        for rp in rpaths(binary):
            if not rp.startswith("@"):
                args += ["-delete_rpath", rp]
        if len(args) > 1:
            run(*args, binary)
            modified.add(binary)

    existing = rpaths(exe)
    for rp in ("@executable_path/../Frameworks", "@executable_path/../../.."):
        if rp not in existing:
            run("install_name_tool", "-add_rpath", rp, exe)
            modified.add(exe)

    # install_name_tool invalidates signatures; arm64 requires valid (ad-hoc) signatures.
    # Untouched libraries (e.g. the vendor-signed libndi.dylib) keep their original signature.
    for path in modified:
        run("codesign", "--force", "--sign", "-", "--timestamp=none", path)

    print(f"Bundled {len(bundled)} libraries into {frameworks}")


if __name__ == "__main__":
    main()
