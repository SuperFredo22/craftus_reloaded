#include <inventory/Item.h>

#define NO_TOOL 255

// Tabelle aller Items (Index entspricht ItemType, NICHT der gespeicherten ID).
static const ItemProps itemProps[Item_Count] = {
    [Item_Stick] = {"Stick", false, NO_TOOL, 0, 0.f, 0, 0},
    [Item_Apple] = {"Apple", false, NO_TOOL, 0, 0.f, 0, 0},
    [Item_Seed] = {"Seeds", false, NO_TOOL, 0, 0.f, 0, 0},

    [Item_WoodPickaxe] = {"Wooden Pickaxe", true, 0, 0, 2.f, 59, 2},
    [Item_WoodAxe] = {"Wooden Axe", true, 1, 0, 2.f, 59, 3},
    [Item_WoodShovel] = {"Wooden Shovel", true, 2, 0, 2.f, 59, 2},
    [Item_WoodSword] = {"Wooden Sword", true, 3, 0, 1.5f, 59, 4},

    [Item_StonePickaxe] = {"Stone Pickaxe", true, 0, 1, 4.f, 131, 3},
    [Item_StoneAxe] = {"Stone Axe", true, 1, 1, 4.f, 131, 4},
    [Item_StoneShovel] = {"Stone Shovel", true, 2, 1, 4.f, 131, 3},
    [Item_StoneSword] = {"Stone Sword", true, 3, 1, 1.5f, 131, 5},

    [Item_Coal] = {"Coal", false, NO_TOOL, 0, 0.f, 0, 0},
    [Item_IronOre] = {"Iron Ore", false, NO_TOOL, 0, 0.f, 0, 0},
    [Item_IronIngot] = {"Iron Ingot", false, NO_TOOL, 0, 0.f, 0, 0},
};

bool Item_IsBlock(uint8_t id) { return id < ITEM_FIRST; }

bool Item_IsTool(uint8_t id) {
	if (Item_IsBlock(id)) return false;
	int index = ITEM_INDEX(id);
	if (index < 0 || index >= Item_Count) return false;
	return itemProps[index].isTool;
}

bool Item_IsFood(uint8_t id) {
	if (Item_IsBlock(id)) return false;
	return id == ITEM_ID(Item_Apple);
}

ItemProps Item_GetProps(uint8_t id) {
	int index = ITEM_INDEX(id);
	if (Item_IsBlock(id) || index < 0 || index >= Item_Count) return (ItemProps){"", false, NO_TOOL, 0, 0.f, 0, 0};
	return itemProps[index];
}
