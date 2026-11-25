#pragma once

// Where the laptop says the target is, relative to the frame center.
// -1..1 on each axis. +x = target right of center, +y = below center.
struct Offset {
  float x;
  float y;
};

// True if a NEW offset arrived since the last call. Never blocks.
// fake_offset.cpp now; mqtt_offset.cpp later, same signature.
bool pollOffset(Offset &out);