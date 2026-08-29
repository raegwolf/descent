# ESPConsole 3D enclosure model

Open `espconsole-assembly.step` in Autodesk Fusion to see the complete two-piece enclosure as 3D solid bodies. The enclosure body is 180 × 78 × 30 mm.

Files:

- `espconsole-assembly.step` — assembled front and rear enclosure, two valid solids, 180 × 78 × 30 mm overall.
- `espconsole-fit-check.step` — assembled enclosure plus measured screen, joystick and ESP32 envelopes. The joystick caps extend the fit-check envelope to 40 mm deep.
- `espconsole-front.step` — editable front solid for Fusion.
- `espconsole-rear.step` — editable rear solid for Fusion.
- `build_3d_enclosure.py` — authoritative CadQuery generator for all four STEP files. It writes files into the same folder by default.

The model includes the screen and joystick front openings, six front screw bosses, six rear clearance towers, four reinforced display standoffs, four standoffs per joystick board, an ESP32 slide cradle, and one top opening spanning both USB-C sockets.

## Fusion dimension annotations

All four STEP files use STEP AP242 semantic and graphical PMI. Fusion should import these under the model's PMI browser node; turn that node or individual dimensions on to display dimension lines, arrows and numeric labels attached to the model as it rotates.

The assembly and fit-check files contain 19 dimensions: overall enclosure width, height and depth; TFT and joystick mount pitches; mounting-plane heights; joystick opening; ESP32 cradle and temporary-wire clearances; dual USB-C opening; and M2 pilot/clearance sizes. The front and rear part files contain the relevant subsets.

Hole diameters are encoded as linear spans with names containing `diameter`. This works around an OpenCascade 7.7 AP242 exporter crash while retaining the correct value and anchored 3D placement in Fusion.

## Current enclosure design values

| Feature | Model value | Notes |
|---|---:|---|
| Overall enclosure | 180 × 78 × 30 | Width × height × closed depth |
| Front shell depth | 8 | Includes 2.4 mm face wall |
| Rear nominal shell depth | 22 | Screen posts extend to Z = 24.1 and nest into front cavity |
| Wall thickness | 2.4 | Nominal |
| Screen PCB mounting plane | 24.1 from rear exterior | Derived from 30 − 0.5 inset − 5.4 module depth |
| Screen stalk reinforcement | Two 10 × 12 × 2.4 triangular gussets per stalk | Eight outward-facing ribs; each stops below the PCB plane |
| Joystick PCB mounting plane | 8 from rear exterior | Places the actuator at Z = 29.8 and cap top at Z = 40 |
| ESP32 PCB plane | 3.2 from rear exterior | 2.4 mm wall plus 0.8 mm support allowance |
| ESP32 temporary-wire top | 23.2 from rear exterior | 20 mm allowance above PCB back plane |
| ESP32 cradle clear width | 29.2 | 28 mm PCB plus 0.6 mm clearance per side |
| Dual USB-C wall opening | 21 × 6.4, R1.5 corners | Final user-specified width; 0.5 mm below and 1 mm above the measured 4.9 mm connector height |
| Joystick front openings | diameter 30 | Clears static 26 mm lower cap; verify full tilt physically |
| Screen window | 74 × 46, R2.5 corners | Provisional until visible glass/bezel is measured |
| M2 thread-forming pilots | diameter 1.7 | Starting value; calibrate for printer/material |
| M2 rear clearances | diameter 2.4 | Starting value; drill/ream if required |

## Printed stalks, screws and material

The screen, joystick and enclosure-closure stalks are intended to be printed as integral plastic features rather than separate metal spacers.

### Screen stalk reinforcement

Each screen stalk is 7 mm in diameter with a 9 mm diameter, 3 mm high base collar. The stalk rises 21.7 mm from the inside rear wall to the screen PCB mounting plane. Because this is relatively tall for a printed boss, every stalk now has two outward-facing triangular gussets. Each gusset extends 10 mm from the stalk centre, rises 12 mm above the inside rear wall and is 2.4 mm thick. The ribs remain well below the screen PCB and face away from the central ESP32/wiring area.

### M2 screw arrangement

| Joint | Hole in model | Suggested starting screw | Notes |
|---|---:|---|---|
| TFT PCB to rear screen stalks | 1.7 mm pilot | M2 × 6 pan-head/thread-forming | Approximately 4.4 mm plastic engagement after the 1.6 mm PCB |
| Joystick PCB to rear stalks | 1.7 mm pilot | M2 × 6 pan-head/thread-forming | Approximately 4.9 mm plastic engagement after the 1.06 mm PCB |
| Rear shell to front shell | 2.4 mm rear clearance; 1.7 mm front pilot | M2 × 25 starting point | Screw passes freely through the rear tower and forms a thread only in the front boss; confirm bite depth before tightening |

Printed holes vary with nozzle, layer height, material and machine calibration. Print a small boss test first. A 1.7 mm pilot is an initial value for thread-forming into plastic, while 2.4 mm is intended as free clearance for an M2 screw. Chase the clearance holes with a 2.4 mm drill if necessary. Do not overtighten: stop when the PCB or case joint is seated.

For frequent disassembly, redesign the front bosses for M2 heat-set inserts rather than repeatedly using thread-forming screws. The present front bosses are designed for direct threading into plastic, not for a specific insert size.

### Recommended plastic and print setup

- **PETG is the default recommendation:** tougher and less prone to splitting than PLA, with enough heat resistance for a handheld enclosure.
- **ASA or ABS is preferable for high-temperature use**, such as storage in a hot vehicle, but requires controlled shrinkage, good ventilation and usually an enclosed printer.
- **PLA is suitable for fit-test prototypes**, but it is more brittle around threaded bosses and can soften or creep in warm conditions.
- Print the front and rear with their exterior faces on the build plate so the stalks grow vertically.
- Use 4–5 perimeters, approximately 0.2 mm layers and at least 35–40% infill. Use a slicer modifier to make stalks, gussets and screw bosses effectively solid where possible.

## Authoritative measured component dimensions

All dimensions below are in millimetres and were measured from the actual components. "Derived" values are calculated from those measurements rather than measured independently.

### TFT screen module

| Feature | Dimension | Status/notes |
|---|---:|---|
| PCB outline | 86 × 50 | Measured |
| Mount-hole pitch, long axis | 76 | Measured centre-to-centre |
| Mount-hole pitch, short axis | 44 | Measured centre-to-centre |
| Hole-centre offset from each 86 mm end | 5 | Derived: `(86 - 76) / 2` |
| Hole-centre offset from each 50 mm edge | 3 | Derived: `(50 - 44) / 2` |
| PCB thickness | 1.6 | Measured |
| Back of PCB to front of TFT | 5.4 | Measured total module thickness |
| TFT stack above front face of PCB | 3.8 | Derived: `5.4 - 1.6` |

The four mounting-hole centres are therefore provisionally represented relative to the PCB centre as X = ±38 and Y = ±22, assuming the holes are symmetrically positioned.

### ESP32-S3 board

| Feature | Dimension | Status/notes |
|---|---:|---|
| Main PCB length, excluding antenna projection | 57 | Measured |
| Antenna projection | 7 | Measured |
| Overall length including antenna | 64 | Derived: `57 + 7` |
| Board width | 28 | Measured |
| Back of PCB to top of pins | 11 | Measured |
| Temporary-wiring clearance | 20 | Required design allowance from back of PCB |
| Back of PCB to top of USB-C connector | 4.9 | Measured |
| USB-C connector pair, outside edge to outside edge | 22 | Measured across the 28 mm board width |
| Margin from connector pair to each board edge | 3 | Measured/confirmed by `28 - 22 = 6` total margin |
| Gap between the two USB-C connectors | 2.5 | Measured |
| Approximate width of each USB-C connector | 9.75 | Derived, assuming equal widths: `(22 - 2.5) / 2` |

The two USB-C sockets are on the end opposite the antenna. The final enclosure opening is user-specified as 21 mm wide. This is 1 mm narrower than the separately measured 22 mm outside-to-outside socket span, so its alignment and access to both cable plugs should be checked against the physical board before printing the full rear shell.

### PS2-style joystick module — each of two modules

| Feature | Dimension | Status/notes |
|---|---:|---|
| PCB outline | 27 × 33.8 | Measured |
| Mount-hole pitch across 27 mm side | 20 | Measured centre-to-centre |
| Mount-hole pitch across 33.8 mm side | 26 | Measured centre-to-centre |
| Hole-centre offset from each 27 mm edge | 3.5 | Derived: `(27 - 20) / 2` |
| Hole-centre offset from each 33.8 mm edge | 3.9 | Derived: `(33.8 - 26) / 2` |
| PCB thickness | 1.06 | Measured |
| Back of PCB to top of switch body, excluding actuator | 13 | Measured |
| Back of PCB to top of actuator | 21.8 | Measured |
| Back of PCB to top of fitted plastic cap | 32 | Measured total assembled height |
| Plastic cap total height | 16.5 | Measured |
| Lower rounded portion of cap | Ø26 | Measured maximum lower diameter |
| Upper thumb-contact portion | Ø20 | Measured maximum upper diameter |
| Cap/actuator overlap | 6.3 | Derived: `21.8 + 16.5 - 32` |

The four mounting-hole centres are provisionally represented relative to each joystick PCB centre as X = ±10 and Y = ±13, assuming symmetric hole placement. The front opening must clear the Ø26 lower cap portion throughout the full joystick tilt range; the final opening diameter should be verified with a physical travel test rather than based on the static diameter alone.

## Fit checks still required

The STEP geometry now uses the measured board outlines, mount pitches, component heights, ESP32 wire allowance and USB-C socket span above. Two physical interfaces were not supplied and remain provisional: the TFT visible-glass/bezel opening and the joystick cap's swept diameter at full tilt. Print a thin front-face test coupon before committing to the complete enclosure, and calibrate the 1.7 mm M2 pilot against the intended filament and printer.
