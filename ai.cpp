#include <cstring>
#include <iostream>

#include <iomanip>

#include "ai.h"
#include "utility.h"

using namespace std;

static void applyLimits(ai_inputs *individual);

const static float maxDistance = 1250;	// in ~5 seconds
const static float maxVelocity = 500;	// While gapping it will be far less
const static float maxAvgVelocity = 450;	// While gapping it will be far less

void calculateScore(ai_outputs *results)
{
	results->score = ((results->maxDistanceAtPosHeight / maxDistance) + (results->maxVelocityY / maxVelocity) + (results->avgVelocityY / maxAvgVelocity));
}

static int generation = 0;

// This function performs tournament selection and puts the parents in the supplied buffer
static void tournamentSelection(ai_inputs *inputs, ai_outputs *outputs, ai_inputs *parents)
{
	// Sort the selected inputs based on score
	bubbleSort(inputs, outputs);

	int curIdx = 0;
	// Select TOURNAMENT_SIZE winners
	for (int i = 0; i < TOURNAMENT_SIZE; i++)
	{
		// Scale the random number so a parent will always be selected
		double totalChance = SELECTION_CHANCE;
		for (int j = 1; j < (POOL_SIZE - curIdx); j++)
			totalChance = (totalChance + (SELECTION_CHANCE * pow((1 - SELECTION_CHANCE), j)));

		double rand = (randomFloat() * totalChance);

		cout << "Generated scaled random float: " << fixed << setprecision(4) << rand << "\n";

		// Check which of the remaining participants will get selected
		double chance = SELECTION_CHANCE;
		for (int j = 0; j < (POOL_SIZE - curIdx); j++)
		{
			if (rand <= chance)
			{
				// We found a new parent
				cout << "Selected parent (" << j << "), with score: " << outputs[j].score << "\n";
				memcpy(&parents[curIdx++], &inputs[j], sizeof(ai_inputs));
				removeFromArray(inputs, j, (POOL_SIZE - curIdx));
				break;
			}

			chance = chance + (SELECTION_CHANCE * pow((1 - SELECTION_CHANCE), (j + 1)));
		}
	}
}

static void applyCrossOver(ai_inputs *parents, int amount, ai_inputs *individuals)
{
	// Make sure parents are divisble by 2
	amount = (amount % 2 == 0) ? amount : (amount - 1);

	// We can do 3 children to prevent early convergence, make sure TOURNAMENT_SIZE is 2/3 of POOL_SIZE
	int idx = 0;
	for (int i = 0; i < amount; i += 2) // Use randomIntRange(0, 1) for a 50/50 chance
	{
		// Customization: third child
		for (int j = 0; j < 3; j++)
		{
			individuals[idx].jumpDelay = parents[i + randomIntRange(0, 1)].jumpDelay;
			individuals[idx].mouseDelay = parents[i + randomIntRange(0, 1)].mouseDelay;
			individuals[idx].mouseDuration = parents[i + randomIntRange(0, 1)].mouseDuration;
			individuals[idx].mouseSpeed = parents[i + randomIntRange(0, 1)].mouseSpeed;
			idx++;
		}

		if (idx > POOL_SIZE)
		{
			cout << "PROBLEM: More individuals than pool size?\n";
			break;
		}
	}
}

static void applyMutation(ai_inputs *children, int amount)
{
	for (int i = 0; i < amount; i++)
	{
		// Determine if the mutation should occur
		if (randomFloat() > MUTATION_CHANCE)
			continue; // Don't mutate this individual

		// Check which parameter should be mutated, and decide randomly if negative or positive
		int polarity = (randomIntRange(0, 1) == 1) ? 1 : -1;
		switch (randomIntRange(0, (PARAMETER_COUNT - 1))) {
		case 0: // jumpDelay
			children[i].jumpDelay += (children[i].jumpDelay / MUTATION_DIVISOR * polarity);
			break;
		case 1: // mouseDelay
			children[i].mouseDelay += (children[i].mouseDelay / MUTATION_DIVISOR * polarity);
			break;
		case 2: // mouseDuration
			children[i].mouseDuration += (children[i].mouseDuration / MUTATION_DIVISOR * polarity);
			break;
		case 3: // mouseSpeed
			children[i].mouseSpeed += (children[i].mouseSpeed / MUTATION_DIVISOR * polarity);
			break;
		}

		applyLimits(&children[i]);
	}
}

void generateNewIndividuals(ai_inputs *inputs, ai_outputs *outputs)
{
	// Select new parents with tournament selection
	ai_inputs parents[TOURNAMENT_SIZE] = { 0 };
	tournamentSelection(inputs, outputs, parents);

	// Clear the results
	memset(outputs, 0, sizeof(ai_outputs) * POOL_SIZE);

	// Create the new individuals
	ai_inputs children[POOL_SIZE] = { 0 };
	applyCrossOver(parents, TOURNAMENT_SIZE, children);
	applyMutation(children, POOL_SIZE);

	memcpy(inputs, children, sizeof(ai_inputs) * POOL_SIZE);

	generation++;
}

// Individual variables' maximum values
const static int jumpDelayRangeMin = 100;
const static int jumpDelayRangeMax = 1500;

const static int mouseHorDelayRangeMin = 0;
const static int mouseHorDelayRangeMax = 500;
const static int mouseDurationRangeMin = 100;
const static int mouseDurationRangeMax = 2000;
const static int mouseHorSpeedRangeMin = 1;
const static int mouseHorSpeedRangeMax = 6;

static void applyLimits(ai_inputs *individual)
{
	individual->jumpDelay = max(jumpDelayRangeMin, min(individual->jumpDelay, jumpDelayRangeMax));
	individual->mouseDelay = max(mouseHorDelayRangeMin, min(individual->mouseDelay, mouseHorDelayRangeMax));
	individual->mouseDuration = max(mouseDurationRangeMin, min(individual->mouseDuration, mouseDurationRangeMax));
	individual->mouseSpeed = max(mouseHorSpeedRangeMin, min(individual->mouseSpeed, mouseHorSpeedRangeMax));
}

void generateFirstGeneration(ai_inputs *individuals, int amount)
{
	for (int i = 0; i < amount; i++)
	{
		// Individual variables: key input
		individuals[i].jumpDelay = randomIntRange(jumpDelayRangeMin, jumpDelayRangeMax);
		// Individual variables: mouse input - no vertical as it's (probably) unnecessary and would overcomplicate things
		individuals[i].mouseDelay = randomIntRange(mouseHorDelayRangeMin, mouseHorDelayRangeMax);
		individuals[i].mouseDuration = randomIntRange(mouseDurationRangeMin, mouseDurationRangeMax);
		individuals[i].mouseSpeed = randomIntRange(mouseHorSpeedRangeMin, mouseHorSpeedRangeMax);
	}
}
