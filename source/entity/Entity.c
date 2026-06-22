#include <entity/Entity.h>

#include <entity/Sheep.h>
#include <entity/Zombie.h>

#include <blocks/Block.h>
#include <misc/NumberUtils.h>
#include <world/World.h>

#define ENTITY_GRAVITY 22.f
#define ENTITY_MAX_FALL (-50.f)

static int nextEntityId = 1;

Entity Entity_Create(EntityType type, float x, float y, float z) {
	Entity e;
	e.type = type;
	e.position = f3_new(x, y, z);
	e.velocity = f3_new(0.f, 0.f, 0.f);
	e.pitch = 0.f;
	e.yaw = 0.f;
	e.collisionBox = f3_new(0.6f, 1.f, 0.6f);
	e.health = 10.f;
	e.id = nextEntityId++;
	e.grounded = false;
	e.collidedHorizontal = false;
	e.removed = false;
	for (int i = 0; i < ENTITY_DATA_SIZE; i++) e.data[i] = 0;
	return e;
}

void Entity_ApplyGravity(Entity* e, float dt) {
	e->velocity.y -= ENTITY_GRAVITY * dt;
	if (e->velocity.y < ENTITY_MAX_FALL) e->velocity.y = ENTITY_MAX_FALL;
}

static inline bool blockBoxOverlap(float3 mn, float3 mx, int bx, int by, int bz) {
	return mn.x < bx + 1 && mx.x > bx && mn.y < by + 1 && mx.y > by && mn.z < bz + 1 && mx.z > bz;
}

// Prüft, ob die Bounding-Box (Fußmittelpunkt = pos) gegen einen festen Block stößt.
static bool boxBlocked(struct World* world, float3 pos, float3 size) {
	float3 mn = f3_new(pos.x - size.x / 2.f, pos.y, pos.z - size.z / 2.f);
	float3 mx = f3_new(pos.x + size.x / 2.f, pos.y + size.y, pos.z + size.z / 2.f);
	for (int x = FastFloor(mn.x); x <= FastFloor(mx.x); x++)
		for (int y = FastFloor(mn.y); y <= FastFloor(mx.y); y++)
			for (int z = FastFloor(mn.z); z <= FastFloor(mx.z); z++) {
				if (World_GetBlock(world, x, y, z) != Block_Air && blockBoxOverlap(mn, mx, x, y, z)) return true;
			}
	return false;
}

// Gibt true zurück, wenn die Entity aktuell in einem festen Block steckt.
bool Entity_CheckCollisions(Entity* e, struct World* world) { return boxBlocked(world, e->position, e->collisionBox); }

void Entity_Update(Entity* e, float dt, struct World* world) {
	if (e->removed) return;
	if (dt > 0.05f) dt = 0.05f;  // Stabilität bei Lag

	// Typspezifische Logik (setzt horizontale Geschwindigkeit, springt ggf.)
	switch (e->type) {
		case EntityType_Sheep:
			Sheep_Update(e, dt, world);
			break;
		case EntityType_Zombie:
			Zombie_Update(e, dt, world);
			break;
		default:
			break;
	}

	Entity_ApplyGravity(e, dt);

	float3 size = e->collisionBox;
	e->collidedHorizontal = false;

	// X-Achse
	float3 np = e->position;
	np.x += e->velocity.x * dt;
	if (boxBlocked(world, np, size)) {
		e->velocity.x = 0.f;
		e->collidedHorizontal = true;
	} else {
		e->position.x = np.x;
	}

	// Z-Achse
	np = e->position;
	np.z += e->velocity.z * dt;
	if (boxBlocked(world, np, size)) {
		e->velocity.z = 0.f;
		e->collidedHorizontal = true;
	} else {
		e->position.z = np.z;
	}

	// Y-Achse
	np = e->position;
	np.y += e->velocity.y * dt;
	if (boxBlocked(world, np, size)) {
		if (e->velocity.y < 0.f) e->grounded = true;
		e->velocity.y = 0.f;
	} else {
		e->position.y = np.y;
		e->grounded = false;
	}

	// horizontale Reibung
	e->velocity.x *= 0.7f;
	e->velocity.z *= 0.7f;
}
