#include <entity/Sheep.h>

#include <misc/NumberUtils.h>
#include <misc/Xorshift.h>
#include <rendering/VertexFmt.h>
#include <world/World.h>

#include <string.h>

extern const WorldVertex cube_sides_lut[6 * 6];

static WorldVertex* sheepVBO;
static Xorshift32 sheepRng;

// Wolle-Farbtabelle (analog Block.c)
static const uint32_t woolColors[] = {16777215, 14188339, 11685080, 6724056, 15066419, 8375321, 15892389, 5000268,
				      10066329, 5013401,  8339378,  3361970, 6704179,  6717235, 10040115, 1644825};

static inline float randf() { return (float)(Xorshift32_Next(&sheepRng) % 10000) / 10000.f; }

Entity Sheep_Create(float x, float y, float z, int color) {
	Entity e = Entity_Create(EntityType_Sheep, x, y, z);
	e.collisionBox = f3_new(0.9f, 1.0f, 0.9f);
	e.health = 8.f;

	SheepData* s = (SheepData*)e.data;
	s->wanderTimer = 1.f + randf() * 4.f;
	s->wanderDir = f3_new(0.f, 0.f, 0.f);
	s->woolColor = color & 15;
	return e;
}

void Sheep_Update(Entity* e, float dt, struct World* world) {
	(void)world;
	SheepData* s = (SheepData*)e->data;

	s->wanderTimer -= dt;
	if (s->wanderTimer <= 0.f) {
		s->wanderTimer = 5.f + randf() * 5.f;  // 5-10 Sekunden
		if (randf() < 0.35f) {
			// stehen bleiben
			s->wanderDir = f3_new(0.f, 0.f, 0.f);
		} else {
			float angle = randf() * 2.f * M_PI;
			s->wanderDir = f3_new(sinf(angle), 0.f, cosf(angle));
			e->yaw = angle;
		}
	}

	const float speed = 1.4f;
	e->velocity.x = s->wanderDir.x * speed;
	e->velocity.z = s->wanderDir.z * speed;

	// Springt, wenn er auf dem Boden steht und gegen ein Hindernis läuft.
	bool moving = (s->wanderDir.x != 0.f || s->wanderDir.z != 0.f);
	if (e->grounded && e->collidedHorizontal && moving) e->velocity.y = 6.5f;
}

void Sheep_InitRender() {
	sheepVBO = linearAlloc(sizeof(cube_sides_lut));
	memcpy(sheepVBO, cube_sides_lut, sizeof(cube_sides_lut));
	sheepRng = Xorshift32_New();
}

void Sheep_DeinitRender() { linearFree(sheepVBO); }

void Sheep_Render(Entity* e, int projUniform, C3D_Mtx* projectionView) {
	SheepData* s = (SheepData*)e->data;
	uint32_t col = woolColors[s->woolColor & 15];
	uint8_t r = (col >> 16) & 0xff, g = (col >> 8) & 0xff, b = col & 0xff;

	for (int i = 0; i < 6 * 6; i++) {
		sheepVBO[i].rgb[0] = r;
		sheepVBO[i].rgb[1] = g;
		sheepVBO[i].rgb[2] = b;
	}

	C3D_Mtx model;
	Mtx_Identity(&model);
	// Box ist im LUT 1x1x1 mit Ursprung in der Ecke -> auf Fußmittelpunkt verschieben & skalieren.
	Mtx_Translate(&model, e->position.x - e->collisionBox.x / 2.f, e->position.y, e->position.z - e->collisionBox.z / 2.f, true);
	Mtx_Scale(&model, e->collisionBox.x, e->collisionBox.y, e->collisionBox.z);

	C3D_Mtx mvp;
	Mtx_Multiply(&mvp, projectionView, &model);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, projUniform, &mvp);

	// Farbe direkt aus dem Primary-Color statt aus der Textur.
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, sheepVBO, sizeof(WorldVertex), 4, 0x3210);

	C3D_DrawArrays(GPU_TRIANGLES, 0, 6 * 6);

	// TexEnv für nachfolgendes Rendering zurücksetzen (Textur modulieren).
	env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
}
