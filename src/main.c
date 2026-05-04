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

i32 main(void)
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
void Core_InitGame() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_UNDECORATED); InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Arcade Survivors Reworked"); SetRandomSeed(time(NULL));
    // Initialize Audio and Assets
    InitAudioDevice(); Assets_Init();
    // Init Global Variables
    globalVariables = (GlobalVariables){ .deathAuraIndex = 65535, .playerStats = Player_GeneratePlayerStats(), .camera = Player_GenerateCamera(), .spawnerData = Spawner_GenerateSpawnerData(), .dropsDefinition = {0.01f, 0.02f} };
    Entity player = Player_GeneratePlayer(); Global_AddEntity(&player); Weapon_Init(); Weapon_AddWeapon((WeaponType)GetRandomValue(1, WEAPON_TYPE_COUNT - 1)); HUD_Init();
    // Start Music
    Music music = Assets_GetMusic(ASSET_MUSIC_TYPE_COMBAT); SetMusicVolume(music, 0.5f); PlayMusicStream(music); SetTargetFPS(240); DisableCursor();
}
void Core_ProcessInput() {
    Entity* player = Global_GetPlayer(); if (!player || player->type == ENTITY_TYPE_UNDEFINED || globalVariables.levelUpState.bShowLevelUp) { if(player && player->type == ENTITY_TYPE_UNDEFINED) globalVariables.bShowInventory = false; return; }
    // Movement Logic (Keyboard & Gamepad)
    Vector2 direction = {0}; if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) direction.y -= 1; if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) direction.y += 1; if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) direction.x -= 1; if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) direction.x += 1;
    if (IsGamepadAvailable(0)) { f32 axisX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X), axisY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y); if (fabsf(axisX) > 0.2f) direction.x = axisX; if (fabsf(axisY) > 0.2f) direction.y = axisY; }
    if (Vector2Length(direction) > 0) player->velocity = Vector2Scale(Vector2Normalize(direction), player->character.speed); else player->velocity = (Vector2){0};
    // Inventory Toggle
    if (IsKeyPressed(KEY_TAB) || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP))) globalVariables.bShowInventory = !globalVariables.bShowInventory;
}
void Core_UpdateGame(f32 deltaTime) {
    // Update Audio & Music
    Audio_Update(deltaTime); 
    // Update Special Events & Timers
    globalVariables.eventState.swarmCycleTimer += deltaTime; globalVariables.eventState.bossCycleTimer += deltaTime;
    if (globalVariables.eventState.activeEventTimer > 0 && (globalVariables.eventState.activeEventTimer -= deltaTime) <= 0) globalVariables.eventState.activeEventType = EVENT_TYPE_UNDEFINED;
    if (globalVariables.eventState.swarmCycleTimer >= 120) { globalVariables.eventState.swarmCycleTimer -= 120; Event_TriggerEvent(EVENT_TYPE_SWARM); }
    if (globalVariables.eventState.bossCycleTimer >= 210) { globalVariables.eventState.bossCycleTimer -= 210; Event_TriggerEvent(EVENT_TYPE_BOSS); }
    if (globalVariables.levelUpState.bShowLevelUp) return;
    if (globalVariables.hudEventTimer > 0) globalVariables.hudEventTimer -= deltaTime;
    // Tick down Active Power-ups
    for (i32 i = 0; i < MAX_ACTIVE_POWERUPS; i++) if (globalVariables.activePowerUps[i].bIsActive && (globalVariables.activePowerUps[i].remainingTime -= deltaTime) <= 0) globalVariables.activePowerUps[i].bIsActive = false;
    // Update Game Systems
    Drop_ProcessPickUp(); Player_ProcessMovement(Global_GetPlayer(), deltaTime); Enemy_ProcessAllMovement(deltaTime); Spawner_ProcessSpawnLogic(deltaTime); Weapon_ProcessAttack(deltaTime); Projectile_ProcessAllMovement(deltaTime); Popup_UpdateAll(deltaTime); XP_MoveCrystals(deltaTime); Global_UpdateGameTimer(deltaTime);
}
void Core_RenderGraphics() {
    BeginDrawing(); ClearBackground(DARKGRAY); BeginMode2D(globalVariables.camera); Render_DrawMap(); Render_DrawAllEntitiesSorted(); EndMode2D();
    HUD_Draw(); if (globalVariables.bShowInventory) HUD_DrawInventory(); if (globalVariables.levelUpState.bShowLevelUp) HUD_DrawLevelUp(); EndDrawing();
}
void Core_CloseGame() { Assets_Unload(); CloseAudioDevice(); CloseWindow(); }
i32 Core_IsGameReadyToClose() { return WindowShouldClose(); }

const char* flashVS = "#version 330\nin vec3 vertexPosition;\nin vec2 vertexTexCoord;\nin vec4 vertexColor;\nout vec2 fragTexCoord;\nout vec4 fragColor;\nuniform mat4 mvp;\nvoid main() { fragTexCoord = vertexTexCoord; fragColor = vertexColor; gl_Position = mvp * vec4(vertexPosition, 1.0); }";
const char* flashFS = "#version 330\nin vec2 fragTexCoord;\nin vec4 fragColor;\nout vec4 finalColor;\nuniform sampler2D texture0;\nuniform vec4 colDiffuse;\nuniform float flashIntensity;\nvoid main() { vec4 texelColor = texture(texture0, fragTexCoord); vec4 baseColor = texelColor * colDiffuse * fragColor; finalColor = mix(baseColor, vec4(1.0, 1.0, 1.0, baseColor.a), flashIntensity); }";
void Assets_Init() {
    const char* sprites[] = {"assets/sprites/PlayerSheet.png", "assets/sprites/GrassTexture.png", "assets/sprites/BatTexture.png"};
    const char* sounds[] = {"assets/sounds/DamageAudio.mp3", "assets/sounds/ExplosionAudio.ogg", "assets/sounds/LevelUpAudio.ogg", "assets/sounds/XpGainAudio.ogg", "assets/sounds/PlayerDamageAudio.ogg"};
    for (i32 i=0; i<3; i++) globalVariables.assets.sprites[i] = LoadTexture(sprites[i]);
    for (i32 i=0; i<5; i++) globalVariables.assets.sounds[i] = LoadSound(sounds[i]);
    globalVariables.assets.musics[0] = LoadMusicStream("assets/music/CombatMusic.ogg");
    globalVariables.assets.flashShader = LoadShaderFromMemory(flashVS, flashFS);
    globalVariables.assets.flashIntensityLoc = GetShaderLocation(globalVariables.assets.flashShader, "flashIntensity");
}
DEFINE_ASSET_GETTER(Texture2D, Sprite, sprites, ASSET_SPRITE_TYPE_COUNT, AssetSpriteType)
DEFINE_ASSET_GETTER(Sound, Sound, sounds, ASSET_SOUND_TYPE_COUNT, AssetSoundType)
DEFINE_ASSET_GETTER(Music, Music, musics, ASSET_MUSIC_TYPE_COUNT, AssetMusicType)
void Assets_Unload() {
    for (i32 i=0; i<ASSET_SPRITE_TYPE_COUNT; i++) UnloadTexture(globalVariables.assets.sprites[i]);
    for (i32 i=0; i<ASSET_SOUND_TYPE_COUNT; i++) UnloadSound(globalVariables.assets.sounds[i]);
    for (i32 i=0; i<ASSET_MUSIC_TYPE_COUNT; i++) UnloadMusicStream(globalVariables.assets.musics[i]);
    UnloadShader(globalVariables.assets.flashShader);
}
//~ End of Assets Implementation

//~ Begin of Audio Implementation
static f32 spammableTimers[3] = {0, 0, 0};
void Audio_PlaySoundVar(AssetSoundType type, bool bIsSpammable) {
    if (bIsSpammable) {
        i32 slot = -1; for (i32 i = 0; i < 3; i++) if (spammableTimers[i] <= 0) { slot = i; break; }
        if (slot == -1) return; spammableTimers[slot] = 0.1f;
    }
    Sound sound = Assets_GetSound(type); SetSoundPitch(sound, 1.0f + (f32)GetRandomValue(-70, 70) / 1000.0f); PlaySound(sound);
}
void Audio_Update(f32 deltaTime) {
    for (i32 i = 0; i < 3; i++) if (spammableTimers[i] > 0) spammableTimers[i] -= deltaTime;
    UpdateMusicStream(Assets_GetMusic(ASSET_MUSIC_TYPE_COMBAT));
}
void Collision_MapBorder(Entity *entity) {
    f32 bound = MAP_HALF_SIZE - entity->radius;
    entity->position.x = Clamp(entity->position.x, -bound, bound); entity->position.y = Clamp(entity->position.y, -bound, bound);
}
void PowerUp_Trigger(PowerUpType type) {
    sprintf(globalVariables.hudEventMessage, "%s ACTIVATED!", PowerUpNames[type]); globalVariables.hudEventTimer = 1.5f;
    if (type == POWERUP_TYPE_NUKE) { for (i32 i = 0; i < globalVariables.lastEntityIndex; i++) { Entity* enemy = &globalVariables.entities[i]; if (enemy->type == ENTITY_TYPE_ENEMY) { XP_GenerateXPCrystal(enemy->position, enemy->enemyCharacter.xpDropAmount); Global_DestroyEntity(i--); } } Audio_PlaySoundVar(ASSET_SOUND_TYPE_EXPLOSION, false); }
    else if (type == POWERUP_TYPE_MAGNET) { for (i32 i = 0; i < globalVariables.lastEntityIndex; i++) if (globalVariables.entities[i].type == ENTITY_TYPE_XP_CRYSTAL) globalVariables.entities[i].xpCrystal.bIsMagnetized = true; }
    else if (type == POWERUP_TYPE_DOUBLE_TROUBLE || type == POWERUP_TYPE_TIME_FREEZE) { globalVariables.activePowerUps[type] = (ActivePowerUp){ .type = type, .remainingTime = 15, .bIsActive = true }; }
}
void Instant_Trigger(InstantDropType type) {
    Entity* player = Global_GetPlayer(); f32 healRatio = type == INSTANT_DROP_TYPE_LIFE ? 0.2f : (type == INSTANT_DROP_TYPE_BIG_LIFE ? 0.5f : 0);
    if (healRatio > 0) { player->character.health = fminf(player->character.maxHealth, player->character.health + player->character.maxHealth * healRatio); sprintf(globalVariables.hudEventMessage, "%s HEALED!", type == INSTANT_DROP_TYPE_LIFE ? "20%" : "50%"); globalVariables.hudEventTimer = 1.5f; }
}
void Drop_GenerateDrop(Vector2 pos) {
    f32 minutes = floorf(globalVariables.gameTimer / 60.0f), roll = (f32)GetRandomValue(0, 10000) / 10000.0f;
    f32 powerUpChance = fmaxf(0.005f, 0.01f - minutes * 0.001f), instantChance = fmaxf(0.01f, 0.02f - minutes * 0.002f);
    if (roll < powerUpChance + instantChance) {
        DropType dropType = roll < powerUpChance ? DROP_TYPE_POWERUP : DROP_TYPE_INSTANT;
        Entity dropEntity = { .type = ENTITY_TYPE_DROP, .position = pos, .radius = 20, .scale = {1, 1}, .drop = { .dropType = dropType } };
        if (dropType == DROP_TYPE_POWERUP) dropEntity.drop.powerUpType = (PowerUpType)GetRandomValue(1, POWERUP_TYPE_COUNT - 1);
        else dropEntity.drop.instantDropType = (InstantDropType)GetRandomValue(1, INSTANT_DROP_TYPE_COUNT - 1);
        Global_AddEntity(&dropEntity);
    }
}
void Drop_ProcessPickUp() {
    Entity* player = Global_GetPlayer(); if (!player || player->type == ENTITY_TYPE_UNDEFINED) return;
    for (i32 i = 0; i < globalVariables.lastEntityIndex; i++) {
        Entity* drop = &globalVariables.entities[i];
        if (drop->type == ENTITY_TYPE_DROP && CheckCollisionCircles(player->position, player->radius, drop->position, drop->radius)) {
            if (drop->drop.dropType == DROP_TYPE_POWERUP) PowerUp_Trigger(drop->drop.powerUpType); else Instant_Trigger(drop->drop.instantDropType);
            Global_DestroyEntity(i--);
        }
    }
}
//~ End of Drop Implementation

//~ Begin of Enemy Implementation
//~ Begin of Enemy Implementation
Entity Enemy_GenerateEnemy(EnemyType enemyType) {
    const struct { f32 health, speed, xp, damage, scale, radius; } typeData[] = {
        {0, 0, 0, 0, 0, 0}, {15, 150, 10, 10, 0.75f, 30}, {10, 225, 15, 5, 0.5f, 30}, {115, 80, 50, 25, 1.5f, 60}, {3500, 120, 5000, 50, 3.0f, 120}
    };
    i32 minutes = (i32)(globalVariables.gameTimer / 60.0f);
    f32 hpScale = powf(1.25f, (f32)minutes), dmgScale = powf(1.15f, (f32)minutes), xpScale = powf(1.05f, (f32)minutes), speedScale = powf(1.03f, (f32)minutes);
    Entity enemy = { .type = ENTITY_TYPE_ENEMY, .scale = {typeData[enemyType].scale, typeData[enemyType].scale}, .radius = typeData[enemyType].radius, .visualType = VISUAL_TYPE_ANIMATED_STATIC_SPRITE, .animatedStaticSprite = { .spriteID = ASSET_SPRITE_TYPE_BAT, .animationDuration = 1, .animationTimer = (f32)GetRandomValue(0, 1000) / 1000.0f }, .enemyCharacter = { .enemyType = enemyType, .health = typeData[enemyType].health, .speed = typeData[enemyType].speed, .xpDropAmount = typeData[enemyType].xp, .damage = typeData[enemyType].damage } };
    if (minutes > 0) { enemy.enemyCharacter.health *= hpScale; enemy.enemyCharacter.damage *= dmgScale; enemy.enemyCharacter.xpDropAmount *= xpScale; enemy.enemyCharacter.speed *= speedScale; }
    return enemy;
}
void Enemy_ProcessAllMovement(f32 deltaTime) {
    Entity *player = Global_GetPlayer(); bool bTimeFrozen = globalVariables.activePowerUps[POWERUP_TYPE_TIME_FREEZE].bIsActive;
    for (i32 entityIndex = 0; entityIndex < globalVariables.lastEntityIndex; entityIndex++) {
        Entity *current = &globalVariables.entities[entityIndex]; if (current->type != ENTITY_TYPE_ENEMY) continue;
        if (!bTimeFrozen) {
            Vector2 direction = Vector2Normalize(Vector2Subtract(player->position, current->position));
            current->velocity = Vector2Scale(direction, current->enemyCharacter.speed);
            if (current->velocity.x != 0) { if (current->visualType == VISUAL_TYPE_ANIMATED_STATIC_SPRITE) current->animatedStaticSprite.flipX = current->velocity.x < 0; else current->sprite.flipX = current->velocity.x < 0; }
            current->position = Vector2Add(current->position, Vector2Scale(current->velocity, deltaTime));
        }
        Vector2 separation = {0};
        for (i32 otherIndex = 0; otherIndex < globalVariables.lastEntityIndex; otherIndex++) {
            Entity *other = &globalVariables.entities[otherIndex]; if (entityIndex == otherIndex || other->type != ENTITY_TYPE_ENEMY) continue;
            f32 distance = Vector2Distance(current->position, other->position), minDist = current->radius + other->radius;
            if (distance < minDist && distance > 0) separation = Vector2Add(separation, Vector2Scale(Vector2Normalize(Vector2Subtract(current->position, other->position)), (minDist - distance) * 1.5f));
        }
        current->position = Vector2Add(current->position, Vector2Scale(separation, deltaTime * 10.0f));
        if (player->character.invulnerableTimer <= 0 && Vector2Distance(current->position, player->position) < (current->radius + player->radius)) {
            player->character.health -= current->enemyCharacter.damage; player->character.invulnerableTimer = player->character.flashTimer = 0.5f; Audio_PlaySoundVar(ASSET_SOUND_TYPE_PLAYER_DAMAGE, false);
            if (player->character.health <= 0) { player->type = ENTITY_TYPE_UNDEFINED; player->character.deathFadeTimer = 2.0f; player->character.health = 0; }
        }
        if (current->enemyCharacter.flashTimer > 0) current->enemyCharacter.flashTimer -= deltaTime;
        if (current->visualType == VISUAL_TYPE_ANIMATED_STATIC_SPRITE && (current->animatedStaticSprite.animationTimer += deltaTime) >= current->animatedStaticSprite.animationDuration) current->animatedStaticSprite.animationTimer -= current->animatedStaticSprite.animationDuration;
    }
}
void Event_TriggerEvent(GameEventType type) {
    globalVariables.eventState.activeEventType = type; globalVariables.eventState.activeEventTimer = 20.0f;
    if (type == EVENT_TYPE_BOSS) {
        Entity boss = Enemy_GenerateEnemy(ENEMY_TYPE_BOSS); f32 angle = (f32)GetRandomValue(0, 360) * DEG2RAD;
        boss.position = Vector2Add(Global_GetPlayer()->position, (Vector2){ cosf(angle) * 800.0f, sinf(angle) * 800.0f });
        Collision_MapBorder(&boss); Global_AddEntity(&boss);
    }
}
//~ End of Event Implementation

//~ Begin of HUD Implementation
void HUD_DrawTextC(const char* text, i32 posX, i32 posY, i32 width, i32 fontSize, Color color, bool outline) {
    i32 textX = posX + (width - MeasureText(text, fontSize)) / 2;
    if (outline) for (i32 outlineX = -2; outlineX <= 2; outlineX += 4) for (i32 outlineY = -2; outlineY <= 2; outlineY += 4) DrawText(text, textX + outlineX, posY + outlineY, fontSize, BLACK);
    DrawText(text, textX, posY, fontSize, color);
}
void HUD_DrawBar(i32 posX, i32 posY, i32 width, i32 height, f32 percentage, Color color) {
    DrawRectangle(posX, posY, width, height, DARKGRAY); DrawRectangle(posX, posY, (i32)(width * Clamp(percentage, 0, 1)), height, color); DrawRectangleLines(posX, posY, width, height, BLACK);
}
void HUD_DrawEvent() {
    f32 swarmTimer = 120 - globalVariables.eventState.swarmCycleTimer, bossTimer = 210 - globalVariables.eventState.bossCycleTimer;
    const char* message = swarmTimer > 0 && swarmTimer <= 5 ? "SWARM INCOMING" : (bossTimer > 0 && bossTimer <= 5 ? "BOSS INCOMING" : NULL);
    if (message) HUD_DrawTextC(message, 0, SCREEN_HEIGHT / 2 - 100, SCREEN_WIDTH, 60, Fade(RED, fabsf(sinf(GetTime() * 10))), true);
}
void HUD_Init() { globalVariables.gameTimer = 0; globalVariables.playerStats.level = 1; }
void HUD_UpdateData() {}
void HUD_Draw() {
    Color xpColor = BLUE; f32 xpPercentage = globalVariables.playerStats.currentXP / globalVariables.playerStats.nextLevelXP;
    if (globalVariables.levelUpState.bShowLevelUp) { xpPercentage = 1; xpColor = ColorFromHSV((f32)((i32)(GetTime() * 300) % 360), 0.8f, 0.9f); }
    HUD_DrawBar(0, 0, SCREEN_WIDTH, 30, xpPercentage, xpColor); HUD_DrawTextC(TextFormat("LEVEL %d", globalVariables.playerStats.level), 0, 5, SCREEN_WIDTH, 20, WHITE, false);
    Entity* player = Global_GetPlayer(); HUD_DrawBar(20, 50, 350, 35, player->character.health / player->character.maxHealth, RED);
    HUD_DrawTextC(TextFormat("%.0f / %.0f", player->character.health, player->character.maxHealth), 20, 57, 350, 20, WHITE, false);
    i32 minutes = (i32)globalVariables.gameTimer / 60, seconds = (i32)globalVariables.gameTimer % 60;
    const char* timerText = TextFormat("%02d:%02d", minutes, seconds); i32 timerX = SCREEN_WIDTH - 20 - MeasureText(timerText, 40);
    for (i32 outlineX = -2; outlineX <= 2; outlineX += 4) for (i32 outlineY = -2; outlineY <= 2; outlineY += 4) DrawText(timerText, timerX + outlineX, 50 + outlineY, 40, BLACK);
    DrawText(timerText, timerX, 50, 40, WHITE); HUD_DrawPowerUps(); HUD_DrawPickupNotification(); HUD_DrawEvent();
}
void HUD_DrawPowerUps() {
    i32 activeCount = 0; for (i32 index = 0; index < MAX_ACTIVE_POWERUPS; index++) if (globalVariables.activePowerUps[index].bIsActive) activeCount++;
    if (!activeCount) return;
    i32 startX = (SCREEN_WIDTH - (activeCount * 55 - 15)) / 2, drawnCount = 0;
    for (i32 index = 0; index < MAX_ACTIVE_POWERUPS; index++) {
        if (!globalVariables.activePowerUps[index].bIsActive) continue;
        ActivePowerUp* powerUp = &globalVariables.activePowerUps[index]; i32 posX = startX + (drawnCount++ * 55);
        Color themeColor = powerUp->type == POWERUP_TYPE_DOUBLE_TROUBLE ? RED : WHITE;
        DrawRectangle(posX, 50, 40, 40, ColorAlpha(themeColor, 0.3f)); DrawRectangleLines(posX, 50, 40, 40, themeColor);
        HUD_DrawTextC(TextFormat("%.0fs", powerUp->remainingTime), posX, 95, 40, 20, WHITE, false);
        HUD_DrawTextC(powerUp->type == POWERUP_TYPE_DOUBLE_TROUBLE ? "DT" : "TF", posX, 60, 40, 20, themeColor, false);
    }
}
void HUD_DrawPickupNotification() {
    if (globalVariables.hudEventTimer <= 0) return;
    u8 alphaValue = (u8)(255 * (globalVariables.hudEventTimer / 1.5f));
    HUD_DrawTextC(globalVariables.hudEventMessage, 0, 153, SCREEN_WIDTH, 50, (Color){0, 0, 0, alphaValue}, false);
    HUD_DrawTextC(globalVariables.hudEventMessage, 0, 150, SCREEN_WIDTH, 50, (Color){255, 235, 59, alphaValue}, false);
}
void HUD_DrawInventory() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.85f));
    i32 padding = 150, colWidth = (SCREEN_WIDTH - 300) / 2, startY = 200, currentY;
    DrawText("EQUIPMENT", padding, startY - 60, 45, GOLD); DrawText("WEAPONS", padding, currentY = startY, 30, GRAY); currentY += 45;
    for (i32 index = 0; index < MAX_WEAPON_CAPACITY; index++) if (globalVariables.inventory.weaponDatas[index].level > 0) { DrawText(TextFormat("Lv.%d %s", globalVariables.inventory.weaponDatas[index].level, WeaponNames[globalVariables.inventory.weaponDatas[index].weaponType]), padding + 20, currentY, 32, WHITE); currentY += 40; }
    DrawText("RELICS", padding, currentY += 30, 30, GRAY); currentY += 45;
    for (i32 index = 0; index < MAX_RELIC_CAPACITY; index++) if (globalVariables.inventory.relicDatas[index].level > 0) { DrawText(TextFormat("Lv.%d %s", globalVariables.inventory.relicDatas[index].level, RelicNames[globalVariables.inventory.relicDatas[index].relicType]), padding + 20, currentY, 32, WHITE); currentY += 40; }
    i32 rightPositionX = padding + colWidth; DrawText("PLAYER STATS", rightPositionX, startY - 60, 45, GOLD); currentY = startY;
    PlayerStats* playerStats = &globalVariables.playerStats; Entity* player = Global_GetPlayer();
    const char* statTextStrings[] = { TextFormat("Health: %.0f / %.0f", player->character.health, player->character.maxHealth), TextFormat("Damage: %.0f%%", playerStats->damageMultiplier * 100), TextFormat("Attack Speed: %.0f%%", playerStats->attackSpeedMultiplier * 100), TextFormat("Move Speed: %.0f%%", playerStats->movementSpeedMultiplier * 100), TextFormat("Area Size: %.0f%%", playerStats->sizeMultiplier * 100), TextFormat("Life Steal: %.0f%%", playerStats->lifeStealMultiplier * 100), TextFormat("XP Gain: %.0f%%", playerStats->xpMultiplier * 100) };
    for (i32 index = 0; index < 7; index++) { DrawText(statTextStrings[index], rightPositionX, currentY, 32, WHITE); currentY += 45; }
}
void HUD_DrawLevelUp() {
    static f32 axisTimer = 0; i32 choiceIndex = -1; bool bSelected = false;
    if (IsKeyPressed(KEY_ONE)) { choiceIndex = 0; bSelected = true; } if (IsKeyPressed(KEY_TWO)) { choiceIndex = 1; bSelected = true; } if (IsKeyPressed(KEY_THREE)) { choiceIndex = 2; bSelected = true; }
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) globalVariables.levelUpState.selectedIndex--;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) globalVariables.levelUpState.selectedIndex++;
    if (IsGamepadAvailable(0)) { f32 axisValue = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X); if (fabsf(axisValue) > 0.5f && axisTimer <= 0) { globalVariables.levelUpState.selectedIndex += (axisValue > 0 ? 1 : -1); axisTimer = 0.2f; } if (axisTimer > 0) axisTimer -= GetFrameTime(); }
    globalVariables.levelUpState.selectedIndex = (globalVariables.levelUpState.selectedIndex + 3) % 3;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) { choiceIndex = globalVariables.levelUpState.selectedIndex; bSelected = true; }
    if (bSelected && choiceIndex >= 0 && choiceIndex < 3) {
        UpgradeOption* selectedOption = &globalVariables.levelUpState.options[choiceIndex];
        if (selectedOption->type == UPGRADE_TYPE_WEAPON) Weapon_AddWeapon(selectedOption->weapon); else Relic_AddRelic(selectedOption->relic);
        if (--globalVariables.levelUpState.pendingCount > 0) HUD_GenerateLevelUpOptions(); else globalVariables.levelUpState.bShowLevelUp = false;
        return;
    }
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.85f)); HUD_DrawTextC("LEVEL UP", 0, 100, SCREEN_WIDTH, 80, GOLD, false);
    i32 startX = (SCREEN_WIDTH - 1470) / 2;
    for (i32 index = 0; index < 3; index++) {
        UpgradeOption* upgradeOption = &globalVariables.levelUpState.options[index]; if (upgradeOption->type == UPGRADE_TYPE_UNDEFINED) continue;
        i32 posX = startX + index * 510; bool bIsSelected = (globalVariables.levelUpState.selectedIndex == index);
        DrawRectangle(posX, 250, 450, 600, bIsSelected ? (Color){40, 40, 45, 255} : (Color){25, 25, 30, 255}); DrawRectangleLinesEx((Rectangle){(f32)posX, 250, 450, 600}, bIsSelected ? 5 : 2, bIsSelected ? GOLD : GRAY);
        const char* upgradeName = upgradeOption->type == UPGRADE_TYPE_WEAPON ? WeaponNames[upgradeOption->weapon] : RelicNames[upgradeOption->relic];
        Color themeColor = upgradeOption->type == UPGRADE_TYPE_WEAPON ? WeaponColors[upgradeOption->weapon] : RelicColors[upgradeOption->relic];
        HUD_DrawTextC(upgradeName, posX, 290, 450, 35, themeColor, false); HUD_DrawTextC(TextFormat("LEVEL %d", upgradeOption->level), posX, 340, 450, 25, LIGHTGRAY, false);
        HUD_DrawTextC(upgradeOption->type == UPGRADE_TYPE_WEAPON ? "WEAPON" : "RELIC", posX, 370, 450, 20, DARKGRAY, false);
        if (upgradeOption->level == 1) { const char* description = upgradeOption->type == UPGRADE_TYPE_WEAPON ? WeaponDescriptions[upgradeOption->weapon] : RelicDescriptions[upgradeOption->relic]; DrawText(TextSubtext(description, 0, 30), posX + 30, 430, 22, RAYWHITE); if (strlen(description) > 30) DrawText(TextSubtext(description, 30, 30), posX + 30, 460, 22, RAYWHITE); }
        else if (upgradeOption->type == UPGRADE_TYPE_WEAPON) {
            WeaponDefinition *oldDefinition = &globalVariables.InventoryDefinitions.weaponDefinitions[upgradeOption->weapon][upgradeOption->level - 2], *newDefinition = &globalVariables.InventoryDefinitions.weaponDefinitions[upgradeOption->weapon][upgradeOption->level - 1];
            i32 descriptionY = 430; if (newDefinition->damage != oldDefinition->damage) { DrawText(TextFormat("- Damage: %.1f -> %.1f", oldDefinition->damage, newDefinition->damage), posX + 30, descriptionY, 20, WHITE); descriptionY += 30; }
            if (newDefinition->delayBetweenAttacks != oldDefinition->delayBetweenAttacks) { DrawText(TextFormat("- Cooldown: %.1fs -> %.1fs", oldDefinition->delayBetweenAttacks, newDefinition->delayBetweenAttacks), posX + 30, descriptionY, 20, WHITE); descriptionY += 30; }
            if (newDefinition->projectileAmount != oldDefinition->projectileAmount) { DrawText(TextFormat("- Amount: %d -> %d", oldDefinition->projectileAmount, newDefinition->projectileAmount), posX + 30, descriptionY, 20, WHITE); descriptionY += 30; }
            f32 oldStatValue = 0, newStatValue = 0; const char* statLabel = NULL;
            if (upgradeOption->weapon == WEAPON_TYPE_CRYSTAL_WAND) { statLabel = "- Pierce: %d -> %d"; oldStatValue = oldDefinition->crystal.penetration; newStatValue = newDefinition->crystal.penetration; }
            else if (upgradeOption->weapon == WEAPON_TYPE_FIREBALL_RING) { statLabel = "- Radius: %.0f -> %.0f"; oldStatValue = oldDefinition->fireball.explosionRadius; newStatValue = newDefinition->fireball.explosionRadius; }
            else if (upgradeOption->weapon == WEAPON_TYPE_BOMB_SHOES) { statLabel = "- Radius: %.0f -> %.0f"; oldStatValue = oldDefinition->bombShoes.explosionRadius; newStatValue = newDefinition->bombShoes.explosionRadius; }
            else if (upgradeOption->weapon == WEAPON_TYPE_NATURE_SPIKES) { statLabel = "- Range: %.0f -> %.0f"; oldStatValue = oldDefinition->natureSpikes.rangeToSpawn; newStatValue = newDefinition->natureSpikes.rangeToSpawn; }
            else if (upgradeOption->weapon == WEAPON_TYPE_DEATH_AURA) { statLabel = "- Size: %.0f -> %.0f"; oldStatValue = oldDefinition->deathAura.size; newStatValue = newDefinition->deathAura.size; }
            if (statLabel && newStatValue != oldStatValue) DrawText(TextFormat(statLabel, (i32)oldStatValue, (i32)newStatValue), posX + 30, descriptionY, 20, WHITE);
        } else DrawText(TextFormat("- Effect: +%.0f%% per level", relicLevelDefinitions[upgradeOption->relic].multiplier * 100), posX + 30, 430, 22, WHITE);
    }
}
void HUD_GenerateLevelUpOptions() {
    UpgradeOption candidates[WEAPON_TYPE_COUNT + RELIC_TYPE_COUNT]; i32 candidateCount = 0, weaponCount = 0, relicCount = 0;
    for (i32 index = 0; index < MAX_WEAPON_CAPACITY; index++) if (globalVariables.inventory.weaponDatas[index].level > 0) weaponCount++;
    for (i32 index = 0; index < MAX_RELIC_CAPACITY; index++) if (globalVariables.inventory.relicDatas[index].level > 0) relicCount++;
    for (i32 typeIndex = 1; typeIndex <= 2; typeIndex++) {
        i32 maxTypesCount = (typeIndex == 1 ? WEAPON_TYPE_COUNT : RELIC_TYPE_COUNT), maxCapacity = (typeIndex == 1 ? MAX_WEAPON_CAPACITY : MAX_RELIC_CAPACITY), currentCapacity = (typeIndex == 1 ? weaponCount : relicCount);
        for (i32 upgradeType = 1; upgradeType < maxTypesCount; upgradeType++) {
            i32 inventoryIndex = -1; for (i32 searchIndex = 0; searchIndex < maxCapacity; searchIndex++) if (typeIndex == 1 ? (globalVariables.inventory.weaponDatas[searchIndex].level > 0 && globalVariables.inventory.weaponDatas[searchIndex].weaponType == upgradeType) : (globalVariables.inventory.relicDatas[searchIndex].level > 0 && globalVariables.inventory.relicDatas[searchIndex].relicType == upgradeType)) { inventoryIndex = searchIndex; break; }
            i32 currentLevel = (inventoryIndex != -1 ? (typeIndex == 1 ? globalVariables.inventory.weaponDatas[inventoryIndex].level : globalVariables.inventory.relicDatas[inventoryIndex].level) : 0);
            if ((currentLevel > 0 && currentLevel < (typeIndex == 1 ? MAX_WEAPON_LEVEL : MAX_RELIC_LEVEL)) || (currentLevel == 0 && currentCapacity < maxCapacity)) { candidates[candidateCount] = (UpgradeOption){.type = typeIndex, .level = currentLevel + 1}; if (typeIndex == 1) candidates[candidateCount++].weapon = upgradeType; else candidates[candidateCount++].relic = upgradeType; }
        }
    }
    for (i32 slotIndex = 0; slotIndex < 3; slotIndex++) { if (candidateCount > 0) { i32 randomCandidateIndex = GetRandomValue(0, candidateCount - 1); globalVariables.levelUpState.options[slotIndex] = candidates[randomCandidateIndex]; candidates[randomCandidateIndex] = candidates[--candidateCount]; } else globalVariables.levelUpState.options[slotIndex].type = UPGRADE_TYPE_UNDEFINED; }
    globalVariables.levelUpState.selectedIndex = 0;
}
//~ End of HUD Implementation

// ~Begin of Player Implementation
Camera2D Player_GenerateCamera() {
    Vector2 offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };
    return (Camera2D){ .target = Global_GetPlayer()->position, .offset = offset, .rotation = 0.0f, .zoom = 1.0f };
}
Entity Player_GeneratePlayer() {
    return (Entity){
        .type = ENTITY_TYPE_PLAYER, .scale = {2, 2}, .radius = 40, .visualType = VISUAL_TYPE_ANIMATED_SPRITE,
        .character = { .speed = 400, .health = 100, .maxHealth = 100 },
        .animatedSprite = { .spriteID = ASSET_SPRITE_TYPE_PLAYER, .frameCount = 2, .frameTime = 0.1f }
    };
}
PlayerStats Player_GeneratePlayerStats() {
    return (PlayerStats){ .nextLevelXP = 200, .healthMultiplier = 1, .damageMultiplier = 1, .attackSpeedMultiplier = 1, .movementSpeedMultiplier = 1, .sizeMultiplier = 1, .xpMultiplier = 1 };
}
void Player_ProcessMovement(Entity* player, f32 deltaTime) {
    if (!player) return;
    if (player->type == ENTITY_TYPE_UNDEFINED) {
        if (player->character.deathFadeTimer > 0) player->character.deathFadeTimer -= deltaTime;
        return;
    }

    player->position = Vector2Add(player->position, Vector2Scale(player->velocity, deltaTime));
    Collision_MapBorder(player);

    globalVariables.camera.target = Vector2Lerp(globalVariables.camera.target, player->position, 10.0f * deltaTime);

    if (player->character.invulnerableTimer > 0) player->character.invulnerableTimer -= deltaTime;
    if (player->character.flashTimer > 0) player->character.flashTimer -= deltaTime;
    Player_AnimateMovement(player, deltaTime);
}
void Player_AnimateMovement(Entity* player, f32 deltaTime) {
    AnimatedSpriteData* sprite = &player->animatedSprite;
    if (Vector2Length(player->velocity) <= 0) {
        sprite->currentFrame = 0; sprite->frameTimer = 0;
        return;
    }

    if ((sprite->frameTimer += deltaTime) >= sprite->frameTime) {
        sprite->frameTimer = 0;
        if (++sprite->currentFrame > sprite->frameCount) sprite->currentFrame = 0;
    }

    if (player->velocity.x != 0) sprite->flipX = (player->velocity.x < 0);
}
//~ End of Player Implementation

//~ Begin of Popup Implementation
Entity Popup_SpawnDamagePopup(Vector2 position, f32 amount) {
    Entity popup = { .type = ENTITY_TYPE_DAMAGE_POPUP, .position = position, .scale = {1, 1}, .damagePopup = { .amount = amount, .timer = 0 } };
    Global_AddEntity(&popup);
    return popup;
}
void Popup_UpdateAll(f32 deltaTime) {
    for (i32 entityIndex = 0; entityIndex < globalVariables.lastEntityIndex; entityIndex++) {
        Entity* entity = &globalVariables.entities[entityIndex];
        if (entity->type != ENTITY_TYPE_DAMAGE_POPUP) continue;

        DamagePopup* popup = &entity->damagePopup;
        if ((popup->timer += deltaTime) >= 0.7f) { Global_DestroyEntity(entityIndex--); continue; }

        if (popup->timer < 0.2f) {
            entity->position.y -= 100.0f * deltaTime;
            entity->scale.x = entity->scale.y = 1.0f + 0.25f * (popup->timer / 0.2f);
        } else {
            f32 easeValue = sinf((popup->timer - 0.2f) / 0.5f * PI * 0.5f);
            entity->scale.x = entity->scale.y = 1.25f - 0.75f * easeValue;
        }
    }
}
//~ End of Popup Implementation

//~ Begin of Projectile Implementation
Entity Projectile_Spawn(ProjectileType type, Vector2 pos, Vector2 vel, f32 damage, f32 lifeTime, u8 penetration) {
    f32 damageMultiplier = globalVariables.playerStats.damageMultiplier;
    if (globalVariables.activePowerUps[POWERUP_TYPE_DOUBLE_TROUBLE].bIsActive) damageMultiplier *= 2.0f;

    Entity projectile = {
        .type = ENTITY_TYPE_PROJECTILE, .position = pos, .velocity = vel, .scale = {1, 1}, .radius = 10,
        .projectile = { .projectileType = type, .damage = damage * damageMultiplier, .lifeTime = lifeTime, .penetration = penetration }
    };

    if (type == PROJECTILE_TYPE_CRYSTAL_SHARD || type == PROJECTILE_TYPE_NATURE_SPIKE) {
        for (i32 hitIndex = 0; hitIndex < 16; hitIndex++) projectile.projectile.hitTracking.hitIds[hitIndex] = 65535;
    } else {
        projectile.projectile.explosive.explosionDamageMultiplier = 1.0f;
        projectile.projectile.explosive.explosionRadius = 0.0f;
    }

    if (type == PROJECTILE_TYPE_NATURE_SPIKE) projectile.projectile.timer = 0.0f;
    return projectile;
}

void Projectile_ProcessAllMovement(f32 deltaTime) {
    Entity* player = Global_GetPlayer();
    for (i32 projectileIndex = 0; projectileIndex < globalVariables.lastEntityIndex; projectileIndex++) {
        Entity* projectile = &globalVariables.entities[projectileIndex];
        if (projectile->type != ENTITY_TYPE_PROJECTILE) continue;
        if ((projectile->projectile.lifeTime -= deltaTime) <= 0) { Global_DestroyEntity(projectileIndex--); continue; }

        ProjectileType type = projectile->projectile.projectileType;
        if (type == PROJECTILE_TYPE_NATURE_SPIKE && (projectile->projectile.timer -= deltaTime) <= 0) {
            projectile->projectile.timer = 0.2f;
            for (i32 hitIndex = 0; hitIndex < 16; hitIndex++) projectile->projectile.hitTracking.hitIds[hitIndex] = 65535;
        }

        if (type == PROJECTILE_TYPE_CRYSTAL_SHARD || type == PROJECTILE_TYPE_FIREBALL) {
            projectile->position = Vector2Add(projectile->position, Vector2Scale(projectile->velocity, deltaTime));
        }

        if (type == PROJECTILE_TYPE_BOMB && (projectile->projectile.timer -= deltaTime) <= 0) {
            Entity explosion = Projectile_Spawn(PROJECTILE_TYPE_EXPLOSION, projectile->position, (Vector2){0}, projectile->projectile.damage, 0.2f, 255);
            explosion.radius = projectile->radius; Global_AddEntity(&explosion);
            Global_DestroyEntity(projectileIndex--); Audio_PlaySoundVar(ASSET_SOUND_TYPE_EXPLOSION, false);
            continue;
        }

        if (type == PROJECTILE_TYPE_DEATH_AURA) {
            projectile->position = player->position;
            if ((projectile->projectile.timer -= deltaTime) <= 0) {
                u8 weaponLevel = 1;
                for (i32 weaponIndex = 0; weaponIndex < MAX_WEAPON_CAPACITY; weaponIndex++) {
                    WeaponData* weapon = &globalVariables.inventory.weaponDatas[weaponIndex];
                    if (weapon->level > 0 && weapon->weaponType == WEAPON_TYPE_DEATH_AURA) {
                        weaponLevel = weapon->level; break;
                    }
                }
                f32 attackSpeed = globalVariables.playerStats.attackSpeedMultiplier;
                projectile->projectile.timer = globalVariables.InventoryDefinitions.weaponDefinitions[WEAPON_TYPE_DEATH_AURA][weaponLevel-1].delayBetweenAttacks / attackSpeed;
                for (i32 enemyIndex = 0; enemyIndex < globalVariables.lastEntityIndex; enemyIndex++) {
                    Entity* enemy = &globalVariables.entities[enemyIndex];
                    if (enemy->type == ENTITY_TYPE_ENEMY && CheckCollisionCircles(projectile->position, projectile->radius, enemy->position, enemy->radius))
                        Global_DealDamageToEnemy(enemyIndex, projectile->projectile.damage, true);
                }
            }
        }

        if (type != PROJECTILE_TYPE_EXPLOSION && type != PROJECTILE_TYPE_BOMB && type != PROJECTILE_TYPE_DEATH_AURA) {
            for (i32 enemyIndex = 0; enemyIndex < globalVariables.lastEntityIndex; enemyIndex++) {
                Entity* enemy = &globalVariables.entities[enemyIndex];
                if (enemy->type != ENTITY_TYPE_ENEMY || !CheckCollisionCircles(projectile->position, projectile->radius, enemy->position, enemy->radius)) continue;

                bool alreadyHit = false;
                if (type == PROJECTILE_TYPE_CRYSTAL_SHARD || type == PROJECTILE_TYPE_NATURE_SPIKE) {
                    for (i32 hitIndex = 0; hitIndex < 16; hitIndex++) if (projectile->projectile.hitTracking.hitIds[hitIndex] == enemy->id) { alreadyHit = true; break; }
                }
                if (alreadyHit) continue;

                Global_DealDamageToEnemy(enemyIndex, projectile->projectile.damage, type == PROJECTILE_TYPE_NATURE_SPIKE);
                if (type == PROJECTILE_TYPE_CRYSTAL_SHARD || type == PROJECTILE_TYPE_NATURE_SPIKE) {
                    for (i32 shiftIndex = 15; shiftIndex > 0; shiftIndex--) projectile->projectile.hitTracking.hitIds[shiftIndex] = projectile->projectile.hitTracking.hitIds[shiftIndex-1];
                    projectile->projectile.hitTracking.hitIds[0] = enemy->id;
                }
                if (type == PROJECTILE_TYPE_FIREBALL) {
                    Entity explosion = Projectile_Spawn(PROJECTILE_TYPE_EXPLOSION, projectile->position, (Vector2){0}, projectile->projectile.damage * projectile->projectile.explosive.explosionDamageMultiplier, 0.2f, 255);
                    explosion.radius = projectile->projectile.explosive.explosionRadius; Global_AddEntity(&explosion);
                    Global_DestroyEntity(projectileIndex--); Audio_PlaySoundVar(ASSET_SOUND_TYPE_EXPLOSION, false);
                    goto next_projectile;
                }
                if (--projectile->projectile.penetration <= 0) { Global_DestroyEntity(projectileIndex--); goto next_projectile; }
            }
        } else if (type == PROJECTILE_TYPE_EXPLOSION) {
            for (i32 enemyIndex = 0; enemyIndex < globalVariables.lastEntityIndex; enemyIndex++) {
                Entity* enemy = &globalVariables.entities[enemyIndex];
                if (enemy->type == ENTITY_TYPE_ENEMY && CheckCollisionCircles(projectile->position, projectile->radius, enemy->position, enemy->radius))
                    Global_DealDamageToEnemy(enemyIndex, projectile->projectile.damage, true);
            }
        }
        next_projectile:;
    }
}
//~ End of Projectile Implementation

//~ Begin of Relic Implementation
void Relic_ApplyEffects() {
    Entity* p = Global_GetPlayer(); if (!p) return;
    f32 oldHP = p->character.maxHealth; PlayerStats* s = &globalVariables.playerStats;
    s->healthMultiplier = s->damageMultiplier = s->attackSpeedMultiplier = s->movementSpeedMultiplier = s->sizeMultiplier = s->xpMultiplier = 1.0f; s->lifeStealMultiplier = 0.0f;
    f32* m[] = {NULL, &s->healthMultiplier, &s->damageMultiplier, &s->attackSpeedMultiplier, &s->movementSpeedMultiplier, &s->sizeMultiplier, &s->lifeStealMultiplier, &s->xpMultiplier};
    for (i32 i = 0; i < MAX_RELIC_CAPACITY; i++) { RelicData* r = &globalVariables.inventory.relicDatas[i]; if (r->level > 0 && r->relicType < 8) *m[r->relicType] += (f32)r->level * relicLevelDefinitions[r->relicType].multiplier; }
    p->character.maxHealth = 100.0f * s->healthMultiplier;
    if (p->character.maxHealth > oldHP && p->character.invulnerableTimer <= 0) p->character.health += (p->character.maxHealth - oldHP);
    p->character.speed = 400.0f * s->movementSpeedMultiplier;
}
void Relic_AddRelic(RelicType type) {
    i32 slot = -1;
    for (i32 i = 0; i < MAX_RELIC_CAPACITY; i++) {
        RelicData* r = &globalVariables.inventory.relicDatas[i];
        if (r->level > 0 && r->relicType == type) { if (r->level < MAX_RELIC_LEVEL) { r->level++; Relic_ApplyEffects(); } return; }
        if (r->level == 0 && slot == -1) slot = i;
    }
    if (slot != -1) { globalVariables.inventory.relicDatas[slot] = (RelicData){type, 1}; Relic_ApplyEffects(); }
}
//~ End of Relic Implementation

//~ Begin of XP Implementation
void XP_GenerateXPCrystal(Vector2 position, f32 amount) { Entity crystal = { .type = ENTITY_TYPE_XP_CRYSTAL, .position = position, .scale = {1, 1}, .radius = 15, .xpCrystal = {amount, false} }; Global_AddEntity(&crystal); }
void XP_MoveCrystals(f32 deltaTime) {
    Entity* player = Global_GetPlayer(); if (!player) return;
    for (i32 index = 0; index < globalVariables.lastEntityIndex; index++) {
        Entity* crystal = &globalVariables.entities[index]; if (crystal->type != ENTITY_TYPE_XP_CRYSTAL) continue;
        f32 distance = Vector2Distance(crystal->position, player->position); if (!crystal->xpCrystal.bIsMagnetized && distance < 100.0f) crystal->xpCrystal.bIsMagnetized = true;
        if (crystal->xpCrystal.bIsMagnetized) {
            f32 speed = Clamp(Vector2Length(crystal->velocity) + 1200.0f * deltaTime, 200.0f, 700.0f);
            crystal->velocity = Vector2Scale(Vector2Normalize(Vector2Subtract(player->position, crystal->position)), speed);
            crystal->position = Vector2Add(crystal->position, Vector2Scale(crystal->velocity, deltaTime));
        }
        if (distance < 20.0f) { XP_GrantXP(crystal->xpCrystal.amount); Global_DestroyEntity(index--); }
    }
}
void XP_GrantXP(f32 amount) { PlayerStats* playerStats = &globalVariables.playerStats; playerStats->currentXP += amount * playerStats->xpMultiplier; Audio_PlaySoundVar(ASSET_SOUND_TYPE_XP_GAIN, true); while (playerStats->currentXP >= playerStats->nextLevelXP) XP_LevelUp(); }
void XP_LevelUp() {
    PlayerStats* playerStats = &globalVariables.playerStats; playerStats->currentXP -= playerStats->nextLevelXP; playerStats->level++;
    playerStats->nextLevelXP = 200.0f * powf(1.07f, (f32)(playerStats->level - 1)); Audio_PlaySoundVar(ASSET_SOUND_TYPE_LEVEL_UP, false);
    if (++globalVariables.levelUpState.pendingCount && !globalVariables.levelUpState.bShowLevelUp) { globalVariables.levelUpState.bShowLevelUp = true; HUD_GenerateLevelUpOptions(); }
}
//~ End of XP Implementation

// ~Begin of Render Implementation
void Render_DrawMap() {
    Texture2D grass = Assets_GetSprite(ASSET_SPRITE_TYPE_GRASS);
    SetTextureWrap(grass, TEXTURE_WRAP_REPEAT);
    DrawTextureRec(grass, (Rectangle){0, 0, 12000, 12000}, (Vector2){-6000, -6000}, WHITE);
    DrawRectangleLinesEx((Rectangle){-MAP_HALF_SIZE, -MAP_HALF_SIZE, MAP_HALF_SIZE * 2, MAP_HALF_SIZE * 2}, 10.0f, RED);
}
void Render_DrawProjectile(Entity* projectile) {
    if (!projectile || projectile->type != ENTITY_TYPE_PROJECTILE) return;
    ProjectileType type = projectile->projectile.projectileType;
    f32 radius = projectile->radius, scale = projectile->scale.x;
    if (type == 1) {
        DrawCircleV(projectile->position, 10 * scale, BLUE);
        DrawCircleLinesV(projectile->position, 10 * scale, SKYBLUE);
    } else if (type == 2) {
        DrawCircleV(projectile->position, 15 * scale, ORANGE);
        DrawCircleLinesV(projectile->position, 15 * scale, RED);
    } else if (type == 3) {
        DrawCircleV(projectile->position, 20 * scale, BLACK);
        DrawCircleLinesV(projectile->position, 20 * scale, GRAY);
    } else if (type == 4) {
        DrawPoly(projectile->position, 3, 30 * scale, 0, LIME);
        DrawPolyLines(projectile->position, 3, 30 * scale, 0, GREEN);
    } else if (type == 5) {
        DrawCircleV(projectile->position, radius, ColorAlpha(BLACK, 0.3f));
    } else if (type == 6) {
        DrawCircleV(projectile->position, radius, ColorAlpha(ORANGE, 0.5f));
    } else {
        DrawCircleV(projectile->position, 5, PINK);
    }
}
void Render_DrawEntity(Entity* entity) {
    if (!entity) return;
    if (entity->type == ENTITY_TYPE_XP_CRYSTAL) {
        DrawPoly(entity->position, 4, 8 * entity->scale.x, 0, BLUE);
        DrawPolyLinesEx(entity->position, 4, 8 * entity->scale.x, 0, 2, SKYBLUE);
        return;
    }
    if (entity->type == ENTITY_TYPE_DROP) {
        Color baseColor = GREEN, outlineColor = LIME;
        if (entity->drop.dropType == DROP_TYPE_POWERUP) {
            PowerUpType powerUpType = entity->drop.powerUpType;
            baseColor = (powerUpType == 1 ? YELLOW : (powerUpType == 2 ? RED : (powerUpType == 4 ? DARKBLUE : WHITE)));
            outlineColor = (powerUpType == 1 ? GOLD : (powerUpType == 2 ? MAROON : (powerUpType == 4 ? BLUE : LIGHTGRAY)));
        }
        DrawPoly(entity->position, 4, 12 * entity->scale.x, 0, baseColor);
        DrawPolyLinesEx(entity->position, 4, 12 * entity->scale.x, 0, 2, outlineColor);
        return;
    }
    if (entity->visualType == VISUAL_TYPE_UNDEFINED) return;
    u8 spriteID = (entity->visualType == VISUAL_TYPE_ANIMATED_SPRITE ? entity->animatedSprite.spriteID : (entity->visualType == VISUAL_TYPE_ANIMATED_STATIC_SPRITE ? entity->animatedStaticSprite.spriteID : entity->sprite.spriteID));
    Texture2D texture = Assets_GetSprite(spriteID);
    Color tint = WHITE;
    if (entity->type == ENTITY_TYPE_ENEMY) {
        EnemyType enemyType = entity->enemyCharacter.enemyType;
        tint = (enemyType == 1 ? WHITE : (enemyType == 2 ? ORANGE : (enemyType == 3 ? PURPLE : RED)));
    }
    Rectangle source = {0, 0, (f32)texture.width, (f32)texture.height};
    Vector2 origin = {texture.width * entity->scale.x / 2.0f, texture.height * entity->scale.y};
    if (entity->visualType == VISUAL_TYPE_ANIMATED_SPRITE) {
        f32 frameWidth = (f32)texture.width / (entity->animatedSprite.frameCount + 1);
        source = (Rectangle){entity->animatedSprite.currentFrame * frameWidth, 0, entity->animatedSprite.flipX ? -frameWidth : frameWidth, (f32)texture.height};
        origin.x = (frameWidth * entity->scale.x) / 2.0f;
    } else if ((entity->visualType == VISUAL_TYPE_ANIMATED_STATIC_SPRITE ? entity->animatedStaticSprite.flipX : entity->sprite.flipX)) {
        source.width = -source.width;
    }
    f32 bobOffset = (entity->visualType == VISUAL_TYPE_ANIMATED_STATIC_SPRITE ? sinf(entity->animatedStaticSprite.animationTimer / entity->animatedStaticSprite.animationDuration * 2 * PI) * 10 : 0);
    Rectangle destination = {entity->position.x, entity->position.y + bobOffset, (source.width < 0 ? -source.width : source.width) * entity->scale.x, texture.height * entity->scale.y};
    f32 flashIntensity = (entity->type == ENTITY_TYPE_ENEMY && entity->enemyCharacter.flashTimer > 0) ? 1.0f : ((entity->type == ENTITY_TYPE_PLAYER && entity->character.flashTimer > 0 && ((i32)(entity->character.flashTimer * 10) % 2) == 0) ? 1.0f : 0);
    if (entity == Global_GetPlayer() && entity->type == ENTITY_TYPE_UNDEFINED) {
        if (entity->character.deathFadeTimer <= 0) return;
        tint = ColorAlpha(tint, entity->character.deathFadeTimer / 2.0f);
    }
    if (flashIntensity > 0) {
        BeginShaderMode(globalVariables.assets.flashShader);
        SetShaderValue(globalVariables.assets.flashShader, globalVariables.assets.flashIntensityLoc, &flashIntensity, SHADER_UNIFORM_FLOAT);
    }
    DrawTexturePro(texture, source, destination, origin, 0, tint);
    if (flashIntensity > 0) EndShaderMode();
}
void Render_DrawAllEntitiesSorted() {
    static i32 bucketCounts[500], bucketOffsets[500], currentOffsets[500];
    static u16 sortedIndices[MAX_ENTITIES_AMOUNT];
    memset(bucketCounts, 0, sizeof(bucketCounts));
    for (i32 index = 0; index < globalVariables.lastEntityIndex; index++) {
        if (globalVariables.entities[index].type == ENTITY_TYPE_DAMAGE_POPUP) continue;
        i32 bucketIndex = (i32)((globalVariables.entities[index].position.y + 5000) / 20.0f);
        bucketCounts[(i32)Clamp(bucketIndex, 0, 499)]++;
    }
    bucketOffsets[0] = 0;
    for (int index = 1; index < 500; index++) bucketOffsets[index] = bucketOffsets[index - 1] + bucketCounts[index - 1];
    memcpy(currentOffsets, bucketOffsets, sizeof(currentOffsets));
    for (i32 index = 0; index < globalVariables.lastEntityIndex; index++) {
        if (globalVariables.entities[index].type == ENTITY_TYPE_DAMAGE_POPUP) continue;
        i32 bucketIndex = (i32)((globalVariables.entities[index].position.y + 5000) / 20.0f);
        sortedIndices[currentOffsets[(i32)Clamp(bucketIndex, 0, 499)]++] = index;
    }
    if (globalVariables.deathAuraIndex < globalVariables.lastEntityIndex) Render_DrawProjectile(&globalVariables.entities[globalVariables.deathAuraIndex]);
    i32 totalCount = 0;
    for (int index = 0; index < 500; index++) totalCount += bucketCounts[index];
    for (i32 index = 0; index < totalCount; index++) {
        Entity* entity = &globalVariables.entities[sortedIndices[index]];
        if (entity->type == ENTITY_TYPE_PROJECTILE) {
            if (entity->projectile.projectileType != 5) Render_DrawProjectile(entity);
        } else {
            Render_DrawEntity(entity);
        }
    }
    for (i32 index = 0; index < globalVariables.lastEntityIndex; index++) {
        Entity* entity = &globalVariables.entities[index];
        if (entity->type != ENTITY_TYPE_DAMAGE_POPUP) continue;
        char text[16];
        sprintf(text, "%.0f", entity->damagePopup.amount);
        f32 fontSize = 20 * entity->scale.x, alpha = entity->damagePopup.timer > 0.2f ? 1.0f - sinf((entity->damagePopup.timer - 0.2f) / 0.5f * PI * 0.5f) : 1.0f;
        Vector2 textSize = MeasureTextEx(GetFontDefault(), text, fontSize, 1), textPosition = {entity->position.x - textSize.x / 2, entity->position.y - textSize.y / 2};
        for (int offsetX = -1; offsetX <= 1; offsetX += 2) {
            for (int offsetY = -1; offsetY <= 1; offsetY += 2) {
                DrawTextEx(GetFontDefault(), text, (Vector2){textPosition.x + offsetX, textPosition.y + offsetY}, fontSize, 1, ColorAlpha(BLACK, alpha));
            }
        }
        DrawTextEx(GetFontDefault(), text, textPosition, fontSize, 1, ColorAlpha(YELLOW, alpha));
    }
}
// ~End of Render Implementation

// ~Begin of Spawner Implementation
SpawnerData Spawner_GenerateSpawnerData() {
    SpawnerData data = { .delayBetweenSpawns = 1.5f, .spawnTimer = 1.5f };
    SpawnDefinition defs[] = {
        {1, 1, { 1,  1}, { 800, 1000}, 100,  0}, {2, 2, { 5,  8}, { 900, 1100},  30,  0}, {1, 4, {10, 15}, {1000, 1200},  20,  0},
        {3, 1, { 1,  1}, {1100, 1300},  10, 15}, {2, 3, { 5, 10}, { 900, 1100},  25,  0}, {1, 2, {11, 18}, { 900, 1100},  35,  0},
        {1, 3, {12, 19}, { 900, 1100},  35,  0}, {2, 2, {15, 25}, { 800, 1000},  40,  5}, {2, 4, {20, 30}, {1000, 1200},  30, 10},
        {3, 2, { 2,  3}, {1100, 1300},  15, 25}, {2, 4, {30, 45}, { 700,  900},  25, 20}, {1, 3, {40, 60}, {1000, 1200},  20, 35},
        {3, 2, { 5,  8}, {1200, 1400},  10, 45}, {2, 2, {50, 80}, { 900, 1100},  30, 60}
    };

    for (int i = 0; i < MAX_SPAWN_DEFINITION; i++) {
        data.spawnsDefinitions[i] = defs[i];
    }
    return data;
}

void Spawner_ProcessSpawnLogic(f32 deltaTime) {
    Entity* player = Global_GetPlayer();
    if (player->type == ENTITY_TYPE_UNDEFINED) return;
    if ((globalVariables.spawnerData.spawnTimer -= deltaTime) > 0) return;

    SpawnerData* data = &globalVariables.spawnerData;
    data->currentDifficulty = (globalVariables.gameTimer / 60.0f) * 10.0f;

    f32 multiplier = (globalVariables.eventState.activeEventType == EVENT_TYPE_SWARM ? 3.0f : 
                     (globalVariables.eventState.activeEventType == EVENT_TYPE_BOSS  ? 1.5f : 1.0f));
    
    data->spawnTimer = (data->delayBetweenSpawns / powf(1.15f, floorf(globalVariables.gameTimer / 60.0f))) / multiplier;

    i32 totalWeight = 0;
    for (int i = 0; i < MAX_SPAWN_DEFINITION; i++) {
        if (data->currentDifficulty >= data->spawnsDefinitions[i].Difficulty) {
            totalWeight += data->spawnsDefinitions[i].chanceToSpawn;
        }
    }

    if (totalWeight <= 0) return;

    i32 randomWeight = GetRandomValue(0, totalWeight - 1), cumulativeWeight = 0;
    SpawnDefinition* def = NULL;

    for (int i = 0; i < MAX_SPAWN_DEFINITION; i++) {
        if (data->currentDifficulty < data->spawnsDefinitions[i].Difficulty) continue;
        if ((cumulativeWeight += data->spawnsDefinitions[i].chanceToSpawn) > randomWeight) {
            def = &data->spawnsDefinitions[i];
            break;
        }
    }

    if (!def) return;

    u16 amount = Helper_GetRandomu16InRange(def->amountToSpawnRange);
    f32 dist = Helper_GetRandomf32InRange(def->distanceToSpawnRange);
    f32 angle = (f32)GetRandomValue(0, 360) * (PI/180.0f);

    Vector2 basePos = {
        player->position.x + cosf(angle) * dist, 
        player->position.y + sinf(angle) * dist
    };

    for (i32 i = 0; i < amount; i++) {
        Entity enemy = Enemy_GenerateEnemy(def->enemyType);
        
        if (def->spawnType == SPAWN_TYPE_SINGLE) {
            enemy.position = basePos;
            Global_AddEntity(&enemy);
            break;
        }

        if (def->spawnType == SPAWN_TYPE_CLUSTER) {
            enemy.position = (Vector2){basePos.x + GetRandomValue(-100, 100), basePos.y + GetRandomValue(-100, 100)};
        } else if (def->spawnType == SPAWN_TYPE_LINE) {
            Vector2 toSpawn = Vector2Subtract(basePos, player->position);
            Vector2 lineDir = Vector2Normalize((Vector2){-toSpawn.y, toSpawn.x});
            enemy.position = Vector2Add(basePos, Vector2Scale(lineDir, (i - (amount / 2.0f)) * 70.0f));
        } else if (def->spawnType == SPAWN_TYPE_AROUND) {
            f32 a = (f32)i * (2.0f * PI / (f32)amount);
            enemy.position = (Vector2){player->position.x + cosf(a) * dist, player->position.y + sinf(a) * dist};
        }
        Global_AddEntity(&enemy);
    }
}
// ~End of Spawner Implementation

//~ Begin of Weapon Implementation
// Specialized Weapon Actions
typedef void (*WeaponTickFn)(WeaponData* weaponData, WeaponDefinition* weaponDefinition, Entity* player, f32 deltaTime, f32 attackSpeed);
static WeaponTickFn weaponTickActions[WEAPON_TYPE_COUNT];
static void Action_CrystalWand(WeaponData* weaponData, WeaponDefinition* weaponDefinition, Entity* player, f32 deltaTime, f32 attackSpeed) {
    if (weaponData->burstRemaining > 0 && (weaponData->burstTimer -= deltaTime) <= 0) {
        i32 targetId = -1; f32 minDistance = 99999.0f;
        for (i32 entityIndex = 0; entityIndex < globalVariables.lastEntityIndex; entityIndex++) if (globalVariables.entities[entityIndex].type == ENTITY_TYPE_ENEMY) { f32 distance = Vector2Distance(player->position, globalVariables.entities[entityIndex].position); if (distance < minDistance) { minDistance = distance; targetId = entityIndex; }}
        if (targetId != -1) { Entity projectile = Projectile_Spawn(PROJECTILE_TYPE_CRYSTAL_SHARD, player->position, Vector2Scale(Vector2Normalize(Vector2Subtract(globalVariables.entities[targetId].position, player->position)), 600.0f), weaponDefinition->damage, 3.0f, weaponDefinition->crystal.penetration); Global_AddEntity(&projectile); weaponData->burstRemaining--; weaponData->burstTimer = 0.1f; }
    }
    if ((weaponData->attackTimer -= deltaTime) <= 0) { weaponData->attackTimer = weaponDefinition->delayBetweenAttacks / attackSpeed; weaponData->burstRemaining = weaponDefinition->projectileAmount; weaponData->burstTimer = 0.0f; }
}
static void Action_FireballRing(WeaponData* weaponData, WeaponDefinition* weaponDefinition, Entity* player, f32 deltaTime, f32 attackSpeed) {
    if ((weaponData->attackTimer -= deltaTime) <= 0) {
        weaponData->attackTimer = weaponDefinition->delayBetweenAttacks / attackSpeed; Vector2 directions[4] = {{0,-1}, {0,1}, {-1,0}, {1,0}};
        for (i32 directionIndex = 0; directionIndex < 4; directionIndex++) for (i32 amountIndex = 0; amountIndex < weaponDefinition->projectileAmount; amountIndex++) {
            Entity projectile = Projectile_Spawn(PROJECTILE_TYPE_FIREBALL, player->position, Vector2Scale(Vector2Rotate(directions[directionIndex], (amountIndex - (weaponDefinition->projectileAmount - 1) / 2.0f) * 0.2f), 400.0f), weaponDefinition->damage, 4.0f, 1);
            projectile.projectile.explosive.explosionRadius = weaponDefinition->fireball.explosionRadius; projectile.projectile.explosive.explosionDamageMultiplier = weaponDefinition->fireball.explosionDamageMultipler; Global_AddEntity(&projectile);
        }
    }
}
static void Action_BombShoes(WeaponData* weaponData, WeaponDefinition* weaponDefinition, Entity* player, f32 deltaTime, f32 attackSpeed) {
    if ((weaponData->attackTimer -= deltaTime) <= 0) {
        weaponData->attackTimer = weaponDefinition->delayBetweenAttacks / attackSpeed; Entity projectile = Projectile_Spawn(PROJECTILE_TYPE_BOMB, player->position, (Vector2){0}, weaponDefinition->damage, 10.0f, 1);
        projectile.projectile.timer = weaponDefinition->bombShoes.delayToExplode; projectile.radius = weaponDefinition->bombShoes.explosionRadius; Global_AddEntity(&projectile);
    }
}
static void Action_NatureSpikes(WeaponData* weaponData, WeaponDefinition* weaponDefinition, Entity* player, f32 deltaTime, f32 attackSpeed) {
    if ((weaponData->attackTimer -= deltaTime) <= 0) {
        weaponData->attackTimer = weaponDefinition->delayBetweenAttacks / attackSpeed;
        for (i32 amountIndex = 0; amountIndex < weaponDefinition->projectileAmount; amountIndex++) {
            i32 candidates[100], candidateCount = 0;
            for (i32 entityIndex = 0; entityIndex < globalVariables.lastEntityIndex && candidateCount < 100; entityIndex++) if (globalVariables.entities[entityIndex].type == ENTITY_TYPE_ENEMY && Vector2Distance(player->position, globalVariables.entities[entityIndex].position) < weaponDefinition->natureSpikes.rangeToSpawn) candidates[candidateCount++] = entityIndex;
            if (candidateCount > 0) { Entity projectile = Projectile_Spawn(PROJECTILE_TYPE_NATURE_SPIKE, globalVariables.entities[candidates[GetRandomValue(0, candidateCount - 1)]].position, (Vector2){0}, weaponDefinition->damage, weaponDefinition->natureSpikes.spikeDuration, weaponDefinition->natureSpikes.spikeMaxDamage); projectile.radius = 40.0f * globalVariables.playerStats.sizeMultiplier; Global_AddEntity(&projectile); }
        }
    }
}
static void Action_DeathAura(WeaponData* weaponData, WeaponDefinition* weaponDefinition, Entity* player, f32 deltaTime, f32 attackSpeed) {
    if ((weaponData->attackTimer -= deltaTime) <= 0) {
        weaponData->attackTimer = weaponDefinition->delayBetweenAttacks / attackSpeed;
        if (globalVariables.deathAuraIndex >= globalVariables.lastEntityIndex) { Entity projectile = Projectile_Spawn(PROJECTILE_TYPE_DEATH_AURA, player->position, (Vector2){0}, weaponDefinition->damage, 99999.0f, 255); projectile.radius = weaponDefinition->deathAura.size * globalVariables.playerStats.sizeMultiplier; projectile.projectile.timer = 0.0f; Global_AddEntity(&projectile); }
        else { Entity* aura = &globalVariables.entities[globalVariables.deathAuraIndex]; aura->radius = weaponDefinition->deathAura.size * globalVariables.playerStats.sizeMultiplier; aura->projectile.damage = weaponDefinition->damage * globalVariables.playerStats.damageMultiplier; }
    }
}
void Weapon_Init() {
    weaponTickActions[WEAPON_TYPE_CRYSTAL_WAND] = Action_CrystalWand;
    weaponTickActions[WEAPON_TYPE_FIREBALL_RING] = Action_FireballRing;
    weaponTickActions[WEAPON_TYPE_BOMB_SHOES] = Action_BombShoes;
    weaponTickActions[WEAPON_TYPE_NATURE_SPIKES] = Action_NatureSpikes;
    weaponTickActions[WEAPON_TYPE_DEATH_AURA] = Action_DeathAura;
    for (i32 levelIndex = 0; levelIndex < MAX_WEAPON_LEVEL; levelIndex++) {
        f32 levelValue = (f32)levelIndex; InventoryDefinitions* definitions = &globalVariables.InventoryDefinitions;
        definitions->weaponDefinitions[WEAPON_TYPE_CRYSTAL_WAND][levelIndex] = (WeaponDefinition){ 15.1f + levelValue * 5, 4.0f, (levelIndex == 14 ? 8 : 1 + levelIndex / 2), .crystal = {2 + levelValue + (levelIndex == 14 ? 14 : 0)} };
        definitions->weaponDefinitions[WEAPON_TYPE_FIREBALL_RING][levelIndex] = (WeaponDefinition){ 30.0f + levelValue * 8, 5.0f, 1 + (levelIndex >= 4) + (levelIndex >= 8) + (levelIndex >= 12) + (levelIndex >= 14) * 2, .fireball = {100.0f + levelValue * 10, 0.5f + levelValue * 0.1f} };
        definitions->weaponDefinitions[WEAPON_TYPE_BOMB_SHOES][levelIndex] = (WeaponDefinition){ 50.0f + levelValue * 35, fmaxf(2.0f, 8.0f - levelValue * 0.428f), 1, .bombShoes = {100.0f + levelValue * 25, 3.0f - levelValue * 0.071f} };
        definitions->weaponDefinitions[WEAPON_TYPE_NATURE_SPIKES][levelIndex] = (WeaponDefinition){ 10.0f + levelValue * 4, fmaxf(2.0f, 5.5f - levelValue * 0.285f), 1 + levelIndex / 7, .natureSpikes = {400.0f + levelValue * 40, 2.0f + levelValue * 0.142f, 100 + levelValue * 80} };
        definitions->weaponDefinitions[WEAPON_TYPE_DEATH_AURA][levelIndex] = (WeaponDefinition){ 5.1f + levelValue * 1.78f, 0.25f - levelValue * 0.0089f, 1, .deathAura = {150.0f + levelValue * 30} };
    }
}

bool Weapon_AddWeapon(WeaponType weaponType) {
    for (i32 index = 0; index < MAX_WEAPON_CAPACITY; index++) if (globalVariables.inventory.weaponDatas[index].level > 0 && globalVariables.inventory.weaponDatas[index].weaponType == weaponType) { if (globalVariables.inventory.weaponDatas[index].level < MAX_WEAPON_LEVEL) globalVariables.inventory.weaponDatas[index].level++; return true; }
    for (i32 index = 0; index < MAX_WEAPON_CAPACITY; index++) if (globalVariables.inventory.weaponDatas[index].level == 0) { globalVariables.inventory.weaponDatas[index] = (WeaponData){ .weaponType = weaponType, .level = 1, .attackTimer = 0.1f }; return true; }
    return false;
}

void Weapon_ProcessAttack(f32 deltaTime) {
    Entity* player = Global_GetPlayer(); if (!player) return;
    f32 attackSpeed = globalVariables.playerStats.attackSpeedMultiplier * (globalVariables.activePowerUps[POWERUP_TYPE_DOUBLE_TROUBLE].bIsActive ? 2.0f : 1.0f);
    for (i32 weaponIndex = 0; weaponIndex < MAX_WEAPON_CAPACITY; weaponIndex++) {
        WeaponData* weaponData = &globalVariables.inventory.weaponDatas[weaponIndex]; if (weaponData->level == 0) continue;
        WeaponDefinition* weaponDefinition = &globalVariables.InventoryDefinitions.weaponDefinitions[weaponData->weaponType][weaponData->level - 1];
        if (weaponTickActions[weaponData->weaponType]) weaponTickActions[weaponData->weaponType](weaponData, weaponDefinition, player, deltaTime, attackSpeed);
    }
}
// ~End of Weapon Implementation

//~ Begin of Global Implementation
void Global_UpdateGameTimer(f32 deltaTime)
{
    Entity* player = Global_GetPlayer();
    if (player && player->type == ENTITY_TYPE_UNDEFINED) return;

    globalVariables.gameTimer += deltaTime;
    HUD_UpdateData();
}
// ~End of Global Implementation