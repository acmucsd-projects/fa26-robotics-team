# embedded/

ESP32-S3 firmware for the pan/tilt camera mount. The ESP32 drives two STS3215
servos over UART (pan = ID 1, tilt = ID 2) and reads a BMI160 IMU over I2C.

> **Status:** the behavior below is what the code is written to do. It was
> checked by reading the code and by syntax-checking it against stub libraries.
> It has not been run on hardware yet. If a real run disagrees with this file,
> fix whichever one is wrong and update this file.

## Running an env

Run these from this folder (the one containing `platformio.ini`):

```
pio run -e sim -t upload -t monitor             # servo test, fake offsets
pio run -e sim_imu -t upload -t monitor         # servo test + IMU printout
pio run -e calibrate_imu -t upload -t monitor   # accel calibration
pio run -t upload -t monitor                    # no -e: the default env (sim)
```

Only one env can be on the board at a time; uploading another replaces it.
In VS Code, click the env name in the bottom toolbar to switch.

| Env | Purpose | Builds |
|---|---|---|
| `sim` | Test the servo path with made-up offsets, no IMU | `main`, `motor`, `fake_offset` |
| `sim_imu` | Same as `sim`, plus read and print the IMU | the above + `IMU` |
| `calibrate_imu` | One-time accelerometer calibration per board | `CalibrateIMU`, `IMU` |
| `product` (not written yet) | Real MQTT offsets from the laptop | `main`, `motor`, `mqtt_offset` |

Serial monitor is 115200 baud.

---

## `sim`

**Goal:** check the servos, limits and "offsets stopped" behavior without
WiFi or the IMU.

**Boot (several seconds, prints as it goes):**
1. `Scanning bus`, then one `Found servo ID:N` line per servo that answers.
   Expect exactly `Found servo ID:1` and `Found servo ID:2`.
2. Servo 2 (tilt) is driven into one end stop, printing `servo2 <pos> <current>`
   as the position steps down. It stops when the current reaches 7 or more.
3. It returns to its start, then is driven into the other end stop, printing
   `servo2 LOOP 2 <current>`.
4. It moves to the midpoint and prints two numbers: `MIN1` then `MAX1`.
   `MIN1` should be smaller than `MAX1` and the gap should look like the real
   travel of the joint. The code does not check this, so look yourself.

Servo 2 pushing into its end stops is expected. Servo 1 (pan) is not
calibrated at boot.

**Running:**
- About 30 fake offsets per second. Pan follows a slow sweep (about 8 s per
  cycle), tilt a slower one (about 13 s per cycle).
- Each offset nudges the target rather than setting it, so motion is smooth
  and may drift off center over time instead of swinging evenly around it.
- Every 10 s the offsets stop for 2 s. The servos should hold still, then
  resume. Nothing is printed for this.
- The first offset sends tilt to a target based on 2048, so servo 2 may jump
  from the calibrated midpoint at that moment.
- No serial output after the boot lines.

**Good:** both servos found, sensible `MIN1`/`MAX1`, smooth motion, a 2 s pause
every 10 s, nothing hits a mechanical stop during the sweep.
**Bad:** a servo missing from the scan (check ID and daisy chain), jerky motion,
a stall against a stop. If a servo keeps pushing into a stop, cut the 12 V.

---

## `sim_imu`

**Goal:** everything in `sim`, plus compare what was commanded to what the
IMU measures. It only observes. The IMU does not move the servos.

**Boot, before the servos move:**
- `Zeroing gyro, keep the head still...` (about 2 s). Keep the head still.
- Then the same boot as `sim`.
- If the IMU does not answer: `BMI160 init failed, running without IMU`, and
  the rest runs like `sim`.

**Running:** the servo behavior is the same as `sim`, and a line prints
5 times a second:

```
cmd tilt   1.2 pan  -3.4 | imu tilt   0.9 pan  -0.2 | a  0.10 -0.05  9.78 | g   0.01  -0.03   0.02 | fails 0
```

| Field | Meaning |
|---|---|
| `cmd tilt`, `cmd pan` | Commanded angle in degrees from 2048 (center) |
| `imu tilt` | Filtered tilt in degrees (gyro + accel) |
| `imu pan` | Gyro-only pan in degrees. It drifts slowly. |
| `a x y z` | Accel in m/s² |
| `g x y z` | Gyro in deg/s |
| `fails` | Count of failed IMU reads |

**What to expect:**
- Still head: the accel magnitude is about 9.8 m/s², and the gyro is near 0
  (within a fraction of a deg/s).
- Uncalibrated accel can be off by roughly 0.2 to 0.4 m/s². Run
  `calibrate_imu` first.
- `fails` should stay 0. If it climbs, check the wiring.
- While a servo moves, some gyro axis should show a rate and some accel
  component should change. Which axes depend on how the board is mounted.
  This env is how you find out.
- `imu tilt` is not expected to match `cmd tilt` yet. The axis mapping and
  sign are unconfirmed.
- If you see the init-failed message, the `a` and `g` values print as zeros
  and `fails` stays 0.

**Check first:** the I2C pins. `main.cpp` and `CalibrateIMU.cpp` use SDA 6 /
SCL 7. An earlier version of `main.cpp` used 8 / 9.

---

## `calibrate_imu`

**Goal:** measure the accelerometer's offset and scale for one board. No
servos. Run it once per board, and again if the IMU mount changes.

**Expected run:**
1. If the chip does not answer: `BMI160 init failed`, repeating every second.
2. Otherwise a prompt repeats every 2 s until you type something in the serial
   monitor and press Enter:
   `point +X up, then press enter`
3. After Enter: `  measuring, keep it still...` (about 4 s), then `  done`.
4. This repeats for six poses in this order: +X up, +X down, +Y up, +Y down,
   +Z up, +Z down. The board must be still during each measurement.
5. It prints once:

```
calibration done, paste this into main.cpp:
Imu imu(6, 7, 0x68, {ox, oy, oz}, {sx, sy, sz});
```

The numbers are offsets in raw counts (expect up to a few hundred) and scale
factors (expect close to 1.00). Copy the line into `main.cpp` in place of
`Imu imu(IMU_SDA, IMU_SCL);`.

**Redo:** the result prints once and the program then idles. Press reset on
the board to run it again.

**Bad runs:** a scale far from 1, `inf`, or `nan` means a pose was wrong
(same side twice, Enter pressed before rotating, axis mislabeled). Redo all
six poses. The code does not check this.

---

## Known gaps

- Servo 2 end-stop search has no timeout. A failed reading can end it early
  or loop forever.
- Servo 1 is not calibrated. `MIN1`/`MAX1` are not used by `main.cpp` yet.
- `MovePos` only limits targets to 0–4095, not to the calibrated range.
- A lost servo command is not reported.
- `product` needs `mqtt_offset.cpp` with the same `pollOffset()` function as
  `fake_offset.cpp`.
- The IMU does not drive the servos in any env yet.
