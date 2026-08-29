# ESPConsole 3D enclosure model

Open `espconsole-assembly.step` in Autodesk Fusion to see the complete two-piece enclosure as 3D solid bodies. The enclosure body is 180 × 78 × 30 mm.

Files:

- `espconsole-assembly.step` — assembled front and rear enclosure, two valid solids, 180 × 78 × 30 mm overall.
- `espconsole-fit-check.step` — assembled enclosure plus measured screen, joystick and ESP32 envelopes. The joystick caps extend the fit-check envelope to 40 mm deep.
- `espconsole-front.step` — editable front solid for Fusion.
- `espconsole-rear.step` — editable rear solid for Fusion.
- `build_3d_enclosure.py` — authoritative CadQuery generator for all four STEP files. It writes files into the same folder by default.

The model includes the screen and joystick front openings, six front screw bosses, six rear clearance towers, four reinforced display standoffs, four standoffs per joystick board, an ESP32 retaining pocket with an antenna-end lip, and one top opening spanning both USB-C sockets.

## Fusion dimension annotations

All four STEP files use STEP AP242 semantic and graphical PMI. Fusion should import these under the model's PMI browser node; turn that node or individual dimensions on to display dimension lines, arrows and numeric labels attached to the model as it rotates.

The assembly and fit-check files contain 28 dimensions: overall enclosure width, height and depth; TFT and joystick mount pitches; mounting-plane heights; joystick opening; ESP32 pocket, border, antenna/USB retaining lips and temporary-wire clearances; dual USB-C opening; perimeter-seam engagement/clearance; and M2 pilot/clearance sizes. The front and rear part files contain the relevant subsets.

Hole diameters are encoded as linear spans with names containing `diameter`. This works around an OpenCascade 7.7 AP242 exporter crash while retaining the correct value and anchored 3D placement in Fusion.

## Current enclosure design values

| Feature | Model value | Notes |
|---|---:|---|
| Overall enclosure | 180 × 78 × 30 | Width × height × closed depth |
| Front nominal shell depth | 8 | Includes 2.4 mm face wall; external front position is unchanged |
| Front maximum part depth | 9 | Includes the 1 mm internal seam ridge projecting into the rear groove |
| Rear nominal shell depth | 22 | Screen posts extend to Z = 24.1 and nest into front cavity |
| Wall thickness | 2.4 | Nominal |
| Screen PCB mounting plane | 24.1 from rear exterior | Derived from 30 − 0.5 inset − 5.4 module depth |
| Screen stalk reinforcement | Two 10 × 12 × 2.4 triangular gussets per stalk | Eight outward-facing ribs; each stops below the PCB plane |
| Joystick PCB mounting plane | 8 from rear exterior | Places the actuator at Z = 29.8 and cap top at Z = 40 |
| ESP32 PCB plane | 3.2 from rear exterior | 2.4 mm wall plus 0.8 mm support allowance |
| ESP32 temporary-wire top | 23.2 from rear exterior | 20 mm allowance above PCB back plane |
| ESP32 pocket internal size | 28 × 63 | Exact user-specified internal width × length; no additional XY print tolerance |
| ESP32 pocket border | 5 high × 2 thick | Integral plastic perimeter; USB-facing short side is opened across the connector cutout |
| ESP32 antenna retaining lip | 3 thick × 5 overlap | Horizontal lip at antenna end; 0.4 mm nominal clearance above PCB top |
| ESP32 USB-socket retaining lip | 3 thick × 5 inward projection | 22 mm wide; underside is 5 mm above PCB back at global Z = 8.2 |
| ESP32 support pad | 0.8 high | Establishes PCB back plane at 3.2 mm from rear exterior |
| Dual USB-C wall opening | 22 × 5, R1.5 corners | Final user-specified opening, beginning at the ESP32 PCB-back plane |
| Joystick front openings | diameter 30 | Clears static 26 mm lower cap; verify full tilt physically |
| TFT protrusion opening | 69 × 50, square corners | Exact user-specified opening; TFT body protrudes through it in landscape orientation |
| M2 thread-forming pilots | diameter 1.7 | Starting value; calibrate for printer/material |
| M2 rear clearances | diameter 2.4 | Starting value; drill/ream if required |

## Interlocking front/rear perimeter seam

The shell joint uses a continuous subtractively formed lap profile around the stadium-shaped perimeter:

- The front retains a 1.2 mm-wide outer ridge that engages 1 mm into the rear.
- The rear retains the complementary inner ridge, 1 mm wide, which engages 1 mm into the front relief.
- Radial clearance between the ridges is 0.2 mm.
- Each receiving relief is 1.2 mm deep, providing 0.2 mm axial end clearance around a 1 mm engagement.
- The ridges are formed by subtracting the adjacent perimeter bands from overlapping seam zones. No material is added to the exterior front or rear faces, the 2.4 mm front-face thickness is unchanged, and the closed enclosure remains 30 mm deep.
- The rear inner ridge is locally relieved around the six front screw bosses, with 0.2 mm radial boss clearance, so the alignment seam does not interfere with the closure fasteners.

This lap joint aligns the two shells, reduces lateral movement and creates a less direct dust/light path than a flat butt seam. The 0.2 mm clearances are starting values for PETG; use a short perimeter test print if the printer tends to produce oversized walls.

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
| Protruding TFT rectangle / front opening | 69 × 50 | Final user-specified; centred in landscape orientation with no corner radius or added clearance |
| Mount-hole pitch, long axis | 76 | Measured centre-to-centre |
| Mount-hole pitch, short axis | 44 | Measured centre-to-centre |
| Hole-centre offset from each 86 mm end | 5 | Derived: `(86 - 76) / 2` |
| Hole-centre offset from each 50 mm edge | 3 | Derived: `(50 - 44) / 2` |
| PCB thickness | 1.6 | Measured |
| Back of PCB to front of TFT | 5.4 | Measured total module thickness |
| TFT stack above front face of PCB | 3.8 | Derived: `5.4 - 1.6` |

The four mounting-hole centres are therefore represented relative to the PCB centre as X = ±38 and Y = ±22, assuming the holes are symmetrically positioned. This puts each hole centre 5 mm from the nearest 86 mm PCB end and 3 mm from the nearest 50 mm PCB edge. Relative to the centred 69 × 50 TFT protrusion/opening, each screw centre is 3.5 mm outside the nearest vertical TFT edge: `38 - (69 / 2) = 3.5`. Vertically, its Y = ±22 position lies 3 mm inside the TFT's top or bottom edge at Y = ±25. The shortest screw-centre-to-TFT-edge distance is therefore 3.5 mm, horizontally.

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

The two USB-C sockets are on the end opposite the antenna. The final enclosure opening is 22 × 5 mm, matching the measured 22 mm outside-to-outside socket span and approximately matching the measured 4.9 mm height.

### ESP32 retaining pocket

The holder is an exact 28 × 63 mm internal pocket surrounded by a 5 mm-high, 2 mm-thick printed border. A 0.8 mm integral support pad raises the PCB-back plane to Z = 3.2 mm. The USB-facing short border has a central 22 mm opening aligned with the case USB opening. At the antenna end, a 3 mm-thick horizontal lip projects 5 mm back over the board; its underside has 0.4 mm nominal clearance above the 1.6 mm PCB.

At the USB end, a second 22 mm-wide, 3 mm-thick lip is anchored 0.2 mm into the inside top wall and projects 5 mm inward over the two socket bodies. Its underside is 5 mm above the PCB-back plane—global Z = 8.2 mm—so it bears against the measured 4.9 mm-high connector tops. The external 22 × 5 mm cable opening occupies Z = 3.2–8.2 mm and remains unobstructed below the lip.

Insert the antenna end under the lip first, then lower the USB end into the pocket. The earlier component measurements derive a 64 mm total board length including the antenna, while the confirmed pocket length is 63 mm. The fit-check model deliberately retains the 64 mm measured board envelope, so verify how the antenna end sits beneath the lip. Also test the exact 28 mm pocket width before printing the full rear shell because it includes no printer clearance.

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

The STEP geometry now uses the measured board outlines, mount pitches, component heights, exact 69 × 50 square-cornered TFT protrusion opening, ESP32 wire allowance and USB-C socket span above. The joystick cap's swept diameter at full tilt was not supplied and remains provisional. Print a thin front-face test coupon before committing to the complete enclosure, and calibrate the 1.7 mm M2 pilot against the intended filament and printer.
