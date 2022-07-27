#include "raylib.h"
#include <stdio.h>

//---------------------------------------------------------------------------------------- STRUCTS

typedef struct Sprite {
	Texture2D* tx;
	Rectangle rec;
	Vector2 loc;
} Sprite;



//---------------------------------------------------------------------------------------- ENUMS

enum gameModes {GAMEMODE_TITLE, GAMEMODE_GAME};



//---------------------------------------------------------------------------------------- GLOBALS

const int screenWidth = 60;
const int screenHeight = 60;
int grid[6][5];
Texture2D textures[10];



//---------------------------------------------------------------------------------------- FUNCTIONS

void PopulateGrid() {
	for (int a=0; a<6; a++) {
		for (int b=0; b<5; b++) {
			grid[a][b] = GetRandomValue(0, 5);
		}
	}
}

void DrawIconGrid(Texture2D sheet, int frame) {
	for (int a=0; a<6; a++) {
		for (int b=0; b<5; b++)
		DrawTextureRec(sheet, (Rectangle){grid[a][b]*10, frame*10, 10, 10}, (Vector2){a*10, b*10}, WHITE);
	}
}



//---------------------------------------------------------------------------------------- MAIN

int main() {

	// Variables
	float animTick = 0;
	const float animRate = 0.04;
	int animFrame = 0;
	int targets = 6;
	int remainingTargets = 3;
	unsigned char gameMode = GAMEMODE_GAME;

	// Init Window Stuff
	const char windowed = 13; // Make 0 for Fullscreen
	float scale, playAreaX;
	SetConfigFlags(FLAG_VSYNC_HINT);
	if (windowed) {
		InitWindow(screenWidth*windowed, screenHeight*windowed, "REEFLEX");
		scale = (float)windowed;
		playAreaX = 0;
	} else {
		InitWindow(GetScreenWidth(), GetScreenHeight(), "REEFLEX");
		scale = (float)GetScreenHeight()/screenHeight;
		playAreaX = (float)(GetScreenWidth()-(screenWidth*scale))*0.5;
		ToggleFullscreen();
		HideCursor();
	}

	// Load assets
	Texture2D TX_sprites = LoadTexture("img/sprites.png");
	Texture2D TX_logo = LoadTexture("img/logo.png");
	Texture2D TX_hud = LoadTexture("img/hud.png");

	// Sprites
	Sprite SP_hud = {&TX_sprites, {0, 20, 60, 10}, {0, 50}};
	Sprite SP_dot1 = {&TX_sprites, {0, 30, 2, 2}, {0, 0}};
	Sprite SP_dot2 = {&TX_sprites, {2, 30, 2, 2}, {0, 0}};

	RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
    SetTargetFPS(60);

	// Init
	PopulateGrid();

    while (!WindowShouldClose()) {

        // UPDATE
		if (gameMode == GAMEMODE_GAME) {

			animTick += animRate;
			if (animTick > 1) {
				animTick = 0;
				animFrame = !animFrame;
			}

		}
		

		// TEXTURE DRAW
		BeginTextureMode(target);
		
			// DRAW EVERYTHING HERE
			ClearBackground((Color){33, 33, 33, 255});

			if (gameMode == GAMEMODE_TITLE) {

				DrawTexture(TX_logo, 0, 0, WHITE);

			} else if (gameMode == GAMEMODE_GAME) {

				DrawIconGrid(TX_sprites, animFrame);
				DrawTextureRec(*SP_hud.tx, SP_hud.rec, SP_hud.loc, WHITE);
				for (int i=0; i<remainingTargets; i++)
				DrawTextureRec(*SP_dot1.tx, SP_dot1.rec, (Vector2){1+(i*3), 53}, WHITE);

			}
			
		
		EndTextureMode();

		// DRAW
        BeginDrawing();
		ClearBackground(BLACK);
		DrawTexturePro(target.texture, (Rectangle){0, 0, screenWidth, -screenHeight}, (Rectangle){playAreaX, 0, screenWidth*scale, screenHeight*scale}, (Vector2){0, 0}, 0, WHITE);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}