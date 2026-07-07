#include <entity/Mob.h>

#include <stdlib.h>
#include <string.h>

#include <3ds.h>

#include <gui/DebugUI.h>
#include <misc/NumberUtils.h>
#include <rendering/TextureMap.h>
#include <rendering/VertexFmt.h>

static Mob mobs[MOBS_MAX];
static World* world;
static Player* player;

static C3D_Tex zombieTexture;

extern const WorldVertex cube_sides_lut[6 * 6];

// Zombie = Steve Textur grün eingefärbt
#define ZOMBIE_R (115)
#define ZOMBIE_G (200)
#define ZOMBIE_B (115)

// 64x64 Pixel Skin, Texturkoordinaten sind 1.15 Fixkomma
#define SKINTEX(px) ((int16_t)((px) * (32768 / 64)))

// Ein Körperteil als Quader, Maße in Skinpixeln
typedef struct {
	int w, h, d;	  // Größe in px (x, y, z)
	int u, v;	  // Ursprung der Texturregion
	float pivotX, pivotY;  // Aufhängepunkt in Blöcken (relativ zu den Füßen)
	float yShift;	  // 0 = wächst vom Pivot nach oben, -1 = hängt nach unten
} MobPart;

#define MOB_PX_SCALE (0.058f)

enum { Part_Head, Part_Body, Part_ArmL, Part_ArmR, Part_LegL, Part_LegR, Part_Count };

static const MobPart mobParts[Part_Count] = {
    {8, 8, 8, 0, 0, 0.f, 24 * MOB_PX_SCALE, 0.f},	 // Kopf
    {8, 12, 4, 16, 16, 0.f, 12 * MOB_PX_SCALE, 0.f},	 // Körper
    {4, 12, 4, 40, 16, -6 * MOB_PX_SCALE, 24 * MOB_PX_SCALE, -1.f},  // linker Arm
    {4, 12, 4, 40, 16, 6 * MOB_PX_SCALE, 24 * MOB_PX_SCALE, -1.f},   // rechter Arm
    {4, 12, 4, 0, 16, -2 * MOB_PX_SCALE, 12 * MOB_PX_SCALE, -1.f},   // linkes Bein
    {4, 12, 4, 0, 16, 2 * MOB_PX_SCALE, 12 * MOB_PX_SCALE, -1.f},    // rechtes Bein
};

static WorldVertex* partVBOs[Part_Count];

static void buildPartVBO(WorldVertex* vbo, const MobPart* part) {
	memcpy(vbo, cube_sides_lut, sizeof(cube_sides_lut));

	int u = part->u, v = part->v, w = part->w, h = part->h, d = part->d;
	// Texturregion (x, y, Breite, Höhe) je Seite, Reihenfolge wie cube_sides_lut:
	// MX, PX, MY, PY, MZ, PZ
	const int rects[6][4] = {
	    {u + d + w, v + d, d, h},  // -X
	    {u, v + d, d, h},	       // +X
	    {u + d + w, v, w, d},      // Unterseite
	    {u + d, v, w, d},	       // Oberseite
	    {u + d, v + d, w, h},      // Vorderseite (-Z)
	    {u + 2 * d + w, v + d, w, h},  // Rückseite
	};

	for (int face = 0; face < 6; face++) {
		for (int j = 0; j < 6; j++) {
			WorldVertex* vtx = &vbo[face * 6 + j];
			int uflag = vtx->uv[0], vflag = vtx->uv[1];
			vtx->uv[0] = SKINTEX(rects[face][0] + (uflag ? rects[face][2] : 0));
			vtx->uv[1] = SKINTEX(rects[face][1] + (vflag ? 0 : rects[face][3]));
			vtx->rgb[0] = ZOMBIE_R;
			vtx->rgb[1] = ZOMBIE_G;
			vtx->rgb[2] = ZOMBIE_B;
		}
	}
}

void Mobs_Init(World* world_, Player* player_) {
	world = world_;
	player = player_;

	memset(mobs, 0, sizeof(mobs));

	Texture_Load(&zombieTexture, "romfs:/textures/entity/steve.png");

	for (int i = 0; i < Part_Count; i++) {
		partVBOs[i] = linearAlloc(sizeof(cube_sides_lut));
		buildPartVBO(partVBOs[i], &mobParts[i]);
	}

	srand(svcGetSystemTick());
}

void Mobs_Deinit() {
	for (int i = 0; i < Part_Count; i++) linearFree(partVBOs[i]);
	C3D_TexDelete(&zombieTexture);
}

void Mobs_Reset() { memset(mobs, 0, sizeof(mobs)); }

static bool mobCollidesWorld(float x, float y, float z) {
	for (int bx = FastFloor(x - MOB_WIDTH / 2.f); bx <= FastFloor(x + MOB_WIDTH / 2.f); bx++)
		for (int by = FastFloor(y); by <= FastFloor(y + MOB_HEIGHT); by++)
			for (int bz = FastFloor(z - MOB_WIDTH / 2.f); bz <= FastFloor(z + MOB_WIDTH / 2.f); bz++)
				if (World_GetBlock(world, bx, by, bz) != Block_Air) return true;
	return false;
}

static float randFloat() { return (float)rand() / (float)RAND_MAX; }

static void trySpawnMob() {
	int freeSlot = -1;
	for (int i = 0; i < MOBS_MAX; i++)
		if (!mobs[i].active) {
			freeSlot = i;
			break;
		}
	if (freeSlot == -1) return;

	float angle = randFloat() * 2.f * M_PI;
	float dist = 14.f + randFloat() * 10.f;
	float x = player->position.x + sinf(angle) * dist;
	float z = player->position.z + cosf(angle) * dist;

	int height = World_GetHeight(world, FastFloor(x), FastFloor(z));
	if (height <= 0 || height >= CHUNK_HEIGHT - 3) return;
	float y = (float)height + 1.05f;

	if (mobCollidesWorld(x, y, z)) return;

	Mob* mob = &mobs[freeSlot];
	memset(mob, 0, sizeof(Mob));
	mob->active = true;
	mob->position = f3_new(x, y, z);
	mob->velocity = f3_new(0.f, 0.f, 0.f);
	mob->hp = MOB_MAX_HP;
	mob->yaw = angle;
}

static void updateMob(Mob* mob, float dt) {
	float3 toPlayer = f3_sub(player->position, mob->position);
	float horizDistSqr = toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z;
	float horizDist = sqrtf(horizDistSqr);

	// zu weit weg -> verschwinden
	if (horizDistSqr > 48.f * 48.f) {
		mob->active = false;
		return;
	}

	if (mob->hurtTimer > 0.f) mob->hurtTimer -= dt;
	if (mob->attackTimer > 0.f) mob->attackTimer -= dt;
	if (mob->jumpTimeout > 0.f) mob->jumpTimeout -= dt;

	// Verfolgung
	float moveX = 0.f, moveZ = 0.f;
	const float speed = 2.f;
	if (horizDist > 0.9f && horizDist < 24.f) {
		moveX = toPlayer.x / horizDist * speed;
		moveZ = toPlayer.z / horizDist * speed;
		mob->yaw = atan2f(-toPlayer.x, -toPlayer.z);
		mob->walkTime += dt * speed * 2.f;
	}

	// Schwerkraft
	mob->velocity.y -= 20.f * dt;
	if (mob->velocity.y < -40.f) mob->velocity.y = -40.f;

	// Y Achse
	float newY = mob->position.y + mob->velocity.y * dt;
	bool wasFalling = mob->velocity.y < 0.f;
	if (mobCollidesWorld(mob->position.x, newY, mob->position.z)) {
		if (wasFalling) {
			mob->grounded = true;
			mob->position.y = (float)(FastFloor(newY) + 1) + 0.001f;
		}
		mob->velocity.y = 0.f;
	} else {
		mob->position.y = newY;
		mob->grounded = false;
	}

	// X/Z Achsen
	bool blocked = false;
	float newX = mob->position.x + (mob->velocity.x + moveX) * dt;
	if (!mobCollidesWorld(newX, mob->position.y, mob->position.z))
		mob->position.x = newX;
	else
		blocked = true;
	float newZ = mob->position.z + (mob->velocity.z + moveZ) * dt;
	if (!mobCollidesWorld(mob->position.x, mob->position.y, newZ))
		mob->position.z = newZ;
	else
		blocked = true;

	// Rückstoß abklingen lassen
	mob->velocity.x *= (1.f - MIN(1.f, dt * 8.f));
	mob->velocity.z *= (1.f - MIN(1.f, dt * 8.f));

	// einen Block hoch springen
	if (blocked && mob->grounded && mob->jumpTimeout <= 0.f) {
		mob->velocity.y = 6.7f;
		mob->grounded = false;
		mob->jumpTimeout = 0.5f;
	}

	// in die Leere gefallen
	if (mob->position.y < -40.f) {
		mob->active = false;
		return;
	}

	// Angriff
	float distSqr = f3_magSqr(toPlayer);
	if (distSqr < 1.6f * 1.6f && mob->attackTimer <= 0.f && player->gamemode == Gamemode_Survival &&
	    player->respawnImmunity <= 0.f) {
		Player_Hurt(player, 3.f);
		if (horizDist > 0.01f) {
			player->velocity.x += toPlayer.x / horizDist * 6.f;
			player->velocity.z += toPlayer.z / horizDist * 6.f;
			player->velocity.y += 3.f;
		}
		mob->attackTimer = 1.2f;
	}
}

static float spawnTimer = 0.f;

void Mobs_Update(float dt) {
	if (dt > 0.05f) dt = 0.05f;

	if (player->gamemode == Gamemode_Survival) {
		spawnTimer -= dt;
		if (spawnTimer <= 0.f) {
			spawnTimer = 4.f;
			trySpawnMob();
		}
	}

	for (int i = 0; i < MOBS_MAX; i++)
		if (mobs[i].active) updateMob(&mobs[i], dt);
}

// Strahl gegen AABB, gibt die Distanz zum Eintrittspunkt zurück (negativ = kein Treffer)
static float rayVsMobBox(Mob* mob, float3 origin, float3 dir) {
	float tmin = 0.f, tmax = 1e30f;

	float boxMin[3] = {mob->position.x - MOB_WIDTH / 2.f, mob->position.y, mob->position.z - MOB_WIDTH / 2.f};
	float boxMax[3] = {mob->position.x + MOB_WIDTH / 2.f, mob->position.y + MOB_HEIGHT, mob->position.z + MOB_WIDTH / 2.f};

	for (int i = 0; i < 3; i++) {
		if (ABS(dir.v[i]) < 1e-6f) {
			if (origin.v[i] < boxMin[i] || origin.v[i] > boxMax[i]) return -1.f;
		} else {
			float t1 = (boxMin[i] - origin.v[i]) / dir.v[i];
			float t2 = (boxMax[i] - origin.v[i]) / dir.v[i];
			if (t1 > t2) {
				float tmp = t1;
				t1 = t2;
				t2 = tmp;
			}
			tmin = MAX(tmin, t1);
			tmax = MIN(tmax, t2);
			if (tmin > tmax) return -1.f;
		}
	}
	return tmin;
}

bool Mobs_TryHit(Player* player_) {
	if (player_->breakPlaceTimeout >= 0.f) return false;

	float3 origin = f3_new(player_->position.x, player_->position.y + PLAYER_EYEHEIGHT, player_->position.z);

	Mob* best = NULL;
	float bestT = 4.f;  // maximale Schlagreichweite
	for (int i = 0; i < MOBS_MAX; i++) {
		if (!mobs[i].active) continue;
		float t = rayVsMobBox(&mobs[i], origin, player_->view);
		if (t >= 0.f && t < bestT) {
			bestT = t;
			best = &mobs[i];
		}
	}
	if (!best) return false;

	// steht ein Block im Weg?
	if (player_->blockInSeight && player_->viewRayCast.distSqr < bestT * bestT) return false;

	best->hp -= 4.f;
	best->hurtTimer = 0.4f;
	best->velocity.x += player_->view.x * 6.f;
	best->velocity.z += player_->view.z * 6.f;
	best->velocity.y = 4.f;
	if (best->hp <= 0.f) best->active = false;

	player_->breakPlaceTimeout = PLAYER_PLACE_REPLACE_TIMEOUT;
	return true;
}

void Mobs_Draw(int projUniform, C3D_Mtx* vp) {
	C3D_TexBind(0, &zombieTexture);
	C3D_AlphaTest(true, GPU_GEQUAL, 255);

	for (int i = 0; i < MOBS_MAX; i++) {
		Mob* mob = &mobs[i];
		if (!mob->active) continue;

		C3D_TexEnv* env = C3D_GetTexEnv(0);
		if (mob->hurtTimer > 0.f) {
			C3D_TexEnvColor(env, 0xff6060ff);
			C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_CONSTANT, 0);
		}

		float legSwing = sinf(mob->walkTime) * 0.6f;
		float armSwing = sinf(mob->walkTime) * 0.15f;
		const float partPitch[Part_Count] = {0.f,
						     0.f,
						     -M_PI / 2.f + armSwing,
						     -M_PI / 2.f - armSwing,
						     legSwing,
						     -legSwing};

		for (int p = 0; p < Part_Count; p++) {
			const MobPart* part = &mobParts[p];

			C3D_Mtx model, pm;
			Mtx_Identity(&model);
			Mtx_Translate(&model, mob->position.x, mob->position.y, mob->position.z, true);
			Mtx_RotateY(&model, mob->yaw, true);
			Mtx_Translate(&model, part->pivotX, part->pivotY, 0.f, true);
			if (partPitch[p] != 0.f) Mtx_RotateX(&model, partPitch[p], true);
			Mtx_Scale(&model, part->w * MOB_PX_SCALE, part->h * MOB_PX_SCALE, part->d * MOB_PX_SCALE);
			Mtx_Translate(&model, -0.5f, part->yShift, -0.5f, true);

			Mtx_Multiply(&pm, vp, &model);
			C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, projUniform, &pm);

			C3D_BufInfo* bufInfo = C3D_GetBufInfo();
			BufInfo_Init(bufInfo);
			BufInfo_Add(bufInfo, partVBOs[p], sizeof(WorldVertex), 4, 0x3210);

			C3D_DrawArrays(GPU_TRIANGLES, 0, 6 * 6);
		}

		if (mob->hurtTimer > 0.f) {
			C3D_TexEnvInit(env);
			C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
			C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
		}
	}

	C3D_AlphaTest(false, GPU_GREATER, 0);
}
