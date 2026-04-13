#pragma once
#include <stdint.h>
#include "raylib.h"
#include "raymath.h"

// Global Constants
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define MAX_ENTITIES_AMOUNT 20000

// ~Begin of Enums
typedef enum EntityType : uint8_t {
    ENTITY_TYPE_UNDEFINED = 0,
    ENTITY_TYPE_ACTOR = 1,
    ENTITY_TYPE_PLAYER = 2
} EntityType;
// ~End of Enums

// ~Begin of Structs
typedef struct Actor{
    uint16_t id;
    bool bIsActive;
    Vector2 position;
    float speed;
} Actor;

typedef struct Entity{
    EntityType type;

    union
    {
        Actor actor;
    };
} Entity;

typedef struct GlobalVariables{
    Camera2D camera;

    Entity entities[MAX_ENTITIES_AMOUNT];
    uint16_t entityCount;
    uint16_t playerIndex;
} GlobalVariables;
// ~End of Structs

// Declaration of Global Variables
extern GlobalVariables globalVariables;

// The game main loop
int main(void);

// ~Begin of Core Implementation
void Core_InitGame();
void Core_ProcessInput();
void Core_UpdateGame(float deltaTime);
void Core_RenderGraphics();
void Core_CloseGame();

int Core_IsGameReadyToClose();
// ~End of Core Implementation

// ~Begin of Render Implementation
void Render_DrawMap();
void Render_DrawPlayer();
// ~End of Render Implementation

// ~Begin of Player Implementation
Camera2D Player_GenerateCamera();
Entity Player_GeneratePlayer();
void Player_ProcessMovement(Entity* player, float deltaTime);
// ~End of Player Implementation

// ~Helpers
inline static Entity* Global_GetPlayer()
{
    return &globalVariables.entities[globalVariables.playerIndex];
}