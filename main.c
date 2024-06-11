#include "sprites.h"
#include "sounds.h"
#include "include/raylib.h"
#include "music.h"
#include <stdlib.h>
#define GRID_WIDTH 6
#define GRID_HEIGHT 5
#define ENEMY_TYPE_COUNT 4
#define STARTING_LEVEL 6
#define STARTING_WORLD 1
#define INVIS_SWITCH_START 0.2
#define SPRITE_ARRAY_SIZE 30



//---------------------------------------------------------------------------------------- STRUCTS

typedef struct Sprite {
	Rectangle rec;
	Vector2 loc;
	bool anim;
	int frames;
	int currentFrame;
	bool repeatAnim;
	bool exists;
	float animTick;
	float animSpeed;
	void (*eventOnFinish)();
	float holdLast;
} Sprite;

typedef struct Circle {
	Vector2 loc;
	int rad;
	int speed;
} Circle;

typedef struct Triangle {
	Vector2 a;
	Vector2 b;
	Vector2 c;
	Vector2 a_velocity;
	Vector2 b_velocity;
	Vector2 c_velocity;
	bool exists;
} Triangle;



//---------------------------------------------------------------------------------------- ENUMS

enum gameModes {
	GAMEMODE_LOADING,
	GAMEMODE_TITLE,
	GAMEMODE_GAME,
	GAMEMODE_HELP,
	GAMEMODE_GAMEOVER,
	GAMEMODE_WIN
};



//---------------------------------------------------------------------------------------- GLOBALS

const int screenWidth = 60;
const int screenHeight = 60;
int grid[GRID_HEIGHT][GRID_WIDTH];
int types[ENEMY_TYPE_COUNT] = {0, 0, 0, 0};
int remainingTargets = 0;
int currentLevel = STARTING_LEVEL;
int mode = 1;
float timeLeft = 56;
int lives = 3;
int shakeCount = 0;
Sprite *sprites;
int eventQueue = false;
bool controlsEnabled = true;
bool winAnimPlaying = false;
unsigned char gameMode = GAMEMODE_TITLE;
Sound SND_gameover;
Sound SND_win;
int world = STARTING_WORLD;
float world2Tick = 0;
bool lightsOn = true;
float invisTick = 0;
float invisSwitch = INVIS_SWITCH_START;
Triangle *triangles;
float scale, playAreaX;



//---------------------------------------------------------------------------------------- FUNCTIONS

void ResetLevel() {

	// Clear grid
	for (int y=0; y<GRID_HEIGHT; y++) {
		for (int x=0; x<GRID_WIDTH; x++) grid[y][x] = 0;
	}
	remainingTargets = 0;
	for (int i=0; i<ENEMY_TYPE_COUNT; i++) types[i] = 0;

	// Populate grid
	int type, x, y;
	while (remainingTargets < currentLevel) {
		y = GetRandomValue(0, GRID_HEIGHT-1);
		x = GetRandomValue(0, GRID_WIDTH-1);
		if (grid[y][x]) continue;
		type = GetRandomValue(1, ENEMY_TYPE_COUNT);
		grid[y][x] = type;
		types[type-1]++;
		remainingTargets++;
	}

	// Reset timer
	timeLeft = 56;
	invisSwitch = INVIS_SWITCH_START;

}

void MoveEnemies(bool printResult) {
	for (int y=0; y<GRID_HEIGHT; y++) {
		for (int x=0; x<GRID_WIDTH; x++) {
			if (grid[y][x]) {
				int moveDirY = GetRandomValue(-1, 1);
				int moveDirX = GetRandomValue(-1, 1);
				if (!moveDirX && !moveDirY) continue;
				if (moveDirY+y < 0 || moveDirY+y > (GRID_HEIGHT-1) || moveDirX+x < 0 || moveDirX+x > (GRID_WIDTH-1)) {
					// Move not allowed
					continue;
				}

				// If space is empty, move enemy there
				if (grid[y+moveDirY][x+moveDirX] == 0) {
					grid[y+moveDirY][x+moveDirX] = grid[y][x];
					grid[y][x] = 0;
				} else {
					// Swap positions with enemy
					int tempSwap = grid[y][x];
					grid[y][x] = grid[y+moveDirY][x+moveDirX];
					grid[y+moveDirY][x+moveDirX] = tempSwap;
				}
			}
		}
	}
}


void DrawIconGrid(Texture2D sheet, int frame, Vector2 shakeVector) {
	for (int y=0; y<GRID_HEIGHT; y++) {
		for (int x=0; x<GRID_WIDTH; x++) {
			if (grid[y][x])
			DrawTextureRec(sheet, (Rectangle){(grid[y][x]-1)*10, frame*10, 10, 10}, (Vector2){(x*10)+shakeVector.x, (y*10)+shakeVector.y}, WHITE);
		}
	}
}

Sprite BlankSprite() {
	Sprite newBlankSprite = {0};
	return newBlankSprite;
}

Sprite NewSprite(Rectangle rec, Vector2 loc, int frames, bool repeatFrames, float animSpeed, void (*eventOnFinish)(), float holdLastFrame) {
	Sprite newSprite;
	newSprite.exists = true;
	newSprite.rec = rec;
	newSprite.loc = loc;
	newSprite.frames = frames;
	newSprite.repeatAnim = repeatFrames;
	newSprite.animTick = 0;
	newSprite.animSpeed = animSpeed;
	newSprite.currentFrame = 0;
	newSprite.eventOnFinish = eventOnFinish;
	newSprite.holdLast = holdLastFrame;
	return newSprite;
}

void InitSpriteArray() {
	for (int i=0; i<SPRITE_ARRAY_SIZE; i++) {
		sprites[i] = BlankSprite();
	}
}

bool AttackEnemy(int type) {
	if (types[type] > 0) {
		types[type]--;
		remainingTargets--;
		// Find enemy of that type and change sprite
		for (int y=0; y<GRID_HEIGHT; y++) {
			for (int x=0; x<GRID_WIDTH; x++) {
				if (grid[y][x] != type+1) continue;
				grid[y][x] = 5;
				return true;
			}
		}
		return true;
	} else {
		return false;
	}
}

Circle NewCircle() {
	Circle newCirc;
	newCirc.loc.x = GetRandomValue(-10, 70);
	newCirc.loc.y = 80;
	newCirc.rad = GetRandomValue(3, 20);
	newCirc.speed = GetRandomValue(1, 3);
	return newCirc;
}

void UpdateSprites() {
	for (int i=0; i<30; i++) {
		if (!sprites[i].exists) continue;
		sprites[i].animTick += sprites[i].animSpeed;
		if (sprites[i].animTick > 1) {
			sprites[i].animTick = 0;
			if (sprites[i].currentFrame == sprites[i].frames-1 && sprites[i].holdLast > 0) {
				sprites[i].holdLast -= 0.1;
				continue;
			}
			sprites[i].currentFrame++;
			if (sprites[i].currentFrame > sprites[i].frames-1) {
				if (sprites[i].repeatAnim) {
					sprites[i].currentFrame = 0;
				} else {
					if (sprites[i].eventOnFinish) sprites[i].eventOnFinish();
					sprites[i].exists = false;
				}
			}
		}
	}
}

void DrawSprites(Texture2D spriteSheet) {
	for (int i=0; i<30; i++) {
		if (sprites[i].exists) {
			Rectangle frameRec;
			frameRec.x = sprites[i].rec.x + sprites[i].rec.width * sprites[i].currentFrame;
			frameRec.y = sprites[i].rec.y;
			frameRec.width = sprites[i].rec.width;
			frameRec.height = sprites[i].rec.height;
			DrawTextureRec(spriteSheet, frameRec, sprites[i].loc, WHITE);
		}
	}
}

void DrawTriangles() {
	for (int i=0; i<30; i++) {
		if (!triangles[i].exists) continue;
		DrawTriangleLines(triangles[i].a, triangles[i].b, triangles[i].c, (Color){238, 238, 238, 255});
	}
}

void EnableControls() {
	controlsEnabled = true;
}

void Gameover() {
	PlaySound(SND_gameover);
	sprites[7] = NewSprite((Rectangle){0, 67, 60, 60}, (Vector2){0, 0}, 14, false, 0.3, EnableControls, 0);
	gameMode = GAMEMODE_GAMEOVER;
}

void WorldChangeAnim() {
	controlsEnabled = false;
	sprites[10] = NewSprite((Rectangle){176, 0, 31, 16}, (Vector2){14, 6}, 8, false, 0.15, false, 1.3);
	sprites[11] = NewSprite((Rectangle){270, 3+(16*world), 16, 16}, (Vector2){21, 21}, 7, false, 0.15, EnableControls, 1.40);
	sprites[9] = NewSprite((Rectangle){0, 127, 60, 51}, (Vector2){0, 0}, 17, false, 0.16, false, 0); // Fade
}

void GameEnding() {
	PlaySound(SND_win);
	controlsEnabled = false;
	winAnimPlaying = true;
	sprites[0] = NewSprite((Rectangle){0, 178, 60, 30}, (Vector2){0, 15}, 6, true, 0.2, false, 0);
}

void UpdateTriangles() {
	for (int i=0; i<30; i++) {
		if (!triangles[i].exists) continue;
		triangles[i].a.x += triangles[i].a_velocity.x;
		triangles[i].a.y += triangles[i].a_velocity.y;
		triangles[i].b.x += triangles[i].b_velocity.x;
		triangles[i].b.y += triangles[i].b_velocity.y;
		triangles[i].c.x += triangles[i].c_velocity.x;
		triangles[i].c.y += triangles[i].c_velocity.y;

		// Check bounds
		if (triangles[i].a.x < -10 || triangles[i].a.x > 70) triangles[i].a_velocity.x *= -1;
		if (triangles[i].a.y < -10 || triangles[i].a.y > 70) triangles[i].a_velocity.y *= -1;
		if (triangles[i].b.x < -10 || triangles[i].b.x > 70) triangles[i].b_velocity.x *= -1;
		if (triangles[i].b.y < -10 || triangles[i].b.y > 70) triangles[i].b_velocity.y *= -1;
		if (triangles[i].c.x < -10 || triangles[i].c.x > 70) triangles[i].c_velocity.x *= -1;
		if (triangles[i].c.y < -10 || triangles[i].c.y > 70) triangles[i].c_velocity.y *= -1;
	}
}

void PopulateTriangles() {
	for (int i=1; i<30; i++) triangles[i] = (Triangle){0}; // Zero triangles
	triangles[0] = (Triangle){(Vector2){20, 0}, (Vector2){0, 20}, (Vector2){20, 45}, (Vector2){0.2, 0.5}, (Vector2){0.5, 0.3}, (Vector2){0.1, 0.5}, true};
	for (int i=1; i<9; i++) {
		triangles[i] = (Triangle){
			(Vector2){triangles[0].a.x+3*i, triangles[0].a.y+3*i},
			(Vector2){triangles[0].b.x+3*i, triangles[0].b.y+3*i},
			(Vector2){triangles[0].c.x+3*i, triangles[0].c.y+3*i},
			(Vector2){triangles[0].a_velocity.x, triangles[0].a_velocity.y},
			(Vector2){triangles[0].b_velocity.x, triangles[0].b_velocity.y},
			(Vector2){triangles[0].c_velocity.x, triangles[0].c_velocity.y},
			true
		};
	}
}

void SwitchFullscreen() {
	SetWindowState(FLAG_WINDOW_UNDECORATED);
	SetWindowSize(GetMonitorWidth(0), GetMonitorHeight(0));
	SetWindowPosition(0, 0);
	HideCursor();
	scale = (float)GetScreenHeight()/screenHeight;
	playAreaX = (float)(GetScreenWidth()-(screenWidth*scale))*0.5;
}



//---------------------------------------------------------------------------------------- MAIN

int main() {

	// Variables
	sprites = malloc(30*sizeof(Sprite));
	triangles = malloc(30*sizeof(Triangle));
	const float enemyAnimRate = 0.04;
	float enemyAnimTick = 0;
	int animFrame = 0;
	InitSpriteArray();
	sprites[0] = NewSprite((Rectangle){43, 61, 27, 5}, (Vector2){16, 47}, 8, true, 0.18, false, 0);
	const Color COL_WHITE = {238, 238, 238, 255};
	const Color COL_BLACK = {33, 33, 33, 255};
	Circle circles[10] = {0};
	int circleArrLength = sizeof(circles)/sizeof(Circle);
	for (int i=0; i<circleArrLength; i++) circles[i] = NewCircle();
	float pressA_scrollY = 40;

	// Shake shake
	Vector2 shakeVector = {0, 0};
	float shakeTick = 0;
	float shakeRate = 0.5;

	// Init Window Stuff
	InitWindow(GetMonitorWidth(0), GetMonitorHeight(0), "REEFLX");
	SetConfigFlags(FLAG_VSYNC_HINT);
	SwitchFullscreen();

	// Audio
	InitAudioDevice();

	// Sprites
	Sprite SP_types = {{0, 0, 40, 10}, {10, 18}};
	Sprite SP_logo = {{0, 40, 41, 27}, {9, 10}};
	Sprite SP_hud = {{0, 20, 60, 10}, {0, 50}};
	Sprite SP_targetRem = {{0, 30, 1, 1}, {0, 0}};
	Sprite SP_timeDot = {{1, 30, 1, 2}, {0, 0}};
	Sprite SP_letter_A = {{3, 35, 3, 5}, {0, 0}};
	Sprite SP_letter_S = {{7, 35, 3, 5}, {0, 0}};
	Sprite SP_letter_K = {{19, 35, 3, 5}, {0, 0}};
	Sprite SP_letter_L = {{23, 35, 3, 5}, {0, 0}};
	Sprite SP_gameover = {{780, 67, 60, 60}, {0, 0}};
	Sprite SP_pressA = {{43, 61, 27, 5}, {16, 47}};

	// Load sprites
	Image sprite_image = LoadImageFromMemory(".png", sprites_png, sprites_png_len);

	// Load sounds
	Texture2D TX_sprites = LoadTextureFromImage(sprite_image);
	Sound SND_bleep = LoadSoundFromWave(LoadWaveFromMemory(".ogg", bleep_ogg, bleep_ogg_len));
	Sound SND_click = LoadSoundFromWave(LoadWaveFromMemory(".ogg", click_ogg, click_ogg_len));
	Sound SND_final_attack = LoadSoundFromWave(LoadWaveFromMemory(".ogg", final_attack_ogg, final_attack_ogg_len));
	SND_gameover = LoadSoundFromWave(LoadWaveFromMemory(".ogg", gameover_ogg, gameover_ogg_len));
	Sound SND_looseLife = LoadSoundFromWave(LoadWaveFromMemory(".ogg", lifeloss_ogg, lifeloss_ogg_len));

	// Load music
	Sound SND_title_music = LoadSoundFromWave(LoadWaveFromMemory(".ogg", title_music_ogg, title_music_ogg_len));
	SND_win = LoadSoundFromWave(LoadWaveFromMemory(".ogg", win_ogg, win_ogg_len));
	Music MUS_world1 = LoadMusicStreamFromMemory(".ogg", world1_ogg, world1_ogg_len);
	Music MUS_world2 = LoadMusicStreamFromMemory(".ogg", world2_ogg, world2_ogg_len);
	Music MUS_world3 = LoadMusicStreamFromMemory(".ogg", world3_ogg, world3_ogg_len);

	// Init
	RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
    SetTargetFPS(60);
	ResetLevel();
	PopulateTriangles();
	PlaySound(SND_title_music);

    while (!WindowShouldClose()) {

        // UPDATE

		if (gameMode == GAMEMODE_TITLE) {

			if (controlsEnabled) {
				switch (GetKeyPressed()) {
					case KEY_ENTER: case KEY_A: case KEY_S: case KEY_K: case KEY_L:
						InitSpriteArray();
						gameMode = GAMEMODE_HELP;
						StopSound(SND_title_music);
						PlaySound(SND_click);
				}
			}
			for (int i=0; i<circleArrLength; i++) {
				circles[i].loc.y -= circles[i].speed;
				if (circles[i].loc.y < -20) {
					circles[i] = NewCircle();
				}
			}

		} else if (gameMode == GAMEMODE_GAME) {

			// Music
			switch (world) {
				case 1: UpdateMusicStream(MUS_world1); break;
				case 2: UpdateMusicStream(MUS_world2); break;
				case 3: UpdateMusicStream(MUS_world3); break;
			}

			// Animation tick / World 2 scramble enemies
			enemyAnimTick += enemyAnimRate;
			if (enemyAnimTick > 1) {
				enemyAnimTick = 0;
				animFrame = !animFrame;
			}

			// World 2
			if (world == 2) {
				world2Tick += 0.015;
				if (world2Tick > 1) {
					world2Tick = 0;
					MoveEnemies(false);
				}
			}

			// World 3
			if (world == 3) {
				invisTick += 0.01;
				if (invisTick > invisSwitch) {
					invisTick = 0;
					lightsOn = !lightsOn;
					invisSwitch *= 1.1;
				}
			}

			// Shake tick
			shakeTick += shakeRate;
			if (shakeTick > 1) {
				if (shakeCount > 0) {
					shakeVector.x = GetRandomValue(-3, 3);
					shakeVector.y = GetRandomValue(-3, 3);
					shakeTick = 0;
					shakeCount--;
				} else {
					shakeVector = (Vector2){0, 0};
				}
				
			}

			// Keyboard input
			if (controlsEnabled) {
				int enemyTypeKey = 4;
				switch (GetKeyPressed()) {
					case KEY_A: enemyTypeKey = 0; break;
					case KEY_S: enemyTypeKey = 1; break;
					case KEY_K: enemyTypeKey = 2; break;
					case KEY_L: enemyTypeKey = 3; break;
				}
				if (enemyTypeKey != 4) {
					if (AttackEnemy(enemyTypeKey)) {
						
						// Enemy attack sucsess
						PlaySound(SND_bleep);

					} else {

						// Enemy attack failed
						lives--;
						controlsEnabled = false;
						PlaySound(SND_looseLife);
						shakeCount = 5;

						// Remove life and play heart animation
						switch (lives) {
							case 2:
								sprites[0] = NewSprite((Rectangle){88, 13, 41, 14}, (Vector2){9, 18}, 1, false, 0.014, EnableControls, 0);
								sprites[1] = NewSprite((Rectangle){80, 0, 13, 12}, (Vector2){10, 19}, 8, false, 0.2, false, 0);
								sprites[2] = NewSprite((Rectangle){80, 0, 13, 12}, (Vector2){23, 19}, 1, false, 0.014, false, 0);
								sprites[3] = NewSprite((Rectangle){80, 0, 13, 12}, (Vector2){36, 19}, 1, false, 0.014, false, 0);
								break;
							case 1:
								sprites[0] = NewSprite((Rectangle){88, 13, 41, 14}, (Vector2){9, 18}, 1, false, 0.014, EnableControls, 0);
								sprites[2] = NewSprite((Rectangle){80, 0, 13, 12}, (Vector2){23, 19}, 8, false, 0.2, false, 0);
								sprites[3] = NewSprite((Rectangle){80, 0, 13, 12}, (Vector2){36, 19}, 1, false, 0.014, false, 0);
								break;
							case 0:
								pressA_scrollY = 50;
								sprites[0] = NewSprite((Rectangle){88, 13, 41, 14}, (Vector2){9, 18}, 1, false, 0.014, Gameover, 0);
								sprites[3] = NewSprite((Rectangle){80, 0, 13, 12}, (Vector2){36, 19}, 8, false, 0.2, false, 0);
								break;
						}
					}
				}
			}

			// Timer runs out
			if (controlsEnabled) timeLeft -= 0.06;
			if (timeLeft <= 0) {
				pressA_scrollY = 50;
				Gameover();
			}

			// Level completed
			if (!types[0] && !types[1] && !types[2] && !types[3]) {
				currentLevel += 6;
				if (currentLevel > 30) {

					// World completed
					currentLevel = STARTING_LEVEL;
					world++;
					
					// World music and animation
					switch (world) {
						case 2:
							StopMusicStream(MUS_world1);
							StopMusicStream(MUS_world2);
							PlayMusicStream(MUS_world2);
							WorldChangeAnim();
							break;
						case 3:
							StopMusicStream(MUS_world2);
							StopMusicStream(MUS_world3);
							PlayMusicStream(MUS_world3);
							WorldChangeAnim();
							break;
						case 4:
							StopMusicStream(MUS_world3);
							PlaySound(SND_final_attack);
							gameMode = GAMEMODE_WIN;
							sprites[20] = NewSprite((Rectangle){432, 0, 60, 60}, (Vector2){0, 0}, 8, false, 0.07, GameEnding, 0);
							break;
					}

				}
				ResetLevel();
			}

		} else if (gameMode == GAMEMODE_HELP) {

			if (controlsEnabled) {
				switch (GetKeyPressed()) {
					case KEY_ENTER: case KEY_A: case KEY_S: case KEY_K: case KEY_L:

						// Reset gameplay / Start game
						PlaySound(SND_click);
						StopMusicStream(MUS_world1);
						PlayMusicStream(MUS_world1);
						world = STARTING_WORLD;
						WorldChangeAnim();
						lightsOn = true;
						ResetLevel();
						gameMode = GAMEMODE_GAME;
				}
			}

		} else if (gameMode == GAMEMODE_GAMEOVER) {

			if (controlsEnabled) {
				switch (GetKeyPressed()) {
					case KEY_ENTER: case KEY_A: case KEY_S: case KEY_K: case KEY_L:
						currentLevel = STARTING_LEVEL;
						gameMode = GAMEMODE_HELP;
						StopSound(SND_gameover);
						PlaySound(SND_click);
						lives = 3;
				}
			}

			if (pressA_scrollY > 0) pressA_scrollY -= 0.4;

		} else if (gameMode == GAMEMODE_WIN) {

			UpdateTriangles();

		}

		// Always run
		UpdateSprites(sprites);

		// TEXTURE DRAW
		BeginTextureMode(target);
		
			// DRAW EVERYTHING HERE
			ClearBackground(COL_BLACK);

			if (gameMode == GAMEMODE_TITLE) {

				for (int i=0; i<circleArrLength; i++) {
					DrawCircle(circles[i].loc.x, circles[i].loc.y, circles[i].rad, COL_WHITE);
				}
				DrawTextureRec(TX_sprites, SP_logo.rec, SP_logo.loc, WHITE);
				DrawRectangle(0, 46, 60, 7, COL_BLACK);
				//DrawTextureRec(TX_sprites, SP_press_a.rec, SP_press_a.loc, WHITE);

			} else if (gameMode == GAMEMODE_HELP) {

				// Draw enemy types
				DrawTextureRec(TX_sprites, SP_types.rec, SP_types.loc, WHITE);

				// Temp draw letters
				DrawTextureRec(TX_sprites, SP_letter_A.rec, (Vector2){14, 30}, WHITE);
				DrawTextureRec(TX_sprites, SP_letter_S.rec, (Vector2){24, 30}, WHITE);
				DrawTextureRec(TX_sprites, SP_letter_K.rec, (Vector2){34, 30}, WHITE);
				DrawTextureRec(TX_sprites, SP_letter_L.rec, (Vector2){44, 30}, WHITE);

			} else if (gameMode == GAMEMODE_GAME) {

				if (lightsOn) DrawIconGrid(TX_sprites, animFrame, shakeVector);
				DrawTextureRec(TX_sprites, SP_hud.rec, SP_hud.loc, WHITE);

				// Draw target tracker
				for (int i=0; i<remainingTargets; i++)
				DrawTextureRec(TX_sprites, SP_targetRem.rec, (Vector2){1+(i*2), 53}, WHITE);

				// Draw time bar
				for (int i=0; i<timeLeft; i++)
				DrawTextureRec(TX_sprites, SP_timeDot.rec, (Vector2){2+i, 56}, WHITE);

			} else if (gameMode == GAMEMODE_GAMEOVER) {

				DrawTextureRec(TX_sprites, SP_gameover.rec, SP_gameover.loc, WHITE);
				DrawTextureRec(TX_sprites, SP_pressA.rec, (Vector2){SP_pressA.loc.x, SP_pressA.loc.y+pressA_scrollY}, WHITE);

			} else if (gameMode == GAMEMODE_WIN) {
				if (winAnimPlaying) {
					DrawTriangles();
				}
			}
			
			// Always draw
			DrawSprites(TX_sprites);
		
		EndTextureMode();

		// DRAW
        BeginDrawing();
		ClearBackground(BLACK);
		DrawTexturePro(target.texture, (Rectangle){0, 0, screenWidth, -screenHeight}, (Rectangle){playAreaX, 0, screenWidth*scale, screenHeight*scale}, (Vector2){0, 0}, 0, WHITE);
        EndDrawing();

    }

	// Unload Assets
	UnloadTexture(TX_sprites);
	UnloadSound(SND_bleep);
	UnloadSound(SND_looseLife);
	UnloadSound(SND_click);
	UnloadSound(SND_win);
	UnloadSound(SND_final_attack);
	UnloadSound(SND_gameover);
	UnloadSound(SND_title_music);
	UnloadMusicStream(MUS_world1);
	UnloadMusicStream(MUS_world2);
	UnloadMusicStream(MUS_world3);
	UnloadRenderTexture(target);

    CloseWindow();

    return 0;
}