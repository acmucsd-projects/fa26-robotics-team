from controller import Robot

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
