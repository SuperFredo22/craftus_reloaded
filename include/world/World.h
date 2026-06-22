#pragma once

#include <world/Chunk.h>
#include <world/WorkQueue.h>
#include <world/WorldTime.h>

#include <entity/Entity.h>

#include <misc/NumberUtils.h>
#include <misc/Xorshift.h>
#include <vec/vec.h>


#define CHUNKCACHE_SIZE (9)

#define UNDEADCHUNKS_COUNT (2 * CHUNKCACHE_SIZE + CHUNKCACHE_SIZE * CHUNKCACHE_SIZE)

#define CHUNKPOOL_SIZE (CHUNKCACHE_SIZE * CHUNKCACHE_SIZE + UNDEADCHUNKS_COUNT)

typedef enum { WorldGen_Smea, WorldGen_SuperFlat, WorldGenTypes_Count } WorldGenType;
typedef struct {
	uint64_t seed;
	WorldGenType type;
	union {
		struct {
			// Keine Einstellungen...
		} superflat;
	} settings;
} GeneratorSettings;

#define WORLD_NAME_SIZE 12

#define WORLD_MAX_ENTITIES 256

typedef struct World {
	char name[WORLD_NAME_SIZE];

	GeneratorSettings genSettings;

	int cacheTranslationX, cacheTranslationZ;

	Chunk chunkPool[CHUNKPOOL_SIZE];
	Chunk* chunkCache[CHUNKCACHE_SIZE][CHUNKCACHE_SIZE];
	vec_t(Chunk*) freeChunks;

	WorkQueue* workqueue;

	Xorshift32 randomTickGen;

	WorldTime time;

	Entity entities[WORLD_MAX_ENTITIES];
	int entityCount;
} World;

inline static int WorldToChunkCoord(int x) { return (x + (int)(x < 0)) / CHUNK_SIZE - (int)(x < 0); }
inline static int WorldToLocalCoord(int x) { return x - WorldToChunkCoord(x) * CHUNK_SIZE; }

void World_Init(World* world, WorkQueue* workqueue);

void World_Reset(World* world);

void World_Tick(World* world);

// Aktualisiert Tageszeit/Wetter und alle Entities (jeden Frame mit echtem dt).
void World_Update(World* world, float dt);

Entity* World_AddEntity(World* world, Entity entity);
void World_RemoveEntity(World* world, int index);

// Erzeugt zufällige Start-Mobs (Schafe) an der Oberfläche rund um den Ursprung.
void World_SpawnInitialMobs(World* world);

Chunk* World_LoadChunk(World* world, int x, int z);
void World_UnloadChunk(World* world, Chunk* chunk);

Chunk* World_GetChunk(World* world, int x, int z);

Block World_GetBlock(World* world, int x, int y, int z);
void World_SetBlock(World* world, int x, int y, int z, Block block);
uint8_t World_GetMetadata(World* world, int x, int y, int z);
void World_SetMetadata(World* world, int x, int y, int z, uint8_t metadata);

void World_SetBlockAndMeta(World* world, int x, int y, int z, Block block, uint8_t metadata);

void World_UpdateChunkCache(World* world, int orginX, int orginZ);

int World_GetHeight(World* world, int x, int z);