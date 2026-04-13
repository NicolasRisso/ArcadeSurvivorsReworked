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
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_UNDECORATED);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Arcade Survivors Reworked");
    
    // Init Global Variables
    globalVariables.playerIndex = 0;
    globalVariables.entities[0] = Player_GeneratePlayer();
    globalVariables.playerStats = Player_GeneratePlayerStats();
    globalVariables.camera = Player_GenerateCamera();
    
    SetTargetFPS(240);
    DisableCursor();
}
void Core_ProcessInput()
{

}
void Core_UpdateGame(float deltaTime)
{
    Player_ProcessMovement(Global_GetPlayer(), deltaTime);
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

// ~Begin of Player Implementation
Camera2D Player_GenerateCamera()
{
    // Basic Camera Setup
    Camera2D camera = {0};
    camera.target = Global_GetPlayer()->position;
    camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    return camera;
}
Entity Player_GeneratePlayer()
{
    Entity player = { 0 };
    player.type = ENTITY_TYPE_PLAYER;
    player.bIsActive = true;
    player.id = 0;
    player.position = (Vector2){ 0, 0 };
    player.velocity = (Vector2){ 0, 0 };
    player.character.speed = 400.0f;
    player.character.health = 100.0f;
    player.character.maxHealth = 100.0f;

    return player;
}
PlayerStats Player_GeneratePlayerStats()
{
    PlayerStats playerStats = { 0 };
    playerStats.currentXP = 0.0f;
    playerStats.nextLevelXP = 100.0f;
    playerStats.healthMultiplier = 1.0f;
    playerStats.damageMultiplier = 1.0f;
    playerStats.attackSpeedMultiplier = 1.0f;
    playerStats.movementSpeedMultiplier = 1.0f;
    playerStats.sizeMultiplier = 1.0f;
    playerStats.lifeStealMultiplier = 0.0f;
    playerStats.xpMultiplier = 1.0f;

    return playerStats;
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
        player->velocity.x = direction.x * player->character.speed;
        player->velocity.y = direction.y * player->character.speed;
    }
    else
    {
        player->velocity = (Vector2){ 0, 0 };
    }

    // Apply Physics
    player->position.x += player->velocity.x * deltaTime;
    player->position.y += player->velocity.y * deltaTime;

    // Update Camera Target
    globalVariables.camera.target = Vector2Lerp(globalVariables.camera.target, player->position, 10.0f * deltaTime);
}
// ~End of Player Implementation

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
    if (player->bIsActive)
    {
        DrawCircleV(player->position, 20, SKYBLUE);
        DrawCircleLinesV(player->position, 20, BLUE);
    }
}
// ~End of Render Implementation