// setup()
    // Start Serial, then imu.begin() (I2C and sensor registers).
    // ensureAccelCalibration: if the flash has values, apply them. If not, run the 6-position routine over Serial, then save and apply.
    // imu.calibrateGyro(): average the gyro while still, and store it as the zero point. This is not saved to flash because gyro bias changes with temperature and power cycles.
    // motorsBegin(): start the servo UART, scan the bus, then for each servo load limits from flash or find them by driving into the end stops.

// loop() every 10ms
    // Read the IMU. The accel is already corrected by the calibration and the gyro by its bias.
    // Compute the real dt since the last reading.
    // Feed the readings to the filters, which return a drift-free angle for each axis.
    // Convert degrees to servo steps (4096 per 360°) and add the servo’s calibrated center.
    // MovePos clamps that target to the servo’s measured limits and sends it.
