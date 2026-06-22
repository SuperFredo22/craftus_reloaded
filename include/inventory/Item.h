#pragma once

#include <stdbool.h>
#include <stdint.h>

// Item-IDs ab 128 sind keine Blöcke mehr, sondern "echte" Items (Werkzeuge, Nahrung, ...).
// Dadurch passen Blöcke (< 128) und Items (>= 128) gemeinsam in das uint8_t-Feld eines ItemStacks.
#define ITEM_FIRST 128

typedef enum {
	Item_Stick,
	Item_Apple,
	Item_Seed,
	// Werkzeuge
	Item_WoodPickaxe,
	Item_WoodAxe,
	Item_WoodShovel,
	Item_WoodSword,
	Item_StonePickaxe,
	Item_StoneAxe,
	Item_StoneShovel,
	Item_StoneSword,
	// Materialien
	Item_Coal,
	Item_IronOre,
	Item_IronIngot,
	Item_Count
} ItemType;

// Wandelt einen ItemType in die im ItemStack gespeicherte ID (>= ITEM_FIRST) um und zurück.
#define ITEM_ID(item) ((uint8_t)(ITEM_FIRST + (item)))
#define ITEM_INDEX(id) ((id)-ITEM_FIRST)

typedef struct {
	const char* name;
	bool isTool;          // wenn true, wird ItemStack.meta als Resthaltbarkeit benutzt
	uint8_t toolCategory; // 0=pick, 1=axe, 2=shovel, 3=sword (255 = kein Werkzeug)
	uint8_t toolLevel;    // 0=wood, 1=stone, ...
	float miningSpeed;    // Multiplikator für die Abbaugeschwindigkeit
	uint8_t maxDurability;
	uint8_t attackDamage;
} ItemProps;

// id ist eine kombinierte Block/Item-ID (siehe ITEM_FIRST).
bool Item_IsBlock(uint8_t id);
bool Item_IsTool(uint8_t id);
bool Item_IsFood(uint8_t id);

// Erwartet eine Item-ID (>= ITEM_FIRST).
ItemProps Item_GetProps(uint8_t id);
