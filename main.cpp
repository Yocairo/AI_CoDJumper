#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <atomic>

#include <Windows.h>
#include <Tlhelp32.h>
#include <math.h>

#include <iomanip>

#include "input.h"
#include "timer.h"
#include "utility.h"
#include "ai.h"

// CoD4 configuration
#define MAXIMIZE_DIRECTION		-1	// 1 = +, -1 = -
#define MAX_BACKWARDS_DISTANCE	-200
#define RESET_DELAY	300			// ms
#define KEY_LOAD	0x45		// 'E'
#define KEY_JUMP	VK_SPACE
#define KEY_SPRINT	VK_SHIFT
#define KEY_FORWARD	0x57		// 'W'

// Program settings
#define PROC_NAME		"iw3mp.exe"
#define DETECT_TIME		250	// ms

// Memory offsets
#define ADDR_BASE		0x00794474 // Memory address for structure
#define ADDR_ORIGINX	(ADDR_BASE + 28)
#define ADDR_ORIGINY	(ADDR_BASE + 32)
#define ADDR_ORIGINZ	(ADDR_BASE + 36)
#define ADDR_YSPEED		(ADDR_BASE + 44)
#define ADDR_INAIR		(ADDR_BASE + 112) // inAir == 1022 means onGround, else false
#define ON_GROUND		1022

using namespace std;

static void mainFunc(HANDLE hproc);

// First, wait until we find iw3mp.exe
int main(void)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	cout << "Waiting for " << PROC_NAME << "..\n\n";

	bool detected = false;
	while (!detected)
	{
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, (DWORD)NULL);

		if (Process32First(snapshot, &entry) == TRUE)
		{
			while (Process32Next(snapshot, &entry) == TRUE)
			{
				if (_stricmp(entry.szExeFile, PROC_NAME) == 0)
				{
					detected = true;

					HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);

					mainFunc(hProcess); // Call the actual main function

					CloseHandle(hProcess);
				}
			}
		}

		CloseHandle(snapshot);

		Sleep(DETECT_TIME);
	}
	return 0;
}

static HANDLE processHandle = NULL;

static atomic<bool> hasJumped = false;
atomic<bool> stopSimulation = false;

static ai_outputs monitorIndividual()
{
	ai_outputs results = { 0 };

	float origin[3] = { 0 };
	float velocityY = 0, totalVelocityY = 0;

	int velocitySamples = 0;

	// Save the starting position
	float start_ypos = ReadFloat(processHandle, ADDR_ORIGINY);
	float start_zpos = ReadFloat(processHandle, ADDR_ORIGINZ);

	//cout << "Found starting position: (x," << fixed << setprecision(1) << start_ypos << "," << fixed << setprecision(1) << start_zpos << ")\n\n";

	while (!stopSimulation)
	{
		// Don't need this for Y distance gaps
		//origin[0] = ReadFloat(processHandle, ADDR_ORIGINX);

		origin[1] = ReadFloat(processHandle, ADDR_ORIGINY);
		origin[2] = ReadFloat(processHandle, ADDR_ORIGINZ);

		// If individual is walking the wrong way - end simulation early
		if (((origin[1] - start_ypos) * MAXIMIZE_DIRECTION) < MAX_BACKWARDS_DISTANCE)
			stopSimulation = true; // This will cause the main loop to end the simulation early

		// If we have a new maximum distance at positive height, update the struct
		if ((origin[2] >= start_zpos) && (((origin[1] - start_ypos) * MAXIMIZE_DIRECTION) > results.maxDistanceAtPosHeight))
			results.maxDistanceAtPosHeight = ((origin[1] - start_ypos) * MAXIMIZE_DIRECTION);

		velocityY = ReadFloat(processHandle, ADDR_YSPEED);

		// Only record average Y velocity after the jump
		if (hasJumped && (origin[2] >= start_zpos))
		{
			totalVelocityY += velocityY;
			velocitySamples++;
		} // Record maximum Y velocity before the jump
		else if ((velocityY * MAXIMIZE_DIRECTION) > results.maxVelocityY)
			results.maxVelocityY = (velocityY * MAXIMIZE_DIRECTION);

		Sleep(25); // Sampling rate of (1000 / (2 * sv_fps==20))
	}

	results.avgVelocityY = (totalVelocityY / velocitySamples) * MAXIMIZE_DIRECTION;
	calculateScore(&results);

	// Between 0 and 1 (should we do this?)
	//results.score = max(results.score, 0);
	//results.score = min(results.score, 1);

	return results;
}

// This function is called when the iw3mp.exe process is detected
static void mainFunc(HANDLE hproc)
{
	processHandle = hproc;

	cout << "Found CoD4 process\n";
	// Check if we have the necessary timer resolution
	if (!initTimer())
	{
		cout << "Minimum timer resolution not sufficient\n\n";
		return;
	}
	else cout << "Initialized timer\n\n";

	// Initialize random number generator
	initRandom();

	// Program variables
	DWORD exitcode = 259;
	bool playback = true;

	// Global AI variables
	ai_inputs individualInput[POOL_SIZE] = { 0 }; // All input parameters of each individual in a generation
	ai_outputs individualOutput[POOL_SIZE] = { 0 }; // All output values and the score of each individual in a generation
	ai_individual bestIndividual = { 0 };
	int curIndividual = (playback) ? 0 : -1; // Will be incremented to 0 at start

	// Individual variables: output
	float o_maxDistance = 0;		// The maximum distance the individual reached
	float o_heightAtDistance = 0;	// The height at the time of maximum distance
	float o_maxVelocity = 0;		// The maximum velocity the individual reached
	// Individual variables: control
	bool jumpDone = false;
	bool fwdDone = false;
	bool mouseDone = false;

	// Too lazy to make a dynamic playback
	if (playback)
	{
		/* 307 gap
		individualInput[0].jumpDelay = 895; // 888
		individualInput[0].mouseDelay = 500; // 506
		individualInput[0].mouseDuration = 670; // 602
		individualInput[0].mouseSpeed = 3; // 3
		*/
	}
	else
	{
		// Create first pool of individuals (generation)
		generateFirstGeneration(individualInput, POOL_SIZE);
	}

	// Wait until the simulation should be started
	cout << "Press F1 to start training AI\n\n";
	while (!(GetAsyncKeyState(VK_F1) & 0x01))
		Sleep(100);

	PlaySound("NonExistingSoundSoItWillPlayAnErrorSound", NULL, SND_ASYNC);

	while (1)
	{
		if (!playback)
		{
			// Check if the user wants to abort
			if (GetAsyncKeyState(VK_F1) & 0x01)
				break;
		}
		else
		{
			while (!(GetAsyncKeyState(VK_F1) & 0x01))
				Sleep(100);
		}

		// Check if CoD4 exited
		GetExitCodeProcess(hproc, &exitcode);
		if (exitcode != 259)
			break;

		// Check if the maximum population size has been reached
		if (!playback)
		{
			if (curIndividual++ == (POOL_SIZE - 1))
			{
				// TODO: Do evolutionary computing stuff and create new individual(s)
				generateNewIndividuals(individualInput, individualOutput);

				curIndividual = 0;
			}
		}

		cout << "================================\n";
		cout << "Testing new individual: (" << curIndividual << ")\n";
		cout << "--------------------------------\n";
		cout << "  jumpDelay\t" << individualInput[curIndividual].jumpDelay << "\n";
		cout << "  mouseDelay\t" << individualInput[curIndividual].mouseDelay << "\n";
		cout << "  mouseDuration\t" << individualInput[curIndividual].mouseDuration << "\n";
		cout << "  mouseSpeed\t" << individualInput[curIndividual].mouseSpeed << "\n";

		// Individual can now be tested
		future<ai_outputs> individualScore = async(&monitorIndividual);
		future<void> waitForMouse;
		chrono::milliseconds start = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch());;
		int64_t timePassed = 0;
		while (timePassed < SIM_TIME && !stopSimulation) // Check if we got an 'external' trigger to stop the simulation
		{
			// Check if any mouse actions should be performed at this time
			if (!mouseDone && (timePassed >= individualInput[curIndividual].mouseDelay))
			{
				waitForMouse = async(&performMouseMovement, individualInput[curIndividual].mouseDuration, individualInput[curIndividual].mouseSpeed, &stopSimulation);
				mouseDone = true;
			}

			// Check if the forward and sprint buttons should be held down from this time on
			if (!fwdDone)
			{
				keyDown(KEY_FORWARD);
				keyDown(KEY_SPRINT);
				fwdDone = true;
			}

			// Check if the jump key should be pressed at this time
			if (!hasJumped && (timePassed >= individualInput[curIndividual].jumpDelay))
			{
				pressKey(KEY_JUMP);
				hasJumped = true;
			}

			// Calculate time passed
			timePassed = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch() - start).count();
		}

		// Reset everything
		stopSimulation = true;
		
		keyUp(KEY_FORWARD);
		keyUp(KEY_SPRINT);

		// Different naming for hasJumped because it's an atomic variable used in two tasks
		hasJumped = false;
		fwdDone = false;
		mouseDone = false;

		// Obtain results
		if(mouseDone) // waitForMouse is uninitialized if simulation stops before calling async
			waitForMouse.get(); // Wait for the mouse function to return

		ai_outputs results = individualScore.get();
		memcpy(&individualOutput[curIndividual], &results, sizeof(ai_outputs));

		stopSimulation = false;

		// Display results
		cout << "Results:\n";
		cout << "  maxDistAtPosHeight\t" << fixed << setprecision(2) << individualOutput[curIndividual].maxDistanceAtPosHeight << "\n";
		cout << "  maxVelocity\t\t" << fixed << setprecision(2) << individualOutput[curIndividual].maxVelocityY << "\n";
		cout << "  avgVelocity\t\t" << fixed << setprecision(2) << individualOutput[curIndividual].avgVelocityY << "\n";
		cout << "  score\t\t\t" << individualOutput[curIndividual].score << "\n";

		// Save (and export) best individual
		if (individualOutput[curIndividual].score > bestIndividual.output.score)
		{
			memcpy(&bestIndividual.input, &individualInput[curIndividual], sizeof(ai_inputs));
			memcpy(&bestIndividual.output, &individualOutput[curIndividual], sizeof(ai_outputs));

			OutputDebugStringA("\nFound new best individual:\n");

			char individualStr[256] = { 0 };
			snprintf(individualStr, 256, "%d %d %d %d -> %.1f %.1f %.1f %.3f\n", bestIndividual.input.jumpDelay, 
				bestIndividual.input.mouseDelay, bestIndividual.input.mouseDuration, bestIndividual.input.mouseSpeed,
				bestIndividual.output.maxDistanceAtPosHeight, bestIndividual.output.maxVelocityY, bestIndividual.output.avgVelocityY, bestIndividual.output.score);
			OutputDebugStringA(individualStr);
		}

		// Load position and wait before testing a new individual
		Sleep(RESET_DELAY / 2);
		pressKey(KEY_LOAD);
		Sleep(RESET_DELAY);

		// Make absolutely sure all memory values are updated (I'm looking at you, inAir!)
		//Sleep(1000);
	}

	deinitTimer();

	// Keep console open until F1 is pressed again
	while (!(GetAsyncKeyState(VK_F1) & 0x01))
		Sleep(100);

	// TODO: Do (safe) exporting etc.
}