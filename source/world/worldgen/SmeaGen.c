#include <world/worldgen/SmeaGen.h>

#include <misc/NumberUtils.h>
#include <sino/sino.h>

void SmeaGen_Init(SmeaGen* gen, World* world) { gen->world = world; }

// deterministischer Positionshash für Erze und Bäume
static inline uint32_t coordHash(int x, int y, int z) {
	uint32_t h = (uint32_t)(x * 374761393) + (uint32_t)(y * 668265263) + (uint32_t)(z * 2147483647u);
	h = (h ^ (h >> 13)) * 1274126177u;
	return h ^ (h >> 16);
}

static void placeTree(Chunk* chunk, int x, int baseY, int z, int trunkHeight) {
	if (baseY + trunkHeight + 2 >= CHUNK_HEIGHT) return;

	// Blätterkrone
	for (int ly = trunkHeight - 2; ly <= trunkHeight - 1; ly++)
		for (int ox = -2; ox <= 2; ox++)
			for (int oz = -2; oz <= 2; oz++) {
				if (ox == 0 && oz == 0) continue;
				if (ABS(ox) == 2 && ABS(oz) == 2 && (coordHash(x + ox, baseY + ly, z + oz) & 1)) continue;
				Chunk_SetBlock(chunk, x + ox, baseY + ly, z + oz, Block_Leaves);
			}
	for (int ox = -1; ox <= 1; ox++)
		for (int oz = -1; oz <= 1; oz++) {
			if (ABS(ox) == 1 && ABS(oz) == 1) continue;
			Chunk_SetBlock(chunk, x + ox, baseY + trunkHeight, z + oz, Block_Leaves);
			if (ox == 0 && oz == 0) continue;
			Chunk_SetBlock(chunk, x + ox, baseY + trunkHeight + 1, z + oz, Block_Leaves);
		}
	Chunk_SetBlock(chunk, x, baseY + trunkHeight + 1, z, Block_Leaves);

	// Stamm
	for (int ly = 0; ly < trunkHeight; ly++) Chunk_SetBlock(chunk, x, baseY + ly, z, Block_Log);
}

// based off https://github.com/smealum/3dscraft/blob/master/source/generation.c
void SmeaGen_Generate(WorkQueue* queue, WorkerItem item, void* this) {
	SmeaGen* gen = this;
	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int z = 0; z < CHUNK_SIZE; z++) {
			int wx = x + item.chunk->x * CHUNK_SIZE;
			int wz = z + item.chunk->z * CHUNK_SIZE;
			float px = (float)wx;
			float pz = (float)wz;

			const int smeasClusterSize = 8;
			const int smeasChunkHeight = 16;
			int height = (int)(sino_2d((px) / (smeasClusterSize * 4), (pz) / (smeasClusterSize * 4)) * smeasClusterSize) +
				     (smeasChunkHeight * smeasClusterSize / 2);

			Chunk_SetBlock(item.chunk, x, 0, z, Block_Bedrock);
			for (int y = 1; y < height - 3; y++) {
				Block block = Block_Stone;
				// Erzadern
				uint32_t h = coordHash(wx, y, wz);
				if (y < height - 6 && (h % 61) == 0)
					block = Block_CoalOre;
				else if (y < height - 24 && (h % 97) == 0)
					block = Block_IronOre;
				Chunk_SetBlock(item.chunk, x, y, z, block);
			}
			for (int y = height - 3; y < height; y++) {
				Chunk_SetBlock(item.chunk, x, y, z, Block_Dirt);
			}
			Chunk_SetBlock(item.chunk, x, height, z, Block_Grass);
		}
	}

	// Bäume (komplett innerhalb des Chunks, deswegen mit Rand)
	for (int x = 2; x < CHUNK_SIZE - 2; x++) {
		for (int z = 2; z < CHUNK_SIZE - 2; z++) {
			int wx = x + item.chunk->x * CHUNK_SIZE;
			int wz = z + item.chunk->z * CHUNK_SIZE;

			uint32_t h = coordHash(wx, -1000, wz);
			if ((h % 89) != 0) continue;

			int y = CHUNK_HEIGHT - 1;
			while (y > 0 && Chunk_GetBlock(item.chunk, x, y, z) == Block_Air) y--;
			if (Chunk_GetBlock(item.chunk, x, y, z) != Block_Grass) continue;

			placeTree(item.chunk, x, y + 1, z, 4 + (int)(h % 3));
		}
	}
}