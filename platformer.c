#include "platformer.h"

typedef struct Level
{
	int** map;
} Level;

typedef struct Game 
{
	Level levels[10];
} Game;

void
draw_rectangle()
{}

global void
game_update_and_render(GameInput *Input)
{}