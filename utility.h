#pragma once

#include <Windows.h>

#include "ai.h"

#define max(a, b)	(((a) > (b)) ? (a) : (b))
#define min(a, b)	(((a) < (b)) ? (a) : (b))
#define abs(a)		((a < 0) ? (a * -1) : (a))

void initRandom(void);
int randomIntRange(int min, int max);
float randomFloat(void);
float ReadFloat(HANDLE hproc, long address);
int ReadInt(HANDLE hproc, long address);
void bubbleSort(ai_inputs *inputs, ai_outputs *outputs);
void removeFromArray(ai_inputs *inputs, int index, int size);
