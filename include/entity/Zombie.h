#pragma once

#include <entity/Entity.h>

#include <citro3d.h>

// Wird im Entity.data-Puffer abgelegt (muss <= ENTITY_DATA_SIZE bleiben).
typedef struct {
	float wanderTimer;     // Sekunden bis zum Richtungswechsel (wenn kein Ziel)
	float3 wanderDir;      // aktuelle Laufrichtung
	float attackCooldown;  // Sekunden bis zum nächsten Angriff
} ZombieData;

struct World;

Entity Zombie_Create(float x, float y, float z);
void Zombie_Update(Entity* entity, float dt, struct World* world);

// Rendering (citro3d). Init/Deinit verwalten das gemeinsame VBO.
void Zombie_InitRender();
void Zombie_DeinitRender();
void Zombie_Render(Entity* entity, int projUniform, C3D_Mtx* projectionView);
