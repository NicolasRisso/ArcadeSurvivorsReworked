#include "main.h"

// Declaration of Global Variables
static GlobalVariables globalVariables;

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
    globalVariables.camera = Render_GenerateCamera();
    
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_UNDECORATED);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Arcade Survivors Reworked");
    SetTargetFPS(240);
    DisableCursor();
}
void Core_ProcessInput()
{

}
void Core_UpdateGame(float deltaTime)
{

}
void Core_RenderGraphics()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);
    
    BeginMode2D(globalVariables.camera);

    Render_DrawMap();

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
Camera2D Render_GenerateCamera()
{
    // Basic Camera Setup
    Camera2D camera = {0};
    camera.target = (Vector2){0, 0};
    camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    return camera;
}
void Render_DrawMap()
{
    // Tmp basic floor
    DrawRectangle(-5000, -5000, 10000, 10000, RAYWHITE);
    for(int x = -5000; x < 5000; x += 500) DrawLine(x, -5000, x, 5000, LIGHTGRAY);
    for(int y = -5000; y < 5000; y += 500) DrawLine(-5000, y, 5000, y, LIGHTGRAY);

    // Draw Map Borders
    DrawRectangleLines(-5000, -5000, 10000, 10000, BLACK);
}
// ~End of Render Implementation