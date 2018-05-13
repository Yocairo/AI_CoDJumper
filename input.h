#pragma once

#include <atomic>

#define PI_CONST	3.14159265358979323846

void movePixels(int x, int y);
void performMouseMovement(int duration, int speed, std::atomic<bool> *ptr_stopSimulation);

void keyDown(WORD key);
void keyUp(WORD key);
void pressKey(WORD key);
