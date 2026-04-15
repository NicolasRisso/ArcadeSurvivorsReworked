#include "main.h"
#include <assert.h>

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

    // Initialize Audio and Assets
    InitAudioDevice();
    Assets_Init();

    // Init Global Variables
    globalVariables.lastEntityIndex = 0;
    globalVariables.playerIndex = 0;
    globalVariables.deathAuraIndex = 65535;

    Entity player = Player_GeneratePlayer();
    Global_AddEntity(&player);

    globalVariables.playerStats = Player_GeneratePlayerStats();
    globalVariables.camera = Player_GenerateCamera();

    globalVariables.spawnerData = Spawner_GenerateSpawnerData();
    Weapon_GenerateWeaponLevels();
    Relic_GenerateRelicDefinition();
    Weapon_AddWeapon((WeaponType)GetRandomValue(0, WEAPON_TYPE_COUNT - 1));
    HUD_Init();

    SetTargetFPS(240);
    DisableCursor();
}
void Core_ProcessInput()
{

}
void Core_UpdateGame(float deltaTime) {
    Player_ProcessMovement(Global_GetPlayer(), deltaTime);
    Enemy_ProcessAllMovement(deltaTime);
    Spawner_ProcessSpawnLogic(deltaTime);
    Weapon_ProcessAttack(deltaTime);
    Projectile_ProcessAllMovement(deltaTime);
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
        LoadMusicStream("assets/musics/CombatMusic.ogg");

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

    TraceLog(LOG_INFO, "ASSETS: All assets unloaded.");
}
// ~End of Assets Implementation

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

    enemy.visualType = VISUAL_TYPE_SPRITE;
    enemy.sprite.spriteID = ASSET_SPRITE_TYPE_BAT;
    enemy.sprite.flipX = false;

    enemy.enemyCharacter.enemyType = enemyType;

    switch (enemyType) {
    case ENEMY_TYPE_NORMAL:
        enemy.enemyCharacter.health = 50.0f;
        enemy.enemyCharacter.speed = 150.0f;
        enemy.enemyCharacter.xpDropAmount = 10.0f;
        enemy.scale = (Vector2){0.75f, 0.75f};
        break;
    case ENEMY_TYPE_FAST:
        enemy.enemyCharacter.health = 30.0f;
        enemy.enemyCharacter.speed = 225.0f;
        enemy.enemyCharacter.xpDropAmount = 15.0f;
        enemy.scale = (Vector2){0.5f, 0.5f};
        break;
    case ENEMY_TYPE_TANK:
        enemy.enemyCharacter.health = 200.0f;
        enemy.enemyCharacter.speed = 80.0f;
        enemy.enemyCharacter.xpDropAmount = 50.0f;
        enemy.scale = (Vector2){1.5f, 1.5f};
        enemy.radius = 60.0f;
        break;
    case ENEMY_TYPE_BOSS:
        enemy.enemyCharacter.health = 1000.0f;
        enemy.enemyCharacter.speed = 120.0f;
        enemy.enemyCharacter.xpDropAmount = 500.0f;
        enemy.scale = (Vector2){3.0f, 3.0f};
        enemy.radius = 120.0f;
        break;
    }

    return enemy;
}
void Enemy_ProcessAllMovement(float deltaTime)
{
    Entity *player = Global_GetPlayer();

    for (int i = 0; i < globalVariables.lastEntityIndex; i++) {
        Entity *current = &globalVariables.entities[i];
        if (!current->bIsActive || current->type != ENTITY_TYPE_ENEMY)
        continue;

        // Simple AI: Move towards player
        Vector2 direction = Vector2Subtract(player->position, current->position);
        if (Vector2Length(direction) > 0) {
        direction = Vector2Normalize(direction);
        current->velocity.x = direction.x * current->enemyCharacter.speed;
        current->velocity.y = direction.y * current->enemyCharacter.speed;

        // Flip sprite based on movement direction
        if (current->velocity.x > 0)
            current->sprite.flipX = false;
        else if (current->velocity.x < 0)
            current->sprite.flipX = true;
        }

        current->position.x += current->velocity.x * deltaTime;
        current->position.y += current->velocity.y * deltaTime;

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
    }
}
//~ End of Enemy Implementation

//~ Begin of HUD Implementation
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
    DrawRectangle(0, 0, SCREEN_WIDTH, 30, DARKGRAY);
    DrawRectangle(0, 0, (int)(SCREEN_WIDTH * xpPercentage), 30, BLUE);

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
    DrawRectangle(padding, hpY, (int)(hpBarWidth * hpPercentage), hpBarHeight,
                    RED);
    DrawRectangleLines(padding, hpY, hpBarWidth, hpBarHeight, BLACK);

    const char *hpText = TextFormat("%.0f / %.0f", player->character.health,
                                    player->character.maxHealth);
    int hpTextWidth = MeasureText(hpText, 20);
    DrawText(hpText, padding + (hpBarWidth / 2) - (hpTextWidth / 2), hpY + 7, 20,
            WHITE);

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
void Player_ProcessMovement(Entity *player, float deltaTime)
{
    Vector2 direction = {0, 0};

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        direction.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        direction.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        direction.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        direction.x += 1.0f;

    if (Vector2Length(direction) > 0) {
        direction = Vector2Normalize(direction);
        player->velocity.x = direction.x * player->character.speed;
        player->velocity.y = direction.y * player->character.speed;
    } else {
        player->velocity = (Vector2){0, 0};
    }

    // Apply Physics
    player->position.x += player->velocity.x * deltaTime;
    player->position.y += player->velocity.y * deltaTime;

    Collision_MapBorder(player);

    // Update Camera Target
    globalVariables.camera.target = Vector2Lerp(
        globalVariables.camera.target, player->position, 10.0f * deltaTime);

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

//~ Begin of Projectile Implementation
Entity Projectile_Spawn(ProjectileType type, Vector2 pos, Vector2 vel, float damage, float lifeTime, uint8_t penetration)
{
    Entity p = {0};
    p.type = ENTITY_TYPE_PROJECTILE;
    p.bIsActive = true;
    p.position = pos;
    p.velocity = vel;
    p.scale = (Vector2){1.0f, 1.0f};
    p.radius = 10.0f;
    p.projectile.projectileType = type;
    p.projectile.damage = damage * globalVariables.playerStats.damageMultiplier;
    p.projectile.lifeTime = lifeTime;
    p.projectile.penetration = penetration;
    for (int i = 0; i < 16; i++) p.projectile.crystal.hitIds[i] = 65535; // None
    return p;
}
void Projectile_ProcessAllMovement(float deltaTime)
{
    Entity* player = Global_GetPlayer();
    for (int i = 0; i < globalVariables.lastEntityIndex; i++) {
        Entity* p = &globalVariables.entities[i];
        if (!p->bIsActive || p->type != ENTITY_TYPE_PROJECTILE) continue;

        p->projectile.lifeTime -= deltaTime;
        if (p->projectile.lifeTime <= 0) { Global_DestroyEntity(i); continue; }

        switch (p->projectile.projectileType) {
            case PROJECTILE_TYPE_CRYSTAL_SHARD:
            case PROJECTILE_TYPE_FIREBALL:
                p->position.x += p->velocity.x * deltaTime;
                p->position.y += p->velocity.y * deltaTime;
                break;
            case PROJECTILE_TYPE_BOMB:
                p->projectile.explosive.timer -= deltaTime;
                if (p->projectile.explosive.timer <= 0) {
                    Entity exp = Projectile_Spawn(PROJECTILE_TYPE_EXPLOSION, p->position, (Vector2){0,0}, p->projectile.damage, 0.2f, 255);
                    exp.radius = p->radius;
                    Global_AddEntity(&exp);
                    Global_DestroyEntity(i);
                    PlaySound(Assets_GetSound(ASSET_SOUND_TYPE_EXPLOSION));
                    continue;
                }
                break;
            case PROJECTILE_TYPE_DEATH_AURA:
                p->position = player->position; 
                break;
            default: break;
        }

        if (p->projectile.projectileType != PROJECTILE_TYPE_EXPLOSION && 
            p->projectile.projectileType != PROJECTILE_TYPE_BOMB) {
            
            for (int j = 0; j < globalVariables.lastEntityIndex; j++) {
                Entity* enemy = &globalVariables.entities[j];
                if (!enemy->bIsActive || enemy->type != ENTITY_TYPE_ENEMY) continue;

                if (CheckCollisionCircles(p->position, p->radius, enemy->position, enemy->radius)) {
                    bool alreadyHit = false;
                    if (p->projectile.projectileType == PROJECTILE_TYPE_CRYSTAL_SHARD) {
                        for (int k = 0; k < 16; k++) {
                            if (p->projectile.crystal.hitIds[k] == enemy->id) { alreadyHit = true; break; }
                        }
                    }

                    if (!alreadyHit) {
                        enemy->enemyCharacter.health -= p->projectile.damage;
                        PlaySound(Assets_GetSound(ASSET_SOUND_TYPE_DAMAGE));

                        if (p->projectile.projectileType == PROJECTILE_TYPE_CRYSTAL_SHARD) {
                            for (int k = 15; k > 0; k--) p->projectile.crystal.hitIds[k] = p->projectile.crystal.hitIds[k-1];
                            p->projectile.crystal.hitIds[0] = enemy->id;
                        }

                        if (enemy->enemyCharacter.health <= 0) {
                            Global_DestroyEntity(j);
                        }

                        if (p->projectile.projectileType == PROJECTILE_TYPE_FIREBALL) {
                             Entity exp = Projectile_Spawn(PROJECTILE_TYPE_EXPLOSION, p->position, (Vector2){0,0}, p->projectile.damage, 0.2f, 255);
                             exp.radius = p->projectile.explosive.explosionRadius; 
                             Global_AddEntity(&exp);
                             Global_DestroyEntity(i);
                             PlaySound(Assets_GetSound(ASSET_SOUND_TYPE_EXPLOSION));
                             goto next_p;
                        }

                        if (p->projectile.projectileType != PROJECTILE_TYPE_DEATH_AURA && 
                            p->projectile.projectileType != PROJECTILE_TYPE_NATURE_SPIKE) {
                            p->projectile.penetration--;
                            if (p->projectile.penetration <= 0) { Global_DestroyEntity(i); goto next_p; }
                        } else if (p->projectile.projectileType == PROJECTILE_TYPE_NATURE_SPIKE) {
                            p->projectile.penetration--; 
                            if (p->projectile.penetration <= 0) { Global_DestroyEntity(i); goto next_p; }
                        }
                    }
                }
            }
        } else if (p->projectile.projectileType == PROJECTILE_TYPE_EXPLOSION) {
            for (int j = 0; j < globalVariables.lastEntityIndex; j++) {
                Entity* enemy = &globalVariables.entities[j];
                if (!enemy->bIsActive || enemy->type != ENTITY_TYPE_ENEMY) continue;
                if (CheckCollisionCircles(p->position, p->radius, enemy->position, enemy->radius)) {
                    // Explosions no longer use hit tracking to save memory, they deal damage once per frame for very short duration
                    enemy->enemyCharacter.health -= p->projectile.damage;
                    if (enemy->enemyCharacter.health <= 0) Global_DestroyEntity(j);
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
        player->character.health += (player->character.maxHealth - oldMaxHealth);
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
//~ End of Relic Implementation

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

    Texture2D texture =
        Assets_GetSprite(entity->visualType == VISUAL_TYPE_ANIMATED_SPRITE
                            ? entity->animatedSprite.spriteID
                            : entity->sprite.spriteID);

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
        source = (Rectangle){0, 0,
                            entity->sprite.flipX ? -(float)texture.width
                                                : (float)texture.width,
                            (float)texture.height};
        origin = (Vector2){((float)texture.width * entity->scale.x) / 2.0f,
                        (float)texture.height * entity->scale.y};
    }

    Rectangle dest = {entity->position.x, entity->position.y,
                        (source.width < 0 ? -source.width : source.width) *
                            entity->scale.x,
                        (float)texture.height * entity->scale.y};

    DrawTexturePro(texture, source, dest, origin, 0, tint);
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
        DrawPoly(entity->position, 3, 20.0f * entity->scale.x, 0, GREEN);
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
        if (!globalVariables.entities[i].bIsActive)
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
        if (!globalVariables.entities[i].bIsActive)
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
                            .Difficulty = 1.0f};

    // 1: Fast Bat Cluster (Medium)
    data.spawnsDefinitions[1] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_FAST,
                            .spawnType = SPAWN_TYPE_CLUSTER,
                            .amountToSpawnRange = {5, 8},
                            .distanceToSpawnRange = {900, 1100},
                            .chanceToSpawn = 30,
                            .Difficulty = 2.0f};

    // 2: Normal Bat Around (Medium)
    data.spawnsDefinitions[2] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_NORMAL,
                            .spawnType = SPAWN_TYPE_AROUND,
                            .amountToSpawnRange = {10, 15},
                            .distanceToSpawnRange = {1000, 1200},
                            .chanceToSpawn = 20,
                            .Difficulty = 3.0f};

    // 3: Tank Bat Single (Rare)
    data.spawnsDefinitions[3] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_TANK,
                            .spawnType = SPAWN_TYPE_SINGLE,
                            .amountToSpawnRange = {1, 1},
                            .distanceToSpawnRange = {1100, 1300},
                            .chanceToSpawn = 10,
                            .Difficulty = 5.0f};

    // 4: Fast Bat Line (Medium)
    data.spawnsDefinitions[4] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_FAST,
                            .spawnType = SPAWN_TYPE_LINE,
                            .amountToSpawnRange = {5, 10},
                            .distanceToSpawnRange = {900, 1100},
                            .chanceToSpawn = 25,
                            .Difficulty = 4.0f};

    // 5: Normal Bat Cluster (Common)
    data.spawnsDefinitions[1] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_NORMAL,
                            .spawnType = SPAWN_TYPE_CLUSTER,
                            .amountToSpawnRange = {11, 18},
                            .distanceToSpawnRange = {900, 1100},
                            .chanceToSpawn = 35,
                            .Difficulty = 2.0f};

    // 6: Normal Bat Line (Common)
    data.spawnsDefinitions[1] =
        (SpawnDefinition){.enemyType = ENEMY_TYPE_NORMAL,
                            .spawnType = SPAWN_TYPE_LINE,
                            .amountToSpawnRange = {12, 19},
                            .distanceToSpawnRange = {900, 1100},
                            .chanceToSpawn = 35,
                            .Difficulty = 2.0f};

    return data;
}
void Spawner_ProcessSpawnLogic(float deltaTime)
{
    globalVariables.spawnerData.spawnTimer -= deltaTime;
    if (globalVariables.spawnerData.spawnTimer > 0)
        return;

    // Reset Timer
    globalVariables.spawnerData.spawnTimer =
        globalVariables.spawnerData.delayBetweenSpawns;

    // Phase 1: Weighted Selection
    int totalWeight = 0;
    for (int i = 0; i < MAX_SPAWN_DEFINITION; i++)
        totalWeight +=
            globalVariables.spawnerData.spawnsDefinitions[i].chanceToSpawn;

    if (totalWeight <= 0)
        return;

    int roll = GetRandomValue(0, totalWeight - 1);
    int currentWeight = 0;
    SpawnDefinition *selectedDef =
        &globalVariables.spawnerData.spawnsDefinitions[0];

    for (int i = 0; i < MAX_SPAWN_DEFINITION; i++) {
        currentWeight +=
            globalVariables.spawnerData.spawnsDefinitions[i].chanceToSpawn;
        if (roll < currentWeight) {
        selectedDef = &globalVariables.spawnerData.spawnsDefinitions[i];
        break;
        }
    }

    // Phase 2: Execution
    uint16_t amount =
        Helper_GetRandomUint16InRange(selectedDef->amountToSpawnRange);
    float distance =
        Helper_GetRandomFloatInRange(selectedDef->distanceToSpawnRange);
    Entity *player = Global_GetPlayer();

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
        WeaponDefinition* def = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.crystalShard[i];
        def->damage = 10.0f + (i * 5.0f);
        def->delayBetweenAttacks = 4.0f;
        def->projectileAmount = 1 + (i / 2);
        def->crystal.penetration = 2 + (i);
        if (i == MAX_WEAPON_LEVEL - 1) {
            def->projectileAmount = 8;
            def->crystal.penetration = 16;
        }
    }

    // Fireball Ring
    for (int i = 0; i < MAX_WEAPON_LEVEL; i++) {
        WeaponDefinition* def = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.fireballRing[i];
        def->damage = 15.0f + (i * 8.0f);
        def->delayBetweenAttacks = 5.0f;
        def->projectileAmount = 1;
        if (i + 1 >= 5) def->projectileAmount = 2;
        if (i + 1 >= 9) def->projectileAmount = 3;
        if (i + 1 >= 13) def->projectileAmount = 4;
        if (i + 1 >= 15) def->projectileAmount = 6;
        def->fireball.explosionRadius = 100.0f + (i * 10.0f);
        def->fireball.explosionDamageMultipler = 1.0f + (i * 0.1f);
    }

    // Bomb Shoes
    for (int i = 0; i < MAX_WEAPON_LEVEL; i++) {
        WeaponDefinition* def = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.bombShoes[i];
        def->damage = 100.0f + (i * 50.0f);
        def->delayBetweenAttacks = 8.0f - (i * 0.428f); 
        if (def->delayBetweenAttacks < 2.0f) def->delayBetweenAttacks = 2.0f;
        def->bombShoes.delayToExplode = 3.0f - (i * 0.071f); 
        def->bombShoes.explosionRadius = 150.0f + (i * 25.0f);
    }

    // Nature Spikes
    for (int i = 0; i < MAX_WEAPON_LEVEL; i++) {
        WeaponDefinition* def = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.natureSpikes[i];
        def->damage = 10.0f + (i * 4.0f);
        def->delayBetweenAttacks = 6.0f - (i * 0.285f); 
        if (def->delayBetweenAttacks < 2.0f) def->delayBetweenAttacks = 2.0f;
        def->projectileAmount = 1 + (i / 7); 
        def->natureSpikes.spikeDuration = 2.0f + (i * 0.142f); 
        def->natureSpikes.rangeToSpawn = 400.0f + (i * 40.0f);
        def->natureSpikes.spikeMaxDamage = 5 + (i * 5); 
    }

    // Death Aura
    for (int i = 0; i < MAX_WEAPON_LEVEL; i++) {
        WeaponDefinition* def = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.deathAura[i];
        def->damage = 5.0f + (i * 1.78f); 
        def->delayBetweenAttacks = 0.25f - (i * 0.0089f); 
        def->deathAura.size = 150.0f + (i * 30.0f);
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
    if (!player) return;

    for (int i = 0; i < MAX_WEAPON_CAPACITY; i++) {
        WeaponData* weapon = &globalVariables.inventory.weaponDatas[i];
        if (weapon->level == 0) continue;

        WeaponDefinition* def = NULL;
        switch (weapon->weaponType) {
            case WEAPON_TYPE_CRYSTAL_WAND: def = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.crystalShard[weapon->level-1]; break;
            case WEAPON_TYPE_FIREBALL_RING: def = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.fireballRing[weapon->level-1]; break;
            case WEAPON_TYPE_BOMB_SHOES: def = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.bombShoes[weapon->level-1]; break;
            case WEAPON_TYPE_NATURE_SPIKES: def = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.natureSpikes[weapon->level-1]; break;
            case WEAPON_TYPE_DEATH_AURA: def = &globalVariables.InventoryDefinitions.weaponLevelsDefinition.deathAura[weapon->level-1]; break;
        }
        if (!def) continue;

        weapon->attackTimer -= deltaTime;

        if (weapon->weaponType == WEAPON_TYPE_CRYSTAL_WAND && weapon->burstRemaining > 0) {
            weapon->burstTimer -= deltaTime;
            if (weapon->burstTimer <= 0) {
                float minDist = 99999.0f;
                int targetIdx = -1;
                for (int j = 0; j < globalVariables.lastEntityIndex; j++) {
                    if (globalVariables.entities[j].bIsActive && globalVariables.entities[j].type == ENTITY_TYPE_ENEMY) {
                        float d = Vector2Distance(player->position, globalVariables.entities[j].position);
                        if (d < minDist) { minDist = d; targetIdx = j; }
                    }
                }

                if (targetIdx != -1) {
                    Vector2 dir = Vector2Normalize(Vector2Subtract(globalVariables.entities[targetIdx].position, player->position));
                    Entity p = Projectile_Spawn(PROJECTILE_TYPE_CRYSTAL_SHARD, player->position, Vector2Scale(dir, 600.0f), def->damage, 3.0f, def->crystal.penetration);
                    Global_AddEntity(&p);
                    weapon->burstRemaining--;
                    weapon->burstTimer = 0.1f;
                }
            }
        }

        if (weapon->attackTimer <= 0) {
            weapon->attackTimer = def->delayBetweenAttacks / globalVariables.playerStats.attackSpeedMultiplier;

            switch (weapon->weaponType) {
                case WEAPON_TYPE_CRYSTAL_WAND:
                    weapon->burstRemaining = def->projectileAmount;
                    weapon->burstTimer = 0.0f;
                    break;
                case WEAPON_TYPE_FIREBALL_RING: {
                    Vector2 dirs[4] = {{0,-1}, {0,1}, {-1,0}, {1,0}};
                    for (int d = 0; d < 4; d++) {
                        for (int amt = 0; amt < def->projectileAmount; amt++) {
                             Vector2 offsetDir = Vector2Rotate(dirs[d], (amt - (def->projectileAmount-1)/2.0f) * 0.2f);
                             Entity p = Projectile_Spawn(PROJECTILE_TYPE_FIREBALL, player->position, Vector2Scale(offsetDir, 400.0f), def->damage, 4.0f, 1);
                             p.projectile.explosive.explosionRadius = def->fireball.explosionRadius; 
                             Global_AddEntity(&p);
                        }
                    }
                } break;
                case WEAPON_TYPE_BOMB_SHOES: {
                    Entity p = Projectile_Spawn(PROJECTILE_TYPE_BOMB, player->position, (Vector2){0,0}, def->damage, 10.0f, 1);
                    p.projectile.explosive.timer = def->bombShoes.delayToExplode;
                    p.radius = def->bombShoes.explosionRadius;
                    Global_AddEntity(&p);
                } break;
                case WEAPON_TYPE_NATURE_SPIKES: {
                    for (int amt = 0; amt < def->projectileAmount; amt++) {
                        int candidates[100]; int cCount = 0;
                        for (int j = 0; j < globalVariables.lastEntityIndex && cCount < 100; j++) {
                            if (globalVariables.entities[j].bIsActive && globalVariables.entities[j].type == ENTITY_TYPE_ENEMY) {
                                if (Vector2Distance(player->position, globalVariables.entities[j].position) < def->natureSpikes.rangeToSpawn) {
                                    candidates[cCount++] = j;
                                }
                            }
                        }
                        if (cCount > 0) {
                            int target = candidates[GetRandomValue(0, cCount-1)];
                            Entity p = Projectile_Spawn(PROJECTILE_TYPE_NATURE_SPIKE, globalVariables.entities[target].position, (Vector2){0,0}, def->damage, def->natureSpikes.spikeDuration, def->natureSpikes.spikeMaxDamage);
                            p.radius = 40.0f * globalVariables.playerStats.sizeMultiplier;
                            Global_AddEntity(&p);
                        }
                    }
                } break;
                case WEAPON_TYPE_DEATH_AURA: {
                    Entity p = Projectile_Spawn(PROJECTILE_TYPE_DEATH_AURA, player->position, (Vector2){0,0}, def->damage, weapon->attackTimer, 255);
                    p.radius = def->deathAura.size * globalVariables.playerStats.sizeMultiplier;
                    Global_AddEntity(&p);
                } break;
            }
        }
    }
}
// ~End of Weapon Implementation

//~ Begin of Global Implementation
void Global_UpdateGameTimer(float deltaTime)
{
    globalVariables.gameTimer += deltaTime;
    HUD_UpdateData();
}
// ~End of Global Implementation