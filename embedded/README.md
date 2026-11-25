# Embedded

## Build

```bash
pio run
```

## Flash and open the serial monitor

```bash
pio run -t upload -t monitor
```

## Calibrate the accelerometer

Separate firmware, run once per board. Follow the prompts, then paste the printed `Imu imu(...)` line into `src/main.cpp`.

```bash
pio run -e calibrate -t upload -t monitor
```

## Compilation database

```bash
pio run -t compiledb
```
