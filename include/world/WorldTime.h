#pragma once

#include <stdbool.h>
#include <stdint.h>

// 1 voller Tag = 1200 Ticks bei 60 Ticks/Sekunde = 20 Minuten Echtzeit.
#define WORLDTIME_TICKS_PER_DAY 1200

typedef struct {
	uint32_t ticks;  // Ticks seit Weltstart
	float timeOfDay; // 0.0 (Mitternacht) bis 1.0 (nächste Mitternacht)

	float skyColor[3]; // interpoliertes RGB des Himmels (0..1)
	float fogDensity;  // für Nebel/Regen
	bool isRaining;    // Wetter

	float weatherTimer; // Sekunden bis zum nächsten Wetterwechsel
} WorldTime;

void WorldTime_Init(WorldTime* wt);
void WorldTime_Update(WorldTime* wt, float dt);
void WorldTime_GetSkyColor(WorldTime* wt, float out[3]);

// Helligkeit 0..1 (nachts dunkel, tags hell), z.B. für Block-Beleuchtung nutzbar.
float WorldTime_GetBrightness(WorldTime* wt);
