#include "core/game_loop.h"

int main(void)
{
    // Step 1: Initialize the game state and subsystems
    InitGame();

    // Step 2: The Main Loop
    while (!IsGameReadyToClose())
    {
        // 2a. Gather Inputs
        ProcessInput();

        // 2b. Advance Logic (using time elapsed this frame)
        UpdateLogic(GetFrameTime());

        // 2c. Render
        RenderGraphics();
    }

    // Step 3: Cleanup and exit
    CloseGame();

    return 0;
}
