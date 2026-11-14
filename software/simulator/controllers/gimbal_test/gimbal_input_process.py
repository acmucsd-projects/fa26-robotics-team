import cv2
import threading

from gimbal_global_state import Global
from controller import Robot
import cv2
import numpy as np
import queue



def threadTarget(global_state: Global):
    """
    Detect a hand in the top third of the frame to initialize
    the gimbal target, then stream frames to the processing queue.
    """

    robot = Robot()
    timestep = int(robot.getBasicTimeStep())

    # Camera
    camera = robot.getDevice("gimbal_camera")
    camera.enable(timestep)

    # Motors
    yaw_motor = robot.getDevice("yaw_motor")
    pitch_motor = robot.getDevice("pitch_motor")

    yaw_motor.setVelocity(1.0)
    pitch_motor.setVelocity(1.0)

    width = camera.getWidth()
    height = camera.getHeight()

    def update_camera(frame_number):
        global running

        raw_image = camera.getImage()

        if raw_image is None:
            return

        # Webots camera returns BGRA
        image_bgra = np.frombuffer(
            raw_image, dtype=np.uint8
        ).reshape((height, width, 4))

        image_bgr = cv2.cvtColor(
            image_bgra, cv2.COLOR_BGRA2BGR
        )

        frame_rgb = cv2.cvtColor(
            image_bgr,
            cv2.COLOR_BGR2RGB
        )

        global_state.input_stack.put((frame_number, frame_rgb))

        # cv2.imshow("Webots Camera", image_bgr)

        # if cv2.waitKey(1) & 0xFF == ord("q"):
        #     running = False


    def delay(seconds):
        global running
        frame_number = 0

        start_time = robot.getTime()

        while global_state.running and robot.step(timestep) != -1:
            update_camera(frame_number)
            frame_number += 1

            if robot.getTime() - start_time >= seconds:
                return True

        running = False
        return False

    frame_number = 0

   
    global_state.running = True

    # cap = cv2.VideoCapture(0)
    # cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

    print("--- Starting Gimbal Diagnostic Test ---")


    try:
        while global_state.running:
                    
            frame_number += 1

            # Initialize camera
            delay(1.0)
        
            print("Action: Pan Left")
            yaw_motor.setPosition(0.5)
            pitch_motor.setPosition(0.0)
            if not delay(2.0):
                break

            print("Action: Pan Right")
            yaw_motor.setPosition(-0.5)
            if not delay(2.0):
                break

            print("Action: Center Yaw")
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

            print("Action: Center Pitch")
            pitch_motor.setPosition(0.0)
            if not delay(2.0):
                break

            print("--- Cycle Complete ---")

    finally:
        cv2.destroyAllWindows()
        global_state.running = False



def start_input_thread(global_state: Global):
    thread = threading.Thread(
        target=threadTarget,
        args=(global_state,),
        daemon=True,
    )

    thread.start()
    return thread
