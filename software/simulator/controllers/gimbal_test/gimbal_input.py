import cv2
import threading
import numpy as np
from gimbal_global_state import Global, put_latest
from controller import Robot

def threadTarget(global_state):
    robot = Robot()
    timestep = int(robot.getBasicTimeStep())

    # Camera
    camera = robot.getDevice("gimbal_camera")
    camera.enable(timestep)

    width = camera.getWidth()
    height = camera.getHeight()

    # Motors
    yaw_motor = robot.getDevice("yaw_motor")
    pitch_motor = robot.getDevice("pitch_motor")

    yaw_motor.setVelocity(1.0)
    pitch_motor.setVelocity(1.0)
    
    def get_camera_frame():
        image = camera.getImage()

        if image is None:
            return

        # Convert Webots BGRA buffer to NumPy image
        frame_bgra = np.frombuffer(
            image, dtype=np.uint8
        ).reshape((height, width, 4))

        # RF-DETR pipeline expects RGB
        frame_rgb = cv2.cvtColor(
            frame_bgra,
            cv2.COLOR_BGRA2RGB
        )

        
        put_latest(global_state.input_queue, (frame_number, frame_rgb))



    def delay(seconds):
        start_time = robot.getTime()

        while robot.step(timestep) != -1:
            if robot.getTime() - start_time >= seconds:
                return True

        return False


    print("--- Starting Gimbal Diagnostic Test ---")

    if delay(1.0):
        while robot.step(timestep) != -1 and global_state.running:
            if frame_number % 2 == 0:
                get_camera_frame()

                print("Action: Pan Left")
                yaw_motor.setPosition(0.5)
                pitch_motor.setPosition(0.0)
                if not delay(2.0):
                    break

                print("Action: Pan Right")
                yaw_motor.setPosition(-0.5)
                if not delay(2.0):
                    break

                yaw_motor.setPosition(0.0)
                if not delay(1.0):
                    break

                print("Action: Tilt Up")
                pitch_motor.setPosition(0.3)
                if not delay(2.0):
                    break

                print("Action: Tilt Down")
                pitch_motor.setPosition(-0.3)
                if not delay(2.0):
                    break

                print("Action: Returning to Center")
                pitch_motor.setPosition(0.0)
                if not delay(2.0):
                    break

            frame_number += 1
            print("--- Cycle Complete ---")


def start_input_thread(global_state):
    thread = threading.Thread(
        target=threadTarget,
        args=(global_state,),
        daemon=True,
    )
    thread.start()
    return thread