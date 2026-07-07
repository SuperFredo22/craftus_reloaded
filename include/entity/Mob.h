#pragma once

#include <stdbool.h>

#include <entity/Player.h>
#include <world/World.h>

#include <citro3d.h>

#define MOBS_MAX (6)

#define MOB_WIDTH (0.6f)
#define MOB_HEIGHT (1.9f)
#define MOB_MAX_HP (10.f)

typedef struct {
	bool active;
	float3 position, velocity;
	float yaw;
	float hp;
	float hurtTimer, attackTimer, walkTime;
	float jumpTimeout;
	bool grounded;
	float simStepAccum;
} Mob;

void Mobs_Init(World* world, Player* player);
void Mobs_Deinit();

// Entfernt alle Mobs (z.B. beim Weltwechsel)
void Mobs_Reset();

void Mobs_Update(float dt);

// Versucht den Mob unter dem Fadenkreuz zu schlagen
bool Mobs_TryHit(Player* player);

void Mobs_Draw(int projUniform, C3D_Mtx* vp);
