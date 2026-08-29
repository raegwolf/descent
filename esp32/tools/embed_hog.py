"""Embed Descent's HOG and PIG as flash-resident linker objects."""

import subprocess
from pathlib import Path

from SCons.Script import Builder

Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.


def embed_resource(target, source, env):
    source_path = Path(source[0].get_abspath())
    target_path = Path(target[0].get_abspath())
    mcu = env.BoardConfig().get("build.mcu", "esp32")
    toolchain = env.PioPlatform().get_package_dir(
        f"toolchain-xtensa-{mcu}"
    )
    objcopy = Path(toolchain) / "bin" / f"xtensa-{mcu}-elf-objcopy"
    subprocess.run(
        [
            str(objcopy),
            "--input-target",
            "binary",
            "--output-target",
            "elf32-xtensa-le",
            "--binary-architecture",
            "xtensa",
            "--rename-section",
            ".data=.rodata.embedded",
            "--set-section-alignment",
            ".data=4",
            source_path.name,
            str(target_path),
        ],
        cwd=source_path.parent,
        check=True,
    )


env.Append(
    BUILDERS={
        "EmbedDescentResource": Builder(
            action=env.VerboseAction(embed_resource, "Embedding $SOURCE")
        )
    }
)

menu_only = env.GetProjectOption("custom_menu_only", "no").lower() in (
    "1", "yes", "true", "on"
)
resource_names = ("descent.hog",) if menu_only else (
    "descent.hog", "descent.pig"
)
embedded_objects = []
for resource_name in resource_names:
    source = env.File(f"$PROJECT_DIR/../resources/{resource_name}")
    target = env.EmbedDescentResource(
        f"$BUILD_DIR/embedded_{resource_name.replace('.', '_')}.o", source
    )
    env.Depends("$PIOMAINPROG", target)
    embedded_objects.append(target)

env.AppendUnique(PIOBUILDFILES=embedded_objects)
