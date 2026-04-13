#include "main.h"

// Definition of Global Variables
GlobalVariables globalVariables;

int main(void)
{
    // Step 1: Initialize the game state and subsystems
    Core_InitGame();

    // Step 2: The Main Loop
    while (!Core_IsGameReadyToClose())
    {
        // 2a. Gather Inputs
        Core_ProcessInput();

        // 2b. Advance Logic (using time elapsed this frame)
        Core_UpdateGame(GetFrameTime());

        // 2c. Render
        Core_RenderGraphics();
    }

    // Step 3: Cleanup and exit
    Core_CloseGame();

    return 0;
}

// ~Begin of Core Implementation
void Core_InitGame()
{
    // Init Global Variables
    globalVariables.entities[0] = Player_GeneratePlayer();
    
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_UNDECORATED);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Arcade Survivors Reworked");
    
    // Initialize things that need the window to be open first
    globalVariables.camera = Player_GenerateCamera();
    
    SetTargetFPS(240);
    DisableCursor();
}
void Core_ProcessInput()
{

}
void Core_UpdateGame(float deltaTime)
{
    // Update Player Movement
    Player_ProcessMovement(Global_GetPlayer(), deltaTime);
    
    // Update Camera to follow player (with a bit of "juice" lerp)
    Vector2 target = globalVariables.entities[globalVariables.playerIndex].actor.position;
    globalVariables.camera.target = Vector2Lerp(globalVariables.camera.target, target, 10.0f * deltaTime);
}
void Core_RenderGraphics()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);
    
    BeginMode2D(globalVariables.camera);

    Render_DrawMap();
    Render_DrawPlayer();

    EndMode2D();
    EndDrawing();
}
void Core_CloseGame()
{
    CloseWindow();
}
int Core_IsGameReadyToClose()
{
    return WindowShouldClose();
}
// ~End of Core Implementation


// ~Begin of Render Implementation
void Render_DrawMap()
{
    // Tmp basic floor
    DrawRectangle(-5000, -5000, 10000, 10000, RAYWHITE);
    for(int x = -5000; x < 5000; x += 500) DrawLine(x, -5000, x, 5000, LIGHTGRAY);
    for(int y = -5000; y < 5000; y += 500) DrawLine(-5000, y, 5000, y, LIGHTGRAY);

    // Draw Map Borders
    DrawRectangleLines(-5000, -5000, 10000, 10000, BLACK);
}
void Render_DrawPlayer()
{
    Entity* player = Global_GetPlayer();
    if (player->actor.bIsActive)
    {
        DrawCircleV(player->actor.position, 20, SKYBLUE);
        DrawCircleLinesV(player->actor.position, 20, BLUE);
    }
}
// ~End of Render Implementation

// ~Begin of Player Implementation
Camera2D Player_GenerateCamera()
{
    // Basic Camera Setup
    Camera2D camera = {0};
    camera.target = globalVariables.entities[globalVariables.playerIndex].actor.position;
    camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    return camera;
}
Entity Player_GeneratePlayer()
{
    Entity player = { 0 };
    player.type = ENTITY_TYPE_PLAYER;
    player.actor.bIsActive = true;
    player.actor.id = 0;
    player.actor.position = (Vector2){ 0, 0 };
    player.actor.speed = 400.0f;

    return player;
}
void Player_ProcessMovement(Entity* player, float deltaTime)
{
    Vector2 direction = { 0, 0 };

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    direction.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  direction.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  direction.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) direction.x += 1.0f;

    if (Vector2Length(direction) > 0)
    {
        direction = Vector2Normalize(direction);
        player->actor.position.x += direction.x * player->actor.speed * deltaTime;
        player->actor.position.y += direction.y * player->actor.speed * deltaTime;
    }
}
// ~End of Player Implementation