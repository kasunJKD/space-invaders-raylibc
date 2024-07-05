#include "raylib.h"
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <vcruntime.h>

#define SCREENWIGTH 640
#define SCREENHEIGTH 320

#define PLAYER_BASE_LEN 20
#define PlAYER_SPEED 200.0
#define PLAYER_OFFSET_BOTTOM 20.0
#define PLAYER_SHAPE_POINTS 8
#define PLAYER_BULLETS 50

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)

typedef struct GameMemory 
{
	size_t PermanantStorageSize;
	void *PermanantStorage;

	size_t TransientStorageSize;
	void *TransientStorage;

	bool IsInitialised;
} GameMemory;

typedef struct Player
{
	Vector2 position;
	float speed;
	Rectangle collider;
	Vector2 playerShape[PLAYER_SHAPE_POINTS];
	float scale;
} Player;

typedef struct Bullet
{
	Vector2 position;
	Vector2 startPos;
	Vector2 velocity;
	Rectangle collider;
} Bullet;

typedef enum 
{
	GAME,
	MAIN,
	PAUSE
} StateType;

typedef enum
{
	FIRST = 10,
	SECOND = 20,
	THIRD = 50,
} LevelEnemyNumber;

typedef struct State 
{
	StateType state;
	Player *player;
	Bullet *playerBullets;
	Bullet *display_playerBullets;
} State;

//functions==================
//
void input(State *state);
void init(GameMemory *game, State *state, Player *player);
void update(State *state);
void drawPlayer(State *state);
//
//===========================


static float shipHeight = 0.0f;
static GameMemory gameMemory = {0};
int main(void)
{
	gameMemory.PermanantStorageSize = Megabytes(64);
	gameMemory.TransientStorageSize = Megabytes(128);
	gameMemory.PermanantStorage = malloc(gameMemory.PermanantStorageSize);
	gameMemory.TransientStorage = malloc(gameMemory.TransientStorageSize);
	gameMemory.IsInitialised = false;

	if (!gameMemory.PermanantStorage || !gameMemory.TransientStorage)
	{
		return -1; // Failed to allocate memory
	}

	Player *player = (Player *)gameMemory.PermanantStorage;
	State *state = (State *)((uint8_t *)gameMemory.PermanantStorage + sizeof(Player));

	init(&gameMemory, state, player);

	update(state);
	
	CloseWindow();

	free(gameMemory.PermanantStorage);
	free(gameMemory.TransientStorage);

	return 0;
}

void init(GameMemory *game, State *state, Player *player)
{
	if (!game->IsInitialised)
	{
		InitWindow(SCREENWIGTH, SCREENHEIGTH, "space invaders");

		shipHeight = (PLAYER_BASE_LEN/2.0) / tanf(20*DEG2RAD);
		player->position = (Vector2){SCREENWIGTH/2.0, SCREENHEIGTH - shipHeight};
		player->speed = PlAYER_SPEED;
		player->collider = (Rectangle){
			player->position.x - (PLAYER_BASE_LEN/2.0), 
			player->position.y - shipHeight,
			PLAYER_BASE_LEN,
			shipHeight
		};
		player->scale = 25.0f;
		Vector2 playershapeinit[PLAYER_SHAPE_POINTS] = {
			(Vector2){0.0f, 0.0f},
			(Vector2){0.0f, -1.0f},
			(Vector2){-0.5f, 0.0f},
			(Vector2){-0.25f, 0.25f},
			(Vector2){0.0f, 0.0f},
			(Vector2){0.25f, 0.25f},
			(Vector2){0.5f, 0.0f},
			(Vector2){0.0f, -1.0f},
		};

		for (int i = 0; i < PLAYER_SHAPE_POINTS; i++) {
			player->playerShape[i] = playershapeinit[i];
		}
		
		//state-data
		state->player = player;
		state->state = GAME;
		state->playerBullets = (Bullet *)((uint8_t *)gameMemory.PermanantStorage + sizeof(Player) + sizeof(State));
		state->display_playerBullets = (Bullet *)((uint8_t *)gameMemory.PermanantStorage + sizeof(Player) + sizeof(State) + (sizeof(Bullet) * PLAYER_BULLETS));

		game->IsInitialised = true;
	}
}

void update(State *state)
{
	while (!WindowShouldClose())
	{
		input(state);
		BeginDrawing();
			ClearBackground(RAYWHITE);
			drawPlayer(state);
		EndDrawing();
	}

}

void drawPlayer(State *state) 
{
	Vector2 scaledShape[PLAYER_SHAPE_POINTS];
	for (int i = 0; i < PLAYER_SHAPE_POINTS; i++)
	{
		scaledShape[i].x = (state->player->position.x) + (state->player->playerShape[i].x * state->player->scale);
		scaledShape[i].y = (state->player->position.y) + (state->player->playerShape[i].y * state->player->scale);
	}

	//colider debugger
	DrawRectangleLinesEx(state->player->collider, 2.0f,RED);

	DrawTriangleFan(scaledShape, PLAYER_SHAPE_POINTS, BLUE);
}

void input(State *state)
{
	if ((IsKeyDown(KEY_RIGHT)|| IsKeyDown(KEY_D)) 
		&& state->player->position.x <= SCREENWIGTH - PLAYER_BASE_LEN) 
	{
		state->player->position.x += state->player->speed * GetFrameTime();
	}
	if ((IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) 
		&& state->player->position.x >= 0 + PLAYER_BASE_LEN)
	{
		state->player->position.x -= state->player->speed * GetFrameTime();
	}
	if ((IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) 
		&& state->player->position.y >= 0 + shipHeight)
	{
		state->player->position.y -= state->player->speed * GetFrameTime();
	}
	if ((IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) 
		&& state->player->position.y <= SCREENHEIGTH - shipHeight)
	{
		state->player->position.y += state->player->speed * GetFrameTime();
	}

	// Update collider position
	state->player->collider.x = state->player->position.x - (PLAYER_BASE_LEN/2.0);
	state->player->collider.y = state->player->position.y - shipHeight;
}
