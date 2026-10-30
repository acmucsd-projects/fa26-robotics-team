import os

Import("env")

# override compilation DB path
env.Replace(COMPILATIONDB_PATH=os.path.join("$BUILD_DIR", "compile_commands.json"))
