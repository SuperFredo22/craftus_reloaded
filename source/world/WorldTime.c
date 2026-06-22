#include <world/WorldTime.h>

#include <misc/NumberUtils.h>
#include <misc/Xorshift.h>

// Schlüsselfarben des Himmels (RGB 0..1).
static const float colorNight[3] = {0.102f, 0.102f, 0.180f};  // #1a1a2e
static const float colorSunrise[3] = {0.98f, 0.55f, 0.25f};   // orange
static const float colorDay[3] = {0.565f, 0.851f, 1.0f};      // #90d9ff (heller Tageshimmel)

static Xorshift32 weatherRng;

static void mix(const float a[3], const float b[3], float t, float out[3]) {
	for (int i = 0; i < 3; i++) out[i] = lerp(a[i], b[i], CLAMP(t, 0.f, 1.f));
}

void WorldTime_Init(WorldTime* wt) {
	wt->ticks = WORLDTIME_TICKS_PER_DAY / 2;  // Start am Mittag
	wt->timeOfDay = 0.5f;
	wt->fogDensity = 0.f;
	wt->isRaining = false;
	wt->weatherTimer = 600.f;  // ~10 Minuten klares Wetter zu Beginn
	weatherRng = Xorshift32_New();
	WorldTime_GetSkyColor(wt, wt->skyColor);
}

void WorldTime_Update(WorldTime* wt, float dt) {
	wt->ticks += (uint32_t)(dt * 60.f + 0.5f);
	wt->timeOfDay = (float)(wt->ticks % WORLDTIME_TICKS_PER_DAY) / (float)WORLDTIME_TICKS_PER_DAY;

	// Wettersteuerung
	wt->weatherTimer -= dt;
	if (wt->weatherTimer <= 0.f) {
		if (wt->isRaining) {
			wt->isRaining = false;
			wt->weatherTimer = 300.f + (Xorshift32_Next(&weatherRng) % 300);  // 5-10 min trocken
		} else {
			// ~20% Chance, dass es nach einer klaren Phase regnet
			wt->isRaining = (Xorshift32_Next(&weatherRng) % 100) < 20;
			wt->weatherTimer = 300.f;  // 5 min bis zur nächsten Prüfung / Regendauer
		}
	}

	// Nebeldichte zieht sanft Richtung Zielwert (mehr Nebel bei Regen).
	float targetFog = wt->isRaining ? 0.6f : 0.f;
	wt->fogDensity += (targetFog - wt->fogDensity) * CLAMP(dt * 0.5f, 0.f, 1.f);

	WorldTime_GetSkyColor(wt, wt->skyColor);
}

void WorldTime_GetSkyColor(WorldTime* wt, float out[3]) {
	float t = wt->timeOfDay;
	if (t < 0.25f) {
		// tiefe Nacht
		out[0] = colorNight[0];
		out[1] = colorNight[1];
		out[2] = colorNight[2];
	} else if (t < 0.30f) {
		// Morgendämmerung: Nacht -> Orange
		mix(colorNight, colorSunrise, (t - 0.25f) / 0.05f, out);
	} else if (t < 0.35f) {
		// Orange -> Tag
		mix(colorSunrise, colorDay, (t - 0.30f) / 0.05f, out);
	} else if (t < 0.65f) {
		// heller Tag
		out[0] = colorDay[0];
		out[1] = colorDay[1];
		out[2] = colorDay[2];
	} else if (t < 0.70f) {
		// Abend: Tag -> Orange
		mix(colorDay, colorSunrise, (t - 0.65f) / 0.05f, out);
	} else if (t < 0.75f) {
		// Orange -> Nacht
		mix(colorSunrise, colorNight, (t - 0.70f) / 0.05f, out);
	} else {
		// Nacht
		out[0] = colorNight[0];
		out[1] = colorNight[1];
		out[2] = colorNight[2];
	}

	// Bei Regen den Himmel etwas abdunkeln.
	if (wt->fogDensity > 0.f) {
		float d = 1.f - wt->fogDensity * 0.4f;
		out[0] *= d;
		out[1] *= d;
		out[2] *= d;
	}
}

float WorldTime_GetBrightness(WorldTime* wt) {
	float t = wt->timeOfDay;
	// dreieckiger Verlauf mit Maximum am Mittag (0.5)
	float b = 1.f - ABS(t - 0.5f) * 2.f;  // 0 bei Mitternacht, 1 bei Mittag
	b = CLAMP(b * 1.6f, 0.15f, 1.f);      // nie komplett dunkel
	if (wt->isRaining) b *= 0.7f;
	return b;
}
