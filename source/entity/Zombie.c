#include <entity/Zombie.h>

#include <entity/Player.h>
#include <misc/NumberUtils.h>
#include <misc/Xorshift.h>
#include <rendering/VertexFmt.h>
#include <world/World.h>

#include <string.h>

extern const WorldVertex cube_sides_lut[6 * 6];

static WorldVertex* zombieVBO;
static Xorshift32 zombieRng;

#define ZOMBIE_DETECT_RANGE 12.f
#define ZOMBIE_ATTACK_RANGE 1.4f
#define ZOMBIE_ATTACK_DAMAGE 3.f
#define ZOMBIE_SPEED 1.8f

static inline float randf() { return (float)(Xorshift32_Next(&zombieRng) % 10000) / 10000.f; }

Entity Zombie_Create(float x, float y, float z) {
	Entity e = Entity_Create(EntityType_Zombie, x, y, z);
	e.collisionBox = f3_new(0.6f, 1.8f, 0.6f);
	e.health = 20.f;

	ZombieData* z2 = (ZombieData*)e.data;
	z2->wanderTimer = 1.f + randf() * 3.f;
	z2->wanderDir = f3_new(0.f, 0.f, 0.f);
	z2->attackCooldown = 0.f;
	return e;
}

void Zombie_Update(Entity* e, float dt, struct World* world) {
	ZombieData* z2 = (ZombieData*)e->data;
	struct Player* player = world->player;

	if (z2->attackCooldown > 0.f) z2->attackCooldown -= dt;

	bool chasing = false;
	if (player) {
		float dx = player->position.x - e->position.x;
		float dz = player->position.z - e->position.z;
		float distSqr = dx * dx + dz * dz;

		if (distSqr < ZOMBIE_DETECT_RANGE * ZOMBIE_DETECT_RANGE) {
			chasing = true;
			float dist = sqrtf(distSqr);
			if (dist > 0.001f) {
				z2->wanderDir = f3_new(dx / dist, 0.f, dz / dist);
				e->yaw = atan2f(dx, dz);
			}

			// Angriff bei Kontakt (nur im Survival-Modus relevant).
			if (dist < ZOMBIE_ATTACK_RANGE && z2->attackCooldown <= 0.f && player->gameMode == GameMode_Survival) {
				player->health -= ZOMBIE_ATTACK_DAMAGE;
				z2->attackCooldown = 1.f;
			}
		}
	}

	if (!chasing) {
		// Ziellos umherwandern, wenn der Spieler außer Reichweite ist.
		z2->wanderTimer -= dt;
		if (z2->wanderTimer <= 0.f) {
			z2->wanderTimer = 4.f + randf() * 4.f;
			if (randf() < 0.4f) {
				z2->wanderDir = f3_new(0.f, 0.f, 0.f);
			} else {
				float angle = randf() * 2.f * M_PI;
				z2->wanderDir = f3_new(sinf(angle), 0.f, cosf(angle));
				e->yaw = angle;
			}
		}
	}

	float speed = chasing ? ZOMBIE_SPEED : ZOMBIE_SPEED * 0.5f;
	e->velocity.x = z2->wanderDir.x * speed;
	e->velocity.z = z2->wanderDir.z * speed;

	bool moving = (z2->wanderDir.x != 0.f || z2->wanderDir.z != 0.f);
	if (e->grounded && e->collidedHorizontal && moving) e->velocity.y = 6.5f;
}

void Zombie_InitRender() {
	zombieVBO = linearAlloc(sizeof(cube_sides_lut));
	memcpy(zombieVBO, cube_sides_lut, sizeof(cube_sides_lut));
	zombieRng = Xorshift32_New();
	// leicht andere Startsequenz als andere Mobs
	Xorshift32_Next(&zombieRng);
	Xorshift32_Next(&zombieRng);
}

void Zombie_DeinitRender() { linearFree(zombieVBO); }

void Zombie_Render(Entity* e, int projUniform, C3D_Mtx* projectionView) {
	// Zombiegrün
	for (int i = 0; i < 6 * 6; i++) {
		zombieVBO[i].rgb[0] = 44;
		zombieVBO[i].rgb[1] = 110;
		zombieVBO[i].rgb[2] = 58;
	}

	C3D_Mtx model;
	Mtx_Identity(&model);
	Mtx_Translate(&model, e->position.x - e->collisionBox.x / 2.f, e->position.y, e->position.z - e->collisionBox.z / 2.f, true);
	Mtx_Scale(&model, e->collisionBox.x, e->collisionBox.y, e->collisionBox.z);

	C3D_Mtx mvp;
	Mtx_Multiply(&mvp, projectionView, &model);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, projUniform, &mvp);

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, zombieVBO, sizeof(WorldVertex), 4, 0x3210);

	C3D_DrawArrays(GPU_TRIANGLES, 0, 6 * 6);

	// TexEnv zurücksetzen (Textur modulieren) für nachfolgendes Rendering.
	env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
}
