import os

Import("env")

env.Replace(COMPILATIONDB_INCLUDE_TOOLCHAIN=True)
env.Replace(
    COMPILATIONDB_PATH=os.path.join("$PROJECT_BUILD_DIR", "compile_commands.json")
)


def generate_compiledb(source, target, env):
    env.Execute(f"pio run -e {env['PIOENV']} -t compiledb")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", generate_compiledb)
