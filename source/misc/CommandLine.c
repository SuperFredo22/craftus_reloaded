#include <misc/CommandLine.h>

#include <3ds.h>

#include <gui/DebugUI.h>

#include <stdio.h>
#include <string.h>

void CommandLine_Activate(World* world, Player* player) {
	static SwkbdState swkbd;
	static char textBuffer[64];
	swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, 64);
	swkbdSetHintText(&swkbd, "Enter command");

	int button = swkbdInputText(&swkbd, textBuffer, sizeof(textBuffer));
	if (button == SWKBD_BUTTON_CONFIRM) {
		CommandLine_Execute(world, player, textBuffer);
	}
}

void CommandLine_Execute(World* world, Player* player, const char* text) {
	int length = strlen(text);
	if (length >= 1 && text[0] == '/') {
		if (length >= 9) {
			float x, y, z;
			if (sscanf(&text[1], "tp %f %f %f", &x, &y, &z) == 3) {
				player->position.x = x;
				player->position.y = y;
				player->position.z = z;
				DebugUI_Log("teleported to %f, %f %f", x, y, z);
				return;
			}
		}
		{
			char mode;
			if (sscanf(&text[1], "gamemode %c", &mode) == 1) {
				if (mode == '0' || mode == 's') {
					player->gamemode = Gamemode_Survival;
					player->flying = false;
					DebugUI_Log("Gamemode: Survival");
				} else if (mode == '1' || mode == 'c') {
					player->gamemode = Gamemode_Creative;
					Player_FillCreativeInventory(player);
					DebugUI_Log("Gamemode: Creative");
				} else {
					DebugUI_Log("Usage: /gamemode <s|c>");
				}
				return;
			}
		}
		if (!strcmp(&text[1], "heal")) {
			player->hp = PLAYER_MAX_HP;
			DebugUI_Log("Healed!");
			return;
		}
		if (length == 2 && text[1] == 'd') {
			extern bool showDebugInfo;
			showDebugInfo ^= true;
		}
	}
}