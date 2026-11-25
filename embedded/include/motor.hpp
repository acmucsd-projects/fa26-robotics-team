#pragma once

// Servo driver. main.cpp calls motorsBegin() once from setup().
void motorsBegin();
void MovePos(int id, int position);
