#pragma once

// We should probably combine these structs
typedef struct _ai_inputs {
	int jumpDelay;		// The delay before pressing the jump key
	int mouseDelay;		// The delay before moving the mouse horizontally
	int mouseDuration;	// The duration for which to move the mouse
	int mouseSpeed;		// The speed at which the mouse will be moved
} ai_inputs;

typedef struct _ai_outputs {
	float maxDistanceAtPosHeight;	// The maximum distance for which the height is still >= starting height
	float maxVelocityY;				// The maximum Y velocity the individual reached
	float avgVelocityY;				// The average Y velocity the individual reached after it has jumped
	float score;					// The score that the individual reached
} ai_outputs;

typedef struct _ai_individual {
	ai_inputs input;
	ai_outputs output;
} ai_individual;

// EC parameters
#define PARAMETER_COUNT		4
#define MUTATION_CHANCE		0.05
#define MUTATION_DIVISOR	10
#define SELECTION_CHANCE	0.65	// the chance the best individual gets during tournament selection
#define POOL_SIZE			96		// individuals (must be divisible by 3 due to customization)
#define TOURNAMENT_SIZE		(((POOL_SIZE / 3 * 2) % 2 == 0) ? (POOL_SIZE / 3 * 2) : ((POOL_SIZE / 3 * 2) - 1)) // Two-thirds of the pool size
#define SIM_TIME			4000	// ms
#define RESET_DISTANCE		-300	// units
#define DISTANCE_DIR		0		// 0 = x, 1 = x+y, 2 = y, 3 = z

void calculateScore(ai_outputs *results);
void generateNewIndividuals(ai_inputs *inputs, ai_outputs *outputs);
void generateFirstGeneration(ai_inputs *individuals, int amount);
