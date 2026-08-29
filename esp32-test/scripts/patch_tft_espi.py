"""Correct TFT_eSPI 2.5.43's ESP32-S3 DMA register mapping."""

from pathlib import Path

Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.

source_path = (
    Path(env.subst("$PROJECT_LIBDEPS_DIR"))
    / env.subst("$PIOENV")
    / "TFT_eSPI"
    / "Processors"
    / "TFT_eSPI_ESP32_S3.c"
)

broken_callback = "WRITE_PERI_REG(SPI_DMA_CONF_REG(spi_host), 0);"
fixed_callback = "WRITE_PERI_REG(SPI_DMA_CONF_REG(SPI_PORT), 0);"

source = source_path.read_text(encoding="utf-8")
if broken_callback in source:
    source_path.write_text(
        source.replace(broken_callback, fixed_callback, 1), encoding="utf-8"
    )
elif fixed_callback not in source:
    raise RuntimeError(
        f"Unsupported TFT_eSPI ESP32-S3 DMA callback in {source_path}"
    )
