import os
import re
import subprocess
Import("env")

# Fail the build if the AVR soft-float library got linked in. A single float
# literal (e.g. 0.2f * a * b) silently pulls in ~600 bytes of flash; the
# firmware is fixed-point everywhere, so any of these symbols is a regression.
FLOAT_SYMBOLS = re.compile(
    r" T __(addsf3|subsf3|mulsf3|divsf3|cmpsf2|floatsisf|floatunsisf|fixsfsi|fixunssfsi)$"
)


def check_no_float(source, target, env):
    elf = str(target[0])
    nm = os.path.join(os.path.dirname(env.subst("$CC")), "avr-nm")
    if not os.path.isabs(nm):
        nm = "avr-nm"
    out = subprocess.check_output([nm, elf], env=env["ENV"]).decode()
    hits = [line.split()[-1] for line in out.splitlines() if FLOAT_SYMBOLS.search(line)]
    if hits:
        print("error: soft-float library linked into %s: %s" % (elf, ", ".join(hits)))
        print("       find the caller with: avr-objdump -d %s | grep -B20 'call.*<__mulsf3>'" % elf)
        env.Exit(1)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", check_no_float)
