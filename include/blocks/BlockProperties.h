#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <blocks/Block.h>

typedef struct {
	float hardness;        // Basis-Abbauzeit in Sekunden (mit passendem Werkzeug)
	uint8_t properTool;    // 0=keins, 1=pick, 2=axe, 3=shovel, 4=sword
	uint8_t requiredLevel; // 0=Hand/Holz, 1=Stein, ...
	uint8_t dropItem;      // kombinierte Block/Item-ID die gedroppt wird (0 = nichts)
	uint8_t dropAmount;
	bool dropRandom;       // wenn true: dropItem nur mit ~30% Chance
} BlockProperties;

BlockProperties Block_GetProperties(Block block);
