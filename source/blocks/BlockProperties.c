#include <blocks/BlockProperties.h>

#include <inventory/Item.h>

// Werkzeugkategorien: 1=pick, 2=axe, 3=shovel, 4=sword
static const BlockProperties properties[Blocks_Count] = {
    [Block_Air] = {0.f, 0, 0, Block_Air, 0, false},
    [Block_Stone] = {3.f, 1, 0, Block_Cobblestone, 1, false},
    [Block_Dirt] = {0.5f, 3, 0, Block_Dirt, 1, false},
    [Block_Grass] = {0.6f, 3, 0, Block_Dirt, 1, false}, // Samen-Bonus wird im Player gesondert behandelt
    [Block_Cobblestone] = {3.f, 1, 0, Block_Cobblestone, 1, false},
    [Block_Sand] = {0.5f, 3, 0, Block_Sand, 1, false},
    [Block_Log] = {2.f, 2, 0, Block_Log, 1, false},
    [Block_Leaves] = {0.2f, 0, 0, Block_Air, 0, false}, // Apfel-Bonus wird im Player gesondert behandelt
    [Block_Glass] = {0.3f, 0, 0, Block_Air, 0, false},
    [Block_Stonebrick] = {3.f, 1, 0, Block_Stonebrick, 1, false},
    [Block_Brick] = {3.f, 1, 0, Block_Brick, 1, false},
    [Block_Planks] = {2.f, 2, 0, Block_Planks, 1, false},
    [Block_Wool] = {0.8f, 0, 0, Block_Wool, 1, false},
    [Block_Bedrock] = {100000.f, 1, 255, Block_Air, 0, false}, // praktisch unzerstörbar
    [Block_IronOre] = {4.f, 1, 1, ITEM_ID(Item_IronOre), 1, false},
    [Block_GoldOre] = {5.f, 1, 1, Block_GoldOre, 1, true},
    [Block_DiamondOre] = {5.f, 1, 2, Block_DiamondOre, 1, true},
};

BlockProperties Block_GetProperties(Block block) {
	if (block >= Blocks_Count) return (BlockProperties){1.f, 0, 0, Block_Air, 0, false};
	return properties[block];
}
