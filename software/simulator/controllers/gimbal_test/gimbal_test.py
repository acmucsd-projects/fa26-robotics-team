import sys
import math
import queue
from pathlib import Path

import cv2
import numpy as np
from controller import Supervisor

# Make your pipeline files importable (software/ folder)
SOFTWARE_DIR = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(SOFTWARE_DIR))

from global_state import Global
from processing import start_process_thread
from output_process import start_output_thread

# ---------------- Config ----------------
TRACK_MODE = "vision"       # "vision"     = aim using gs.center_point from processing.py
                            # "supervisor" = aim using the skin's true position (no detector needed)

FRAME_EVERY = 2        # send every Nth sim step to the model
RAISE_HAND_AT = 1.0    # sim seconds

# --- Gimbal motion (smooth, rate-limited) ---
KP_YAW = 1.2           # rad/s per unit of error
KP_PITCH = 0.9
DEADBAND = 0.03        # vision mode: ignore tiny offsets (normalised)
MAX_RATE = 1.5         # rad/s, hard limit on gimbal speed
SMOOTHING = 0.25       # 0..1, lower = smoother, higher = snappier
YAW_SIGN = 1           # flip to -1 if the gimbal pans away from the target
PITCH_SIGN = 1         # flip to -1 if the gimbal tilts away from the target

# --- Supervisor mode geometry ---
YAW_OFFSET = 0.0       # radians; the direction the camera faces at yaw=0 (0 = +x axis).
                       # Try math.pi/2, -math.pi/2 or math.pi if it looks the wrong way.
AIM_HEIGHT = 0.9       # metres above the skin's origin to aim at (chest height)
TILT_ENABLED = True    # set False to keep pitch fixed

# --- Vision mode: search when the target is lost ---
LOST_TIMEOUT = 1.5     # seconds without a target before searching
SEARCH_SPEED = 0.5     # rad/s pan speed while searching
SEARCH_ENABLED = True

# Extra fields for CharacterSkin. Check CharacterSkin.proto for the real
# field names (e.g. a model selector) and add them here once confirmed.
SKIN_EXTRA_FIELDS = ""
SKIN_START = (4.0, -1.5, 0.0)   # x, y, z start position; adjust as needed

# Back-and-forth walking
WALK_SPEED = 0.8       # metres per second
WALK_DISTANCE = 3.0    # metres between the two turnaround points (along +y)
FACING_OFFSET = 0.0    # radians; add math.pi/2 or math.pi if the skin faces sideways/backwards

# ---------------- Webots setup ----------------
robot = Supervisor()
timestep = int(robot.getBasicTimeStep())
dt = timestep / 1000.0


def spawn_character_skin():
    """Spawn a CharacterSkin (wrapped in a Robot) at runtime. Returns the node or None.

    A Skin node can only live inside a Robot, so it can't be inserted
    directly at the top level of the world.
    """
    if robot.getFromDef("SKIN_NODE") is not None:
        return robot.getFromDef("SKIN_NODE")  # already exists, don't duplicate
    children = robot.getRoot().getField("children")
    children.importMFNodeFromString(
        -1,
        f'DEF SKIN_NODE Robot {{ '
        f'translation {SKIN_START[0]} {SKIN_START[1]} {SKIN_START[2]} '
        f'name "skin_holder" '
        f'controller "<none>" '
        f'children [ CharacterSkin {{ name "skin" {SKIN_EXTRA_FIELDS} }} ] }}'
    )
    node = robot.getFromDef("SKIN_NODE")
    if node is None:
        print("Warning: CharacterSkin spawn failed. Check the Webots console.")
    else:
        print("CharacterSkin spawned.")
    return node


def walk_pose(t):
    """Triangle-wave motion along +y. Returns (y_offset, heading_angle)."""
    period = 2.0 * WALK_DISTANCE
    phase = (WALK_SPEED * t) % period
    if phase < WALK_DISTANCE:
        return phase, math.pi / 2          # walking toward +y
    return period - phase, -math.pi / 2    # walking back toward -y


def wrap_angle(a):
    """Wrap to [-pi, pi]."""
    return (a + math.pi) % (2.0 * math.pi) - math.pi


camera = robot.getDevice("gimbal_camera")
camera.enable(timestep)
W, H = camera.getWidth(), camera.getHeight()
print(f"Camera resolution: {W}x{H}")

yaw_motor = robot.getDevice("yaw_motor")
pitch_motor = robot.getDevice("pitch_motor")
# Motor speed limit is a bit above our own rate limit so the motor never lags the target
yaw_motor.setVelocity(MAX_RATE * 1.5)
pitch_motor.setVelocity(MAX_RATE * 1.5)


def limits(motor):
    lo, hi = motor.getMinPosition(), motor.getMaxPosition()
    if lo == hi:                      # unlimited joint
        return -math.pi, math.pi
    return lo, hi


yaw_lo, yaw_hi = limits(yaw_motor)
pitch_lo, pitch_hi = limits(pitch_motor)
yaw_target = 0.0
pitch_target = 0.0
yaw_rate = 0.0       # smoothed rates (rad/s)
pitch_rate = 0.0
search_dir = 1.0


def clamp(v, lo, hi):
    return max(lo, min(hi, v))


# Own position (the gimbal base) for supervisor-mode aiming
self_node = robot.getSelf()
self_pos = self_node.getField("translation")

# Pedestrian arm (first pedestrian only; the second is DEF PEDESTRIAN_2)
pedestrian = robot.getFromDef("PEDESTRIAN")
right_arm = right_lower_arm = None
if pedestrian is not None:
    right_arm = pedestrian.getField("rightArmAngle")
    right_lower_arm = pedestrian.getField("rightLowerArmAngle")
else:
    print("Warning: DEF 'PEDESTRIAN' not found.")
hand_raised = False
character_skin = None
skin_translation = None
skin_rotation = None
skin_spawned = False

# ---------------- Start your pipeline ----------------
gs = Global()
gs.running = True
process_thread = start_process_thread(gs)
output_thread = start_output_thread(gs)

frame_number = 0
step_count = 0
latest_rgb = None
last_seen_time = robot.getTime()
last_debug = 0.0

print(f"Tracking mode: {TRACK_MODE}")

# ---------------- Main loop ----------------
while robot.step(timestep) != -1:
    step_count += 1
    now = robot.getTime()

    # Spawn the skin once, after the simulation has started
    if not skin_spawned:
        character_skin = spawn_character_skin()
        skin_spawned = True
        if character_skin is not None:
            skin_translation = character_skin.getField("translation")
            skin_rotation = character_skin.getField("rotation")

    # Walk the skin back and forth
    if skin_translation is not None:
        y_off, heading = walk_pose(now)
        skin_translation.setSFVec3f(
            [SKIN_START[0], SKIN_START[1] + y_off, SKIN_START[2]]
        )
        skin_rotation.setSFRotation([0, 0, 1, heading + FACING_OFFSET])

    # Raise pedestrian hand once
    if not hand_raised and now >= RAISE_HAND_AT and right_arm is not None:
        right_arm.setSFFloat(-2.8)
        right_lower_arm.setSFFloat(-0.3)
        hand_raised = True

    # 1) Webots camera -> input queue (BGRA -> RGB)
    raw = camera.getImage()
    if raw and step_count % FRAME_EVERY == 0 and gs.input_stack.qsize() < 2:
        bgra = np.frombuffer(raw, np.uint8).reshape((H, W, 4))
        rgb = cv2.cvtColor(bgra, cv2.COLOR_BGRA2RGB)
        gs.input_stack.put((frame_number, rgb))
        frame_number += 1

    # 2) Show the newest annotated frame from processing.py
    try:
        while True:
            _, latest_rgb = gs.output_stack.get_nowait()
            gs.output_stack.task_done()
    except queue.Empty:
        pass

    if latest_rgb is not None:
        cv2.imshow("Gimbal Tracking", cv2.cvtColor(latest_rgb, cv2.COLOR_RGB2BGR))
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    # 3) Gimbal control
    want_yaw_rate = 0.0
    want_pitch_rate = 0.0

    if TRACK_MODE == "supervisor":
        if skin_translation is not None:
            tx, ty, tz = skin_translation.getSFVec3f()
            bx, by, bz = self_pos.getSFVec3f()
            dx, dy = tx - bx, ty - by
            horiz = math.hypot(dx, dy)

            desired_yaw = YAW_OFFSET + YAW_SIGN * math.atan2(dy, dx)
            yaw_err = wrap_angle(desired_yaw - yaw_target)
            want_yaw_rate = KP_YAW * yaw_err

            if TILT_ENABLED:
                desired_pitch = PITCH_SIGN * math.atan2((tz + AIM_HEIGHT) - bz, horiz)
                desired_pitch = clamp(desired_pitch, pitch_lo, pitch_hi)
                want_pitch_rate = KP_PITCH * (desired_pitch - pitch_target)

    else:  # "vision"
        center = gs.center_point
        if center is not None:
            last_seen_time = now
            err_x = (center[0] - W / 2) / (W / 2)   # + = target right of centre
            err_y = (center[1] - H / 2) / (H / 2)   # + = target below centre
            want_yaw_rate = 0.0 if abs(err_x) < DEADBAND else -YAW_SIGN * KP_YAW * err_x
            want_pitch_rate = 0.0 if abs(err_y) < DEADBAND else -PITCH_SIGN * KP_PITCH * err_y
        elif SEARCH_ENABLED and (now - last_seen_time) > LOST_TIMEOUT:
            # Lost the target for a while: slowly pan back and forth to find them
            if yaw_target >= yaw_hi - 0.05:
                search_dir = -1.0
            elif yaw_target <= yaw_lo + 0.05:
                search_dir = 1.0
            want_yaw_rate = search_dir * SEARCH_SPEED

        # Debug once per second so you can see whether the detector locks on
        if now - last_debug > 1.0:
            print(f"[t={now:5.1f}] center_point = {center}")
            last_debug = now

    want_yaw_rate = clamp(want_yaw_rate, -MAX_RATE, MAX_RATE)
    want_pitch_rate = clamp(want_pitch_rate, -MAX_RATE, MAX_RATE)

    # Low-pass filter the rate so starts/stops are gentle, like a real gimbal
    yaw_rate += SMOOTHING * (want_yaw_rate - yaw_rate)
    pitch_rate += SMOOTHING * (want_pitch_rate - pitch_rate)

    yaw_target = clamp(yaw_target + yaw_rate * dt, yaw_lo, yaw_hi)
    pitch_target = clamp(pitch_target + pitch_rate * dt, pitch_lo, pitch_hi)

    yaw_motor.setPosition(yaw_target)
    pitch_motor.setPosition(pitch_target)

# ---------------- Shutdown ----------------
gs.running = False
process_thread.join()
output_thread.join()
cv2.destroyAllWindows()