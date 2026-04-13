#pragma once
#include <stdint.h>
#include "raylib.h"

// Global Constants
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

// ~Begin of Enums
typedef enum : uint8_t EntityType{
    ENTITY_TYPE_UNDEFINED = 0,
    ENTITY_TYPE_ACTOR = 1
} EntityType;
// ~End of Enums

// ~Begin of Structs
typedef struct Actor{
    int32_t id;
    bool bIsActive;
    Vector2 position;
} Actor;

typedef struct Object{
    EntityType type;

    union
    {
        Actor actor;
    };
} Object;

typedef struct GlobalVariables{
    Camera2D camera;
} GlobalVariables;
// ~End of Structs

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
Camera2D Render_GenerateCamera();
void Render_DrawMap();
// ~End of Render Implementation