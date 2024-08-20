#include "raylib.h"
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vcruntime.h>

#define SCREENWIGTH 640
#define SCREENHEIGTH 320

#define PLAYER_BASE_LEN 20
#define PlAYER_SPEED 200.0
#define PLAYER_OFFSET_BOTTOM 20.0
#define PLAYER_SHAPE_POINTS 8
#define PLAYER_BULLETS 50
#define ENEMEY_NUMBER 5

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

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
	bool active;
} Bullet;

typedef struct Enemy
{
	Vector2 position;
	float scale;
	bool active;
	Rectangle collider;
	int num_shape_points;
	Vector2 shape_points[14];
} Enemy;

typedef struct EnemeyWave
{
	int32_t enemy_number;
	Vector2 wave_position;
	Enemy *enemies;
	int32_t enemyType;
	bool is_moving;
	float move_timer;
} EnemyWave;

typedef enum 
{
	GAME,
	MAIN,
	PAUSE
} StateType;

typedef enum
{
	Alien,
	Boss
} EnemyType ;

typedef struct State 
{
	StateType state;
	Player *player;
	Bullet *playerBullets;
	Bullet *display_playerBullets;
	int bulletCount;
	EnemyWave *enemyWave;
} State;

//functions==================
//
void input(State *state);
void init(GameMemory *game, State *state, Player *player);
void update(State *state);
void drawPlayer(State *state);
void shootBullet(State *state);
void updateBullets(State *state);
void drawBullets(State *state);
void clearBullets(State *state);
Enemy* initSingularEnemey(void *gamememory, int32_t type, int index);
void drawEnemies(State *state);
void enemyWaveRandomMovement(EnemyWave *wave);

float random_float(float min, float max);
bool checkCollision(Rectangle a, Rectangle b);
float easeInOut(float t);
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
		state->bulletCount = 0;

		for (int i = 0; i < PLAYER_BULLETS; i++) {
		    state->playerBullets[i].active = false;
		    state->display_playerBullets[i].active = false;
		}

		state->enemyWave = (EnemyWave *)game->TransientStorage;
		state->enemyWave->enemyType = Alien;
		state->enemyWave->is_moving = false;
		if(state->enemyWave->enemyType == Alien)
		{
			state->enemyWave->enemy_number= ENEMEY_NUMBER;
		}
		state->enemyWave->wave_position = (Vector2){100.0f, 50.0f};
		state->enemyWave->enemies = (Enemy *)((uint8_t *)gameMemory.TransientStorage + sizeof(EnemyWave));

		for(int i = 0; i < state->enemyWave->enemy_number; i++)
		{
			Enemy *enemy = initSingularEnemey((uint8_t *)game->TransientStorage + sizeof(EnemyWave) + i * sizeof(Enemy), Alien, i);
			enemy->position = (Vector2){state->enemyWave->wave_position.x + i * 50.0f, state->enemyWave->wave_position.y};
			enemy->active = true;
			state->enemyWave->enemies[i] = *enemy;

		}

		game->IsInitialised = true;
	}
}

void update(State *state)
{
	while (!WindowShouldClose())
	{
		input(state);
		updateBullets(state);
		clearBullets(state);

		BeginDrawing();
			ClearBackground(RAYWHITE);
			drawPlayer(state);
			drawBullets(state);
			drawEnemies(state);
			enemyWaveRandomMovement(state->enemyWave);
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
	if (IsKeyPressed(KEY_SPACE)) 
	{
		//initialise bullets
		shootBullet(state);
	}

	// Update collider position
	state->player->collider.x = state->player->position.x - (PLAYER_BASE_LEN/2.0);
	state->player->collider.y = state->player->position.y - shipHeight;
}

void shootBullet(State *state)
{
	for(int i = 0; i < PLAYER_BULLETS; i++)	
	{
		if(!state->playerBullets[i].active)
		{
			state->playerBullets[i].position = (Vector2){ state->player->position.x, state->player->position.y - shipHeight };
			state->playerBullets[i].velocity = (Vector2){ 0, -500 }; // Bullets move up
			state->playerBullets[i].collider = (Rectangle){ state->playerBullets[i].position.x - 2.5f, state->playerBullets[i].position.y, 5, 10 };
			state->playerBullets[i].active = true;
			state->bulletCount++;
			break;
		}

	}
}

void updateBullets(State *state)
{
    for (int i = 0; i < PLAYER_BULLETS; i++) {
        if (state->playerBullets[i].active) {
            state->playerBullets[i].position.y += state->playerBullets[i].velocity.y * GetFrameTime();

            // Update collider position
            state->playerBullets[i].collider.x = state->playerBullets[i].position.x - 2.5f;
            state->playerBullets[i].collider.y = state->playerBullets[i].position.y;

            // Check if bullet is out of screen
            if (state->playerBullets[i].position.y < 0) {
                state->playerBullets[i].active = false;
                state->bulletCount--;
            } else {
                // Add to display_playerBullets
                state->display_playerBullets[i] = state->playerBullets[i];
            }
        }
    }
}

void clearBullets(State *state)
{
    for (int i = 0; i < PLAYER_BULLETS; i++) {
        if (!state->playerBullets[i].active) {
            state->display_playerBullets[i].active = false;
        }
    }
}

void drawBullets(State *state)
{
    for (int i = 0; i < PLAYER_BULLETS; i++) {
        if (state->display_playerBullets[i].active) {
            DrawRectangleRec(state->display_playerBullets[i].collider, RED);
        }
    }
}

Enemy* initSingularEnemey(void *gamememory, int32_t type, int index)
{
	
	Vector2 alien_shape_points[] = {
			(Vector2){0.0f, 0.0f},
			(Vector2){0.0f, -1.0f},
			(Vector2){-0.5f, -0.5f},
			(Vector2){-1.0f, 0.0f},
			(Vector2){-0.5f, 0.25f},
			(Vector2){0.0f, 0.25f},
			(Vector2){-0.25f, 0.25f},
			(Vector2){0.0f, 1.0f},
			(Vector2){0.25f, 0.25f},
			(Vector2){0.0f, 0.25f},
			(Vector2){0.5f, 0.25f},
			(Vector2){1.0f, 0.0f},
			(Vector2){0.5f, -0.5f},
			(Vector2){0.0f, -1.0f},
		};

	Vector2 boss_shape_points[] = {
		(Vector2){0.0f, 0.0f},
		(Vector2){1.0f, 0.0f},
		(Vector2){1.0f, 1.0f},
		(Vector2){0.0f, 1.0f},
		(Vector2){-1.0f, 1.0f},
		(Vector2){-1.0f, 0.0f},
		(Vector2){0.0f, -1.0f},
		(Vector2){1.0f, -1.0f},
	    };

	int num_points;
	Vector2 *shape_points;

	if (type == Alien)
	{
		num_points = sizeof(alien_shape_points) / sizeof(Vector2);
		shape_points = alien_shape_points;
	}
	else if (type == Boss)
	{
		num_points = sizeof(boss_shape_points) / sizeof(Vector2);
		shape_points = boss_shape_points;
	}
	else
	{
		return NULL; // Invalid enemy type
	}
	
	 // Calculate the memory needed for the Enemy struct plus the shape points
    size_t memory_needed = sizeof(Enemy) + num_points * sizeof(Vector2);

    // Allocate the required memory
    Enemy* enemy = (Enemy*)((uint8_t*)gamememory + index * memory_needed);

	// Ensure each enemy has its own space in memory
	//Enemy* enemy = (Enemy*)((uint8_t*)gamememory + index * sizeof(Enemy));
	enemy->scale = (type == Alien) ? 22.0f : 50.0f;
	enemy->active = false;
	enemy->position = (Vector2){0.0f, 0.0f};
	enemy->collider = (Rectangle){0, 0, 50, 50};
	enemy->num_shape_points = num_points;

	for (int i = 0; i < num_points; i++)
	{
		enemy->shape_points[i] = alien_shape_points[i];
	}

	return enemy;
}

void drawEnemies(State *state)
{
    for (int i = 0; i < state->enemyWave->enemy_number; i++)
    {
        if (state->enemyWave->enemies[i].active)
        {
            Enemy *enemy = &state->enemyWave->enemies[i];
            int num_points = enemy->num_shape_points;
            Vector2 scaledShape[num_points];
            for (int j = 0; j < num_points; j++)
            {
                scaledShape[j].x = (enemy->position.x) + (enemy->shape_points[j].x * enemy->scale);
                scaledShape[j].y = (enemy->position.y) + (enemy->shape_points[j].y * enemy->scale);
            }

            // Collider debugger
            // Adjust the collider position to the enemy's world position
            Rectangle adjustedCollider = {
		enemy->position.x - (enemy->collider.width) / 2,
                enemy->position.y - (enemy->collider.height) / 2,
                enemy->collider.width,
                enemy->collider.height
            };

	    enemy->collider = adjustedCollider;

	    DrawRectangleLinesEx(adjustedCollider, 2.0f, RED);
		

            DrawTriangleFan(scaledShape, num_points, GREEN);
        }
    }
}

float random_float(float min, float max)
{
	return ((float)rand() / RAND_MAX) * (max - min) + min;
}

bool checkCollision(Rectangle a, Rectangle b) 
{
    return (a.x < b.x + b.width &&
            a.x + a.width > b.x &&
            a.y < b.y + b.height &&
            a.y + a.height > b.y);
}


float easeInOut(float t)
{
	return -(cos(M_PI * t) - 1) / 2;
}

void enemyWaveRandomMovement(EnemyWave *wave)
{
	static Vector2 start_position = {0.0f, 0.0f};
	static Vector2 target_position = {0.0f, 0.0f};
	static float elapsed_time = 0.0f;
	static bool target_set = false;

	if (!wave->is_moving) 
	{
		wave->move_timer += GetFrameTime(); // Update the timer

		// Check if 5 seconds have passed
		if (wave->move_timer >= 5.0f) 
		{
		    wave->is_moving = true;
		    wave->move_timer = 0.0f; // Reset the timer
		    start_position = wave->wave_position;

			target_position.x = random_float(-100.0, 100.0);
			elapsed_time = 0.0f;
			target_set = true;
		}
	}

	if (wave->is_moving && target_set)
	{
		 elapsed_time += GetFrameTime();
        float t = elapsed_time / 1.0f; // Duration of 1 second for the ease-in-out movement
        if (t >= 1.0f) 
        {
            t = 1.0f;
            wave->is_moving = false; // Stop moving after reaching the target
            target_set = false; // Reset the target flag
        }
	// Apply ease-in-out to the interpolation
        float ease = easeInOut(t);
        Vector2 new_wave_position = {
            start_position.x + (target_position.x - start_position.x) * ease,
            start_position.y + (target_position.y - start_position.y) * ease
        };
	printf("vector x: %f, y:%f\n", wave->wave_position.x, wave->wave_position.y);
	//wave->wave_position.y = random_float(20.0, 50.0);
	
	 for (int i = 0; i < wave->enemy_number; i++) 
        {
            if (wave->enemies[i].active) 
            {
                float new_x = wave->enemies[i].position.x + (new_wave_position.x - wave->wave_position.x);
                float new_x_col = wave->enemies[i].collider.x + (new_wave_position.x - wave->wave_position.x);

                // Check if the new X position is within screen bounds
                if (new_x >= 0 && new_x_col + wave->enemies[i].collider.width <= SCREENWIGTH) 
                {
                    wave->enemies[i].position.x = new_x;
                    wave->enemies[i].collider.x = new_x_col;
                }
            }
        }

        wave->wave_position = new_wave_position;
	
	}
}
