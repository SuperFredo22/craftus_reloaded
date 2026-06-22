#pragma once

#include <entity/Entity.h>

#include <citro3d.h>

// Wird im Entity.data-Puffer abgelegt (muss <= ENTITY_DATA_SIZE bleiben).
typedef struct {
	float wanderTimer;  // Sekunden bis zum Richtungswechsel
	float3 wanderDir;   // aktuelle Laufrichtung (normalisiert oder Null = stehen)
	int woolColor;      // 0-15 wie Wolle
} SheepData;

struct World;

Entity Sheep_Create(float x, float y, float z, int color);
void Sheep_Update(Entity* entity, float dt, struct World* world);

// Rendering (citro3d). Init/Deinit verwalten das gemeinsame VBO.
void Sheep_InitRender();
void Sheep_DeinitRender();
void Sheep_Render(Entity* entity, int projUniform, C3D_Mtx* projectionView);
