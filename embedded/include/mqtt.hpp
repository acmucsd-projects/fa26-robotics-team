#pragma once

// Where the laptop says the target is, relative to the frame center.
// -1..1 on each axis. +x = target right of center, +y = below center.
struct Offset {
  float x;
  float y;
};

// True if a NEW offset arrived since the last call. Never blocks for long.
// Also starts Wi-Fi and the MQTT connection on the first call and keeps them
// alive (reconnects at most every few seconds), so main.cpp just calls this
// every loop.
bool pollOffset(Offset &out);