#include <entity/Player.h>

#include <blocks/BlockProperties.h>
#include <entity/Sheep.h>
#include <inventory/Item.h>
#include <misc/Collision.h>
#include <world/Direction.h>

// Fügt amount Items (Block- oder Item-ID) ins Inventar ein. Erst Hotbar, dann Hauptinventar.
// Gibt true zurück, wenn alles untergebracht wurde.
static bool Player_GiveItem(Player* player, uint8_t id, uint8_t meta, int amount) {
	bool stackable = !Item_IsTool(id);
	// 1. In vorhandene Stacks einsortieren (nur stapelbare Items).
	if (stackable) {
		for (int i = 0; i < player->quickSelectBarSlots && amount > 0; i++) {
			ItemStack* s = &player->quickSelectBar[i];
			if (s->amount > 0 && s->block == id && s->meta == meta) {
				int add = MIN(amount, ITEMSTACK_MAX - s->amount);
				s->amount += add;
				amount -= add;
			}
		}
		int invCount = sizeof(player->inventory) / sizeof(ItemStack);
		for (int i = 0; i < invCount && amount > 0; i++) {
			ItemStack* s = &player->inventory[i];
			if (s->amount > 0 && s->block == id && s->meta == meta) {
				int add = MIN(amount, ITEMSTACK_MAX - s->amount);
				s->amount += add;
				amount -= add;
			}
		}
	}
	// 2. Leere Slots befüllen.
	while (amount > 0) {
		ItemStack* dst = NULL;
		for (int i = 0; i < player->quickSelectBarSlots; i++)
			if (player->quickSelectBar[i].amount == 0) {
				dst = &player->quickSelectBar[i];
				break;
			}
		if (!dst) {
			int invCount = sizeof(player->inventory) / sizeof(ItemStack);
			for (int i = 0; i < invCount; i++)
				if (player->inventory[i].amount == 0) {
					dst = &player->inventory[i];
					break;
				}
		}
		if (!dst) return false;  // voll
		int add = stackable ? MIN(amount, ITEMSTACK_MAX) : 1;
		*dst = (ItemStack){id, meta, add};
		amount -= add;
	}
	return true;
}

void Player_Init(Player* player, World* world) {
	player->position = f3_new(0.f, 0.f, 0.f);

	player->bobbing = 0.f;
	player->pitch = 0.f;
	player->yaw = 0.f;

	player->grounded = false;
	player->sprinting = false;
	player->world = world;

	player->fovAdd = 0.f;
	player->crouchAdd = 0.f;

	player->view = f3_new(0, 0, -1);

	player->crouching = false;
	player->flying = false;

	player->blockInSeight = false;
	player->blockInActionRange = false;

	player->velocity = f3_new(0, 0, 0);
	player->simStepAccum = 0.f;

	player->breakPlaceTimeout = 0.f;

	player->quickSelectBarSlots = INVENTORY_QUICKSELECT_MAXSLOTS;
	player->quickSelectBarSlot = 0;

	player->breakProgress = 0.f;
	player->isBreakingBlock = false;
	player->breakingBlockX = player->breakingBlockY = player->breakingBlockZ = 0;
	player->attackCooldown = 0.f;

	// Standard ist Kreativ (Verhalten exakt wie zuvor). main.c überschreibt das nach der Weltauswahl.
	Player_SetGameMode(player, GameMode_Creative);

	player->autoJumpEnabled = true;
}

static void fillCreativeInventory(Player* player) {
	int l = 0;
	player->inventory[l++] = (ItemStack){Block_Stone, 0, 1};
	player->inventory[l++] = (ItemStack){Block_Dirt, 0, 1};
	player->inventory[l++] = (ItemStack){Block_Grass, 0, 1};
	player->inventory[l++] = (ItemStack){Block_Cobblestone, 0, 1};
	player->inventory[l++] = (ItemStack){Block_Sand, 0, 1};
	player->inventory[l++] = (ItemStack){Block_Log, 0, 1};
	player->inventory[l++] = (ItemStack){Block_Leaves, 0, 1};
	player->inventory[l++] = (ItemStack){Block_Glass, 0, 1};
	player->inventory[l++] = (ItemStack){Block_Stonebrick, 0, 1};
	player->inventory[l++] = (ItemStack){Block_Brick, 0, 1};
	player->inventory[l++] = (ItemStack){Block_Planks, 0, 1};
	for (int i = 0; i < 16; i++) player->inventory[l++] = (ItemStack){Block_Wool, i, 1};
	player->inventory[l++] = (ItemStack){Block_Bedrock, 0, 1};

	for (int i = 0; i < INVENTORY_QUICKSELECT_MAXSLOTS; i++) player->quickSelectBar[i] = (ItemStack){Block_Air, 0, 0};
}

void Player_SetGameMode(Player* player, GameMode mode) {
	player->gameMode = mode;

	player->health = 20.f;
	player->breakProgress = 0.f;
	player->isBreakingBlock = false;

	// Inventar leeren
	int invCount = sizeof(player->inventory) / sizeof(ItemStack);
	for (int i = 0; i < invCount; i++) player->inventory[i] = (ItemStack){Block_Air, 0, 0};
	for (int i = 0; i < INVENTORY_QUICKSELECT_MAXSLOTS; i++) player->quickSelectBar[i] = (ItemStack){Block_Air, 0, 0};

	if (mode == GameMode_Creative) {
		player->hunger = 20.f;
		player->saturation = 20.f;
		fillCreativeInventory(player);
	} else {
		player->hunger = 20.f;
		player->saturation = 5.f;
		// Ein paar Start-Items zum schnellen Testen.
		Player_GiveItem(player, ITEM_ID(Item_WoodPickaxe), Item_GetProps(ITEM_ID(Item_WoodPickaxe)).maxDurability, 1);
		Player_GiveItem(player, ITEM_ID(Item_WoodAxe), Item_GetProps(ITEM_ID(Item_WoodAxe)).maxDurability, 1);
		Player_GiveItem(player, ITEM_ID(Item_Apple), 0, 1);
	}
}

void Player_Update(Player* player) {
	player->view = f3_new(-sinf(player->yaw) * cosf(player->pitch), sinf(player->pitch), -cosf(player->yaw) * cosf(player->pitch));

	player->blockInSeight =
	    Raycast_Cast(player->world, f3_new(player->position.x, player->position.y + PLAYER_EYEHEIGHT, player->position.z), player->view,
			 &player->viewRayCast);
	player->blockInActionRange = player->blockInSeight && player->viewRayCast.distSqr < 5.f * 5.f * 5.f;
}

bool Player_CanMove(Player* player, float newX, float newY, float newZ) {
	for (int x = -1; x < 2; x++) {
		for (int y = 0; y < 3; y++) {
			for (int z = -1; z < 2; z++) {
				int pX = FastFloor(newX) + x;
				int pY = FastFloor(newY) + y;
				int pZ = FastFloor(newZ) + z;
				if (World_GetBlock(player->world, pX, pY, pZ) != Block_Air) {
					if (AABB_Overlap(newX - PLAYER_COLLISIONBOX_SIZE / 2.f, newY, newZ - PLAYER_COLLISIONBOX_SIZE / 2.f,
							 PLAYER_COLLISIONBOX_SIZE, PLAYER_HEIGHT, PLAYER_COLLISIONBOX_SIZE, pX, pY, pZ, 1.f,
							 1.f, 1.f)) {
						return false;
					}
				}
			}
		}
	}
	return true;
}

void Player_Jump(Player* player, float3 accl) {
	if (player->grounded && !player->flying) {
		player->velocity.x = accl.x * 1.1f;
		player->velocity.z = accl.z * 1.1f;
		player->velocity.y = 6.7f;
		player->jumped = true;
		player->crouching = false;
	}
}
#include <gui/DebugUI.h>
const float MaxWalkVelocity = 4.3f;
const float MaxFallVelocity = -50.f;
const float GravityPlusFriction = 10.f;
void Player_Move(Player* player, float dt, float3 accl) {
	player->breakPlaceTimeout -= dt;
	player->simStepAccum += dt;
	const float SimStep = 1.f / 60.f;
	while (player->simStepAccum >= SimStep) {
		player->velocity.y -= GravityPlusFriction * SimStep * 2.f;
		if (player->velocity.y < MaxFallVelocity) player->velocity.y = MaxFallVelocity;

		if (player->flying) player->velocity.y = 0.f;

		float speedFactor = 1.f;
		if (!player->grounded && !player->flying) {
			if (player->jumped)
				speedFactor = 0.2f;
			else
				speedFactor = 0.6f;
		} else if (player->flying)
			speedFactor = 2.f;
		else if (player->crouching)
			speedFactor = 0.5f;
		float3 newPos = f3_add(player->position, f3_add(f3_scl(player->velocity, SimStep), f3_scl(accl, SimStep * speedFactor)));
		float3 finalPos = player->position;

		bool wallCollision = false, wasGrounded = player->grounded;

		player->grounded = false;
		for (int j = 0; j < 3; j++) {
			int i = (int[]){0, 2, 1}[j];
			bool collision = false;
			float3 axisStep = /*f3_new(i == 0 ? newPos.x : player->position.x, i == 1 ? newPos.y : player->position.y,
						 i == 2 ? newPos.z : player->position.z)*/ finalPos;
			axisStep.v[i] = newPos.v[i];
			Box playerBox =
			    Box_Create(axisStep.x - PLAYER_COLLISIONBOX_SIZE / 2.f, axisStep.y, axisStep.z - PLAYER_COLLISIONBOX_SIZE / 2.f,
				       PLAYER_COLLISIONBOX_SIZE, PLAYER_HEIGHT, PLAYER_COLLISIONBOX_SIZE);

			for (int x = -1; x < 2; x++) {
				for (int y = 0; y < 3; y++) {
					for (int z = -1; z < 2; z++) {
						int pX = FastFloor(axisStep.x) + x;
						int pY = FastFloor(axisStep.y) + y;
						int pZ = FastFloor(axisStep.z) + z;
						if (World_GetBlock(player->world, pX, pY, pZ) != Block_Air) {
							Box blockBox = Box_Create(pX, pY, pZ, 1, 1, 1);

							float3 normal = f3_new(0.f, 0.f, 0.f);
							float depth = 0.f;
							int face = 0;

							bool intersects =
							    Collision_BoxIntersect(blockBox, playerBox, 0, &normal, &depth, &face);
							collision |= intersects;
						}
					}
				}
			}
			if (!collision)
				finalPos.v[i] = newPos.v[i];
			else if (i == 1) {
				if (player->velocity.y < 0.f || accl.y < 0.f) player->grounded = true;
				// Fallschaden: nur in Survival und nur bei harter Landung.
				if (player->gameMode == GameMode_Survival && player->velocity.y < -16.f)
					player->health -= (-player->velocity.y - 16.f) * 0.5f;
				player->jumped = false;
				player->velocity.x = 0.f;
				player->velocity.y = 0.f;
				player->velocity.z = 0.f;
			} else {
				wallCollision = true;
				if (i == 0)
					player->velocity.x = 0.f;
				else
					player->velocity.z = 0.f;
			}
		}

		float3 movDiff = f3_sub(finalPos, player->position);

		if (player->grounded && player->flying) player->flying = false;

		if (wallCollision && player->autoJumpEnabled) {
			float3 nrmDiff = f3_nrm(f3_sub(newPos, player->position));
			Block block = World_GetBlock(player->world, FastFloor(finalPos.x + nrmDiff.x),
						     FastFloor(finalPos.y + nrmDiff.y) + 2, FastFloor(finalPos.z + nrmDiff.z));
			Block landingBlock = World_GetBlock(player->world, FastFloor(finalPos.x + nrmDiff.x),
							    FastFloor(finalPos.y + nrmDiff.y) + 1, FastFloor(finalPos.z + nrmDiff.z));
			if (block == Block_Air && landingBlock != Block_Air) Player_Jump(player, accl);
		}

		if (player->crouching && player->crouchAdd > -0.3f) player->crouchAdd -= SimStep * 2.f;
		if (!player->crouching && player->crouchAdd < 0.0f) player->crouchAdd += SimStep * 2.f;

		if (player->crouching && !player->grounded && wasGrounded && finalPos.y < player->position.y && movDiff.x != 0.f &&
		    movDiff.z != 0.f) {
			finalPos = player->position;
			player->grounded = true;
			player->velocity.y = 0.f;
		}

		player->position = finalPos;
		player->velocity = f3_new(player->velocity.x * 0.95f, player->velocity.y, player->velocity.z * 0.95f);
		if (ABS(player->velocity.x) < 0.1f) player->velocity.x = 0.f;
		if (ABS(player->velocity.z) < 0.1f) player->velocity.z = 0.f;

		player->simStepAccum -= SimStep;
	}
}

void Player_PlaceBlock(Player* player) {
	ItemStack* held = &player->quickSelectBar[player->quickSelectBarSlot];

	// In Survival: Essen (Apfel) statt platzieren, wenn ein Nahrungs-Item gewählt ist.
	if (player->gameMode == GameMode_Survival && held->amount > 0 && Item_IsFood(held->block)) {
		if (player->breakPlaceTimeout < 0.f && player->hunger < 20.f) {
			player->hunger = MIN(20.f, player->hunger + 4.f);
			player->saturation = MIN(player->hunger, player->saturation + 2.4f);
			if (--held->amount == 0) *held = (ItemStack){Block_Air, 0, 0};
			player->breakPlaceTimeout = PLAYER_PLACE_REPLACE_TIMEOUT;
		}
		return;
	}

	if (player->world && player->blockInActionRange && player->breakPlaceTimeout < 0.f) {
		// Nichts platzieren wenn leer oder ein (nicht platzierbares) Item gewählt ist.
		if (player->gameMode == GameMode_Survival && (held->amount == 0 || !Item_IsBlock(held->block))) return;

		const int* offset = DirectionToOffset[player->viewRayCast.direction];
		if (AABB_Overlap(player->position.x - PLAYER_COLLISIONBOX_SIZE / 2.f, player->position.y,
				 player->position.z - PLAYER_COLLISIONBOX_SIZE / 2.f, PLAYER_COLLISIONBOX_SIZE, PLAYER_HEIGHT,
				 PLAYER_COLLISIONBOX_SIZE, player->viewRayCast.x + offset[0], player->viewRayCast.y + offset[1],
				 player->viewRayCast.z + offset[2], 1.f, 1.f, 1.f))
			return;
		World_SetBlockAndMeta(player->world, player->viewRayCast.x + offset[0], player->viewRayCast.y + offset[1],
				      player->viewRayCast.z + offset[2], held->block, held->meta);

		// In Survival verbraucht das Platzieren einen Block.
		if (player->gameMode == GameMode_Survival && --held->amount == 0) *held = (ItemStack){Block_Air, 0, 0};
	}
	if (player->breakPlaceTimeout < 0.f) player->breakPlaceTimeout = PLAYER_PLACE_REPLACE_TIMEOUT;
}

static void Player_FinishBreaking(Player* player, int x, int y, int z) {
	Block block = World_GetBlock(player->world, x, y, z);
	BlockProperties props = Block_GetProperties(block);

	// Werkzeugbewertung
	ItemStack* held = &player->quickSelectBar[player->quickSelectBarSlot];
	bool properTool = (props.properTool == 0);
	if (held->amount > 0 && Item_IsTool(held->block)) {
		ItemProps tp = Item_GetProps(held->block);
		if (props.properTool != 0 && tp.toolCategory == props.properTool - 1 && tp.toolLevel >= props.requiredLevel)
			properTool = true;
	}

	World_SetBlock(player->world, x, y, z, Block_Air);

	// Drops nur, wenn das richtige Werkzeug benutzt wurde.
	if (properTool && props.dropItem != Block_Air && props.dropAmount > 0) {
		bool give = true;
		if (props.dropRandom) give = (Xorshift32_Next(&player->world->randomTickGen) % 100) < 30;
		if (give) {
			uint8_t dropMeta = (props.dropItem == Block_Wool) ? World_GetMetadata(player->world, x, y, z) : 0;
			Player_GiveItem(player, props.dropItem, dropMeta, props.dropAmount);
		}
	}

	// Bonus-Drops: Gras -> Samen, Laub -> Apfel (jeweils ~30%).
	if (block == Block_Grass && (Xorshift32_Next(&player->world->randomTickGen) % 100) < 30)
		Player_GiveItem(player, ITEM_ID(Item_Seed), 0, 1);
	if (block == Block_Leaves && (Xorshift32_Next(&player->world->randomTickGen) % 100) < 30)
		Player_GiveItem(player, ITEM_ID(Item_Apple), 0, 1);

	// Werkzeugverschleiß
	if (held->amount > 0 && Item_IsTool(held->block)) {
		if (held->meta > 0) held->meta--;
		if (held->meta == 0) *held = (ItemStack){Block_Air, 0, 0};
	}

	// Abbau kostet etwas Sättigung.
	player->saturation = MAX(0.f, player->saturation - 0.1f);
}

void Player_BreakBlock(Player* player, float dt) {
	if (!(player->world && player->blockInActionRange)) {
		player->isBreakingBlock = false;
		player->breakProgress = 0.f;
		return;
	}

	int x = player->viewRayCast.x, y = player->viewRayCast.y, z = player->viewRayCast.z;

	// Kreativ: sofortiges Abbauen (Verhalten wie zuvor, mit Timeout-Drossel).
	if (player->gameMode == GameMode_Creative) {
		if (player->breakPlaceTimeout < 0.f) {
			World_SetBlock(player->world, x, y, z, Block_Air);
			player->breakPlaceTimeout = PLAYER_PLACE_REPLACE_TIMEOUT;
		}
		return;
	}

	// Survival: fortschrittsbasiertes Abbauen.
	if (!player->isBreakingBlock || x != player->breakingBlockX || y != player->breakingBlockY || z != player->breakingBlockZ) {
		player->isBreakingBlock = true;
		player->breakingBlockX = x;
		player->breakingBlockY = y;
		player->breakingBlockZ = z;
		player->breakProgress = 0.f;
	}

	Block block = World_GetBlock(player->world, x, y, z);
	if (block == Block_Air) {
		player->isBreakingBlock = false;
		player->breakProgress = 0.f;
		return;
	}
	BlockProperties props = Block_GetProperties(block);

	float speed = 1.f;
	ItemStack* held = &player->quickSelectBar[player->quickSelectBarSlot];
	if (held->amount > 0 && Item_IsTool(held->block)) {
		ItemProps tp = Item_GetProps(held->block);
		if (props.properTool != 0 && tp.toolCategory == props.properTool - 1)
			speed = tp.miningSpeed;
	}

	float hardness = props.hardness < 0.05f ? 0.05f : props.hardness;
	player->breakProgress += dt * speed / hardness;

	if (player->breakProgress >= 1.f) {
		Player_FinishBreaking(player, x, y, z);
		player->breakProgress = 0.f;
		player->isBreakingBlock = false;
	}
}

// Strahl-gegen-AABB (Slab-Methode). Schreibt den Trefferabstand nach tHit.
static bool rayIntersectsAABB(float3 ro, float3 rd, float3 mn, float3 mx, float* tHit) {
	float tmin = 0.f, tmax = 1e9f;
	for (int i = 0; i < 3; i++) {
		float o = ro.v[i], d = rd.v[i];
		if (ABS(d) < 1e-6f) {
			if (o < mn.v[i] || o > mx.v[i]) return false;
		} else {
			float t1 = (mn.v[i] - o) / d;
			float t2 = (mx.v[i] - o) / d;
			if (t1 > t2) {
				float tmp = t1;
				t1 = t2;
				t2 = tmp;
			}
			if (t1 > tmin) tmin = t1;
			if (t2 < tmax) tmax = t2;
			if (tmin > tmax) return false;
		}
	}
	*tHit = tmin;
	return true;
}

bool Player_AttackEntity(Player* player, float dt) {
	if (player->attackCooldown > 0.f) player->attackCooldown -= dt;
	World* world = player->world;
	if (!world) return false;

	float3 eye = f3_new(player->position.x, player->position.y + PLAYER_EYEHEIGHT, player->position.z);
	float3 dir = player->view;
	const float reach = 4.f;

	int best = -1;
	float bestT = reach;
	for (int i = 0; i < world->entityCount; i++) {
		Entity* e = &world->entities[i];
		float3 mn = f3_new(e->position.x - e->collisionBox.x / 2.f, e->position.y, e->position.z - e->collisionBox.z / 2.f);
		float3 mx = f3_new(e->position.x + e->collisionBox.x / 2.f, e->position.y + e->collisionBox.y,
				   e->position.z + e->collisionBox.z / 2.f);
		float t;
		if (rayIntersectsAABB(eye, dir, mn, mx, &t) && t >= 0.f && t < bestT) {
			bestT = t;
			best = i;
		}
	}
	if (best < 0) return false;

	if (player->attackCooldown <= 0.f) {
		Entity* e = &world->entities[best];

		float dmg = 1.f;  // bloße Faust
		ItemStack* held = &player->quickSelectBar[player->quickSelectBarSlot];
		if (held->amount > 0 && Item_IsTool(held->block)) {
			ItemProps tp = Item_GetProps(held->block);
			if (tp.attackDamage > 0) dmg = tp.attackDamage;
			// Waffe nutzt sich beim Angriff ab.
			if (held->meta > 0) held->meta--;
			if (held->meta == 0) *held = (ItemStack){Block_Air, 0, 0};
		}
		e->health -= dmg;

		// Rückstoß weg vom Spieler.
		float3 kb = f3_new(e->position.x - player->position.x, 0.f, e->position.z - player->position.z);
		float m = f3_mag(kb);
		if (m > 0.001f) kb = f3_scl(kb, 1.f / m);
		e->velocity.x += kb.x * 5.f;
		e->velocity.z += kb.z * 5.f;
		e->velocity.y += 3.f;

		if (e->health <= 0.f) {
			// Getötetes Schaf gibt seine Wolle.
			if (e->type == EntityType_Sheep) {
				SheepData* s = (SheepData*)e->data;
				Player_GiveItem(player, Block_Wool, (uint8_t)(s->woolColor & 15), 1);
			}
			e->removed = true;
		}
		player->attackCooldown = 0.4f;
	}
	return true;
}

void Player_UpdateSurvival(Player* player, float dt) {
	if (player->gameMode != GameMode_Survival) return;

	// Hunger baut langsam ab; Sättigung puffert.
	if (player->saturation > 0.f) {
		player->saturation = MAX(0.f, player->saturation - dt * 0.05f);
	} else {
		player->hunger = MAX(0.f, player->hunger - dt * 0.15f);
	}

	// Regeneration bei vollem Hunger.
	if (player->hunger >= 18.f && player->health < 20.f) {
		player->health = MIN(20.f, player->health + dt * 0.5f);
		player->saturation = MAX(0.f, player->saturation - dt * 0.2f);
	}
	// Verhungern.
	if (player->hunger <= 0.f && player->health > 0.f) {
		player->health -= dt * 0.5f;
	}

	// Tod: Stats zurücksetzen, Inventar leeren, am Spawn neu starten.
	if (player->health <= 0.f) {
		Player_SetGameMode(player, GameMode_Survival);
		int spawnY = World_GetHeight(player->world, 0, 0) + 1;
		Player_Teleport(player, 0.5f, (float)spawnY, 0.5f);
	}
}

void Player_Teleport(Player* player, float x, float y, float z) {
	player->position.x = x;
	player->position.y = y;
	player->position.z = z;

	player->velocity = f3_new(0, 0, 0);
	Player_Update(player);
}