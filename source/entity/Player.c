#include <entity/Player.h>

#include <limits.h>

#include <gui/DebugUI.h>
#include <misc/Collision.h>

void Player_FillCreativeInventory(Player* player) {
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
	player->inventory[l++] = (ItemStack){Block_CoalOre, 0, 1};
	for (int i = 0; i < 16; i++) player->inventory[l++] = (ItemStack){Block_Wool, i, 1};
	while (l < (int)(sizeof(player->inventory) / sizeof(ItemStack))) player->inventory[l++] = (ItemStack){Block_Air, 0, 0};

	for (int i = 0; i < INVENTORY_QUICKSELECT_MAXSLOTS; i++) player->quickSelectBar[i] = (ItemStack){Block_Air, 0, 0};
}

void Player_ClearInventory(Player* player) {
	for (int i = 0; i < (int)(sizeof(player->inventory) / sizeof(ItemStack)); i++)
		player->inventory[i] = (ItemStack){Block_Air, 0, 0};
	for (int i = 0; i < INVENTORY_QUICKSELECT_MAXSLOTS; i++) player->quickSelectBar[i] = (ItemStack){Block_Air, 0, 0};
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

	player->gamemode = Gamemode_Creative;
	player->hp = PLAYER_MAX_HP;
	player->hurtTimer = 0.f;
	player->fallDistance = 0.f;
	player->respawnImmunity = 0.f;
	player->spawnPos = f3_new(0.f, 64.f, 0.f);

	player->breakProgress = 0.f;
	player->breakProgressMax = 0.f;
	player->breakX = player->breakY = player->breakZ = INT_MIN;

	Player_FillCreativeInventory(player);

	player->autoJumpEnabled = true;
}

bool Player_CollectItem(Player* player, Block block, uint8_t meta) {
	if (block == Block_Air) return true;

	ItemStack* stackGroups[2] = {player->quickSelectBar, player->inventory};
	int stackGroupSizes[2] = {INVENTORY_QUICKSELECT_MAXSLOTS, sizeof(player->inventory) / sizeof(ItemStack)};

	// erst versuchen auf existierende Stapel zu legen...
	for (int g = 0; g < 2; g++)
		for (int i = 0; i < stackGroupSizes[g]; i++) {
			ItemStack* stack = &stackGroups[g][i];
			if (stack->amount > 0 && stack->amount < ITEMSTACK_MAX && stack->block == block && stack->meta == meta) {
				stack->amount++;
				return true;
			}
		}
	// ...dann einen freien Platz suchen
	for (int g = 0; g < 2; g++)
		for (int i = 0; i < stackGroupSizes[g]; i++) {
			ItemStack* stack = &stackGroups[g][i];
			if (stack->amount == 0) {
				*stack = (ItemStack){block, meta, 1};
				return true;
			}
		}
	return false;
}

void Player_Hurt(Player* player, float damage) {
	if (player->gamemode != Gamemode_Survival || player->respawnImmunity > 0.f) return;
	if (player->hurtTimer > 0.f) return;

	player->hp -= damage;
	player->hurtTimer = 0.5f;

	if (player->hp <= 0.f) {
		DebugUI_Log("You died!");

		player->hp = PLAYER_MAX_HP;
		player->hurtTimer = 0.f;
		player->breakProgress = 0.f;

		float3 spawn = player->spawnPos;
		int height = World_GetHeight(player->world, FastFloor(spawn.x), FastFloor(spawn.z));
		if (height > 0) spawn.y = (float)height + 1.f;

		Player_Teleport(player, spawn.x, spawn.y, spawn.z);
		// kurze Schwebephase, damit die Chunks am Spawn nachladen können
		player->respawnImmunity = 2.f;
		player->fallDistance = -1000.f;  // der erste Sturz nach dem Respawn ist frei
		player->flying = false;
		player->crouching = false;
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
	if (player->hurtTimer > 0.f) player->hurtTimer -= dt;

	if (player->respawnImmunity > 0.f) {
		player->respawnImmunity -= dt;
		player->velocity = f3_new(0.f, 0.f, 0.f);
		player->simStepAccum = 0.f;
		return;
	}

	// in die Leere gefallen
	if (player->gamemode == Gamemode_Survival && player->position.y < -30.f) {
		player->hurtTimer = 0.f;
		Player_Hurt(player, PLAYER_MAX_HP + 1.f);
		return;
	}

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

		if (finalPos.y < player->position.y && !player->flying) player->fallDistance += player->position.y - finalPos.y;
		if (player->grounded) {
			if (player->fallDistance > 3.f) Player_Hurt(player, floorf(player->fallDistance - 3.f));
			player->fallDistance = 0.f;
		} else if (player->flying) {
			player->fallDistance = 0.f;
		}

		player->position = finalPos;
		player->velocity = f3_new(player->velocity.x * 0.95f, player->velocity.y, player->velocity.z * 0.95f);
		if (ABS(player->velocity.x) < 0.1f) player->velocity.x = 0.f;
		if (ABS(player->velocity.z) < 0.1f) player->velocity.z = 0.f;

		player->simStepAccum -= SimStep;
	}
}

void Player_PlaceBlock(Player* player) {
	if (player->world && player->blockInActionRange && player->breakPlaceTimeout < 0.f) {
		ItemStack* stack = &player->quickSelectBar[player->quickSelectBarSlot];
		if (stack->block == Block_Air || (player->gamemode == Gamemode_Survival && stack->amount == 0)) {
			if (player->breakPlaceTimeout < 0.f) player->breakPlaceTimeout = PLAYER_PLACE_REPLACE_TIMEOUT;
			return;
		}
		const int* offset = DirectionToOffset[player->viewRayCast.direction];
		if (AABB_Overlap(player->position.x - PLAYER_COLLISIONBOX_SIZE / 2.f, player->position.y,
				 player->position.z - PLAYER_COLLISIONBOX_SIZE / 2.f, PLAYER_COLLISIONBOX_SIZE, PLAYER_HEIGHT,
				 PLAYER_COLLISIONBOX_SIZE, player->viewRayCast.x + offset[0], player->viewRayCast.y + offset[1],
				 player->viewRayCast.z + offset[2], 1.f, 1.f, 1.f))
			return;
		World_SetBlockAndMeta(player->world, player->viewRayCast.x + offset[0], player->viewRayCast.y + offset[1],
				      player->viewRayCast.z + offset[2], stack->block, stack->meta);
		if (player->gamemode == Gamemode_Survival && --stack->amount == 0) *stack = (ItemStack){Block_Air, 0, 0};
	}
	if (player->breakPlaceTimeout < 0.f) player->breakPlaceTimeout = PLAYER_PLACE_REPLACE_TIMEOUT;
}

void Player_BreakBlock(Player* player, float dt) {
	if (!player->world || !player->blockInActionRange) {
		player->breakProgress = 0.f;
		return;
	}

	if (player->gamemode == Gamemode_Creative) {
		if (player->breakPlaceTimeout < 0.f) {
			World_SetBlock(player->world, player->viewRayCast.x, player->viewRayCast.y, player->viewRayCast.z, Block_Air);
			player->breakPlaceTimeout = PLAYER_PLACE_REPLACE_TIMEOUT;
		}
		return;
	}

	int x = player->viewRayCast.x, y = player->viewRayCast.y, z = player->viewRayCast.z;
	if (x != player->breakX || y != player->breakY || z != player->breakZ) {
		player->breakProgress = 0.f;
		player->breakX = x;
		player->breakY = y;
		player->breakZ = z;
	}

	Block block = World_GetBlock(player->world, x, y, z);
	if (block == Block_Air) {
		player->breakProgress = 0.f;
		return;
	}
	float hardness = Block_GetHardness(block);
	if (hardness < 0.f) return;  // unzerstörbar

	// Schlaganimation am Laufen halten
	if (player->breakPlaceTimeout < -0.1f) player->breakPlaceTimeout = PLAYER_PLACE_REPLACE_TIMEOUT;

	player->breakProgressMax = hardness;
	player->breakProgress += dt;
	if (player->breakProgress >= hardness) {
		uint8_t meta = World_GetMetadata(player->world, x, y, z);
		World_SetBlock(player->world, x, y, z, Block_Air);
		Player_CollectItem(player, Block_GetDrop(block), meta);
		player->breakProgress = 0.f;
	}
}

void Player_Teleport(Player* player, float x, float y, float z) {
	player->position.x = x;
	player->position.y = y;
	player->position.z = z;

	player->velocity = f3_new(0, 0, 0);
	Player_Update(player);
}