Import("env")
# Beispiel: Nach-Build-Hook (platzhalter)
def _post_prog(source, target, env):
    print("[post_build] finished target:", target)
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", _post_prog)