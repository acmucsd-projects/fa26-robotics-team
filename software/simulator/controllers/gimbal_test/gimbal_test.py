import cv2

from controller import Robot
import numpy as np

# Initialize the Robot instance
robot = Robot()

# Get simulation time step (milliseconds)
timestep = int(robot.getBasicTimeStep())

# 1. Enable Camera
camera = robot.getDevice("gimbal_camera")
camera.enable(timestep)

# 2. Get Motor Devices
yaw_motor = robot.getDevice("yaw_motor")
pitch_motor = robot.getDevice("pitch_motor")

# Set motor speed (radians per second)
yaw_motor.setVelocity(1.0)
pitch_motor.setVelocity(1.0)

# Helper function to pause for simulation time in seconds
def delay(seconds):
    start_time = robot.getTime()
    while robot.step(timestep) != -1:
        if robot.getTime() - start_time >= seconds:
            break

# MODEL_PATH = os.path.abspath("software/simulator/controllers/gimbal_test/Arnavrfdetr_2.pth")
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
MODEL_PATH = os.path.join(BASE_DIR, "Arnavrfdetr_2.pth")


# Load checkpoint using absolute path
model = RFDETR.from_checkpoint(MODEL_PATH)

def process_frame_and_infer():
    raw_bytes = camera.getImage()
    if raw_bytes is None:
        return None

    height = camera.getHeight()
    width = camera.getWidth()
    frame_bgra = np.frombuffer(raw_bytes, np.uint8).reshape((height, width, 4))

    frame_rgb = cv2.cvtColor(frame_bgra, cv2.COLOR_BGRA2RGB)
    frame_bgr = cv2.cvtColor(frame_bgra, cv2.COLOR_BGRA2BGR)

    detections = model.predict(frame_rgb, threshold=0.2)

print("--- Starting Gimbal Diagnostic Test ---")

# Ensure camera initializes
delay(1.0)

while robot.step(timestep) != -1:
    # 1. Move Left
    print("Action: Pan Left")
    yaw_motor.setPosition(0.5)   # ~28 degrees Left
    pitch_motor.setPosition(0.0)
    delay(2.0)

    # 2. Move Right
    print("Action: Pan Right")
    yaw_motor.setPosition(-0.5)  # ~28 degrees Right
    delay(2.0)

    # Return Yaw to Center
    yaw_motor.setPosition(0.0)
    delay(1.0)

    # 3. Tilt Up
    print("Action: Tilt Up")
    pitch_motor.setPosition(0.3)  # ~17 degrees Up
    delay(2.0)

    # 4. Tilt Down
    print("Action: Tilt Down")
    pitch_motor.setPosition(-0.3) # ~17 degrees Down
    delay(2.0)

    # Return Pitch to Center
    print("Action: Returning to Center")
    pitch_motor.setPosition(0.0)
    delay(2.0)

    print("--- Cycle Complete. Repeating... ---")


# import cv2
# import queue

# from gimbal_global_state import Global
# from gimbal_input import start_input_thread
# from gimbal_processing import start_process_thread

# robot = Robot()
# timestep = int(robot.getBasicTimeStep())


# camera = robot.getDevice("gimbal_camera")
# camera.enable(timestep)

# width = camera.getWidth()
# height = camera.getHeight()


# yaw_motor = robot.getDevice("yaw_motor")
# pitch_motor = robot.getDevice("pitch_motor")

# yaw_motor.setVelocity(1.0)
# pitch_motor.setVelocity(1.0)


# global_state = Global()
# global_state.running = True

# process_thread = start_process_thread(global_state)
# input_thread = start_input_thread(global_state)

# try:
    # while (
        # input_thread.is_alive()
        # or process_thread.is_alive()
        # or not global_state.output_queue.empty()
    # ):
        # try:
            # _, frame_rgb = global_state.output_queue.get(timeout=0.01)
            # frame_bgr = cv2.cvtColor(frame_rgb, cv2.COLOR_RGB2BGR)
            # cv2.imshow("RF-DETR Live Feed", frame_bgr)
       
        # except queue.Empty:
            # pass

        # if cv2.waitKey(1) & 0xFF == ord("q"):
            # global_state.running = False
# finally:
    # global_state.running = False
    # input_thread.join()
    # process_thread.join()
    # cv2.destroyAllWindows()