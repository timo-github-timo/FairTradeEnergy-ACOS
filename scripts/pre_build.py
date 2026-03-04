Import("env")
# Beispiel: setze ein Macro mit Versionsinfo, falls noch nicht vorhanden
if "APP_VERSION" not in env:
    env.Append(CPPDEFINES=[('APP_VERSION', '\\"dev\\"')])
print("[pre_build] running (placeholder)")