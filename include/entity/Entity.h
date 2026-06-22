#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <misc/VecMath.h>

// Vorwärtsdeklaration; die echte Definition steht in world/World.h.
struct World;

typedef enum { EntityType_Sheep, EntityType_Zombie, EntityType_Count } EntityType;

// Generischer, typunabhängiger Datenpuffer für entity-spezifische Daten (z.B. SheepData).
#define ENTITY_DATA_SIZE 32

typedef struct Entity {
	EntityType type;
	float3 position, velocity;
	float pitch, yaw;
	float3 collisionBox;  // Abmessungen (Breite, Höhe, Tiefe)
	float health;
	int id;

	bool grounded;
	bool collidedHorizontal;
	bool removed;

	// Typspezifische Daten, von z.B. Sheep_* per Cast benutzt.
	uint8_t data[ENTITY_DATA_SIZE];
} Entity;

Entity Entity_Create(EntityType type, float x, float y, float z);

void Entity_Update(Entity* entity, float dt, struct World* world);
void Entity_ApplyGravity(Entity* entity, float dt);
bool Entity_CheckCollisions(Entity* entity, struct World* world);
