Import("env")

import re
from pathlib import Path


ENGINE_SOURCES = """
CFILE/CFILE.C
2D/PCX.C 2D/CANVAS.C 2D/BITMAP.C 2D/BITBLT.C
2D/PIXEL.C 2D/GPIXEL.C 2D/RECT.C 2D/LINE.C
2D/SCANLINE.C 2D/RLE.C 2D/FONT.C 2D/DISC.C
2D/SCALE.C 2D/IBITBLT.C 2D/LINEAR_C.C 2D/GR.C 2D/PALETTE.C
MISC/ERROR.C IFF/IFF.C
MAIN/TITLES.C MAIN/TEXT.C MAIN/MENU.C MAIN/CONFIG.C MAIN/NEWMENU.C
MAIN/KCONFIG.C MAIN/GAMEFONT.C MAIN/GAMESEQ.C MAIN/BM.C MAIN/PIGGY.C
MAIN/HASH.C MAIN/CNTRLCEN.C MAIN/FUELCEN.C MAIN/GAUGES.C MAIN/OBJECT.C
MAIN/AI.C MAIN/WALL.C MAIN/GAMESEG.C MAIN/LASER.C MAIN/PHYSICS.C
MAIN/COLLIDE.C MAIN/FIREBALL.C MAIN/POWERUP.C MAIN/WEAPON.C MAIN/GAME.C
MAIN/SLEW.C MAIN/AIPATH.C MAIN/LIGHTING.C MAIN/RENDER.C MAIN/AUTOMAP.C
MAIN/FVI.C MAIN/FVI_OFLOW_C.C MAIN/HOSTAGE.C MAIN/NEWDEMO.C
MAIN/NEWDEMO_SHAREWARE_C.C MAIN/SWITCH.C MAIN/ENDLEVEL.C
MAIN/TERRAIN.C MAIN/GAMEMINE.C MAIN/HUD.C MAIN/MGLOBAL.C MAIN/VCLIP.C
MAIN/EFFECTS.C MAIN/ROBOT.C MAIN/TEXMERGE.C MAIN/POLYOBJ.C MAIN/INFERNO.C
MAIN/ARGS.C MAIN/SCORES.C MAIN/CREDITS.C MAIN/MISSION.C
MAIN/PLAYSAVE.C MAIN/GAMESAVE.C MAIN/MORPH.C MAIN/PAGING.C MAIN/CONTROLS.C
MAIN/BMREAD.C MAIN/JOYDEFS.C MAIN/STATE.C MAIN/STATE_SHAREWARE_C.C
FIX/FIX_C.C FIX/TABLES_C.C VECMAT/VECMAT_C.C VECMAT/VM_ALIAS_C.C
TEXMAP/NTMAP.C TEXMAP/TMAPFLAT.C TEXMAP/SCANLINE.C
3D/CLIPPER_C.C 3D/DRAW_C.C 3D/GLOBVARS_C.C 3D/HORIZON_C.C
3D/INSTANCE_C.C 3D/INTERP_C.C 3D/MATRIX_C.C 3D/POINTS_C.C
3D/ROD_C.C 3D/SETUP_C.C
""".split()

SOURCE_DIRS = [
    "INCLUDES", "CFILE", "BIOS", "2D", "3D", "FIX", "VECMAT", "MISC",
    "MEM", "IFF", "PSLIB", "TEXMAP", "MAIN",
]

project_dir = Path(env.subst("$PROJECT_DIR"))
repository = project_dir.parent
stage_dir = Path(env.subst("$BUILD_DIR")) / "descent_engine_source"
object_dir = Path(env.subst("$BUILD_DIR")) / "descent_engine_object"

env.Append(CFLAGS=["-std=gnu89"])


def sanitized_content(source):
    content = source.read_bytes().replace(b"\x1a", b"")
    text = content.decode("latin-1")
    text = re.sub(
        r'(^\s*#\s*include\s*[<"][^>"\r\n]*)\\([^>"\r\n]*[>"])',
        r"\1/\2",
        text,
        flags=re.MULTILINE,
    )
    return text.encode("latin-1")


def update_staged_file(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    encoded = sanitized_content(source)
    if not destination.exists() or destination.read_bytes() != encoded:
        destination.write_bytes(encoded)


for directory in SOURCE_DIRS:
    for source in (repository / directory).iterdir():
        if source.is_file() and source.suffix.lower() == ".h":
            destination = stage_dir / directory / source.name
            update_staged_file(source, destination)

env.Prepend(CPPPATH=[str(stage_dir / directory) for directory in SOURCE_DIRS])

expected_sources = {
    stage_dir / Path(relative_name).with_suffix(".c")
    for relative_name in ENGINE_SOURCES
}

# The staged directory survives normal incremental builds.  Remove generated C
# files that are no longer in the explicit manifest so BuildSources cannot
# retain an obsolete engine module (notably MEM.C, whose DOS symbols collide
# with lwIP's allocator).
if stage_dir.exists():
    for staged_source in stage_dir.rglob("*.c"):
        if staged_source not in expected_sources:
            staged_source.unlink()

for relative_name in ENGINE_SOURCES:
    source = repository / relative_name
    destination = stage_dir / Path(relative_name).with_suffix(".c")
    encoded = (b"#define DESCENT_ENGINE_BUILD 1\n"
               b"#include \"arduino_compat.h\"\n" + sanitized_content(source))
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_bytes() != encoded:
        destination.write_bytes(encoded)

env.BuildSources(str(object_dir), str(stage_dir))
