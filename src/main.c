#include "main.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>

//~ Begin of Assets Macros
#define DEFINE_ASSET_GETTER(Type, Name, Array, Count, IDType)                  \
  Type Assets_Get##Name(IDType id) {                                           \
    assert(id < Count && "Accessing " #Name " out of bounds!");                \
    return globalVariables.assets.Array[id];                                   \
  }
//~ End of Assets Macros...

// Definition of Global Variables
GlobalVariables globalVariables;

int main(void)
{
    Core_InitGame();

    while (!Core_IsGameReadyToClose()) {
        Core_ProcessInput();
        Core_UpdateGame(GetFrameTime());
        Core_RenderGraphics();
    }

    Core_CloseGame();

    return 0;
}

// ~Begin of Core Implementation
void Core_InitGame()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_UNDECORATED);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Arcade Survivors Reworked");
    SetRandomSeed(time(NULL));

    // Initialize Audio and Assets
    InitAudioDevice();
    Assets_Init();

    // Init Global Variables
    globalVariables.lastEntityIndex = 0;
    globalVariables.playerIndex = 0;
    globalVariables.deathAuraIndex = 65535;
    globalVariables.nextEntityId = 0;

    Entity player = Player_GeneratePlayer();
    Global_AddEntity(&player);

    globalVariables.playerStats = Player_GeneratePlayerStats();
    globalVariables.camera = Player_GenerateCamera();

    globalVariables.spawnerData = Spawner_GenerateSpawnerData();
    Weapon_GenerateWeaponLevels();
    Relic_GenerateRelicDefinition();
    Weapon_AddWeapon((WeaponType)GetRandomValue(0, WEAPON_TYPE_COUNT - 1));

    // Initialize Drops
    globalVariables.dropsDefinition.chanceToPowerUp = 0.01f;
    globalVariables.dropsDefinition.chanceToInstant = 0.02f;

    // Initialize Active Power-ups
    for (int i = 0; i < MAX_ACTIVE_POWERUPS; i++) {
        globalVariables.activePowerUps[i].bIsActive = false;
    }
    globalVariables.hudEventTimer = 0.0f;

    HUD_Init();
    
    // Start Music
    Music music = Assets_GetMusic(ASSET_MUSIC_TYPE_COMBAT);
    SetMusicVolume(music, 0.5f);
    PlayMusicStream(music);

    // Init Event State
    globalVariables.eventState.swarmCycleTimer = 0.0f;
    globalVariables.eventState.bossCycleTimer = 0.0f;
    globalVariables.eventState.activeEventTimer = 0.0f;
    globalVariables.eventState.activeEventType = EVENT_TYPE_NONE;

    // Init Level Up State
    globalVariables.levelUpState.bShowLevelUp = false;
    globalVariables.levelUpState.pendingCount = 0;
    
    SetTargetFPS(240);
    DisableCursor();
}
void Core_ProcessInput()
{
    Entity* player = Global_GetPlayer();
    if (!player || !player->bIsActive || player->character.bIsDead) {
        globalVariables.bShowInventory = false;
        return;
    }

    if (globalVariables.levelUpState.bShowLevelUp) return;

    Vector2 direction = {0, 0};

    // Keyboard Movement
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        direction.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        direction.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        direction.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        direction.x += 1.0f;

    // Gamepad Movement (Axis)
    if (IsGamepadAvailable(0)) {
        float ax = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        float ay = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        float deadzone = 0.2f;
        if (fabsf(ax) > deadzone) direction.x = ax;
        if (fabsf(ay) > deadzone) direction.y = ay;
    }

    if (Vector2Length(direction) > 0) {
        direction = Vector2Normalize(direction);
        player->velocity.x = direction.x * player->character.speed;
        player->velocity.y = direction.y * player->character.speed;
    } else {
        player->velocity = (Vector2){0, 0};
    }

    // Inventory Toggle
    if (IsKeyPressed(KEY_TAB)) {
        globalVariables.bShowInventory = !globalVariables.bShowInventory;
    }
    if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
        globalVariables.bShowInventory = !globalVariables.bShowInventory;
    }
}
void Core_UpdateGame(float deltaTime) {
    // Update Audio & Music (must update every frame for Raylib music stream)
    Audio_Update(deltaTime);

    // Update Special Events
    globalVariables.eventState.swarmCycleTimer += deltaTime;
    globalVariables.eventState.bossCycleTimer += deltaTime;
    
    if (globalVariables.eventState.activeEventTimer > 0) {
        globalVariables.eventState.activeEventTimer -= deltaTime;
        if (globalVariables.eventState.activeEventTimer <= 0) {
            globalVariables.eventState.activeEventType = EVENT_TYPE_NONE;
        }
    }

    if (globalVariables.eventState.swarmCycleTimer >= 120.0f) {
        globalVariables.eventState.swarmCycleTimer -= 120.0f;
        Event_TriggerEvent(EVENT_TYPE_SWARM);
    }
    if (globalVariables.eventState.bossCycleTimer >= 210.0f) {
        globalVariables.eventState.bossCycleTimer -= 210.0f;
        Event_TriggerEvent(EVENT_TYPE_BOSS);
    }

    if (globalVariables.levelUpState.bShowLevelUp) return;

    // Tick down HUD event
    if (globalVariables.hudEventTimer > 0) globalVariables.hudEventTimer -= deltaTime;

    // Tick down Active Power-ups
    for (int i = 0; i < MAX_ACTIVE_POWERUPS; i++) {
        if (globalVariables.activePowerUps[i].bIsActive) {
            globalVariables.activePowerUps[i].remainingTime -= deltaTime;
            if (globalVariables.activePowerUps[i].remainingTime <= 0) {
                globalVariables.activePowerUps[i].bIsActive = false;
            }
        }
    }

    Drop_ProcessPickUp();

    Player_ProcessMovement(Global_GetPlayer(), deltaTime);
    Enemy_ProcessAllMovement(deltaTime);
    Spawner_ProcessSpawnLogic(deltaTime);
    Weapon_ProcessAttack(deltaTime);
    Projectile_ProcessAllMovement(deltaTime);
    Popup_UpdateAll(deltaTime);
    XP_MoveCrystals(deltaTime);
    // Update Global Game Timer
    Global_UpdateGameTimer(deltaTime);
}
void Core_RenderGraphics() {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    BeginMode2D(globalVariables.camera);

    Render_DrawMap();
    Render_DrawAllEntitiesSorted();

    EndMode2D();

    HUD_Draw();
    if (globalVariables.bShowInventory) HUD_DrawInventory();
    if (globalVariables.levelUpState.bShowLevelUp) HUD_DrawLevelUp();

    EndDrawing();
}

void Core_CloseGame() {
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
const char* flashVS = 
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec4 vertexColor;\n"
    "out vec2 fragTexCoord;\n"
    "out vec4 fragColor;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    fragColor = vertexColor;\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

const char* flashFS = 
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "uniform float flashIntensity;\n"
    "void main() {\n"
    "    vec4 texelColor = texture(texture0, fragTexCoord);\n"
    "    vec4 baseColor = texelColor * colDiffuse * fragColor;\n"
    "    finalColor = mix(baseColor, vec4(1.0, 1.0, 1.0, baseColor.a), flashIntensity);\n"
    "}\n";

void Assets_Init() {
    TraceLog(LOG_INFO, "ASSETS: Starting assets loading...");

    // Loading Sprites
    globalVariables.assets.sprites[ASSET_SPRITE_TYPE_PLAYER] =
        LoadTexture("assets/sprites/PlayerSheet.png");
    globalVariables.assets.sprites[ASSET_SPRITE_TYPE_GRASS] =
        LoadTexture("assets/sprites/GrassTexture.png");
    globalVariables.assets.sprites[ASSET_SPRITE_TYPE_BAT] =
        LoadTexture("assets/sprites/BatTexture.png");

    // Loading Sounds
    globalVariables.assets.sounds[ASSET_SOUND_TYPE_DAMAGE] =
        LoadSound("assets/sounds/DamageAudio.mp3");
    globalVariables.assets.sounds[ASSET_SOUND_TYPE_EXPLOSION] =
        LoadSound("assets/sounds/ExplosionAudio.ogg");
    globalVariables.assets.sounds[ASSET_SOUND_TYPE_LEVEL_UP] =
        LoadSound("assets/sounds/LevelUpAudio.ogg");
    globalVariables.assets.sounds[ASSET_SOUND_TYPE_XP_GAIN] =
        LoadSound("assets/sounds/XpGainAudio.ogg");
    globalVariables.assets.sounds[ASSET_SOUND_TYPE_PLAYER_DAMAGE] =
        LoadSound("assets/sounds/PlayerDamageAudio.ogg");

    // Loading Music
    globalVariables.assets.musics[ASSET_MUSIC_TYPE_COMBAT] =
        LoadMusicStream("assets/music/CombatMusic.ogg");

    // Loading Shaders
    globalVariables.assets.flashShader = LoadShaderFromMemory(flashVS, flashFS);
    globalVariables.assets.flashIntensityLoc = GetShaderLocation(globalVariables.assets.flashShader, "flashIntensity");

    TraceLog(LOG_INFO, "ASSETS: All assets loaded successfully!");
}

DEFINE_ASSET_GETTER(Texture2D, Sprite, sprites, ASSET_SPRITE_TYPE_COUNT, AssetSpriteType)
DEFINE_ASSET_GETTER(Sound, Sound, sounds, ASSET_SOUND_TYPE_COUNT, AssetSoundType)
DEFINE_ASSET_GETTER(Music, Music, musics, ASSET_MUSIC_TYPE_COUNT, AssetMusicType)

void Assets_Unload()
{
    TraceLog(LOG_INFO, "ASSETS: Unloading assets...");

    for (int i = 0; i < ASSET_SPRITE_TYPE_COUNT; i++)
        UnloadTexture(globalVariables.assets.sprites[i]);
    for (int i = 0; i < ASSET_SOUND_TYPE_COUNT; i++)
        UnloadSound(globalVariables.assets.sounds[i]);
    for (int i = 0; i < ASSET_MUSIC_TYPE_COUNT; i++)
        UnloadMusicStream(globalVariables.assets.musics[i]);

    UnloadShader(globalVariables.assets.flashShader);

    TraceLog(LOG_INFO, "ASSETS: All assets unloaded.");
}
//~ End of Assets Implementation

//~ Begin of Audio Implementation
static float spammableTimers[3] = {0, 0, 0};

void Audio_PlaySoundVar(AssetSoundType type, bool bIsSpammable)
{
    Sound s = Assets_GetSound(type);
    
    if (bIsSpammable) {
        int slot = -1;
        for (int i = 0; i < 3; i++) {
            if (spammableTimers[i] <= 0) {
                slot = i;
                break;
            }
        }
        
        if (slot == -1) return; // All slots busy, drop the sound
        
        // Set timer (debounce)
        spammableTimers[slot] = 0.1f;
    }

    // Apply pitch modulation (+/- 7%)
    float pitch = 1.0f + ((float)GetRandomValue(-70, 70) / 1000.0f);
    SetSoundPitch(s, pitch);
    PlaySound(s);
}

void Audio_Update(float deltaTime)
{
    for (int i = 0; i < 3; i++) {
        if (spammableTimers[i] > 0) spammableTimers[i] -= deltaTime;
    }
    
    UpdateMusicStream(Assets_GetMusic(ASSET_MUSIC_TYPE_COMBAT));
}
// ~End of Audio Implementation

//~ Begin of Collision Implementation
void Collision_MapBorder(Entity *entity)
{
    float min = -MAP_HALF_SIZE + entity->radius;
    float max = MAP_HALF_SIZE - entity->radius;

    if (entity->position.x < min)
        entity->position.x = min;
    if (entity->position.x > max)
        entity->position.x = max;
    if (entity->position.y < min)
        entity->position.y = min;
    if (entity->position.y > max)
        entity->position.y = max;
}
//~ End of Collision Implementation

//~ Begin of Drop Implementation
void Drop_GenerateDrop(Vector2 pos)
{
    float powerUpChance = 0.01f - (floorf(globalVariables.gameTimer / 60.0f) * 0.001f);
    if (powerUpChance < 0.005f) powerUpChance = 0.005f;

    float instantChance = 0.02f - (floorf(globalVariables.gameTimer / 60.0f) * 0.002f);
    if (instantChance < 0.01f) instantChance = 0.01f;

    float roll = (float)GetRandomValue(0, 10000) / 10000.0f;

    DropType dropType;
    bool bSpawn = false;

    if (roll < powerUpChance) {
        dropType = DROP_TYPE_POWERUP;
        bSpawn = true;
    } else if (roll < (powerUpChance + instantChance)) {
        dropType = DROP_TYPE_INSTANT;
        bSpawn = true;
    }

    if (bSpawn) {
        Entity dropEntity = {0};
        dropEntity.type = ENTITY_TYPE_DROP;
        dropEntity.bIsActive = true;
        dropEntity.position = pos;
        dropEntity.radius = 20.0f;
        dropEntity.scale = (Vector2){1.0f, 1.0f};
        dropEntity.drop.dropType = dropType;

        if (dropType == DROP_TYPE_POWERUP) {
            dropEntity.drop.powerUpType = (PowerUpType)GetRandomValue(0, 3);
        } else {
            dropEntity.drop.instantDropType = (InstantDropType)GetRandomValue(0, 1);
        }
        Global_AddEntity(&dropEntity);
    }
}

void PowerUp_Trigger(PowerUpType type)
{
    const char* names[] = { "NUKE", "DOUBLE TROUBLE", "TIME FREEZE", "MAGNET" };
    sprintf(globalVariables.hudEventMessage, "%s ACTIVATED!", names[type]);
    globalVariables.hudEventTimer = 1.5f;

    switch (type) {
        case POWERUP_TYPE_NUKE:
            for (int i = 0; i < globalVariables.lastEntityIndex; i++) {
                Entity* enemy = &globalVariables.entities[i];
                if (enemy->bIsActive && enemy->type == ENTITY_TYPE_ENEMY) {
                    XP_GenerateXPCrystal(enemy->position, enemy->enemyCharacter.xpDropAmount);
                    Global_DestroyEntity(i);
                    i--;
                }
            }
            Audio_PlaySoundVar(ASSET_SOUND_TYPE_EXPLOSION, false);
            break;
        case POWERUP_TYPE_MAGNET:
            for (int i = 0; i < globalVariables.lastEntityIndex; i++) {
                Entity* crystal = &globalVariables.entities[i];
                if (crystal->bIsActive && crystal->type == ENTITY_TYPE_XP_CRYSTAL) {
                    crystal->xpCrystal.bIsMagnetized = true;
                }
            }
            break;
        case POWERUP_TYPE_DOUBLE_TROUBLE:
        case POWERUP_TYPE_TIME_FREEZE:
            globalVariables.activePowerUps[type].type = type;
            globalVariables.activePowerUps[type].remainingTime = 15.0f;
            globalVariables.activePowerUps[type].bIsActive = true;
            break;
    }
}

void Instant_Trigger(InstantDropType type)
{
    Entity* player = Global_GetPlayer();
    float healPercent = (type == INSTANT_DROP_TYPE_LIFE) ? 0.2f : 0.5f;
    player->character.health += player->character.maxHealth * healPercent;
    if (player->character.health > player->character.maxHealth) {
        player->character.health = player->character.maxHealth;
    }
    
    sprintf(globalVariables.hudEventMessage, "%s HEALED!", (type == INSTANT_DROP_TYPE_LIFE) ? "20%" : "50%");
    globalVariables.hudEventTimer = 1.5f;
}

void Drop_ProcessPickUp()
{
    Entity* player = Global_GetPlayer();
    if (!player || player->character.bIsDead) return;

    for (int i = 0; i < globalVariables.lastEntityIndex; i++) {
        Entity* dropEntity = &globalVariables.entities[i];
        if (!dropEntity->bIsActive || dropEntity->type != ENTITY_TYPE_DROP) continue;

        if (CheckCollisionCircles(player->position, player->radius, dropEntity->position, dropEntity->radius)) {
            if (dropEntity->drop.dropType == DROP_TYPE_POWERUP) {
                PowerUp_Trigger(dropEntity->drop.powerUpType);
            } else {
                Instant_Trigger(dropEntity->drop.instantDropType);
            }
            Global_DestroyEntity(i);
            i--;
        }
    }
}
//~ End of Drop Implementation

//~ Begin of Enemy Implementation
Entity Enemy_GenerateEnemy(EnemyType enemyType)
{
    Entity enemy = {0};
    enemy.type = ENTITY_TYPE_ENEMY;
    enemy.bIsActive = true;
    enemy.position = (Vector2){0, 0}; // Should be set by spawner
    enemy.velocity = (Vector2){0, 0};
    enemy.scale = (Vector2){2.0f, 2.0f};
    enemy.radius = 30.0f;

    enemy.visualType = VISUAL_TYPE_ANIMATED_STATIC_SPRITE;
    enemy.animatedStaticSprite.spriteID = ASSET_SPRITE_TYPE_BAT;
    enemy.animatedStaticSprite.flipX = false;
    enemy.animatedStaticSprite.animationDuration = 1.0f;
    enemy.animatedStaticSprite.animationTimer = (float)GetRandomValue(0, 1000) / 1000.0f;

    enemy.enemyCharacter.enemyType = enemyType;

    switch (enemyType) {
    case ENEMY_TYPE_NORMAL:
        enemy.enemyCharacter.health = 15.0f;
        enemy.enemyCharacter.speed = 150.0f;
        enemy.enemyCharacter.xpDropAmount = 10.0f;
        enemy.scale = (Vector2){0.75f, 0.75f};
        break;
    case ENEMY_TYPE_FAST:
        enemy.enemyCharacter.health = 10.0f;
        enemy.enemyCharacter.speed = 225.0f;
        enemy.enemyCharacter.xpDropAmount = 15.0f;
        enemy.scale = (Vector2){0.5f, 0.5f};
        break;
    case ENEMY_TYPE_TANK:
        enemy.enemyCharacter.health = 115.0f;
        enemy.enemyCharacter.speed = 80.0f;
        enemy.enemyCharacter.xpDropAmount = 50.0f;
        enemy.scale = (Vector2){1.5f, 1.5f};
        enemy.radius = 60.0f;
        break;
    case ENEMY_TYPE_BOSS:
        enemy.enemyCharacter.health = 3500.0f;
        enemy.enemyCharacter.speed = 120.0f;
        enemy.enemyCharacter.xpDropAmount = 5000.0f;
        enemy.enemyCharacter.damage = 50.0f;
        enemy.scale = (Vector2){3.0f, 3.0f};
        enemy.radius = 120.0f;
        break;
    }

    // Apply Progression Scaling
    int minutes = (int)(globalVariables.gameTimer / 60.0f);
    if (minutes > 0) {
        float hpScale = powf(1.25f, (float)minutes);
        float dmgScale = powf(1.15f, (float)minutes);
        float xpScale = powf(1.05f, (float)minutes);
        float speedScale = powf(1.03f, (float)minutes);

        enemy.enemyCharacter.health *= hpScale;
        enemy.enemyCharacter.damage *= dmgScale;
        enemy.enemyCharacter.xpDropAmount *= xpScale;
        enemy.enemyCharacter.speed *= speedScale;
    }

    // Set damage cases for others
    if (enemyType == ENEMY_TYPE_NORMAL) enemy.enemyCharacter.damage = 10.0f;
    else if (enemyType == ENEMY_TYPE_FAST) enemy.enemyCharacter.damage = 5.0f;
    else if (enemyType == ENEMY_TYPE_TANK) enemy.enemyCharacter.damage = 25.0f;

    // Scale damage after set if minutes > 0
    if (minutes > 0 && enemyType != ENEMY_TYPE_BOSS) {
        enemy.enemyCharacter.damage *= powf(1.15f, (float)minutes);
    }

    return enemy;
}
void Enemy_ProcessAllMovement(float deltaTime)
{
    Entity *player = Global_GetPlayer();

    bool bTimeFrozen = globalVariables.activePowerUps[POWERUP_TYPE_TIME_FREEZE].bIsActive;

    for (int i = 0; i < globalVariables.lastEntityIndex; i++) {
        Entity *current = &globalVariables.entities[i];
        if (!current->bIsActive || current->type != ENTITY_TYPE_ENEMY)
        continue;

        if (!bTimeFrozen) {
            // Simple AI: Move towards player
            Vector2 direction = Vector2Subtract(player->position, current->position);
            if (Vector2Length(direction) > 0) {
                direction = Vector2Normalize(direction);
                current->velocity.x = direction.x * current->enemyCharacter.speed;
                current->velocity.y = direction.y * current->enemyCharacter.speed;

                // Flip sprite based on movement direction
                if (current->velocity.x > 0) {
                    if (current->visualType == VISUAL_TYPE_ANIMATED_STATIC_SPRITE) current->animatedStaticSprite.flipX = false;
                    else current->sprite.flipX = false;
                } else if (current->velocity.x < 0) {
                    if (current->visualType == VISUAL_TYPE_ANIMATED_STATIC_SPRITE) current->animatedStaticSprite.flipX = true;
                    else current->sprite.flipX = true;
                }
            }

            current->position.x += current->velocity.x * deltaTime;
            current->position.y += current->velocity.y * deltaTime;
        }

        // Separation Pass (Avoidance)
        Vector2 separation = {0, 0};
        for (int j = 0; j < globalVariables.lastEntityIndex; j++) {
        if (i == j)
            continue;
        Entity *other = &globalVariables.entities[j];
        if (!other->bIsActive || other->type != ENTITY_TYPE_ENEMY)
            continue;

        float dist = Vector2Distance(current->position, other->position);
        float minDist = current->radius + other->radius;
        if (dist < minDist && dist > 0) {
            Vector2 push = Vector2Subtract(current->position, other->position);
            push = Vector2Normalize(push);
            float overlap = minDist - dist;
            separation.x += push.x * overlap * 1.5f;
            separation.y += push.y * overlap * 1.5f;
        }
        }
        current->position.x += separation.x * deltaTime * 10.0f;
        current->position.y += separation.y * deltaTime * 10.0f;

        // Player Contact Damage
        if (!player->character.bIsDead && player->character.invulnerableTimer <= 0) {
            float distToPlayer = Vector2Distance(current->position, player->position);
            if (distToPlayer < (current->radius + player->radius)) {
                float damage = current->enemyCharacter.damage;
                
                player->character.health -= damage;
                player->character.invulnerableTimer = 0.5f;
                player->character.flashTimer = 0.5f;
                Audio_PlaySoundVar(ASSET_SOUND_TYPE_PLAYER_DAMAGE, false);

                if (player->character.health <= 0) {
                    player->character.bIsDead = true;
                    player->character.deathFadeTimer = 2.0f;
                    player->character.health = 0;
                }
            }
        }

        // Tick down enemy flash timer
        if (current->enemyCharacter.flashTimer > 0) {
            current->enemyCharacter.flashTimer -= deltaTime;
        }

        // Update Static Sprite Animation
        if (current->visualType == VISUAL_TYPE_ANIMATED_STATIC_SPRITE) {
            current->animatedStaticSprite.animationTimer += deltaTime;
            if (current->animatedStaticSprite.animationTimer >= current->animatedStaticSprite.animationDuration) {
                current->animatedStaticSprite.animationTimer -= current->animatedStaticSprite.animationDuration;
            }
        }
    }
}
//~ End of Enemy Implementation

//~ Begin of Event Implementation
void Event_TriggerEvent(GameEventType type)
{
    globalVariables.eventState.activeEventType = type;
    globalVariables.eventState.activeEventTimer = 20.0f;

    if (type == EVENT_TYPE_BOSS) {
        // Spawn a Boss at a random location away from the player
        Entity boss = Enemy_GenerateEnemy(ENEMY_TYPE_BOSS);
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float dist = 800.0f;
        boss.position = Vector2Add(Global_GetPlayer()->position, (Vector2){ cosf(angle) * dist, sinf(angle) * dist });
        Collision_MapBorder(&boss);
        Global_AddEntity(&boss);
    }
}
//~ End of Event Implementation

//~ Begin of HUD Implementation
void HUD_DrawEvent()
{
    const char* msg = NULL;
    float alpha = 0.0f;

    // Check Swarm Warning (120s cycle)
    float swarmRemaining = 120.0f - globalVariables.eventState.swarmCycleTimer;
    if (swarmRemaining > 0 && swarmRemaining <= 5.0f) {
        msg = "SWARM INCOMING";
    }

    // Check Boss Warning (210s cycle)
    float bossRemaining = 210.0f - globalVariables.eventState.bossCycleTimer;
    if (bossRemaining > 0 && bossRemaining <= 5.0f) {
        msg = "BOSS INCOMING";
    }

    if (msg) {
        alpha = fabsf(sinf(GetTime() * 10.0f));
        int fontSize = 60;
        int textWidth = MeasureText(msg, fontSize);
        DrawText(msg, SCREEN_WIDTH/2 - textWidth/2, SCREEN_HEIGHT/2 - 100, fontSize, Fade(RED, alpha));
    }
}
void HUD_Init()
{
    globalVariables.gameTimer = 0.0f;
    globalVariables.playerStats.level = 1;
}
void HUD_UpdateData()
{
    // Currently, statistical tracking is handled directly in structures.
    // This could be used for text caching or localized animations in the future.
}
void HUD_Draw()
{
    // 1. XP Bar (Top of screen)
    float xpPercentage = globalVariables.playerStats.currentXP / globalVariables.playerStats.nextLevelXP;
    
    Color xpColor = BLUE;
    if (globalVariables.levelUpState.bShowLevelUp) {
        xpPercentage = 1.0f;
        float hue = (float)((int)(GetTime() * 300) % 360);
        xpColor = ColorFromHSV(hue, 0.8f, 0.9f);
    }

    DrawRectangle(0, 0, SCREEN_WIDTH, 30, DARKGRAY);
    DrawRectangle(0, 0, (int)(SCREEN_WIDTH * xpPercentage), 30, xpColor);

    const char *levelText = TextFormat("LEVEL %d", globalVariables.playerStats.level);
    int levelTextWidth = MeasureText(levelText, 20);
    DrawText(levelText, (SCREEN_WIDTH / 2) - (levelTextWidth / 2), 5, 20, WHITE);

    // 2. Health Bar (Left corner, below XP)
    int hpBarWidth = 350;
    int hpBarHeight = 35;
    int padding = 20;
    int hpY = 30 + padding;

    Entity *player = Global_GetPlayer();
    float hpPercentage = player->character.health / player->character.maxHealth;

    DrawRectangle(padding, hpY, hpBarWidth, hpBarHeight, DARKGRAY);
    DrawRectangle(padding, hpY, (int)(hpBarWidth * hpPercentage), hpBarHeight, RED);
    DrawRectangleLines(padding, hpY, hpBarWidth, hpBarHeight, BLACK);

    const char *hpText = TextFormat("%.0f / %.0f", player->character.health, player->character.maxHealth);
    int hpTextWidth = MeasureText(hpText, 20);
    DrawText(hpText, padding + (hpBarWidth / 2) - (hpTextWidth / 2), hpY + 7, 20, WHITE);

    // 3. Timer (Right corner)
    int minutes = (int)globalVariables.gameTimer / 60;
    int seconds = (int)globalVariables.gameTimer % 60;
    const char *timerText = TextFormat("%02d:%02d", minutes, seconds);
    int timerTextWidth = MeasureText(timerText, 40);
    int timerX = SCREEN_WIDTH - padding - timerTextWidth;
    int timerY = hpY;

    // Draw text with outline/border
    DrawText(timerText, timerX - 2, timerY - 2, 40, BLACK);
    DrawText(timerText, timerX + 2, timerY - 2, 40, BLACK);
    DrawText(timerText, timerX - 2, timerY + 2, 40, BLACK);
    DrawText(timerText, timerX + 2, timerY + 2, 40, BLACK);
    DrawText(timerText, timerX, timerY, 40, WHITE);
    HUD_DrawPowerUps();
    HUD_DrawPickupNotification();
    HUD_DrawEvent();
}

void HUD_DrawPowerUps()
{
    int activeCount = 0;
    for (int i = 0; i < MAX_ACTIVE_POWERUPS; i++) {
        if (globalVariables.activePowerUps[i].bIsActive) activeCount++;
    }
    if (activeCount == 0) return;

    int iconSize = 40;
    int spacing = 15;
    int totalWidth = (activeCount * iconSize) + ((activeCount - 1) * spacing);
    int startX = (SCREEN_WIDTH / 2) - (totalWidth / 2);
    int startY = 50;

    int drawn = 0;
    for (int i = 0; i < MAX_ACTIVE_POWERUPS; i++) {
        if (!globalVariables.activePowerUps[i].bIsActive) continue;

        ActivePowerUp* powerUp = &globalVariables.activePowerUps[i];
        int x = startX + (drawn * (iconSize + spacing));
        
        Color c = WHITE;
        const char* label = "";
        switch (powerUp->type) {
            case POWERUP_TYPE_DOUBLE_TROUBLE: c = RED; label = "DT"; break;
            case POWERUP_TYPE_TIME_FREEZE: c = WHITE; label = "TF"; break;
            default: break;
        }

        DrawRectangle(x, startY, iconSize, iconSize, ColorAlpha(c, 0.3f));
        DrawRectangleLines(x, startY, iconSize, iconSize, c);
        
        const char* timeText = TextFormat("%.0fs", powerUp->remainingTime);
        int timeW = MeasureText(timeText, 20);
        DrawText(timeText, x + (iconSize/2) - (timeW/2), startY + iconSize + 5, 20, WHITE);
        
        int labelW = MeasureText(label, 20);
        DrawText(label, x + (iconSize/2) - (labelW/2), startY + (iconSize/2) - 10, 20, c);

        drawn++;
    }
}

void HUD_DrawPickupNotification()
{
    if (globalVariables.hudEventTimer <= 0) return;

    // Use a large bold font style (using default font for now but sized up)
    int fontSize = 50;
    int textWidth = MeasureText(globalVariables.hudEventMessage, fontSize);
    int x = (SCREEN_WIDTH - textWidth) / 2;
    int y = 150; // Positioned below the timer/xp bar

    // Draw Drop Shadow
    DrawText(globalVariables.hudEventMessage, x + 3, y + 3, fontSize, (Color){ 0, 0, 0, (unsigned char)(255 * (globalVariables.hudEventTimer / 1.5f)) });
    // Draw Main Text (Yellow for prominence)
    DrawText(globalVariables.hudEventMessage, x, y, fontSize, (Color){ 255, 235, 59, (unsigned char)(255 * (globalVariables.hudEventTimer / 1.5f)) });
}

void HUD_DrawInventory()
{
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.85f));

    int padding = 150;
    int colWidth = (SCREEN_WIDTH - (padding * 2)) / 2;
    int startY = 200;

    // Left Column: Equipment
    int leftX = padding;
    DrawText("EQUIPMENT", leftX, startY - 60, 45, GOLD);
    
    int currentY = startY;
    DrawText("WEAPONS", leftX, currentY, 30, GRAY);
    currentY += 45;

    for (int i = 0; i < MAX_WEAPON_CAPACITY; i++) {
        WeaponData* weapon = &globalVariables.inventory.weaponDatas[i];
        if (weapon->level > 0) {
            DrawText(TextFormat("Lv.%d %s", weapon->level, Weapon_GetWeaponName(weapon->weaponType)), leftX + 20, currentY, 32, WHITE);
            currentY += 40;
        }
    }

    currentY += 30;
    DrawText("RELICS", leftX, currentY, 30, GRAY);
    currentY += 45;

    for (int i = 0; i < MAX_RELIC_CAPACITY; i++) {
        RelicData* relic = &globalVariables.inventory.relicDatas[i];
        if (relic->level > 0) {
            DrawText(TextFormat("Lv.%d %s", relic->level, Relic_GetRelicName(relic->relicType)), leftX + 20, currentY, 32, WHITE);
            currentY += 40;
        }
    }

    // Right Column: Player Stats
    int rightX = padding + colWidth;
    DrawText("PLAYER STATS", rightX, startY - 60, 45, GOLD);
    
    currentY = startY;
    PlayerStats* playerStats = &globalVariables.playerStats;
    Entity* player = Global_GetPlayer();

    DrawText(TextFormat("Health: %.0f / %.0f", player->character.health, player->character.maxHealth), rightX, currentY, 32, WHITE); currentY += 45;
    DrawText(TextFormat("Damage: %.0f%%", playerStats->damageMultiplier * 100.0f), rightX, currentY, 32, WHITE); currentY += 45;
    DrawText(TextFormat("Attack Speed: %.0f%%", playerStats->attackSpeedMultiplier * 100.0f), rightX, currentY, 32, WHITE); currentY += 45;
    DrawText(TextFormat("Move Speed: %.0f%%", playerStats->movementSpeedMultiplier * 100.0f), rightX, currentY, 32, WHITE); currentY += 45;
    DrawText(TextFormat("Area Size: %.0f%%", playerStats->sizeMultiplier * 100.0f), rightX, currentY, 32, WHITE); currentY += 45;
    DrawText(TextFormat("Life Steal: %.0f%%", playerStats->lifeStealMultiplier * 100.0f), rightX, currentY, 32, WHITE); currentY += 45;
    DrawText(TextFormat("XP Gain: %.0f%%", playerStats->xpMultiplier * 100.0f), rightX, currentY, 32, WHITE); currentY += 45;
}

void HUD_DrawLevelUp()
{
    // Brain Part: Handle Input
    bool selected = false;
    int choice = -1;

    if (IsKeyPressed(KEY_ONE)) { choice = 0; selected = true; }
    if (IsKeyPressed(KEY_TWO)) { choice = 1; selected = true; }
    if (IsKeyPressed(KEY_THREE)) { choice = 2; selected = true; }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) globalVariables.levelUpState.selectedIndex--;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) globalVariables.levelUpState.selectedIndex++;
    
    // Analog stick selection (single press feel)
    static float axisTimer = 0.0f;
    if (IsGamepadAvailable(0)) {
        float ax = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        if (fabsf(ax) > 0.5f && axisTimer <= 0) {
            if (ax > 0) globalVariables.levelUpState.selectedIndex++;
            else globalVariables.levelUpState.selectedIndex--;
            axisTimer = 0.2f;
        }
        if (axisTimer > 0) axisTimer -= GetFrameTime();
    }

    if (globalVariables.levelUpState.selectedIndex < 0) globalVariables.levelUpState.selectedIndex = 2;
    if (globalVariables.levelUpState.selectedIndex > 2) globalVariables.levelUpState.selectedIndex = 0;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
        choice = globalVariables.levelUpState.selectedIndex;
        selected = true;
    }

    if (selected && choice >= 0 && choice < 3) {
        UpgradeOption* opt = &globalVariables.levelUpState.options[choice];
        if (opt->type == UPGRADE_TYPE_WEAPON) {
            Weapon_AddWeapon(opt->weapon);
        } else if (opt->type == UPGRADE_TYPE_RELIC) {
            Relic_AddRelic(opt->relic);
        }

        globalVariables.levelUpState.pendingCount--;
        if (globalVariables.levelUpState.pendingCount > 0) {
            HUD_GenerateLevelUpOptions();
        } else {
            globalVariables.levelUpState.bShowLevelUp = false;
        }
        return;
    }

    // Body Part: Draw
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.85f));

    DrawText("LEVEL UP", SCREEN_WIDTH/2 - MeasureText("LEVEL UP", 80)/2, 100, 80, GOLD);

    int cardWidth = 450;
    int cardHeight = 600;
    int spacing = 60;
    int totalWidth = (cardWidth * 3) + (spacing * 2);
    int startX = (SCREEN_WIDTH - totalWidth) / 2;
    int startY = 250;

    for (int i = 0; i < 3; i++) {
        UpgradeOption* opt = &globalVariables.levelUpState.options[i];
        if (opt->type == (UpgradeType)-1) continue;

        int x = startX + i * (cardWidth + spacing);
        Rectangle cardRect = { (float)x, (float)startY, (float)cardWidth, (float)cardHeight };

        bool isSelected = (globalVariables.levelUpState.selectedIndex == i);
        Color cardBg = isSelected ? (Color){40, 40, 45, 255} : (Color){25, 25, 30, 255};
        Color border = isSelected ? GOLD : GRAY;

        DrawRectangleRec(cardRect, cardBg);
        DrawRectangleLinesEx(cardRect, isSelected ? 5.0f : 2.0f, border);

        // Get Name and Color
        const char* name = "";
        Color themeColor = WHITE;

        if (opt->type == UPGRADE_TYPE_WEAPON) {
            name = Weapon_GetWeaponName(opt->weapon);
            switch (opt->weapon) {
                case WEAPON_TYPE_CRYSTAL_WAND: themeColor = (Color){135, 206, 250, 255}; break; // Sky Blue
                case WEAPON_TYPE_FIREBALL_RING: themeColor = ORANGE; break;
                case WEAPON_TYPE_BOMB_SHOES: themeColor = GRAY; break;
                case WEAPON_TYPE_NATURE_SPIKES: themeColor = LIME; break;
                case WEAPON_TYPE_DEATH_AURA: themeColor = (Color){50, 50, 50, 255}; break;
            }
        } else {
            name = Relic_GetRelicName(opt->relic);
            switch (opt->relic) {
                case RELIC_TYPE_HEALTH: themeColor = PINK; break;
                case RELIC_TYPE_DAMAGE: themeColor = RED; break;
                case RELIC_TYPE_ATTACK_SPEED: themeColor = YELLOW; break;
                case RELIC_TYPE_MOVEMENT_SPEED: themeColor = BLUE; break;
                case RELIC_TYPE_SIZE: themeColor = PURPLE; break;
                case RELIC_TYPE_LIFE_STEAL: themeColor = (Color){128, 0, 32, 255}; break; // Wine
                case RELIC_TYPE_XP: themeColor = SKYBLUE; break;
            }
        }

        // Draw Name
        int nameSize = 35;
        int nameWidth = MeasureText(name, nameSize);
        DrawText(name, x + cardWidth/2 - nameWidth/2, startY + 40, nameSize, themeColor);

        // Draw Level
        const char* lvText = TextFormat("LEVEL %d", opt->level);
        DrawText(lvText, x + cardWidth/2 - MeasureText(lvText, 25)/2, startY + 90, 25, LIGHTGRAY);

        // Draw Type (Weapon or Relic)
        const char* typeText = (opt->type == UPGRADE_TYPE_WEAPON) ? "WEAPON" : "RELIC";
        DrawText(typeText, x + cardWidth/2 - MeasureText(typeText, 20)/2, startY + 120, 20, DARKGRAY);

        // Draw Description / Stats
        int descY = startY + 180;
        if (opt->level == 1) {
            const char* desc = "";
            if (opt->type == UPGRADE_TYPE_WEAPON) {
                switch (opt->weapon) {
                    case WEAPON_TYPE_CRYSTAL_WAND: desc = "Fires piercing crystals at the nearest enemy."; break;
                    case WEAPON_TYPE_FIREBALL_RING: desc = "Fires fireballs in 4 directions that explode on impact."; break;
                    case WEAPON_TYPE_BOMB_SHOES: desc = "Leaves bombs behind that explode after a short delay."; break;
                    case WEAPON_TYPE_NATURE_SPIKES: desc = "Spawns spikes under random nearby enemies."; break;
                    case WEAPON_TYPE_DEATH_AURA: desc = "Deals damage to enemies around the player."; break;
                }
            } else {
                switch (opt->relic) {
                    case RELIC_TYPE_HEALTH: desc = "Increases Max Health."; break;
                    case RELIC_TYPE_DAMAGE: desc = "Increases all damage dealt."; break;
                    case RELIC_TYPE_ATTACK_SPEED: desc = "Increases how fast you attack."; break;
                    case RELIC_TYPE_MOVEMENT_SPEED: desc = "Increases how fast you move."; break;
                    case RELIC_TYPE_SIZE: desc = "Increases the size of your attacks."; break;
                    case RELIC_TYPE_LIFE_STEAL: desc = "Heals you when you deal damage."; break;
                    case RELIC_TYPE_XP: desc = "Increases XP gained from crystals."; break;
                }
            }
            // Word wrap simple
            DrawText(TextSubtext(desc, 0, 30), x + 30, descY, 22, RAYWHITE);
            if (strlen(desc) > 30) DrawText(TextSubtext(desc, 30, 30), x + 30, descY + 30, 22, RAYWHITE);
        } else {
            // Bullet points of what changed
            if (opt->type == UPGRADE_TYPE_WEAPON) {
                WeaponLevelsDefinition* wlds = &globalVariables.InventoryDefinitions.weaponLevelsDefinition;
                WeaponDefinition* oldD = NULL;
                WeaponDefinition* newD = NULL;
                switch (opt->weapon) {
                    case WEAPON_TYPE_CRYSTAL_WAND: oldD = &wlds->crystalShard[opt->level-2]; newD = &wlds->crystalShard[opt->level-1]; break;
                    case WEAPON_TYPE_FIREBALL_RING: oldD = &wlds->fireballRing[opt->level-2]; newD = &wlds->fireballRing[opt->level-1]; break;
                    case WEAPON_TYPE_BOMB_SHOES: oldD = &wlds->bombShoes[opt->level-2]; newD = &wlds->bombShoes[opt->level-1]; break;
                    case WEAPON_TYPE_NATURE_SPIKES: oldD = &wlds->natureSpikes[opt->level-2]; newD = &wlds->natureSpikes[opt->level-1]; break;
                    case WEAPON_TYPE_DEATH_AURA: oldD = &wlds->deathAura[opt->level-2]; newD = &wlds->deathAura[opt->level-1]; break;
                }
                
                int bY = descY;
                if (newD->damage != oldD->damage) { DrawText(TextFormat("- Damage: %.1f -> %.1f", oldD->damage, newD->damage), x + 30, bY, 20, WHITE); bY += 30; }
                if (newD->delayBetweenAttacks != oldD->delayBetweenAttacks) { DrawText(TextFormat("- Cooldown: %.1fs -> %.1fs", oldD->delayBetweenAttacks, newD->delayBetweenAttacks), x + 30, bY, 20, WHITE); bY += 30; }
                if (newD->projectileAmount != oldD->projectileAmount) { DrawText(TextFormat("- Amount: %d -> %d", oldD->projectileAmount, newD->projectileAmount), x + 30, bY, 20, WHITE); bY += 30; }
                
                if (opt->weapon == WEAPON_TYPE_CRYSTAL_WAND) {
                    if (newD->crystal.penetration != oldD->crystal.penetration) DrawText(TextFormat("- Pierce: %d -> %d", oldD->crystal.penetration, newD->crystal.penetration), x + 30, bY, 20, WHITE);
                } else if (opt->weapon == WEAPON_TYPE_FIREBALL_RING) {
                    if (newD->fireball.explosionRadius != oldD->fireball.explosionRadius) DrawText(TextFormat("- Radius: %.0f -> %.0f", oldD->fireball.explosionRadius, newD->fireball.explosionRadius), x + 30, bY, 20, WHITE);
                } else if (opt->weapon == WEAPON_TYPE_BOMB_SHOES) {
                    if (newD->bombShoes.explosionRadius != oldD->bombShoes.explosionRadius) DrawText(TextFormat("- Radius: %.0f -> %.0f", oldD->bombShoes.explosionRadius, newD->bombShoes.explosionRadius), x + 30, bY, 20, WHITE);
                } else if (opt->weapon == WEAPON_TYPE_NATURE_SPIKES) {
                    if (newD->natureSpikes.rangeToSpawn != oldD->natureSpikes.rangeToSpawn) DrawText(TextFormat("- Range: %.0f -> %.0f", oldD->natureSpikes.rangeToSpawn, newD->natureSpikes.rangeToSpawn), x + 30, bY, 20, WHITE);
                } else if (opt->weapon == WEAPON_TYPE_DEATH_AURA) {
                    if (newD->deathAura.size != oldD->deathAura.size) DrawText(TextFormat("- Size: %.0f -> %.0f", oldD->deathAura.size, newD->deathAura.size), x + 30, bY, 20, WHITE);
                }
            } else {
                float mult = globalVariables.InventoryDefinitions.RelicDefinitions[opt->relic].multiplier;
                DrawText(TextFormat("- Effect: +%.0f%% per level", mult * 100.0f), x + 30, descY, 22, WHITE);
            }

        }
    }
}
void HUD_GenerateLevelUpOptions()
{
    // Collect all candidates
    UpgradeOption candidates[WEAPON_TYPE_COUNT + RELIC_TYPE_COUNT];
    int count = 0;

    // Check current weapon count
    int currentWeaponCount = 0;
    for (int i = 0; i < MAX_WEAPON_CAPACITY; i++) if (globalVariables.inventory.weaponDatas[i].level > 0) currentWeaponCount++;

    // Weapon Candidates
    for (int i = 0; i < WEAPON_TYPE_COUNT; i++) {
        WeaponType type = (WeaponType)i;
        int invIdx = -1;
        for (int j = 0; j < MAX_WEAPON_CAPACITY; j++) {
            if (globalVariables.inventory.weaponDatas[j].level > 0 && globalVariables.inventory.weaponDatas[j].weaponType == type) {
                invIdx = j; break;
            }
        }

        if (invIdx != -1) {
            // Already have it, check if not maxed
            if (globalVariables.inventory.weaponDatas[invIdx].level < MAX_WEAPON_LEVEL) {
                candidates[count].type = UPGRADE_TYPE_WEAPON;
                candidates[count].weapon = type;
                candidates[count].level = globalVariables.inventory.weaponDatas[invIdx].level + 1;
                count++;
            }
        } else if (currentWeaponCount < MAX_WEAPON_CAPACITY) {
            // New weapon
            candidates[count].type = UPGRADE_TYPE_WEAPON;
            candidates[count].weapon = type;
            candidates[count].level = 1;
            count++;
        }
    }

    // Check current relic count
    int currentRelicCount = 0;
    for (int i = 0; i < MAX_RELIC_CAPACITY; i++) if (globalVariables.inventory.relicDatas[i].level > 0) currentRelicCount++;

    // Relic Candidates
    for (int i = 0; i < RELIC_TYPE_COUNT; i++) {
        RelicType type = (RelicType)i;
        int invIdx = -1;
        for (int j = 0; j < MAX_RELIC_CAPACITY; j++) {
            if (globalVariables.inventory.relicDatas[j].level > 0 && globalVariables.inventory.relicDatas[j].relicType == type) {
                invIdx = j; break;
            }
        }

        if (invIdx != -1) {
            if (globalVariables.inventory.relicDatas[invIdx].level < MAX_RELIC_LEVEL) {
                candidates[count].type = UPGRADE_TYPE_RELIC;
                candidates[count].relic = type;
                candidates[count].level = globalVariables.inventory.relicDatas[invIdx].level + 1;
                count++;
            }
        } else if (currentRelicCount < MAX_RELIC_CAPACITY) {
            candidates[count].type = UPGRADE_TYPE_RELIC;
            candidates[count].relic = type;
            candidates[count].level = 1;
            count++;
        }
    }

    // Shuffle and pick 3
    for (int i = 0; i < 3; i++) {
        if (count > 0) {
            int idx = GetRandomValue(0, count - 1);
            globalVariables.levelUpState.options[i] = candidates[idx];
            // Remove from candidates to avoid duplicates
            candidates[idx] = candidates[count - 1];
            count--;
        } else {
            // No options left? (Should not happen if balance is correct)
            globalVariables.levelUpState.options[i].type = (UpgradeType)-1; 
        }
    }
    
    globalVariables.levelUpState.selectedIndex = 0;
}
//~ End of HUD Implementation

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
    Entity player = {0};
    player.type = ENTITY_TYPE_PLAYER;
    player.bIsActive = true;
    player.id = 0;
    player.position = (Vector2){0, 0};
    player.velocity = (Vector2){0, 0};
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
    player.scale = (Vector2){2.0f, 2.0f};
    player.radius = 40.0f;

    return player;
}
PlayerStats Player_GeneratePlayerStats()
{
    PlayerStats playerStats = {0};
    playerStats.currentXP = 0.0f;
    playerStats.nextLevelXP = 200.0f;
    playerStats.healthMultiplier = 1.0f;
    playerStats.damageMultiplier = 1.0f;
    playerStats.attackSpeedMultiplier = 1.0f;
    playerStats.movementSpeedMultiplier = 1.0f;
    playerStats.sizeMultiplier = 1.0f;
    playerStats.lifeStealMultiplier = 0.0f;
    playerStats.xpMultiplier = 1.0f;

    return playerStats;
}
void Player_ProcessMovement(Entity *player, float deltaTime)
{
    if (!player || !player->bIsActive) return;
    if (player->character.bIsDead) {
        if (player->character.deathFadeTimer > 0) player->character.deathFadeTimer -= deltaTime;
        return;
    }

    // Applying velocity set in Core_ProcessInput
    player->position.x += player->velocity.x * deltaTime;
    player->position.y += player->velocity.y * deltaTime;

    Collision_MapBorder(player);

    // Update Camera Target
    globalVariables.camera.target = Vector2Lerp(
        globalVariables.camera.target, player->position, 10.0f * deltaTime);

    // Tick down timers
    if (player->character.invulnerableTimer > 0) player->character.invulnerableTimer -= deltaTime;
    if (player->character.flashTimer > 0) player->character.flashTimer -= deltaTime;

    Player_AnimateMovement(player, deltaTime);
}
void Player_AnimateMovement(Entity *player, float deltaTime)
{
    if (Vector2Length(player->velocity) > 0) {
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
//~ End of Player Implementation

//~ Begin of Popup Implementation
Entity Popup_SpawnDamagePopup(Vector2 pos, float amount) {
    Entity popup = {0};
    popup.type = ENTITY_TYPE_DAMAGE_POPUP;
    popup.bIsActive = true;
    popup.position = pos;
    popup.scale = (Vector2){1.0f, 1.0f};
    popup.damagePopup.amount = amount;
    popup.damagePopup.timer = 0.0f;
    Global_AddEntity(&popup);
    return popup;
}
void Popup_UpdateAll(float deltaTime) {
    for (int i = 0; i < globalVariables.lastEntityIndex; i++) {
        Entity* popup = &globalVariables.entities[i];
        if (!popup->bIsActive || popup->type != ENTITY_TYPE_DAMAGE_POPUP) continue;

        popup->damagePopup.timer += deltaTime;
        if (popup->damagePopup.timer >= 0.7f) {
            Global_DestroyEntity(i);
            continue;
        }

        float t = popup->damagePopup.timer / 0.7f;
        if (popup->damagePopup.timer < 0.2f) {
            // 0 to 0.2: upward movement and scale 1 to 1.25
            float subt = popup->damagePopup.timer / 0.2f;
            popup->position.y -= 100.0f * deltaTime;
            popup->scale.x = 1.0f + (0.25f * subt);
            popup->scale.y = popup->scale.x;
        } else {
            // 0.2 to 0.7: scale 1.25 to 0.5
            float subt = (popup->damagePopup.timer - 0.2f) / 0.5f;
            float ease = (float)sin(subt * PI * 0.5f); // Ease Out
            popup->scale.x = 1.25f - (0.75f * ease);
            popup->scale.y = popup->scale.x;
        }
    }
}
//~ End of Popup Implementation

//~ Begin of Projectile Implementation
Entity Projectile_Spawn(ProjectileType type, Vector2 pos, Vector2 vel, float damage, float lifeTime, uint8_t penetration)
{
    Entity projectile = {0};
    projectile.type = ENTITY_TYPE_PROJECTILE;
    projectile.bIsActive = true;
    projectile.position = pos;
    projectile.velocity = vel;
    projectile.scale = (Vector2){1.0f, 1.0f};
    projectile.radius = 10.0f;
    projectile.projectile.projectileType = type;
    float finalDamage = damage * globalVariables.playerStats.damageMultiplier;
    if (globalVariables.activePowerUps[POWERUP_TYPE_DOUBLE_TROUBLE].bIsActive) finalDamage *= 2.0f;
    projectile.projectile.damage = finalDamage;
    projectile.projectile.lifeTime = lifeTime;
    projectile.projectile.penetration = penetration;
    if (type == PROJECTILE_TYPE_CRYSTAL_SHARD || type == PROJECTILE_TYPE_NATURE_SPIKE) {
        for (int i = 0; i < 16; i++) projectile.projectile.hitTracking.hitIds[i] = 65535;
    } else {
        projectile.projectile.explosive.explosionDamageMultiplier = 1.0f;
        projectile.projectile.explosive.explosionRadius = 0.0f;
    }
    if (type == PROJECTILE_TYPE_NATURE_SPIKE) projectile.projectile.timer = 0.0f; // Reset timer for first hit
    return projectile;
}
void Projectile_ProcessAllMovement(float deltaTime)
{
    Entity* player = Global_GetPlayer();
    for (int i = 0; i < globalVariables.lastEntityIndex; i++) {
        Entity* projectile = &globalVariables.entities[i];
        if (!projectile->bIsActive || projectile->type != ENTITY_TYPE_PROJECTILE) continue;

        projectile->projectile.lifeTime -= deltaTime;
        if (projectile->projectile.lifeTime <= 0) { Global_DestroyEntity(i); continue; }

        switch (projectile->projectile.projectileType) {
            case PROJECTILE_TYPE_NATURE_SPIKE:
                projectile->projectile.timer -= deltaTime;
                if (projectile->projectile.timer <= 0) {
                    projectile->projectile.timer = 0.2f;
                    for (int k = 0; k < 16; k++) projectile->projectile.hitTracking.hitIds[k] = 65535;
                }
                break;
            case PROJECTILE_TYPE_CRYSTAL_SHARD:
                projectile->position.x += projectile->velocity.x * deltaTime;
                projectile->position.y += projectile->velocity.y * deltaTime;
                break;
            case PROJECTILE_TYPE_FIREBALL:
                projectile->position.x += projectile->velocity.x * deltaTime;
                projectile->position.y += projectile->velocity.y * deltaTime;
                break;
            case PROJECTILE_TYPE_BOMB:
                projectile->projectile.timer -= deltaTime;
                if (projectile->projectile.timer <= 0) {
                    Entity explosion = Projectile_Spawn(PROJECTILE_TYPE_EXPLOSION, projectile->position, (Vector2){0,0}, projectile->projectile.damage, 0.2f, 255);
                    explosion.radius = projectile->radius;
                    Global_AddEntity(&explosion);
                    Global_DestroyEntity(i);
                    Audio_PlaySoundVar(ASSET_SOUND_TYPE_EXPLOSION, false);
                    continue;
                }
                break;
            case PROJECTILE_TYPE_DEATH_AURA: {
                projectile->position = player->position; 
                projectile->projectile.timer -= deltaTime;
                if (projectile->projectile.timer <= 0) {
                    uint8_t level = 1;
                    for (int w = 0; w < MAX_WEAPON_CAPACITY; w++) {
                        if (globalVariables.inventory.weaponDatas[w].level > 0 && globalVariables.inventory.weaponDatas[w].weaponType == WEAPON_TYPE_DEATH_AURA) {
                            level = globalVariables.inventory.weaponDatas[w].level;
                            break;
                        }
                    }
                    WeaponDefinition* weaponDef = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.deathAura[level-1];
                    projectile->projectile.timer = weaponDef->delayBetweenAttacks / globalVariables.playerStats.attackSpeedMultiplier;
                    
                    for (int j = 0; j < globalVariables.lastEntityIndex; j++) {
                        Entity* enemy = &globalVariables.entities[j];
                        if (enemy->bIsActive && enemy->type == ENTITY_TYPE_ENEMY) {
                            if (CheckCollisionCircles(projectile->position, projectile->radius, enemy->position, enemy->radius)) {
                                Global_DealDamageToEnemy(j, projectile->projectile.damage, true);
                            }
                        }
                    }
                }
            } break;
            default: break;
        }

        if (projectile->projectile.projectileType != PROJECTILE_TYPE_EXPLOSION && 
            projectile->projectile.projectileType != PROJECTILE_TYPE_BOMB &&
            projectile->projectile.projectileType != PROJECTILE_TYPE_DEATH_AURA) {
            
            for (int j = 0; j < globalVariables.lastEntityIndex; j++) {
                Entity* enemy = &globalVariables.entities[j];
                if (!enemy->bIsActive || enemy->type != ENTITY_TYPE_ENEMY) continue;

                if (CheckCollisionCircles(projectile->position, projectile->radius, enemy->position, enemy->radius)) {
                    bool alreadyHit = false;
                    if (projectile->projectile.projectileType == PROJECTILE_TYPE_CRYSTAL_SHARD || 
                        projectile->projectile.projectileType == PROJECTILE_TYPE_NATURE_SPIKE) {
                        for (int k = 0; k < 16; k++) {
                            if (projectile->projectile.hitTracking.hitIds[k] == enemy->id) { alreadyHit = true; break; }
                        }
                    }

                    if (!alreadyHit) {
                        float dmgApplied = projectile->projectile.damage;
                        bool isAOE = (projectile->projectile.projectileType == PROJECTILE_TYPE_NATURE_SPIKE);
                        Global_DealDamageToEnemy(j, dmgApplied, isAOE);

                        if (projectile->projectile.projectileType == PROJECTILE_TYPE_CRYSTAL_SHARD || 
                            projectile->projectile.projectileType == PROJECTILE_TYPE_NATURE_SPIKE) {
                            for (int k = 15; k > 0; k--) projectile->projectile.hitTracking.hitIds[k] = projectile->projectile.hitTracking.hitIds[k-1];
                            projectile->projectile.hitTracking.hitIds[0] = enemy->id;
                        }

                        if (projectile->projectile.projectileType == PROJECTILE_TYPE_FIREBALL) {
                             float expDmg = projectile->projectile.damage * projectile->projectile.explosive.explosionDamageMultiplier;
                             Entity explosion = Projectile_Spawn(PROJECTILE_TYPE_EXPLOSION, projectile->position, (Vector2){0,0}, expDmg, 0.2f, 255);
                             explosion.radius = projectile->projectile.explosive.explosionRadius; 
                             Global_AddEntity(&explosion);
                             Global_DestroyEntity(i);
                             Audio_PlaySoundVar(ASSET_SOUND_TYPE_EXPLOSION, false);
                             goto next_p;
                        }

                        if (projectile->projectile.projectileType != PROJECTILE_TYPE_DEATH_AURA && 
                            projectile->projectile.projectileType != PROJECTILE_TYPE_NATURE_SPIKE) {
                            projectile->projectile.penetration--;
                            if (projectile->projectile.penetration <= 0) { Global_DestroyEntity(i); goto next_p; }
                        } else if (projectile->projectile.projectileType == PROJECTILE_TYPE_NATURE_SPIKE) {
                            projectile->projectile.penetration--; 
                            if (projectile->projectile.penetration <= 0) { Global_DestroyEntity(i); goto next_p; }
                        }
                    }
                }
            }
        } else if (projectile->projectile.projectileType == PROJECTILE_TYPE_EXPLOSION) {
            for (int j = 0; j < globalVariables.lastEntityIndex; j++) {
                Entity* enemy = &globalVariables.entities[j];
                if (!enemy->bIsActive || enemy->type != ENTITY_TYPE_ENEMY) continue;
                if (CheckCollisionCircles(projectile->position, projectile->radius, enemy->position, enemy->radius)) {
                    Global_DealDamageToEnemy(j, projectile->projectile.damage, true);
                }
            }
        }
        next_p:;
    }
}
//~ End of Projectile Implementation

//~ Begin of Relic Implementation
void Relic_GenerateRelicDefinition() {
    globalVariables.InventoryDefinitions.RelicDefinitions[RELIC_TYPE_HEALTH].multiplier = 0.07f;
    globalVariables.InventoryDefinitions.RelicDefinitions[RELIC_TYPE_DAMAGE].multiplier = 0.08f;
    globalVariables.InventoryDefinitions.RelicDefinitions[RELIC_TYPE_ATTACK_SPEED].multiplier = 0.06f;
    globalVariables.InventoryDefinitions.RelicDefinitions[RELIC_TYPE_MOVEMENT_SPEED].multiplier = 0.10f;
    globalVariables.InventoryDefinitions.RelicDefinitions[RELIC_TYPE_SIZE].multiplier = 0.15f;
    globalVariables.InventoryDefinitions.RelicDefinitions[RELIC_TYPE_LIFE_STEAL].multiplier = 0.01f;
    globalVariables.InventoryDefinitions.RelicDefinitions[RELIC_TYPE_XP].multiplier = 0.09f;
}
void Relic_ApplyEffects() {
    Entity* player = Global_GetPlayer();
    if (!player) return;

    float oldMaxHealth = player->character.maxHealth;

    // Reset multipliers
    globalVariables.playerStats.healthMultiplier = 1.0f;
    globalVariables.playerStats.damageMultiplier = 1.0f;
    globalVariables.playerStats.attackSpeedMultiplier = 1.0f;
    globalVariables.playerStats.movementSpeedMultiplier = 1.0f;
    globalVariables.playerStats.sizeMultiplier = 1.0f;
    globalVariables.playerStats.lifeStealMultiplier = 0.0f;
    globalVariables.playerStats.xpMultiplier = 1.0f;

    for (int i = 0; i < MAX_RELIC_CAPACITY; i++) {
        RelicData* relic = &globalVariables.inventory.relicDatas[i];
        if (relic->level == 0) continue;

        float bonus = (float)relic->level * globalVariables.InventoryDefinitions.RelicDefinitions[relic->relicType].multiplier;

        switch (relic->relicType) {
            case RELIC_TYPE_HEALTH: globalVariables.playerStats.healthMultiplier += bonus; break;
            case RELIC_TYPE_DAMAGE: globalVariables.playerStats.damageMultiplier += bonus; break;
            case RELIC_TYPE_ATTACK_SPEED: globalVariables.playerStats.attackSpeedMultiplier += bonus; break;
            case RELIC_TYPE_MOVEMENT_SPEED: globalVariables.playerStats.movementSpeedMultiplier += bonus; break;
            case RELIC_TYPE_SIZE: globalVariables.playerStats.sizeMultiplier += bonus; break;
            case RELIC_TYPE_LIFE_STEAL: globalVariables.playerStats.lifeStealMultiplier += bonus; break;
            case RELIC_TYPE_XP: globalVariables.playerStats.xpMultiplier += bonus; break;
            default: break;
        }
    }

    // Apply Health changes
    player->character.maxHealth = 100.0f * globalVariables.playerStats.healthMultiplier;
    if (player->character.maxHealth > oldMaxHealth) {
        if (player->character.invulnerableTimer <= 0) {
            player->character.health += (player->character.maxHealth - oldMaxHealth);
        }
    }
    
    // Apply speed changes
    player->character.speed = 400.0f * globalVariables.playerStats.movementSpeedMultiplier;
}
void Relic_AddRelic(RelicType relicType) {
    for (int i = 0; i < MAX_RELIC_CAPACITY; i++) {
        if (globalVariables.inventory.relicDatas[i].level > 0 && 
            globalVariables.inventory.relicDatas[i].relicType == relicType) {
            if (globalVariables.inventory.relicDatas[i].level < MAX_RELIC_LEVEL) {
                globalVariables.inventory.relicDatas[i].level++;
                Relic_ApplyEffects();
            }
            return;
        }
    }

    // Add new relic
    for (int i = 0; i < MAX_RELIC_CAPACITY; i++) {
        if (globalVariables.inventory.relicDatas[i].level == 0) {
            globalVariables.inventory.relicDatas[i].relicType = relicType;
            globalVariables.inventory.relicDatas[i].level = 1;
            Relic_ApplyEffects();
            return;
        }
    }
}
const char* Relic_GetRelicName(RelicType relicType)
{
    switch (relicType) {
        case RELIC_TYPE_HEALTH: return "Health Relic";
        case RELIC_TYPE_DAMAGE: return "Damage Relic";
        case RELIC_TYPE_ATTACK_SPEED: return "Attack Speed Relic";
        case RELIC_TYPE_MOVEMENT_SPEED: return "Move Speed Relic";
        case RELIC_TYPE_SIZE: return "Size Relic";
        case RELIC_TYPE_LIFE_STEAL: return "Life Steal Relic";
        case RELIC_TYPE_XP: return "XP Relic";
        default: return "Unknown Relic";
    }
}
//~ End of Relic Implementation

//~ Begin of XP Implementation
void XP_GenerateXPCrystal(Vector2 position, float amount)
{
    Entity crystal = {0};
    crystal.type = ENTITY_TYPE_XP_CRYSTAL;
    crystal.bIsActive = true;
    crystal.position = position;
    crystal.velocity = (Vector2){0, 0};
    crystal.scale = (Vector2){1.0f, 1.0f};
    crystal.radius = 15.0f;
    crystal.visualType = VISUAL_TYPE_NONE;
    crystal.xpCrystal.amount = amount;
    crystal.xpCrystal.bIsMagnetized = false;
    Global_AddEntity(&crystal);
}
void XP_MoveCrystals(float deltaTime)
{
    Entity* player = Global_GetPlayer();
    if (!player || player->character.bIsDead) return;

    for (int i = 0; i < globalVariables.lastEntityIndex; i++) {
        Entity* crystal = &globalVariables.entities[i];
        if (!crystal->bIsActive || crystal->type != ENTITY_TYPE_XP_CRYSTAL) continue;

        float dist = Vector2Distance(crystal->position, player->position);
        
        if (!crystal->xpCrystal.bIsMagnetized && dist < 100.0f) {
            crystal->xpCrystal.bIsMagnetized = true;
        }

        if (crystal->xpCrystal.bIsMagnetized) {
            // Move DIRECTLY towards player
            Vector2 dir = Vector2Normalize(Vector2Subtract(player->position, crystal->position));
            
            // Speed up over time
            float accel = 1200.0f; 
            float currentSpeed = Vector2Length(crystal->velocity) + (accel * deltaTime);
            if (currentSpeed > 700.0f) currentSpeed = 700.0f;
            if (currentSpeed < 200.0f) currentSpeed = 200.0f; // Minimum move speed

            crystal->velocity = Vector2Scale(dir, currentSpeed);
            crystal->position = Vector2Add(crystal->position, Vector2Scale(crystal->velocity, deltaTime));
        }

        if (dist < 20.0f) {
            XP_GrantXP(crystal->xpCrystal.amount);
            Global_DestroyEntity(i);
        }
    }
}
void XP_GrantXP(float amount)
{
    globalVariables.playerStats.currentXP += amount * globalVariables.playerStats.xpMultiplier;
    Audio_PlaySoundVar(ASSET_SOUND_TYPE_XP_GAIN, true);

    while (globalVariables.playerStats.currentXP >= globalVariables.playerStats.nextLevelXP) {
        XP_LevelUp();
    }
}
void XP_LevelUp()
{
    globalVariables.playerStats.currentXP -= globalVariables.playerStats.nextLevelXP;
    globalVariables.playerStats.level++;
    
    // nextLevelXP = 200 * (1.07)^(level - 1)
    globalVariables.playerStats.nextLevelXP = 200.0f * powf(1.07f, (float)(globalVariables.playerStats.level - 1));
    
    Audio_PlaySoundVar(ASSET_SOUND_TYPE_LEVEL_UP, false);
    
    globalVariables.levelUpState.pendingCount++;
    if (!globalVariables.levelUpState.bShowLevelUp) {
        globalVariables.levelUpState.bShowLevelUp = true;
        HUD_GenerateLevelUpOptions();
    }
}
//~ End of XP Implementation

// ~Begin of Render Implementation
void Render_DrawMap()
{
    Texture2D grass = Assets_GetSprite(ASSET_SPRITE_TYPE_GRASS);
    SetTextureWrap(grass, TEXTURE_WRAP_REPEAT);

    // Fill the map area with grass texture using wrapping - expanded to -6000 to
    // cover camera edges
    Rectangle source = {0, 0, 12000, 12000};
    Vector2 position = {-6000, -6000};
    DrawTextureRec(grass, source, position, WHITE);

    // Draw Map Borders (Red boundary)
    DrawRectangleLinesEx(
        (Rectangle){-MAP_HALF_SIZE, -MAP_HALF_SIZE, MAP_SIZE, MAP_SIZE}, 10.0f,
        RED);
}
void Render_DrawEntity(Entity *entity)
{
    if (!entity || !entity->bIsActive)
        return;

    if (entity->type == ENTITY_TYPE_XP_CRYSTAL) {
        // Diamond shape (square rotated 0 degrees in DrawPoly gives vertex at top)
        DrawPoly(entity->position, 4, 8.0f * entity->scale.x, 0, BLUE);
        DrawPolyLinesEx(entity->position, 4, 8.0f * entity->scale.x, 0, 2.0f, SKYBLUE);
        return;
    }

    if (entity->type == ENTITY_TYPE_DROP) {
        Color baseColor = GREEN;
        Color outlineColor = LIME;

        if (entity->drop.dropType == DROP_TYPE_POWERUP) {
            switch (entity->drop.powerUpType) {
                case POWERUP_TYPE_NUKE: baseColor = YELLOW; outlineColor = GOLD; break;
                case POWERUP_TYPE_DOUBLE_TROUBLE: baseColor = RED; outlineColor = MAROON; break;
                case POWERUP_TYPE_MAGNET: baseColor = DARKBLUE; outlineColor = BLUE; break;
                case POWERUP_TYPE_TIME_FREEZE: baseColor = WHITE; outlineColor = LIGHTGRAY; break;
            }
        }

        DrawPoly(entity->position, 4, 12.0f * entity->scale.x, 0, baseColor);
        DrawPolyLinesEx(entity->position, 4, 12.0f * entity->scale.x, 0, 2.0f, outlineColor);
        return;
    }

    if (entity->visualType == VISUAL_TYPE_NONE) 
        return;

    Texture2D texture;
    if (entity->visualType == VISUAL_TYPE_ANIMATED_SPRITE) {
        texture = Assets_GetSprite(entity->animatedSprite.spriteID);
    } else if (entity->visualType == VISUAL_TYPE_ANIMATED_STATIC_SPRITE) {
        texture = Assets_GetSprite(entity->animatedStaticSprite.spriteID);
    } else {
        texture = Assets_GetSprite(entity->sprite.spriteID);
    }

    // Choose color tint
    Color tint = WHITE;
    if (entity->type == ENTITY_TYPE_ENEMY) {
        switch (entity->enemyCharacter.enemyType) {
        case ENEMY_TYPE_NORMAL:
        tint = WHITE;
        break;
        case ENEMY_TYPE_FAST:
        tint = ORANGE;
        break;
        case ENEMY_TYPE_TANK:
        tint = PURPLE;
        break;
        case ENEMY_TYPE_BOSS:
        tint = RED;
        break;
        }
    }

    Rectangle source = {0, 0, 0, 0};
    Vector2 origin = {0, 0};

    if (entity->visualType == VISUAL_TYPE_ANIMATED_SPRITE) {
        float frameWidth =
            (float)texture.width / (entity->animatedSprite.frameCount + 1);
        source =
            (Rectangle){entity->animatedSprite.currentFrame * frameWidth, 0,
                        entity->animatedSprite.flipX ? -frameWidth : frameWidth,
                        (float)texture.height};
        origin = (Vector2){(frameWidth * entity->scale.x) / 2.0f,
                        (float)texture.height * entity->scale.y};
    } else {
        bool flipX = (entity->visualType == VISUAL_TYPE_ANIMATED_STATIC_SPRITE) 
                     ? entity->animatedStaticSprite.flipX 
                     : entity->sprite.flipX;
        
        source = (Rectangle){0, 0,
                            flipX ? -(float)texture.width : (float)texture.width,
                            (float)texture.height};
        origin = (Vector2){((float)texture.width * entity->scale.x) / 2.0f,
                        (float)texture.height * entity->scale.y};
    }

    float bounceOffset = 0.0f;
    if (entity->visualType == VISUAL_TYPE_ANIMATED_STATIC_SPRITE) {
        float t = entity->animatedStaticSprite.animationTimer / entity->animatedStaticSprite.animationDuration;
        bounceOffset = sinf(t * 2.0f * PI) * 10.0f; // 10 pixel amplitude
    }

    Rectangle dest = {entity->position.x, entity->position.y + bounceOffset,
                        (source.width < 0 ? -source.width : source.width) *
                            entity->scale.x,
                        (float)texture.height * entity->scale.y};

    // Damage Flash Logic
    float flashIntensity = 0.0f;
    if (entity->type == ENTITY_TYPE_ENEMY && entity->enemyCharacter.flashTimer > 0) {
        flashIntensity = 1.0f;
    } else if (entity->type == ENTITY_TYPE_PLAYER && entity->character.flashTimer > 0) {
        // Flash every 0.1s
        if (((int)(entity->character.flashTimer * 10) % 2) == 0) flashIntensity = 1.0f;
    }

    // Death Fade Logic
    if (entity->type == ENTITY_TYPE_PLAYER && entity->character.bIsDead) {
        if (entity->character.deathFadeTimer <= 0) return; // Completely gone
        float alpha = entity->character.deathFadeTimer / 2.0f;
        tint = ColorAlpha(tint, alpha);
    }

    if (flashIntensity > 0.0f) {
        BeginShaderMode(globalVariables.assets.flashShader);
        SetShaderValue(globalVariables.assets.flashShader, globalVariables.assets.flashIntensityLoc, &flashIntensity, SHADER_UNIFORM_FLOAT);
    }

    DrawTexturePro(texture, source, dest, origin, 0, tint);

    if (flashIntensity > 0.0f) EndShaderMode();
}
void Render_DrawProjectile(Entity *entity)
{
    if (!entity || !entity->bIsActive || entity->type != ENTITY_TYPE_PROJECTILE)
        return;

    switch (entity->projectile.projectileType) {
    case PROJECTILE_TYPE_CRYSTAL_SHARD:
        DrawCircleV(entity->position, 10.0f * entity->scale.x, BLUE);
        DrawCircleLinesV(entity->position, 10.0f * entity->scale.x, SKYBLUE);
        break;
    case PROJECTILE_TYPE_FIREBALL:
        DrawCircleV(entity->position, 15.0f * entity->scale.x, ORANGE);
        DrawCircleLinesV(entity->position, 15.0f * entity->scale.x, RED);
        break;
    case PROJECTILE_TYPE_BOMB:
        DrawCircleV(entity->position, 20.0f * entity->scale.x, BLACK);
        DrawCircleLinesV(entity->position, 20.0f * entity->scale.x, GRAY);
        break;
    case PROJECTILE_TYPE_NATURE_SPIKE:
        DrawPoly(entity->position, 3, 30.0f * entity->scale.x, 0, LIME);
        DrawPolyLines(entity->position, 3, 30.0f * entity->scale.x, 0, GREEN);
        break;
    case PROJECTILE_TYPE_DEATH_AURA:
        DrawCircleV(entity->position, entity->radius, ColorAlpha(BLACK, 0.3f));
        break;
    case PROJECTILE_TYPE_EXPLOSION:
        DrawCircleV(entity->position, entity->radius, ColorAlpha(ORANGE, 0.5f));
        break;
    default:
        DrawCircleV(entity->position, 5.0f, PINK);
        break;
    }
}

#define BUCKET_COUNT 500
#define MAP_HEIGHT 10000.0f
#define MAP_Y_MIN -5000.0f

void Render_DrawAllEntitiesSorted()
{
    static int bucketCounts[BUCKET_COUNT];
    static int bucketOffsets[BUCKET_COUNT];
    static uint16_t sortedIndices[MAX_ENTITIES_AMOUNT];

    // Clear bucket counts
    for (int i = 0; i < BUCKET_COUNT; i++)
        bucketCounts[i] = 0;

    // Pass 1: Count entities per bucket
    for (int i = 0; i < globalVariables.lastEntityIndex; i++) {
        if (!globalVariables.entities[i].bIsActive || globalVariables.entities[i].type == ENTITY_TYPE_DAMAGE_POPUP)
            continue;

        float y = globalVariables.entities[i].position.y;
        int bucket = (int)((y - MAP_Y_MIN) / (MAP_HEIGHT / BUCKET_COUNT));

        if (bucket < 0)
            bucket = 0;
        if (bucket >= BUCKET_COUNT)
            bucket = BUCKET_COUNT - 1;

        bucketCounts[bucket]++;
    }

    // Prefix sum to get bucket start positions
    bucketOffsets[0] = 0;
    for (int i = 1; i < BUCKET_COUNT; i++) {
        bucketOffsets[i] = bucketOffsets[i - 1] + bucketCounts[i - 1];
    }

    // Pass 2: Fill sorted array
    // We use a temporary copy of offsets to keep track of insertion points
    static int currentOffsets[BUCKET_COUNT];
    for (int i = 0; i < BUCKET_COUNT; i++)
        currentOffsets[i] = bucketOffsets[i];

    for (int i = 0; i < globalVariables.lastEntityIndex; i++) {
        if (!globalVariables.entities[i].bIsActive || globalVariables.entities[i].type == ENTITY_TYPE_DAMAGE_POPUP)
            continue;

        float y = globalVariables.entities[i].position.y;
        int bucket = (int)((y - MAP_Y_MIN) / (MAP_HEIGHT / BUCKET_COUNT));

        if (bucket < 0)
            bucket = 0;
        if (bucket >= BUCKET_COUNT)
            bucket = BUCKET_COUNT - 1;

        sortedIndices[currentOffsets[bucket]] = i;
        currentOffsets[bucket]++;
    }

    // Pass 0: Draw Death Aura first if it exists
    if (globalVariables.deathAuraIndex < globalVariables.lastEntityIndex) {
        Render_DrawProjectile(&globalVariables.entities[globalVariables.deathAuraIndex]);
    }

    // Pass 3: Draw in order
    int totalActive = 0;
    for (int i = 0; i < BUCKET_COUNT; i++)
        totalActive += bucketCounts[i];

    for (int i = 0; i < totalActive; i++) {
        Entity* entity = &globalVariables.entities[sortedIndices[i]];
        if (entity->type == ENTITY_TYPE_PROJECTILE) {
            if (entity->projectile.projectileType == PROJECTILE_TYPE_DEATH_AURA) continue;
            Render_DrawProjectile(entity);
        } else {
            Render_DrawEntity(entity);
        }
    }

    // Pass 4: Draw Popups on top of everything else in world space
    for (int i = 0; i < globalVariables.lastEntityIndex; i++) {
        Entity* entity = &globalVariables.entities[i];
        if (!entity->bIsActive || entity->type != ENTITY_TYPE_DAMAGE_POPUP) continue;

        char dmgText[16];
        sprintf(dmgText, "%.0f", entity->damagePopup.amount);
        float fontSize = 20.0f * entity->scale.x;
        Vector2 size = MeasureTextEx(GetFontDefault(), dmgText, fontSize, 1.0f);
        Vector2 drawPos = { entity->position.x - size.x / 2.0f, entity->position.y - size.y / 2.0f };
        
        float alpha = 1.0f;
        if (entity->damagePopup.timer > 0.2f) {
            float t = (entity->damagePopup.timer - 0.2f) / 0.5f;
            alpha = 1.0f - (float)sin(t * PI * 0.5f); // Ease Out Alpha
        }
        Color textColor = ColorAlpha(YELLOW, alpha);
        Color outlineColor = ColorAlpha(BLACK, alpha);

        // Draw Outline
        DrawTextEx(GetFontDefault(), dmgText, (Vector2){drawPos.x + 1, drawPos.y + 1}, fontSize, 1.0f, outlineColor);
        DrawTextEx(GetFontDefault(), dmgText, (Vector2){drawPos.x - 1, drawPos.y - 1}, fontSize, 1.0f, outlineColor);
        DrawTextEx(GetFontDefault(), dmgText, (Vector2){drawPos.x + 1, drawPos.y - 1}, fontSize, 1.0f, outlineColor);
        DrawTextEx(GetFontDefault(), dmgText, (Vector2){drawPos.x - 1, drawPos.y + 1}, fontSize, 1.0f, outlineColor);
        
        // Draw Main Text
        DrawTextEx(GetFontDefault(), dmgText, drawPos, fontSize, 1.0f, textColor);
    }
}
// ~End of Render Implementation

// ~Begin of Spawner Implementation
SpawnerData Spawner_GenerateSpawnerData()
{
    SpawnerData data = {0};
    data.delayBetweenSpawns = 1.5f;
    data.spawnTimer = data.delayBetweenSpawns;

    // Default Definitions
    // 0: Normal Bat Single Spawn (Common)
    data.spawnsDefinitions[0] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_NORMAL,
                            .spawnType = SPAWN_TYPE_SINGLE,
                            .amountToSpawnRange = {1, 1},
                            .distanceToSpawnRange = {800, 1000},
                            .chanceToSpawn = 100, // Weighted value
                            .Difficulty = 0.0f};

    // 1: Fast Bat Cluster (Medium)
    data.spawnsDefinitions[1] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_FAST,
                            .spawnType = SPAWN_TYPE_CLUSTER,
                            .amountToSpawnRange = {5, 8},
                            .distanceToSpawnRange = {900, 1100},
                            .chanceToSpawn = 30,
                            .Difficulty = 0.0f};

    // 2: Normal Bat Around (Medium)
    data.spawnsDefinitions[2] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_NORMAL,
                            .spawnType = SPAWN_TYPE_AROUND,
                            .amountToSpawnRange = {10, 15},
                            .distanceToSpawnRange = {1000, 1200},
                            .chanceToSpawn = 20,
                            .Difficulty = 0.0f};

    // 3: Tank Bat Single (Rare)
    data.spawnsDefinitions[3] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_TANK,
                            .spawnType = SPAWN_TYPE_SINGLE,
                            .amountToSpawnRange = {1, 1},
                            .distanceToSpawnRange = {1100, 1300},
                            .chanceToSpawn = 10,
                            .Difficulty = 15.0f};

    // 4: Fast Bat Line (Medium)
    data.spawnsDefinitions[4] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_FAST,
                            .spawnType = SPAWN_TYPE_LINE,
                            .amountToSpawnRange = {5, 10},
                            .distanceToSpawnRange = {900, 1100},
                            .chanceToSpawn = 25,
                            .Difficulty = 0.0f};

    // 5: Normal Bat Cluster (Common)
    data.spawnsDefinitions[5] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_NORMAL,
                            .spawnType = SPAWN_TYPE_CLUSTER,
                            .amountToSpawnRange = {11, 18},
                            .distanceToSpawnRange = {900, 1100},
                            .chanceToSpawn = 35,
                            .Difficulty = 0.0f};

    // 6: Normal Bat Line (Common)
    data.spawnsDefinitions[6] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_NORMAL,
                            .spawnType = SPAWN_TYPE_LINE,
                            .amountToSpawnRange = {12, 19},
                            .distanceToSpawnRange = {900, 1100},
                            .chanceToSpawn = 35,
                            .Difficulty = 0.0f};

    // --- NEW SELECTIONS (Difficulty 7-13) ---

    // 7: Fast Cluster (Aggressive) - Difficulty 5
    data.spawnsDefinitions[7] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_FAST,
                            .spawnType = SPAWN_TYPE_CLUSTER,
                            .amountToSpawnRange = {15, 25},
                            .distanceToSpawnRange = {800, 1000},
                            .chanceToSpawn = 40,
                            .Difficulty = 5.0f};

    // 8: Normal + Fast Mix (Around) - Difficulty 10
    data.spawnsDefinitions[8] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_FAST, // Selection logic picks one type for the whole spawn
                            .spawnType = SPAWN_TYPE_AROUND,
                            .amountToSpawnRange = {20, 30},
                            .distanceToSpawnRange = {1000, 1200},
                            .chanceToSpawn = 30,
                            .Difficulty = 10.0f};

    // 9: Tank Pair - Difficulty 25
    data.spawnsDefinitions[9] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_TANK,
                            .spawnType = SPAWN_TYPE_CLUSTER,
                            .amountToSpawnRange = {2, 3},
                            .distanceToSpawnRange = {1100, 1300},
                            .chanceToSpawn = 15,
                            .Difficulty = 25.0f};

    // 10: Fast Circle (Surrounding) - Difficulty 20
    data.spawnsDefinitions[10] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_FAST,
                            .spawnType = SPAWN_TYPE_AROUND,
                            .amountToSpawnRange = {30, 45},
                            .distanceToSpawnRange = {700, 900},
                            .chanceToSpawn = 25,
                            .Difficulty = 20.0f};

    // 11: Normal Giant Line - Difficulty 35
    data.spawnsDefinitions[11] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_NORMAL,
                            .spawnType = SPAWN_TYPE_LINE,
                            .amountToSpawnRange = {40, 60},
                            .distanceToSpawnRange = {1000, 1200},
                            .chanceToSpawn = 20,
                            .Difficulty = 35.0f};

    // 12: Tank Cluster (Dangerous) - Difficulty 45
    data.spawnsDefinitions[12] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_TANK,
                            .spawnType = SPAWN_TYPE_CLUSTER,
                            .amountToSpawnRange = {5, 8},
                            .distanceToSpawnRange = {1200, 1400},
                            .chanceToSpawn = 10,
                            .Difficulty = 45.0f};

    // 13: Fast Overlap (Hyper Cluster) - Difficulty 60
    data.spawnsDefinitions[13] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_FAST,
                            .spawnType = SPAWN_TYPE_CLUSTER,
                            .amountToSpawnRange = {50, 80},
                            .distanceToSpawnRange = {900, 1100},
                            .chanceToSpawn = 30,
                            .Difficulty = 60.0f};

    return data;
}
void Spawner_ProcessSpawnLogic(float deltaTime)
{
    Entity* player = Global_GetPlayer();
    if (player->character.bIsDead) return;

    // Update Difficulty: +10 per minute
    globalVariables.spawnerData.currentDifficulty = (globalVariables.gameTimer / 60.0f) * 10.0f;

    globalVariables.spawnerData.spawnTimer -= deltaTime;
    if (globalVariables.spawnerData.spawnTimer > 0)
        return;

    // Calculate Current Spawner Delay based on Progression and Events
    int minutes = (int)(globalVariables.gameTimer / 60.0f);
    float spawnerSpeedScale = powf(1.15f, (float)minutes);
    float eventMultiplier = 1.0f;
    
    if (globalVariables.eventState.activeEventType == EVENT_TYPE_SWARM) {
        eventMultiplier = 3.0f; // +200%
    } else if (globalVariables.eventState.activeEventType == EVENT_TYPE_BOSS) {
        eventMultiplier = 1.5f; // +50%
    }

    float actualDelay = (globalVariables.spawnerData.delayBetweenSpawns / spawnerSpeedScale) / eventMultiplier;

    // Reset Timer
    globalVariables.spawnerData.spawnTimer = actualDelay;

    // Phase 1: Weighted Selection (Filtering by Difficulty)
    int totalWeight = 0;
    float currentDiff = globalVariables.spawnerData.currentDifficulty;

    for (int i = 0; i < MAX_SPAWN_DEFINITION; i++) {
        if (currentDiff >= globalVariables.spawnerData.spawnsDefinitions[i].Difficulty) {
            totalWeight += globalVariables.spawnerData.spawnsDefinitions[i].chanceToSpawn;
        }
    }

    if (totalWeight <= 0)
        return;

    int roll = GetRandomValue(0, totalWeight - 1);
    int cumulativeWeight = 0;
    SpawnDefinition *selectedDef = NULL;

    for (int i = 0; i < MAX_SPAWN_DEFINITION; i++) {
        if (currentDiff < globalVariables.spawnerData.spawnsDefinitions[i].Difficulty) 
            continue;

        cumulativeWeight += globalVariables.spawnerData.spawnsDefinitions[i].chanceToSpawn;
        if (roll < cumulativeWeight) {
            selectedDef = &globalVariables.spawnerData.spawnsDefinitions[i];
            break;
        }
    }

    if (!selectedDef) return;

    // Phase 2: Execution
    uint16_t amount =
        Helper_GetRandomUint16InRange(selectedDef->amountToSpawnRange);
    float distance =
        Helper_GetRandomFloatInRange(selectedDef->distanceToSpawnRange);

    // Random Direction
    float baseAngle = (float)GetRandomValue(0, 360) * (PI / 180.0f);
    Vector2 spawnPos = {player->position.x + cosf(baseAngle) * distance,
                        player->position.y + sinf(baseAngle) * distance};

    switch (selectedDef->spawnType) {
    case SPAWN_TYPE_SINGLE: {
        Entity enemy = Enemy_GenerateEnemy(selectedDef->enemyType);
        enemy.position = spawnPos;
        Global_AddEntity(&enemy);
    } break;

    case SPAWN_TYPE_CLUSTER: {
        for (int i = 0; i < amount; i++) {
        Entity enemy = Enemy_GenerateEnemy(selectedDef->enemyType);
        // Spread slightly
        enemy.position.x = spawnPos.x + (float)GetRandomValue(-100, 100);
        enemy.position.y = spawnPos.y + (float)GetRandomValue(-100, 100);
        Global_AddEntity(&enemy);
        }
    } break;

    case SPAWN_TYPE_LINE: {
        Vector2 toSpawn = Vector2Subtract(spawnPos, player->position);
        Vector2 lineDir = {-toSpawn.y, toSpawn.x}; // Perpendicular to player vector
        lineDir = Vector2Normalize(lineDir);

        for (int i = 0; i < amount; i++) {
        Entity enemy = Enemy_GenerateEnemy(selectedDef->enemyType);
        // Center the line on spawnPos
        float offset = (i - (amount / 2.0f)) * 70.0f;
        enemy.position.x = spawnPos.x + lineDir.x * offset;
        enemy.position.y = spawnPos.y + lineDir.y * offset;
        Global_AddEntity(&enemy);
        }
    } break;

    case SPAWN_TYPE_AROUND: {
        for (int i = 0; i < amount; i++) {
        float angle = (float)i * (2.0f * PI / (float)amount);
        Entity enemy = Enemy_GenerateEnemy(selectedDef->enemyType);
        enemy.position.x = player->position.x + cosf(angle) * distance;
        enemy.position.y = player->position.y + sinf(angle) * distance;
        Global_AddEntity(&enemy);
        }
    } break;
    }
}
// ~End of Spawner Implementation

//~ Begin of Weapon Implementation
void Weapon_GenerateWeaponLevels()
{
    // Crystal Wand
    for (int i = 0; i < MAX_WEAPON_LEVEL; i++) {
        WeaponDefinition* weaponDef = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.crystalShard[i];
        weaponDef->damage = 15.1f + (i * 5.0f);
        weaponDef->delayBetweenAttacks = 4.0f;
        weaponDef->projectileAmount = 1 + (i / 2);
        weaponDef->crystal.penetration = 2 + (i);
        if (i == MAX_WEAPON_LEVEL - 1) {
            weaponDef->projectileAmount = 8;
            weaponDef->crystal.penetration = 16;
        }
    }

    // Fireball Ring
    for (int i = 0; i < MAX_WEAPON_LEVEL; i++) {
        WeaponDefinition* weaponDef = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.fireballRing[i];
        weaponDef->damage = 30.0f + (i * 8.0f);
        weaponDef->delayBetweenAttacks = 5.0f;
        weaponDef->projectileAmount = 1;
        if (i + 1 >= 5) weaponDef->projectileAmount = 2;
        if (i + 1 >= 9) weaponDef->projectileAmount = 3;
        if (i + 1 >= 13) weaponDef->projectileAmount = 4;
        if (i + 1 >= 15) weaponDef->projectileAmount = 6;
        weaponDef->fireball.explosionRadius = 100.0f + (i * 10.0f);
        weaponDef->fireball.explosionDamageMultipler = 0.5f + (i * 0.1f);
    }

    // Bomb Shoes
    for (int i = 0; i < MAX_WEAPON_LEVEL; i++) {
        WeaponDefinition* weaponDef = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.bombShoes[i];
        weaponDef->damage = 50.0f + (i * 35.0f);
        weaponDef->delayBetweenAttacks = 8.0f - (i * 0.428f); 
        if (weaponDef->delayBetweenAttacks < 2.0f) weaponDef->delayBetweenAttacks = 2.0f;
        weaponDef->bombShoes.delayToExplode = 3.0f - (i * 0.071f); 
        weaponDef->bombShoes.explosionRadius = 100.0f + (i * 25.0f);
    }

    // Nature Spikes
    for (int i = 0; i < MAX_WEAPON_LEVEL; i++) {
        WeaponDefinition* weaponDef = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.natureSpikes[i];
        weaponDef->damage = 10.0f + (i * 4.0f);
        weaponDef->delayBetweenAttacks = 5.5f - (i * 0.285f); 
        if (weaponDef->delayBetweenAttacks < 2.0f) weaponDef->delayBetweenAttacks = 2.0f;
        weaponDef->projectileAmount = 1 + (i / 7); 
        weaponDef->natureSpikes.spikeDuration = 2.0f + (i * 0.142f); 
        weaponDef->natureSpikes.rangeToSpawn = 400.0f + (i * 40.0f);
        weaponDef->natureSpikes.spikeMaxDamage = 100 + (i * 80); 
    }

    // Death Aura
    for (int i = 0; i < MAX_WEAPON_LEVEL; i++) {
        WeaponDefinition* weaponDef = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.deathAura[i];
        weaponDef->damage = 5.1f + (i * 1.78f); 
        weaponDef->delayBetweenAttacks = 0.25f - (i * 0.0089f); 
        weaponDef->deathAura.size = 150.0f + (i * 30.0f);
    }
}
bool Weapon_AddWeapon(WeaponType weaponType)
{
    for (int i = 0; i < MAX_WEAPON_CAPACITY; i++) {
        if (globalVariables.inventory.weaponDatas[i].level > 0 && 
            globalVariables.inventory.weaponDatas[i].weaponType == weaponType) {
            if (globalVariables.inventory.weaponDatas[i].level < MAX_WEAPON_LEVEL) {
                globalVariables.inventory.weaponDatas[i].level++;
            }
            return true;
        }
    }

    for (int i = 0; i < MAX_WEAPON_CAPACITY; i++) {
        if (globalVariables.inventory.weaponDatas[i].level == 0) {
            globalVariables.inventory.weaponDatas[i].weaponType = weaponType;
            globalVariables.inventory.weaponDatas[i].level = 1;
            globalVariables.inventory.weaponDatas[i].attackTimer = 0.1f;
            globalVariables.inventory.weaponDatas[i].burstRemaining = 0;
            return true;
        }
    }
    return false;
}
void Weapon_ProcessAttack(float deltaTime)
{
    Entity* player = Global_GetPlayer();
    if (!player || player->character.bIsDead) return;

    for (int i = 0; i < MAX_WEAPON_CAPACITY; i++) {
        WeaponData* weapon = &globalVariables.inventory.weaponDatas[i];
        if (weapon->level == 0) continue;

        WeaponDefinition* weaponDef = NULL;
        switch (weapon->weaponType) {
            case WEAPON_TYPE_CRYSTAL_WAND: weaponDef = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.crystalShard[weapon->level-1]; break;
            case WEAPON_TYPE_FIREBALL_RING: weaponDef = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.fireballRing[weapon->level-1]; break;
            case WEAPON_TYPE_BOMB_SHOES: weaponDef = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.bombShoes[weapon->level-1]; break;
            case WEAPON_TYPE_NATURE_SPIKES: weaponDef = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.natureSpikes[weapon->level-1]; break;
            case WEAPON_TYPE_DEATH_AURA: weaponDef = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.deathAura[weapon->level-1]; break;
        }
        if (!weaponDef) continue;

        weapon->attackTimer -= deltaTime;

        if (weapon->weaponType == WEAPON_TYPE_CRYSTAL_WAND && weapon->burstRemaining > 0) {
            weapon->burstTimer -= deltaTime;
            if (weapon->burstTimer <= 0) {
                float minDist = 99999.0f;
                int targetIdx = -1;
                for (int j = 0; j < globalVariables.lastEntityIndex; j++) {
                    if (globalVariables.entities[j].bIsActive && globalVariables.entities[j].type == ENTITY_TYPE_ENEMY) {
                        float dist = Vector2Distance(player->position, globalVariables.entities[j].position);
                        if (dist < minDist) { minDist = dist; targetIdx = j; }
                    }
                }

                if (targetIdx != -1) {
                    Vector2 dir = Vector2Normalize(Vector2Subtract(globalVariables.entities[targetIdx].position, player->position));
                    Entity projectile = Projectile_Spawn(PROJECTILE_TYPE_CRYSTAL_SHARD, player->position, Vector2Scale(dir, 600.0f), weaponDef->damage, 3.0f, weaponDef->crystal.penetration);
                    Global_AddEntity(&projectile);
                    weapon->burstRemaining--;
                    weapon->burstTimer = 0.1f;
                }
            }
        }

        float attackSpeedMult = globalVariables.playerStats.attackSpeedMultiplier;
        if (globalVariables.activePowerUps[POWERUP_TYPE_DOUBLE_TROUBLE].bIsActive) attackSpeedMult *= 2.0f;

        if (weapon->attackTimer <= 0) {
            weapon->attackTimer = weaponDef->delayBetweenAttacks / attackSpeedMult;

            switch (weapon->weaponType) {
                case WEAPON_TYPE_CRYSTAL_WAND:
                    weapon->burstRemaining = weaponDef->projectileAmount;
                    weapon->burstTimer = 0.0f;
                    break;
                case WEAPON_TYPE_FIREBALL_RING: {
                    Vector2 dirs[4] = {{0,-1}, {0,1}, {-1,0}, {1,0}};
                    for (int d = 0; d < 4; d++) {
                        for (int amt = 0; amt < weaponDef->projectileAmount; amt++) {
                             Vector2 offsetDir = Vector2Rotate(dirs[d], (amt - (weaponDef->projectileAmount-1)/2.0f) * 0.2f);
                             Entity projectile = Projectile_Spawn(PROJECTILE_TYPE_FIREBALL, player->position, Vector2Scale(offsetDir, 400.0f), weaponDef->damage, 4.0f, 1);
                             projectile.projectile.explosive.explosionRadius = weaponDef->fireball.explosionRadius; 
                             projectile.projectile.explosive.explosionDamageMultiplier = weaponDef->fireball.explosionDamageMultipler;
                             Global_AddEntity(&projectile);
                        }
                    }
                } break;
                case WEAPON_TYPE_BOMB_SHOES: {
                    Entity projectile = Projectile_Spawn(PROJECTILE_TYPE_BOMB, player->position, (Vector2){0,0}, weaponDef->damage, 10.0f, 1);
                    projectile.projectile.timer = weaponDef->bombShoes.delayToExplode;
                    projectile.radius = weaponDef->bombShoes.explosionRadius;
                    Global_AddEntity(&projectile);
                } break;
                case WEAPON_TYPE_NATURE_SPIKES: {
                    for (int amt = 0; amt < weaponDef->projectileAmount; amt++) {
                        int candidates[100]; int cCount = 0;
                        for (int j = 0; j < globalVariables.lastEntityIndex && cCount < 100; j++) {
                            if (globalVariables.entities[j].bIsActive && globalVariables.entities[j].type == ENTITY_TYPE_ENEMY) {
                                if (Vector2Distance(player->position, globalVariables.entities[j].position) < weaponDef->natureSpikes.rangeToSpawn) {
                                    candidates[cCount++] = j;
                                }
                            }
                        }
                        if (cCount > 0) {
                            int targetIdx = candidates[GetRandomValue(0, cCount-1)];
                            Entity projectile = Projectile_Spawn(PROJECTILE_TYPE_NATURE_SPIKE, globalVariables.entities[targetIdx].position, (Vector2){0,0}, weaponDef->damage, weaponDef->natureSpikes.spikeDuration, weaponDef->natureSpikes.spikeMaxDamage);
                            projectile.radius = 40.0f * globalVariables.playerStats.sizeMultiplier;
                            Global_AddEntity(&projectile);
                        }
                    }
                } break;
                case WEAPON_TYPE_DEATH_AURA: {
                    if (globalVariables.deathAuraIndex >= globalVariables.lastEntityIndex) {
                        Entity projectile = Projectile_Spawn(PROJECTILE_TYPE_DEATH_AURA, player->position, (Vector2){0,0}, weaponDef->damage, 99999.0f, 255);
                        projectile.radius = weaponDef->deathAura.size * globalVariables.playerStats.sizeMultiplier;
                        projectile.projectile.timer = 0.0f; // Use this as tick timer
                        Global_AddEntity(&projectile);
                    } else {
                        // Update existing aura stats if needed
                        Entity* aura = &globalVariables.entities[globalVariables.deathAuraIndex];
                        aura->radius = weaponDef->deathAura.size * globalVariables.playerStats.sizeMultiplier;
                        aura->projectile.damage = weaponDef->damage * globalVariables.playerStats.damageMultiplier;
                    }
                } break;
            }
        }
    }
}
const char* Weapon_GetWeaponName(WeaponType weaponType)
{
    switch (weaponType) {
        case WEAPON_TYPE_CRYSTAL_WAND: return "Crystal Staff";
        case WEAPON_TYPE_FIREBALL_RING: return "Fireball Ring";
        case WEAPON_TYPE_BOMB_SHOES: return "Bomb Shoes";
        case WEAPON_TYPE_NATURE_SPIKES: return "Nature Spikes";
        case WEAPON_TYPE_DEATH_AURA: return "Death Aura";
        default: return "Unknown Weapon";
    }
}
// ~End of Weapon Implementation

//~ Begin of Global Implementation
void Global_UpdateGameTimer(float deltaTime)
{
    Entity* player = Global_GetPlayer();
    if (player && player->character.bIsDead) return;

    globalVariables.gameTimer += deltaTime;
    HUD_UpdateData();
}
// ~End of Global Implementation