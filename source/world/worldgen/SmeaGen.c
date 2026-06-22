#include <world/worldgen/SmeaGen.h>

#include <sino/sino.h>

void SmeaGen_Init(SmeaGen* gen, World* world) { gen->world = world; }

// based off https://github.com/smealum/3dscraft/blob/master/source/generation.c
void SmeaGen_Generate(WorkQueue* queue, WorkerItem item, void* this) {
	SmeaGen* gen = this;
	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int z = 0; z < CHUNK_SIZE; z++) {
			float px = (float)(x + item.chunk->x * CHUNK_SIZE);
			float pz = (float)(z + item.chunk->z * CHUNK_SIZE);

			const int smeasClusterSize = 8;
			const int smeasChunkHeight = 16;
			int height = (int)(sino_2d((px) / (smeasClusterSize * 4), (pz) / (smeasClusterSize * 4)) * smeasClusterSize) +
				     (smeasChunkHeight * smeasClusterSize / 2);

			for (int y = 0; y < height - 3; y++) {
				Block block = Block_Stone;
				// Deterministische, seltene Erzadern abhängig von der Weltposition.
				uint32_t h = (uint32_t)(px * 73856093) ^ (uint32_t)(y * 19349663) ^ (uint32_t)(pz * 83492791);
				h = (h ^ (h >> 13)) * 1274126177u;
				uint32_t r = h % 1000;
				if (y < 16 && r < 2)
					block = Block_DiamondOre;  // sehr selten, tief
				else if (y < 32 && r < 6)
					block = Block_GoldOre;  // selten
				else if (r < 18)
					block = Block_IronOre;  // gelegentlich
				Chunk_SetBlock(item.chunk, x, y, z, block);
			}
			for (int y = height - 3; y < height; y++) {
				Chunk_SetBlock(item.chunk, x, y, z, Block_Dirt);
			}
			Chunk_SetBlock(item.chunk, x, height, z, Block_Grass);
		}
	}
}