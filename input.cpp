#include <Windows.h>

#include <atomic>
#include <iostream>
#include <math.h>

#include "input.h"

// Mouse sensitivity parameters
const static float windows_sens_multipliers[] = { 0.0625f, 0.0125f, 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f };
const static int win_sens_idx = 6;
const static int dpi = 800;
const static float ingame_sens = 2.0;
const static float m_yaw = 0.022f;
const static float m_pitch = 0.022f;
const static float fov = 80 * 1.125; // cg_fov * cg_fovscale
const static float res_x = 1920;
const static float res_y = 1080;
// Probably not the correct formula, but the most important thing is that it's constant
const static float sens_x = (dpi * windows_sens_multipliers[win_sens_idx - 1] * ingame_sens * m_yaw * fov / res_x);
const static float sens_y = (dpi * windows_sens_multipliers[win_sens_idx - 1] * ingame_sens * m_pitch * fov / res_y);

void movePixels(int x, int y)
{
	double fx = x * ((double)0xFFFF / res_x);
	double fy = y * ((double)0xFFFF / res_y);

	INPUT input = {0};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;// | MOUSEEVENTF_ABSOLUTE;
	input.mi.dx = (LONG)fx;
	input.mi.dy = (LONG)fy;

	SendInput(1, &input, sizeof(INPUT));
}

void performMouseMovement(int duration, int speed, std::atomic<bool> *ptr_stopSimulation)
{
	const int sleep_time = (11 - speed);
	while (duration > 0 && !(*ptr_stopSimulation))
	{
		movePixels(-1, 0);

		Sleep(sleep_time);

		// Safe because duration is signed
		duration -= sleep_time;
	}
}

/*
void addViewAngle(float yaw, float pitch)
{
	int dx = (yaw / PI_CONST);
	int dy = (pitch / PI_CONST);

	while (dx || dy)
	{
		movePixels((dx) ? 1 : 0, (dy) ? 1 : 0);

		if (dx)
			dx--;

		if (dy)
			dy--;

		Sleep(1);
	}
}
*/

void keyDown(WORD key)
{
	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = key;
	input.ki.dwFlags = 0;

	SendInput(1, &input, sizeof(INPUT));
}

void keyUp(WORD key)
{
	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = key;
	input.ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(1, &input, sizeof(INPUT));
}

void pressKey(WORD key)
{
	keyDown(key);
	keyUp(key);
}