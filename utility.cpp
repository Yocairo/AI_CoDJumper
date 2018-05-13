#include <Windows.h>

#include <iostream>
#include <random>

#include "ai.h"

using namespace std;

static mt19937 rng;

void initRandom(void)
{
	rng.seed(random_device()());
}

int randomIntRange(int min, int max)
{
	uniform_int_distribution<mt19937::result_type> dist6(min, max);

	return dist6(rng);
}

float randomFloat(void)
{
	return ((float)randomIntRange(0, (65535 - 1)) / 65535);
}

float ReadFloat(HANDLE hproc, long address)
{
	float value;
	ReadProcessMemory(hproc, (void*)address, &value, sizeof(value), 0);
	return value;
}

int ReadInt(HANDLE hproc, long address)
{
	int value;
	ReadProcessMemory(hproc, (void*)address, &value, sizeof(value), 0);
	return value;
}

void bubbleSort(ai_inputs *inputs, ai_outputs *outputs)
{
	cout << "Before sort: ";

	for (int i = 0; i < POOL_SIZE; i++)
		cout << outputs[i].score << " ";

	cout << "\n";

	for (int i = 0; i < (POOL_SIZE - 1); i++)
	{
		for (int j = 0; j < (POOL_SIZE - i - 1); j++)
		{
			if (outputs[j].score < outputs[j + 1].score)
			{
				// Swap the inputs
				ai_inputs tmp_input = { 0 };
				memcpy(&tmp_input, &inputs[j], sizeof(ai_inputs));
				memcpy(&inputs[j], &inputs[j + 1], sizeof(ai_inputs));
				memcpy(&inputs[j + 1], &tmp_input, sizeof(ai_inputs));

				// Swap the outputs
				ai_outputs tmp_output = { 0 };
				memcpy(&tmp_output, &outputs[j], sizeof(ai_outputs));
				memcpy(&outputs[j], &outputs[j + 1], sizeof(ai_outputs));
				memcpy(&outputs[j + 1], &tmp_output, sizeof(ai_outputs));
			}
		}
	}

	cout << "After sort: ";

	for (int i = 0; i < POOL_SIZE; i++)
		cout << outputs[i].score << " ";

	cout << "\n\n";
}

void removeFromArray(ai_inputs *inputs, int index, int size)
{
	for (int i = index; i < (size - 1); i++)
		inputs[i] = inputs[i + 1];

	memset(&inputs[size - 1], 0, sizeof(ai_inputs));
}
