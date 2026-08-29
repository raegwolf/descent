"""Generate the ESPConsole enclosure and AP242 STEP PMI dimensions.

The component measurements in README.md are authoritative. Dimensions shown
on the STEP models are limited to enclosure envelope and mechanical interfaces;
component outline sizes stay in the README to keep the 3D view readable.
"""

from dataclasses import dataclass
from math import atan2, degrees
import os
from pathlib import Path

import cadquery as cq
from OCP.gp import gp_Ax2, gp_Dir, gp_Pnt
from OCP.IFSelect import IFSelect_ReturnStatus
from OCP.Interface import Interface_Static
from OCP.STEPCAFControl import STEPCAFControl_Writer
from OCP.STEPControl import STEPControl_AsIs
from OCP.TCollection import TCollection_ExtendedString, TCollection_HAsciiString
from OCP.TDataStd import TDataStd_Name
from OCP.TDocStd import TDocStd_Document
from OCP.XCAFApp import XCAFApp_Application
from OCP.XCAFDimTolObjects import XCAFDimTolObjects_DimensionObject, XCAFDimTolObjects_DimensionType
from OCP.XCAFDoc import XCAFDoc_Dimension, XCAFDoc_DocumentTool


OUT = Path(os.environ.get("ESPCONSOLE_OUT", Path(__file__).resolve().parent))
OUT.mkdir(parents=True, exist_ok=True)

# Enclosure dimensions, millimetres.
CASE_W = 180.0
CASE_H = 78.0
CASE_D = 30.0
FRONT_D = 8.0
REAR_D = CASE_D - FRONT_D
WALL = 2.4

# Front interfaces. The 69 x 50 mm TFT body protrudes through this
# landscape-oriented opening.
# It is intentionally a sharp-cornered rectangle with no fit allowance.
SCREEN_WINDOW_W = 69.0
SCREEN_WINDOW_H = 50.0
SCREEN_WINDOW_R = 0.0
JOYSTICK_OPENING_D = 30.0
JOYSTICK_X = 65.0

# M2 printed-hole starting dimensions; tune for the printer/material.
POST_OD = 7.0
POST_BASE_OD = 9.0
PILOT_D = 1.7
CLEARANCE_D = 2.4
SCREEN_GUSSET_LENGTH = 10.0
SCREEN_GUSSET_HEIGHT = 12.0
SCREEN_GUSSET_THICKNESS = 2.4
SEAM_ENGAGEMENT = 1.0
SEAM_GROOVE_DEPTH = 1.2
SEAM_FRONT_RIDGE_W = 1.2
SEAM_RADIAL_CLEARANCE = 0.2

# Measured component mounting interfaces.
SCREEN_PITCH_X = 76.0
SCREEN_PITCH_Y = 44.0
SCREEN_HOLES = [
    (-SCREEN_PITCH_X / 2, -SCREEN_PITCH_Y / 2),
    (-SCREEN_PITCH_X / 2, SCREEN_PITCH_Y / 2),
    (SCREEN_PITCH_X / 2, -SCREEN_PITCH_Y / 2),
    (SCREEN_PITCH_X / 2, SCREEN_PITCH_Y / 2),
]

JOYSTICK_PITCH_X = 20.0
JOYSTICK_PITCH_Y = 26.0
JOYSTICK_HOLES = [
    (module_x + dx, y)
    for module_x in (-JOYSTICK_X, JOYSTICK_X)
    for dx in (-JOYSTICK_PITCH_X / 2, JOYSTICK_PITCH_X / 2)
    for y in (-JOYSTICK_PITCH_Y / 2, JOYSTICK_PITCH_Y / 2)
]

CLOSURE_POINTS = [
    (-84.0, 0.0),
    (84.0, 0.0),
    (-47.0, -33.0),
    (47.0, -33.0),
    (-47.0, 33.0),
    (47.0, 33.0),
]

# Mounting planes in assembled coordinates, measured from rear exterior Z=0.
SCREEN_FRONT_INSET = 0.5
SCREEN_MODULE_DEPTH = 5.4
SCREEN_PCB_Z = CASE_D - SCREEN_FRONT_INSET - SCREEN_MODULE_DEPTH  # 24.1
JOYSTICK_PCB_Z = 8.0
ESP32_PCB_Z = WALL + 0.8  # 3.2
ESP32_WIRE_CLEARANCE = 20.0
ESP32_POCKET_W = 28.0
ESP32_POCKET_L = 63.0
ESP32_BORDER_H = 5.0
ESP32_BORDER_T = 2.0
ESP32_BOARD_T = 1.6
ESP32_LIP_T = 3.0
ESP32_LIP_OVERLAP = 5.0
ESP32_LIP_CLEARANCE = 0.4
ESP32_USB_LIP_T = 3.0
ESP32_USB_LIP_PROJECTION = 5.0
ESP32_USB_LIP_HEIGHT_FROM_PCB = 5.0
ESP32_USB_LIP_WALL_OVERLAP = 0.2
USB_OPENING_W = 22.0
USB_OPENING_H = 5.0
USB_OPENING_R = 1.5
USB_OPENING_BELOW_BOARD = 0.0


@dataclass(frozen=True)
class PmiDimension:
    name: str
    value: float
    p1: tuple
    p2: tuple
    text: tuple
    plane_normal: tuple = (0.0, 0.0, 1.0)
    direction: tuple = (1.0, 0.0, 0.0)
    plane_xdir: tuple = (1.0, 0.0, 0.0)
    diameter: bool = False


def stadium(width: float, height: float, depth: float, z0: float = 0.0) -> cq.Workplane:
    """Create a capsule/stadium solid centred on XY and starting at z0."""
    straight = width - height
    centre = cq.Workplane("XY").box(straight, height, depth, centered=(True, True, False)).translate((0, 0, z0))
    caps = (
        cq.Workplane("XY")
        .pushPoints([(-straight / 2, 0), (straight / 2, 0)])
        .circle(height / 2)
        .extrude(depth)
        .translate((0, 0, z0))
    )
    return centre.union(caps).clean()


def rounded_rectangle_prism(width: float, height: float, radius: float, depth: float, z0: float = 0.0) -> cq.Workplane:
    horizontal = cq.Workplane("XY").box(width - 2 * radius, height, depth, centered=(True, True, False)).translate((0, 0, z0))
    vertical = cq.Workplane("XY").box(width, height - 2 * radius, depth, centered=(True, True, False)).translate((0, 0, z0))
    corners = (
        cq.Workplane("XY")
        .pushPoints([
            (-width / 2 + radius, -height / 2 + radius),
            (-width / 2 + radius, height / 2 - radius),
            (width / 2 - radius, -height / 2 + radius),
            (width / 2 - radius, height / 2 - radius),
        ])
        .circle(radius)
        .extrude(depth)
        .translate((0, 0, z0))
    )
    return horizontal.union(vertical).union(corners).clean()


def stadium_band(width, height, inset_start, inset_end, depth, z0):
    """Create a stadium-shaped perimeter band between two inward offsets."""
    outer = stadium(width - 2 * inset_start, height - 2 * inset_start, depth, z0)
    inner = stadium(
        width - 2 * inset_end,
        height - 2 * inset_end,
        depth + 0.4,
        z0 - 0.2,
    )
    return outer.cut(inner).clean()


def cylinders(points, diameter, z0, height):
    return cq.Workplane("XY").pushPoints(points).circle(diameter / 2).extrude(height).translate((0, 0, z0))


def vertical_gusset(origin, direction, length, height, thickness):
    """Create a printable triangular rib extending radially from a stalk."""
    overlap = POST_OD / 2 - 1.0
    local = (
        cq.Workplane("XZ")
        .polyline([(overlap, 0), (overlap, height), (length, 0)])
        .close()
        .extrude(thickness / 2, both=True)
    )
    angle = degrees(atan2(direction[1], direction[0]))
    return local.rotate((0, 0, 0), (0, 0, 1), angle).translate((origin[0], origin[1], WALL))


def make_front_shell() -> cq.Workplane:
    # Start with an inward overlap zone, then subtract everything except the
    # 1.2 mm-wide outer ridge. This keeps the exterior face and closed depth fixed.
    outer = stadium(CASE_W, CASE_H, FRONT_D + SEAM_ENGAGEMENT)
    extension_relief = stadium(
        CASE_W - 2 * SEAM_FRONT_RIDGE_W,
        CASE_H - 2 * SEAM_FRONT_RIDGE_W,
        SEAM_ENGAGEMENT + 0.4,
        FRONT_D - 0.2,
    )
    outer = outer.cut(extension_relief)
    cavity = stadium(CASE_W - 2 * WALL, CASE_H - 2 * WALL, FRONT_D - WALL + 0.2, WALL)
    front = outer.cut(cavity)
    # Recess the inner portion of the front wall to accept the rear inner ridge.
    front_inner_groove = stadium_band(
        CASE_W,
        CASE_H,
        SEAM_FRONT_RIDGE_W,
        WALL + 0.2,
        SEAM_GROOVE_DEPTH + 0.4,
        FRONT_D - SEAM_GROOVE_DEPTH,
    )
    front = front.cut(front_inner_groove)
    screen_cut = (
        cq.Workplane("XY")
        .box(SCREEN_WINDOW_W, SCREEN_WINDOW_H, WALL + 0.4, centered=(True, True, False))
        .translate((0, 0, -0.2))
    )
    stick_cuts = cylinders([(-JOYSTICK_X, 0), (JOYSTICK_X, 0)], JOYSTICK_OPENING_D, -0.2, WALL + 0.4)
    front = front.cut(screen_cut).cut(stick_cuts)
    front = front.union(cylinders(CLOSURE_POINTS, 7.5, WALL, FRONT_D - WALL - 0.3))
    front = front.cut(cylinders(CLOSURE_POINTS, PILOT_D, FRONT_D - 5.0, 5.3))
    return front.clean()


def make_rear_shell() -> cq.Workplane:
    # Extend the nominal seam zone internally, then subtract its outer band. The
    # remaining inner band is the rear ridge and adds no exterior enclosure depth.
    outer = stadium(CASE_W, CASE_H, REAR_D + SEAM_ENGAGEMENT)
    cavity = stadium(
        CASE_W - 2 * WALL,
        CASE_H - 2 * WALL,
        REAR_D + SEAM_ENGAGEMENT - WALL + 0.2,
        WALL,
    )
    rear = outer.cut(cavity)
    rear_outer_relief = stadium_band(
        CASE_W,
        CASE_H,
        0.0,
        SEAM_FRONT_RIDGE_W + SEAM_RADIAL_CLEARANCE,
        SEAM_GROOVE_DEPTH + SEAM_ENGAGEMENT + 0.4,
        REAR_D - SEAM_GROOVE_DEPTH,
    )
    rear = rear.cut(rear_outer_relief)

    # One rounded 22 x 5 mm opening spans both USB-C sockets, matching the final
    # requested cutout and beginning at the ESP32 PCB-back plane.
    usb_bottom = ESP32_PCB_Z - USB_OPENING_BELOW_BOARD
    usb_centre_z = usb_bottom + USB_OPENING_H / 2
    usb_cut = (
        cq.Workplane("XZ", origin=(0, CASE_H / 2, usb_centre_z))
        .sketch()
        .rect(USB_OPENING_W, USB_OPENING_H)
        .vertices()
        .fillet(USB_OPENING_R)
        .finalize()
        .extrude((WALL + 4.0) / 2, both=True)
    )
    rear = rear.cut(usb_cut)

    rear = rear.union(cylinders(CLOSURE_POINTS, 5.5, WALL, REAR_D - WALL - 0.3))
    rear = rear.cut(cylinders(CLOSURE_POINTS, CLEARANCE_D, -0.2, REAR_D + 0.4))
    # Locally interrupt the rear seam ridge around the six 7.5 mm front bosses.
    # The 0.2 mm radial allowance prevents the interlock from clipping a boss.
    seam_boss_reliefs = cylinders(
        CLOSURE_POINTS,
        7.5 + 2 * SEAM_RADIAL_CLEARANCE,
        REAR_D - 0.1,
        SEAM_ENGAGEMENT + 0.5,
    )
    rear = rear.cut(seam_boss_reliefs)

    rear = rear.union(cylinders(SCREEN_HOLES, POST_BASE_OD, WALL, 3.0))
    rear = rear.union(cylinders(SCREEN_HOLES, POST_OD, WALL, SCREEN_PCB_Z - WALL))
    # Two outward-facing triangular gussets reinforce every 21.7 mm screen
    # stalk. They stop well below the PCB plane and avoid the central electronics.
    for x, y in SCREEN_HOLES:
        directions = (
            (1 if x > 0 else -1, 0),
            (0, 1 if y > 0 else -1),
        )
        for direction in directions:
            rear = rear.union(
                vertical_gusset(
                    (x, y),
                    direction,
                    SCREEN_GUSSET_LENGTH,
                    SCREEN_GUSSET_HEIGHT,
                    SCREEN_GUSSET_THICKNESS,
                )
            )
    rear = rear.cut(cylinders(SCREEN_HOLES, PILOT_D, SCREEN_PCB_Z - 5.0, 5.3))

    rear = rear.union(cylinders(JOYSTICK_HOLES, POST_BASE_OD, WALL, 3.0))
    rear = rear.union(cylinders(JOYSTICK_HOLES, POST_OD, WALL, JOYSTICK_PCB_Z - WALL))
    rear = rear.cut(cylinders(JOYSTICK_HOLES, PILOT_D, JOYSTICK_PCB_Z - 5.0, 5.3))

    # ESP32 pocket: exact 28 x 63 mm internal size with a 5 mm-high perimeter.
    # A 0.8 mm floor pad establishes the existing Z=3.2 PCB-back plane.
    pad_height = ESP32_PCB_Z - WALL
    pocket_floor = (
        cq.Workplane("XY")
        .box(ESP32_POCKET_W, ESP32_POCKET_L, pad_height, centered=(True, True, False))
        .translate((0, 0, WALL))
    )
    outer_w = ESP32_POCKET_W + 2 * ESP32_BORDER_T
    outer_l = ESP32_POCKET_L + 2 * ESP32_BORDER_T
    border_outer = (
        cq.Workplane("XY")
        .box(outer_w, outer_l, ESP32_BORDER_H, centered=(True, True, False))
        .translate((0, 0, WALL))
    )
    border_inner = (
        cq.Workplane("XY")
        .box(ESP32_POCKET_W, ESP32_POCKET_L, ESP32_BORDER_H + 0.4, centered=(True, True, False))
        .translate((0, 0, WALL - 0.2))
    )
    pocket_border = border_outer.cut(border_inner)

    # Clear the USB-facing short border across the same 22 mm connector opening.
    pocket_usb_cut = (
        cq.Workplane("XY")
        .box(USB_OPENING_W, ESP32_BORDER_T + 0.8, ESP32_BORDER_H + 0.4, centered=(True, True, False))
        .translate((0, ESP32_POCKET_L / 2 + ESP32_BORDER_T / 2, WALL - 0.2))
    )
    pocket_border = pocket_border.cut(pocket_usb_cut)

    # At the antenna end, a 3 mm-thick horizontal lip projects 5 mm back over
    # the PCB. Its underside has 0.4 mm clearance above the 1.6 mm board.
    lip_bottom = ESP32_PCB_Z + ESP32_BOARD_T + ESP32_LIP_CLEARANCE
    lip_length = ESP32_BORDER_T + ESP32_LIP_OVERLAP
    lip_centre_y = -(ESP32_POCKET_L / 2 + ESP32_BORDER_T - lip_length / 2)
    antenna_lip = (
        cq.Workplane("XY")
        .box(outer_w, lip_length, ESP32_LIP_T, centered=(True, True, False))
        .translate((0, lip_centre_y, lip_bottom))
    )

    # The USB-side lip is anchored into the inside top wall and projects 5 mm
    # inward over the 22 mm socket pair. Its underside is exactly 5 mm above the
    # PCB-back plane, so it bears at the measured connector-top height while the
    # 22 x 5 mm external opening below remains unobstructed.
    inside_top_wall_y = CASE_H / 2 - WALL
    usb_lip_length = ESP32_USB_LIP_PROJECTION + ESP32_USB_LIP_WALL_OVERLAP
    usb_lip_centre_y = inside_top_wall_y - ESP32_USB_LIP_PROJECTION / 2 + ESP32_USB_LIP_WALL_OVERLAP / 2
    usb_lip_bottom = ESP32_PCB_Z + ESP32_USB_LIP_HEIGHT_FROM_PCB
    usb_socket_lip = (
        cq.Workplane("XY")
        .box(USB_OPENING_W, usb_lip_length, ESP32_USB_LIP_T, centered=(True, True, False))
        .translate((0, usb_lip_centre_y, usb_lip_bottom))
    )
    return (
        rear
        .union(pocket_floor)
        .union(pocket_border)
        .union(antenna_lip)
        .union(usb_socket_lip)
        .clean()
    )


def make_reference_parts():
    screen_pcb = cq.Workplane("XY").box(86.0, 50.0, 1.6, centered=(True, True, False)).translate((0, 0, SCREEN_PCB_Z))
    # Visual TFT stack: derived 3.8 mm above the PCB front, ending 0.5 mm below exterior.
    screen_tft = cq.Workplane("XY").box(SCREEN_WINDOW_W, SCREEN_WINDOW_H, 3.8, centered=(True, True, False)).translate((0, 0, SCREEN_PCB_Z + 1.6))
    joystick_parts = []
    for x in (-JOYSTICK_X, JOYSTICK_X):
        pcb = cq.Workplane("XY").box(27.0, 33.8, 1.06, centered=(True, True, False)).translate((x, 0, JOYSTICK_PCB_Z))
        switch = cq.Workplane("XY").box(16.0, 16.0, 11.94, centered=(True, True, False)).translate((x, 0, JOYSTICK_PCB_Z + 1.06))
        actuator = cq.Workplane("XY").center(x, 0).circle(4.0).extrude(8.8).translate((0, 0, JOYSTICK_PCB_Z + 13.0))
        cap = cq.Workplane("XY").center(x, 0).circle(13.0).extrude(10.2).translate((0, 0, JOYSTICK_PCB_Z + 21.8))
        cap_top = cq.Workplane("XY").center(x, 0).circle(10.0).extrude(6.3).translate((0, 0, JOYSTICK_PCB_Z + 25.7))
        joystick_parts.extend([pcb, switch, actuator, cap, cap_top])
    esp32 = cq.Workplane("XY").box(28.0, 64.0, 1.6, centered=(True, True, False)).translate((0, 0, ESP32_PCB_Z))
    wire_envelope = cq.Workplane("XY").box(28.0, 57.0, ESP32_WIRE_CLEARANCE, centered=(True, True, False)).translate((0, 3.5, ESP32_PCB_Z))
    return [screen_pcb, screen_tft, *joystick_parts, esp32, wire_envelope]


def linear(name, value, p1, p2, text, normal=(0, 0, 1), direction=(1, 0, 0), xdir=(1, 0, 0)):
    return PmiDimension(name, value, p1, p2, text, normal, direction, xdir)


def diameter(name, value, point, text, normal=(0, 0, 1)):
    # OCCT 7.7 can crash while exporting a diameter PMI associated only with a
    # compound. Encode the same interface as a linear span across the diameter;
    # the semantic name retains the diameter symbol and Fusion placement.
    x, y, z = point
    return PmiDimension(f"{name} - diameter {value:g}", value, (x - value / 2, y, z), (x + value / 2, y, z), text, normal)


def _vector_add(a, b):
    return tuple(a[index] + b[index] for index in range(3))


def _vector_subtract(a, b):
    return tuple(a[index] - b[index] for index in range(3))


def _vector_scale(vector, scale):
    return tuple(component * scale for component in vector)


def _vector_dot(a, b):
    return sum(a[index] * b[index] for index in range(3))


def _vector_cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def _vector_normalize(vector):
    length = _vector_dot(vector, vector) ** 0.5
    return tuple(component / length for component in vector)


def make_pmi_presentation(dimension):
    """Build Fusion-visible graphical PMI attached to a semantic dimension."""
    direction = _vector_normalize(dimension.direction)
    normal = _vector_normalize(dimension.plane_normal)
    perpendicular = _vector_normalize(_vector_cross(normal, direction))

    # Project each measured point onto the dimension line through the requested
    # text position, then connect it back to the measured feature.
    first_offset = _vector_dot(_vector_subtract(dimension.p1, dimension.text), direction)
    second_offset = _vector_dot(_vector_subtract(dimension.p2, dimension.text), direction)
    line_start = _vector_add(dimension.text, _vector_scale(direction, first_offset))
    line_end = _vector_add(dimension.text, _vector_scale(direction, second_offset))
    span = abs(second_offset - first_offset)
    arrow_length = min(2.0, max(0.18, span * 0.25))
    arrow_half_width = arrow_length * 0.38
    span_direction = direction if second_offset >= first_offset else _vector_scale(direction, -1)

    def edge(start, end):
        return cq.Edge.makeLine(cq.Vector(*start), cq.Vector(*end))

    edges = [
        edge(dimension.p1, line_start),
        edge(dimension.p2, line_end),
        edge(line_start, line_end),
    ]
    for point, inward in (
        (line_start, span_direction),
        (line_end, _vector_scale(span_direction, -1)),
    ):
        arrow_base = _vector_add(point, _vector_scale(inward, arrow_length))
        edges.append(edge(point, _vector_add(arrow_base, _vector_scale(perpendicular, arrow_half_width))))
        edges.append(edge(point, _vector_add(arrow_base, _vector_scale(perpendicular, -arrow_half_width))))

    prefix = "D" if "diameter" in dimension.name.lower() else ""
    display_text = f"{prefix}{dimension.value:g} mm"
    text_origin = _vector_add(dimension.text, _vector_scale(normal, 0.05))
    text_plane = cq.Plane(
        origin=cq.Vector(*text_origin),
        xDir=cq.Vector(*direction),
        normal=cq.Vector(*normal),
    )
    text_shape = cq.Workplane(text_plane).text(
        display_text,
        2.2,
        0.02,
        combine=False,
        halign="center",
        valign="center",
    ).val()
    return cq.Compound.makeCompound([*edges, text_shape]), display_text


def overall_dimensions(depth):
    return [
        linear("Overall width", CASE_W, (-90, -39, depth), (90, -39, depth), (0, -48, depth)),
        linear("Overall height", CASE_H, (-90, -39, depth), (-90, 39, depth), (-101, 0, depth), direction=(0, 1, 0)),
        linear("Overall depth", depth, (90, 39, 0), (90, 39, depth), (101, 39, depth / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
    ]


def contact_dimensions():
    usb_bottom = ESP32_PCB_Z - USB_OPENING_BELOW_BOARD
    return [
        linear("TFT mount pitch X (centre-to-centre)", SCREEN_PITCH_X, (-38, -22, SCREEN_PCB_Z), (38, -22, SCREEN_PCB_Z), (0, -30, SCREEN_PCB_Z)),
        linear("TFT mount pitch Y (centre-to-centre)", SCREEN_PITCH_Y, (-38, -22, SCREEN_PCB_Z), (-38, 22, SCREEN_PCB_Z), (-48, 0, SCREEN_PCB_Z), direction=(0, 1, 0)),
        linear("TFT PCB mounting plane from rear", SCREEN_PCB_Z, (0, 0, 0), (0, 0, SCREEN_PCB_Z), (-52, 0, SCREEN_PCB_Z / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
        linear("TFT front inset", SCREEN_FRONT_INSET, (42, 0, CASE_D - SCREEN_FRONT_INSET), (42, 0, CASE_D), (52, 0, CASE_D - SCREEN_FRONT_INSET / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
        diameter("TFT and joystick M2 pilot", PILOT_D, (-38, -22, SCREEN_PCB_Z), (-30, -16, SCREEN_PCB_Z)),
        linear("Joystick mount pitch X (centre-to-centre)", JOYSTICK_PITCH_X, (-75, -13, JOYSTICK_PCB_Z), (-55, -13, JOYSTICK_PCB_Z), (-65, -21, JOYSTICK_PCB_Z)),
        linear("Joystick mount pitch Y (centre-to-centre)", JOYSTICK_PITCH_Y, (-75, -13, JOYSTICK_PCB_Z), (-75, 13, JOYSTICK_PCB_Z), (-84, 0, JOYSTICK_PCB_Z), direction=(0, 1, 0)),
        linear("Joystick PCB mounting plane from rear", JOYSTICK_PCB_Z, (65, 0, 0), (65, 0, JOYSTICK_PCB_Z), (78, 0, JOYSTICK_PCB_Z / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
        diameter("Joystick front opening", JOYSTICK_OPENING_D, (-JOYSTICK_X, 0, CASE_D), (-JOYSTICK_X, 20, CASE_D)),
        linear("ESP32 pocket internal width", ESP32_POCKET_W, (-ESP32_POCKET_W / 2, 0, WALL + ESP32_BORDER_H), (ESP32_POCKET_W / 2, 0, WALL + ESP32_BORDER_H), (0, 10, WALL + ESP32_BORDER_H)),
        linear("ESP32 pocket internal length", ESP32_POCKET_L, (-ESP32_POCKET_W / 2, -ESP32_POCKET_L / 2, WALL + ESP32_BORDER_H), (-ESP32_POCKET_W / 2, ESP32_POCKET_L / 2, WALL + ESP32_BORDER_H), (-24, 0, WALL + ESP32_BORDER_H), direction=(0, 1, 0)),
        linear("ESP32 pocket border height", ESP32_BORDER_H, (18, 0, WALL), (18, 0, WALL + ESP32_BORDER_H), (25, 0, WALL + ESP32_BORDER_H / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
        linear("ESP32 antenna lip overlap", ESP32_LIP_OVERLAP, (0, -ESP32_POCKET_L / 2, WALL + ESP32_BORDER_H), (0, -ESP32_POCKET_L / 2 + ESP32_LIP_OVERLAP, WALL + ESP32_BORDER_H), (8, -ESP32_POCKET_L / 2 + ESP32_LIP_OVERLAP / 2, WALL + ESP32_BORDER_H), direction=(0, 1, 0)),
        linear("ESP32 antenna lip thickness", ESP32_LIP_T, (0, -ESP32_POCKET_L / 2, ESP32_PCB_Z + ESP32_BOARD_T + ESP32_LIP_CLEARANCE), (0, -ESP32_POCKET_L / 2, ESP32_PCB_Z + ESP32_BOARD_T + ESP32_LIP_CLEARANCE + ESP32_LIP_T), (10, -ESP32_POCKET_L / 2, ESP32_PCB_Z + ESP32_BOARD_T + ESP32_LIP_CLEARANCE + ESP32_LIP_T / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
        linear("ESP32 PCB plane from rear", ESP32_PCB_Z, (25, 0, 0), (25, 0, ESP32_PCB_Z), (34, 0, ESP32_PCB_Z / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
        linear("ESP32 temporary-wire clearance", ESP32_WIRE_CLEARANCE, (0, 3.5, ESP32_PCB_Z), (0, 3.5, ESP32_PCB_Z + ESP32_WIRE_CLEARANCE), (32, 3.5, ESP32_PCB_Z + ESP32_WIRE_CLEARANCE / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
        linear("Dual USB-C opening width", USB_OPENING_W, (-USB_OPENING_W / 2, 39, usb_bottom), (USB_OPENING_W / 2, 39, usb_bottom), (0, 47, usb_bottom)),
        linear("Dual USB-C opening height", USB_OPENING_H, (USB_OPENING_W / 2, 39, usb_bottom), (USB_OPENING_W / 2, 39, usb_bottom + USB_OPENING_H), (24, 39, usb_bottom + USB_OPENING_H / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
        linear("USB socket lip underside above PCB back", ESP32_USB_LIP_HEIGHT_FROM_PCB, (0, CASE_H / 2 - WALL - ESP32_USB_LIP_PROJECTION / 2, ESP32_PCB_Z), (0, CASE_H / 2 - WALL - ESP32_USB_LIP_PROJECTION / 2, ESP32_PCB_Z + ESP32_USB_LIP_HEIGHT_FROM_PCB), (-18, CASE_H / 2 - WALL - ESP32_USB_LIP_PROJECTION / 2, ESP32_PCB_Z + ESP32_USB_LIP_HEIGHT_FROM_PCB / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
        linear("USB socket lip inward projection", ESP32_USB_LIP_PROJECTION, (0, CASE_H / 2 - WALL, ESP32_PCB_Z + ESP32_USB_LIP_HEIGHT_FROM_PCB + ESP32_USB_LIP_T), (0, CASE_H / 2 - WALL - ESP32_USB_LIP_PROJECTION, ESP32_PCB_Z + ESP32_USB_LIP_HEIGHT_FROM_PCB + ESP32_USB_LIP_T), (12, CASE_H / 2 - WALL - ESP32_USB_LIP_PROJECTION / 2, ESP32_PCB_Z + ESP32_USB_LIP_HEIGHT_FROM_PCB + ESP32_USB_LIP_T), direction=(0, 1, 0)),
        linear("USB socket lip thickness", ESP32_USB_LIP_T, (USB_OPENING_W / 2, CASE_H / 2 - WALL - ESP32_USB_LIP_PROJECTION / 2, ESP32_PCB_Z + ESP32_USB_LIP_HEIGHT_FROM_PCB), (USB_OPENING_W / 2, CASE_H / 2 - WALL - ESP32_USB_LIP_PROJECTION / 2, ESP32_PCB_Z + ESP32_USB_LIP_HEIGHT_FROM_PCB + ESP32_USB_LIP_T), (22, CASE_H / 2 - WALL - ESP32_USB_LIP_PROJECTION / 2, ESP32_PCB_Z + ESP32_USB_LIP_HEIGHT_FROM_PCB + ESP32_USB_LIP_T / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
        linear("Perimeter seam engagement", SEAM_ENGAGEMENT, (89.4, 0, REAR_D - SEAM_ENGAGEMENT), (89.4, 0, REAR_D), (98, 0, REAR_D - SEAM_ENGAGEMENT / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
        linear("Perimeter seam radial clearance", SEAM_RADIAL_CLEARANCE, (CASE_W / 2 - SEAM_FRONT_RIDGE_W - SEAM_RADIAL_CLEARANCE, 0, REAR_D - SEAM_ENGAGEMENT / 2), (CASE_W / 2 - SEAM_FRONT_RIDGE_W, 0, REAR_D - SEAM_ENGAGEMENT / 2), (CASE_W / 2 - SEAM_FRONT_RIDGE_W - SEAM_RADIAL_CLEARANCE / 2, 8, REAR_D - SEAM_ENGAGEMENT / 2)),
        diameter("Rear M2 screw clearance", CLEARANCE_D, (-84, 0, REAR_D), (-75, 8, REAR_D)),
        diameter("Front M2 screw pilot", PILOT_D, (-84, 0, CASE_D - FRONT_D), (-75, -8, CASE_D - FRONT_D)),
    ]


def add_pmi(doc, shape_label, dimensions):
    dim_tool = XCAFDoc_DocumentTool.DimTolTool_s(doc.Main())
    for dimension in dimensions:
        dim_label = dim_tool.AddDimension()
        attribute = XCAFDoc_Dimension.Set_s(dim_label)
        obj = XCAFDimTolObjects_DimensionObject()
        if dimension.diameter:
            obj.SetType(XCAFDimTolObjects_DimensionType.XCAFDimTolObjects_DimensionType_Size_Diameter)
        else:
            obj.SetType(XCAFDimTolObjects_DimensionType.XCAFDimTolObjects_DimensionType_Location_LinearDistance_FromOuterToOuter)
        obj.SetValue(dimension.value)
        obj.SetSemanticName(TCollection_HAsciiString(dimension.name))
        obj.SetPoint(gp_Pnt(*dimension.p1))
        if not dimension.diameter:
            obj.SetPoint2(gp_Pnt(*dimension.p2))
        obj.SetPointTextAttach(gp_Pnt(*dimension.text))
        obj.SetPlane(gp_Ax2(gp_Pnt(*dimension.p1), gp_Dir(*dimension.plane_normal), gp_Dir(*dimension.plane_xdir)))
        obj.SetDirection(gp_Dir(*dimension.direction))
        presentation, display_text = make_pmi_presentation(dimension)
        obj.SetPresentation(
            presentation.wrapped,
            TCollection_HAsciiString(f"{display_text} - {dimension.name}"),
        )
        attribute.SetObject(obj)
        TDataStd_Name.Set_s(dim_label, TCollection_ExtendedString(dimension.name))
        if dimension.diameter:
            dim_tool.SetDimension(shape_label, dim_label)
        else:
            dim_tool.SetDimension(shape_label, shape_label, dim_label)


def export_ap242(name, shapes, path, dimensions):
    # Keep bodies as independent XDE roots. PMI is associated with the first
    # body; its placement points may reference the complete multi-body model.
    # OCCT 7.7 crashes when PMI is attached directly to a compound/assembly label.
    app = XCAFApp_Application.GetApplication_s()
    doc = TDocStd_Document(TCollection_ExtendedString("XmlOcaf"))
    app.InitDocument(doc)
    shape_tool = XCAFDoc_DocumentTool.ShapeTool_s(doc.Main())
    labels = []
    for index, shape in enumerate(shapes, 1):
        label = shape_tool.AddShape(shape.wrapped, False)
        part_name = name if len(shapes) == 1 else f"{name} body {index}"
        TDataStd_Name.Set_s(label, TCollection_ExtendedString(part_name))
        labels.append(label)
    top_label = labels[0]
    add_pmi(doc, top_label, dimensions)
    writer = STEPCAFControl_Writer()
    Interface_Static.SetIVal_s("write.step.schema", 5)  # AP242 DIS
    writer.ChangeWriter().Model(True)
    writer.SetColorMode(True)
    writer.SetNameMode(True)
    writer.SetDimTolMode(True)
    if not writer.Transfer(doc, STEPControl_AsIs):
        raise RuntimeError(f"Could not transfer {path.name} to STEP writer")
    status = writer.Write(str(path))
    if status != IFSelect_ReturnStatus.IFSelect_RetDone:
        raise RuntimeError(f"Could not write {path}")


def export_all():
    front = make_front_shell()
    rear = make_rear_shell()
    if not front.val().isValid() or not rear.val().isValid():
        raise RuntimeError("Generated enclosure contains an invalid solid")

    front_dims = overall_dimensions(FRONT_D + SEAM_ENGAGEMENT) + [
        linear(
            "TFT protrusion opening width",
            SCREEN_WINDOW_W,
            (-SCREEN_WINDOW_W / 2, -SCREEN_WINDOW_H / 2, 0),
            (SCREEN_WINDOW_W / 2, -SCREEN_WINDOW_H / 2, 0),
            (0, -SCREEN_WINDOW_H / 2 - 8, 0),
        ),
        linear(
            "TFT protrusion opening height",
            SCREEN_WINDOW_H,
            (-SCREEN_WINDOW_W / 2, -SCREEN_WINDOW_H / 2, 0),
            (-SCREEN_WINDOW_W / 2, SCREEN_WINDOW_H / 2, 0),
            (-SCREEN_WINDOW_W / 2 - 10, 0, 0),
            direction=(0, 1, 0),
        ),
        diameter("Joystick front opening", JOYSTICK_OPENING_D, (-JOYSTICK_X, 0, 0), (-JOYSTICK_X, 20, 0)),
        diameter("Front M2 screw pilot", PILOT_D, (-84, 0, FRONT_D), (-75, -8, FRONT_D)),
        linear("Front outer seam ridge width", SEAM_FRONT_RIDGE_W, (CASE_W / 2 - SEAM_FRONT_RIDGE_W, 0, FRONT_D + SEAM_ENGAGEMENT), (CASE_W / 2, 0, FRONT_D + SEAM_ENGAGEMENT), (CASE_W / 2 - SEAM_FRONT_RIDGE_W / 2, 8, FRONT_D + SEAM_ENGAGEMENT)),
        linear("Front seam ridge engagement", SEAM_ENGAGEMENT, (89.4, 0, FRONT_D), (89.4, 0, FRONT_D + SEAM_ENGAGEMENT), (98, 0, FRONT_D + SEAM_ENGAGEMENT / 2), normal=(0, 1, 0), direction=(0, 0, 1)),
    ]
    export_ap242("ESPConsole front", [front.val()], OUT / "espconsole-front.step", front_dims)

    # Screen posts extend 2.1 mm beyond the nominal 22 mm rear shell seam and
    # nest into the front cavity, so the standalone rear part is 24.1 mm deep.
    contacts = contact_dimensions()
    rear_dims = overall_dimensions(SCREEN_PCB_Z) + [
        dimension
        for dimension in contacts
        if not dimension.name.startswith((
            "TFT front inset",
            "Joystick front opening",
            "Front M2 screw pilot",
        ))
    ]
    export_ap242("ESPConsole rear", [rear.val()], OUT / "espconsole-rear.step", rear_dims)

    front_location = cq.Location(cq.Vector(0, 0, CASE_D), cq.Vector(1, 0, 0), 180)
    assembled_shapes = [rear.val(), front.val().moved(front_location)]
    all_dims = overall_dimensions(CASE_D) + contact_dimensions()
    export_ap242("ESPConsole enclosure", assembled_shapes, OUT / "espconsole-assembly.step", all_dims)

    fit_shapes = assembled_shapes + [part.val() for part in make_reference_parts()]
    export_ap242("ESPConsole fit check", fit_shapes, OUT / "espconsole-fit-check.step", all_dims)

    print(f"front valid={front.val().isValid()} volume={front.val().Volume():.1f} bbox={front.val().BoundingBox()}")
    print(f"rear valid={rear.val().isValid()} volume={rear.val().Volume():.1f} bbox={rear.val().BoundingBox()}")
    for path in sorted(OUT.glob("*.step")):
        print(f"{path.name}: {path.stat().st_size} bytes")


if __name__ == "__main__":
    export_all()
