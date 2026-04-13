#include "main.h"
#include <assert.h>

//~ Begin of Assets Macros
#define DEFINE_ASSET_GETTER(Type, Name, Array, Count, IDType) \
    Type Assets_Get##Name(IDType id) { \
        assert(id < Count && "Accessing " #Name " out of bounds!"); \
        return globalVariables.assets.Array[id]; \
    }
//~ End of Assets Macros...

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
    
    // Initialize Audio and Assets
    InitAudioDevice();
    Assets_Init();
    
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
    Assets_Unload();
    CloseAudioDevice();
    CloseWindow();
}
int Core_IsGameReadyToClose()
{
    return WindowShouldClose();
}
// ~End of Core Implementation

// ~Begin of Assets Implementation
void Assets_Init()
{
    TraceLog(LOG_INFO, "ASSETS: Starting assets loading...");

    // Loading Sprites
    globalVariables.assets.sprites[ASSET_SPRITE_TYPE_PLAYER] = LoadTexture("assets/sprites/PlayerSheet.png");
    globalVariables.assets.sprites[ASSET_SPRITE_TYPE_GRASS]  = LoadTexture("assets/sprites/GrassTexture.png");
    globalVariables.assets.sprites[ASSET_SPRITE_TYPE_BAT]    = LoadTexture("assets/sprites/BatTexture.png");

    // Loading Sounds
    globalVariables.assets.sounds[ASSET_SOUND_TYPE_DAMAGE]        = LoadSound("assets/sounds/DamageAudio.mp3");
    globalVariables.assets.sounds[ASSET_SOUND_TYPE_EXPLOSION]     = LoadSound("assets/sounds/ExplosionAudio.ogg");
    globalVariables.assets.sounds[ASSET_SOUND_TYPE_LEVEL_UP]     = LoadSound("assets/sounds/LevelUpAudio.ogg");
    globalVariables.assets.sounds[ASSET_SOUND_TYPE_XP_GAIN]      = LoadSound("assets/sounds/XpGainAudio.ogg");
    globalVariables.assets.sounds[ASSET_SOUND_TYPE_PLAYER_DAMAGE] = LoadSound("assets/sounds/PlayerDamageAudio.ogg");

    // Loading Music
    globalVariables.assets.musics[ASSET_MUSIC_TYPE_COMBAT] = LoadMusicStream("assets/musics/CombatMusic.ogg");

    TraceLog(LOG_INFO, "ASSETS: All assets loaded successfully!");
}

DEFINE_ASSET_GETTER(Texture2D, Sprite, sprites, ASSET_SPRITE_TYPE_COUNT, AssetSpriteType)
DEFINE_ASSET_GETTER(Sound, Sound, sounds, ASSET_SOUND_TYPE_COUNT, AssetSoundType)
DEFINE_ASSET_GETTER(Music, Music, musics, ASSET_MUSIC_TYPE_COUNT, AssetMusicType)

void Assets_Unload()
{
    TraceLog(LOG_INFO, "ASSETS: Unloading assets...");

    for (int i = 0; i < ASSET_SPRITE_TYPE_COUNT; i++) UnloadTexture(globalVariables.assets.sprites[i]);
    for (int i = 0; i < ASSET_SOUND_TYPE_COUNT; i++)  UnloadSound(globalVariables.assets.sounds[i]);
    for (int i = 0; i < ASSET_MUSIC_TYPE_COUNT; i++)  UnloadMusicStream(globalVariables.assets.musics[i]);

    TraceLog(LOG_INFO, "ASSETS: All assets unloaded.");
}
// ~End of Assets Implementation

//~ Begin of Collision Implementation
void Collision_MapBorder(Entity* entity)
{
    float min = -MAP_HALF_SIZE + entity->radius;
    float max = MAP_HALF_SIZE - entity->radius;
    
    if (entity->position.x < min) entity->position.x = min;
    if (entity->position.x > max) entity->position.x = max;
    if (entity->position.y < min) entity->position.y = min;
    if (entity->position.y > max) entity->position.y = max;
}
//~ End of Collision Implementation

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
    player.visualType = VISUAL_TYPE_ANIMATED_SPRITE;
    player.animatedSprite.spriteID = ASSET_SPRITE_TYPE_PLAYER;
    player.animatedSprite.frameCount = 2; // Total of 3 frames, counting 0
    player.animatedSprite.currentFrame = 0;
    player.animatedSprite.frameTimer = 0.0f;
    player.animatedSprite.frameTime = 0.1f;
    player.animatedSprite.flipX = false;
    player.scale = (Vector2){ 2.0f, 2.0f };
    player.radius = 40.0f;

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

    Collision_MapBorder(player);

    // Update Camera Target
    globalVariables.camera.target = Vector2Lerp(globalVariables.camera.target, player->position, 10.0f * deltaTime);

    Player_AnimateMovement(player, deltaTime);
}
void Player_AnimateMovement(Entity* player, float deltaTime)
{
    if (Vector2Length(player->velocity) > 0)
    {
        player->animatedSprite.frameTimer += deltaTime;
        if (player->animatedSprite.frameTimer >= player->animatedSprite.frameTime)
        {
            player->animatedSprite.frameTimer = 0.0f;
            player->animatedSprite.currentFrame++;
            if (player->animatedSprite.currentFrame > player->animatedSprite.frameCount)
            {
                player->animatedSprite.currentFrame = 0;
            }
        }

        if (player->velocity.x > 0) player->animatedSprite.flipX = false;
        else if (player->velocity.x < 0) player->animatedSprite.flipX = true;
    }
    else
    {
        player->animatedSprite.currentFrame = 0;
        player->animatedSprite.frameTimer = 0.0f;
    }
}
// ~End of Player Implementation

// ~Begin of Render Implementation
void Render_DrawMap()
{
    Texture2D grass = Assets_GetSprite(ASSET_SPRITE_TYPE_GRASS);
    SetTextureWrap(grass, TEXTURE_WRAP_REPEAT);
    
    // Fill the map area with grass texture using wrapping - expanded to -6000 to cover camera edges
    Rectangle source = { 0, 0, 12000, 12000 };
    Vector2 position = { -6000, -6000 };
    DrawTextureRec(grass, source, position, WHITE);

    // Draw Map Borders (Red boundary)
    DrawRectangleLinesEx((Rectangle){ -MAP_HALF_SIZE, -MAP_HALF_SIZE, MAP_SIZE, MAP_SIZE }, 10.0f, RED);
}
void Render_DrawPlayer()
{
    Entity* player = Global_GetPlayer();
    if (player->bIsActive)
    {
        Texture2D texture = Assets_GetSprite(player->animatedSprite.spriteID);
        float frameWidth = (float)texture.width / (player->animatedSprite.frameCount + 1);
        
        Rectangle source = { 
            player->animatedSprite.currentFrame * frameWidth, 
            0, 
            player->animatedSprite.flipX ? -frameWidth : frameWidth, 
            (float)texture.height 
        };
        
        Rectangle dest = { 
            player->position.x, 
            player->position.y, 
            frameWidth * player->scale.x, 
            (float)texture.height * player->scale.y 
        };
        
        Vector2 origin = { (frameWidth * player->scale.x) / 2.0f, ((float)texture.height * player->scale.y) / 2.0f };
        
        DrawTexturePro(texture, source, dest, origin, 0, WHITE);
    }
}
// ~End of Render Implementation