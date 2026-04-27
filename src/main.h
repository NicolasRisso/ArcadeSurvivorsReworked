#pragma once
#include <stdint.h>
#include "raylib.h"
#include "raymath.h"

// Modern Types
#define f32 float
#define i32 int32_t
#define u16 uint16_t
#define u8 uint8_t

// Global Constants
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define MAX_ENTITIES_AMOUNT 20000

#define MAX_SPAWN_DEFINITION 14

#define MAX_WEAPON_CAPACITY 4
#define MAX_WEAPON_LEVEL 15
#define MAX_RELIC_CAPACITY 4
#define MAX_RELIC_LEVEL 15
#define MAX_ACTIVE_POWERUPS 4


#define MAP_HALF_SIZE 5000

//~ Begin of Utility Structs
typedef struct f32Range { f32 min; f32 max; } f32Range;
typedef struct u16Range { u16 min; u16 max; } u16Range;
//~ End of Utility Structs

// ~Begin of Enums
typedef enum EntityType : u8 {
    ENTITY_TYPE_UNDEFINED = 0,
    ENTITY_TYPE_CHARACTER = 1,
    ENTITY_TYPE_PLAYER = 2,
    ENTITY_TYPE_PROJECTILE = 3,
    ENTITY_TYPE_ENEMY = 4,
    ENTITY_TYPE_DAMAGE_POPUP = 5,
    ENTITY_TYPE_XP_CRYSTAL = 6,
    ENTITY_TYPE_DROP = 7
} EntityType;

typedef enum ProjectileType : u8 {
    PROJECTILE_TYPE_UNDEFINED = 0,
    PROJECTILE_TYPE_CRYSTAL_SHARD = 1,
    PROJECTILE_TYPE_FIREBALL = 2,
    PROJECTILE_TYPE_BOMB = 3,
    PROJECTILE_TYPE_NATURE_SPIKE = 4,
    PROJECTILE_TYPE_DEATH_AURA = 5,
    PROJECTILE_TYPE_EXPLOSION = 6
} ProjectileType;

typedef enum WeaponType : u8 {
    WEAPON_TYPE_UNDEFINED = 0,
    WEAPON_TYPE_CRYSTAL_WAND = 1,
    WEAPON_TYPE_FIREBALL_RING = 2,
    WEAPON_TYPE_BOMB_SHOES = 3,
    WEAPON_TYPE_NATURE_SPIKES = 4,
    WEAPON_TYPE_DEATH_AURA = 5,
    WEAPON_TYPE_COUNT = 6
} WeaponType;

typedef enum RelicType : u8 {
    RELIC_TYPE_UNDEFINED = 0,
    RELIC_TYPE_HEALTH = 1,
    RELIC_TYPE_DAMAGE = 2,
    RELIC_TYPE_ATTACK_SPEED = 3,
    RELIC_TYPE_MOVEMENT_SPEED = 4,
    RELIC_TYPE_SIZE = 5,
    RELIC_TYPE_LIFE_STEAL = 6,
    RELIC_TYPE_XP = 7,
    RELIC_TYPE_COUNT = 8
} RelicType;

typedef enum EnemyType : u8 {
    ENEMY_TYPE_UNDEFINED = 0,
    ENEMY_TYPE_NORMAL = 1,
    ENEMY_TYPE_FAST = 2,
    ENEMY_TYPE_TANK = 3,
    ENEMY_TYPE_BOSS = 4
} EnemyType;

typedef enum PowerUpType : u8 {
    POWERUP_TYPE_UNDEFINED = 0,
    POWERUP_TYPE_NUKE = 1,
    POWERUP_TYPE_DOUBLE_TROUBLE = 2,
    POWERUP_TYPE_TIME_FREEZE = 3,
    POWERUP_TYPE_MAGNET = 4
} PowerUpType;

typedef enum InstantDropType : u8 {
    INSTANT_DROP_TYPE_UNDEFINED = 0,
    INSTANT_DROP_TYPE_LIFE = 1,
    INSTANT_DROP_TYPE_BIG_LIFE = 2
} InstantDropType;

typedef enum DropType : u8 {
    DROP_TYPE_UNDEFINED = 0,
    DROP_TYPE_INSTANT = 1,
    DROP_TYPE_POWERUP = 2
} DropType;

typedef enum AssetSpriteType : u8 {
    ASSET_SPRITE_TYPE_UNDEFINED = 0,
    ASSET_SPRITE_TYPE_PLAYER = 1,
    ASSET_SPRITE_TYPE_GRASS = 2,
    ASSET_SPRITE_TYPE_BAT = 3,
    ASSET_SPRITE_TYPE_COUNT = 4
} AssetSpriteType;

typedef enum AssetSoundType : u8 {
    ASSET_SOUND_TYPE_UNDEFINED = 0,
    ASSET_SOUND_TYPE_DAMAGE = 1,
    ASSET_SOUND_TYPE_EXPLOSION = 2,
    ASSET_SOUND_TYPE_LEVEL_UP = 3,
    ASSET_SOUND_TYPE_XP_GAIN = 4,
    ASSET_SOUND_TYPE_PLAYER_DAMAGE = 5,
    ASSET_SOUND_TYPE_COUNT = 6
} AssetSoundType;

typedef enum AssetMusicType : u8 {
    ASSET_MUSIC_TYPE_UNDEFINED = 0,
    ASSET_MUSIC_TYPE_COMBAT = 1,
    ASSET_MUSIC_TYPE_COUNT = 2
} AssetMusicType;

typedef enum VisualType : u8 {
    VISUAL_TYPE_UNDEFINED = 0, // For entities that have no sprite
    VISUAL_TYPE_SPRITE = 1, // For entities with sprites
    VISUAL_TYPE_ANIMATED_SPRITE = 2, // For entities with animated sprites
    VISUAL_TYPE_ANIMATED_STATIC_SPRITE = 3 // For entities with animated static sprites (e.g. bounce)
} VisualType;

typedef enum SpawnType : u8 {
    SPAWN_TYPE_UNDEFINED = 0,
    SPAWN_TYPE_SINGLE = 1,
    SPAWN_TYPE_CLUSTER = 2,
    SPAWN_TYPE_LINE = 3,
    SPAWN_TYPE_AROUND = 4
} SpawnType;

typedef enum UpgradeType : u8 {
    UPGRADE_TYPE_UNDEFINED = 0,
    UPGRADE_TYPE_WEAPON = 1,
    UPGRADE_TYPE_RELIC = 2
} UpgradeType;

typedef enum GameEventType : u8 {
    EVENT_TYPE_UNDEFINED = 0,
    EVENT_TYPE_SWARM = 1,
    EVENT_TYPE_BOSS = 2
} GameEventType;
// ~End of Enums

typedef struct Drop {
    f32 radius;
    DropType dropType;
    union{ PowerUpType powerUpType; InstantDropType instantDropType; };
} Drop;

typedef struct DropsDefinition{
    f32 chanceToPowerUp;
    f32 chanceToInstant;
} DropsDefinition;

// ~Begin of Structs
typedef struct SpriteData {
    u8 spriteID;
    f32 flipX;
} SpriteData;

typedef struct AnimatedSpriteData {
    u8 spriteID;
    u8 frameCount;
    u8 currentFrame;
    f32 frameTimer;
    f32 frameTime;
    bool flipX;
} AnimatedSpriteData;

typedef struct AnimatedStaticSpriteData {
    u8 spriteID;
    f32 animationDuration;
    f32 animationTimer;
    bool flipX;
} AnimatedStaticSpriteData;

typedef struct Character{
    f32 health;
    f32 maxHealth;
    f32 speed;
    f32 flashTimer;
    f32 invulnerableTimer;
    f32 deathFadeTimer;
} Character;

typedef struct EnemyCharacter{
    f32 health;
    f32 speed;
    f32 damage;
    EnemyType enemyType;
    f32 xpDropAmount;
    f32 flashTimer;
} EnemyCharacter;

typedef struct Projectile{
    ProjectileType projectileType;
    f32 damage;
    f32 lifeTime;
    u8 penetration;
    u16 ownerID;
    f32 timer;
    union { struct { u16 hitIds[16];} hitTracking; struct { f32 explosionRadius; f32 explosionDamageMultiplier; } explosive; };
} Projectile;

typedef struct DamagePopup {
    f32 amount;
    f32 timer;
} DamagePopup;

typedef struct XPCrystal {
    f32 amount;
    bool bIsMagnetized;
} XPCrystal;

typedef struct Entity{
    EntityType type;
    u16 id;
    u8 spriteID;
    Vector2 position;
    Vector2 velocity;
    Vector2 scale;
    f32 radius;
    union { Character character; EnemyCharacter enemyCharacter; Projectile projectile; DamagePopup damagePopup; XPCrystal xpCrystal; Drop drop; };
    VisualType visualType;
    union { SpriteData sprite; AnimatedSpriteData animatedSprite; AnimatedStaticSpriteData animatedStaticSprite; };
} Entity;

typedef struct Assets{
    Texture2D sprites[ASSET_SPRITE_TYPE_COUNT];
    Sound sounds[ASSET_SOUND_TYPE_COUNT];
    Music musics[ASSET_MUSIC_TYPE_COUNT];
    Shader flashShader;
    i32 flashIntensityLoc;
} Assets;

typedef struct SpawnDefinition{
    EnemyType enemyType;
    SpawnType spawnType;
    u16Range amountToSpawnRange;
    f32Range distanceToSpawnRange;
    u16 chanceToSpawn;
    f32 Difficulty;
} SpawnDefinition;

typedef struct SpawnerData{
    f32 delayBetweenSpawns;
    f32 spawnTimer;
    f32 currentDifficulty;
    SpawnDefinition spawnsDefinitions[MAX_SPAWN_DEFINITION];
} SpawnerData;

// Removed duplicate WeaponData

typedef struct WeaponCrystalDefinition {
    u8 penetration;
} WeaponCrystalDefinition;

typedef struct WeaponFireballDefinition {
    f32 explosionRadius;
    f32 explosionDamageMultipler;
} WeaponFireballDefinition;

typedef struct WeaponBombShoesDefinition {
    f32 explosionRadius;
    f32 delayToExplode;
} WeaponBombShoesDefinition;

typedef struct WeaponNatureSpikesDefinition {
    f32 rangeToSpawn;
    f32 spikeDuration;
    f32 spikeMaxDamage;
} WeaponNatureSpikesDefinition;

typedef struct WeaponDeathAuraDefinition {
    f32 size;
} WeaponDeathAuraDefinition;

typedef struct WeaponDefinition {
    f32 damage;
    f32 delayBetweenAttacks;
    u8 projectileAmount;
    union { WeaponCrystalDefinition crystal; WeaponFireballDefinition fireball; WeaponBombShoesDefinition bombShoes; WeaponNatureSpikesDefinition natureSpikes; WeaponDeathAuraDefinition deathAura; };
} WeaponDefinition;

typedef struct WeaponData {
    WeaponType weaponType;
    u8 level;
    f32 attackTimer;
    u8 burstRemaining;
    f32 burstTimer;
} WeaponData;

typedef struct WeaponLevelsDefinition {
    WeaponDefinition crystalShard[MAX_WEAPON_LEVEL];
    WeaponDefinition fireballRing[MAX_WEAPON_LEVEL];
    WeaponDefinition bombShoes[MAX_WEAPON_LEVEL];
    WeaponDefinition natureSpikes[MAX_WEAPON_LEVEL];
    WeaponDefinition deathAura[MAX_WEAPON_LEVEL];
} WeaponLevelsDefinition;

typedef struct RelicDefinition {
    f32 multiplier;
} RelicDefinition;

typedef struct RelicData {
    u8 level;
    RelicType relicType;
    RelicDefinition RelicDefinition;
} RelicData;

typedef struct Inventory {
    WeaponData weaponDatas[MAX_WEAPON_CAPACITY];
    RelicData relicDatas[MAX_RELIC_CAPACITY];
} Inventory;

typedef struct InventoryDefinitions {
    WeaponLevelsDefinition weaponLevelsDefinition;
    RelicDefinition RelicDefinitions[RELIC_TYPE_COUNT];
} InventoryDefinitions;

typedef struct UpgradeOption {
    UpgradeType type;
    union { WeaponType weapon; RelicType relic; };
    u8 level;
} UpgradeOption;

typedef struct LevelUpState {
    bool bShowLevelUp;
    UpgradeOption options[3];
    u8 pendingCount;
    u8 selectedIndex;
} LevelUpState;

typedef struct PlayerStats{
    f32 currentXP;
    f32 nextLevelXP;
    u16 level;
    f32 healthMultiplier;
    f32 damageMultiplier;
    f32 attackSpeedMultiplier;
    f32 movementSpeedMultiplier;
    f32 sizeMultiplier;
    f32 lifeStealMultiplier;
    f32 xpMultiplier;
} PlayerStats;
 
typedef struct GameEventState {
    GameEventType activeEventType;
    f32 activeEventTimer;
    f32 swarmCycleTimer;
    f32 bossCycleTimer;
} GameEventState;

typedef struct ActivePowerUp {
    PowerUpType type;
    f32 remainingTime;
    bool bIsActive;
} ActivePowerUp;

typedef struct GlobalVariables{
    Assets assets;
    Camera2D camera;
    PlayerStats playerStats;
    Entity entities[MAX_ENTITIES_AMOUNT];
    u16 lastEntityIndex;
    u16 playerIndex;
    DropsDefinition dropsDefinition;
    Inventory inventory;
    InventoryDefinitions InventoryDefinitions;
    SpawnerData spawnerData;
    f32 gameTimer;
    u16 deathAuraIndex;
    u16 nextEntityId;
    bool bShowInventory;
    LevelUpState levelUpState;
    GameEventState eventState;
    ActivePowerUp activePowerUps[MAX_ACTIVE_POWERUPS];
    char hudEventMessage[64];
    f32 hudEventTimer;
} GlobalVariables;
// ~End of Structs

// Declaration of Global Variables
extern GlobalVariables globalVariables;

// The game main loop
i32 main(void);

//~ Begin of Core Implementation
void Core_InitGame();
void Core_ProcessInput();
void Core_UpdateGame(f32 deltaTime);
void Core_RenderGraphics();
void Core_CloseGame();

i32 Core_IsGameReadyToClose();
//~ End of Core Implementation

//~ Begin of Assets Implementation
void Assets_Init();
void Assets_Unload();
Texture2D Assets_GetSprite(AssetSpriteType spriteID);
Sound Assets_GetSound(AssetSoundType soundID);
Music Assets_GetMusic(AssetMusicType musicID);
//~ End of Assets Implementation

//~ Begin of Audio Implementation
void Audio_PlaySoundVar(AssetSoundType type, bool bIsSpammable);
void Audio_Update(f32 deltaTime);
//~ End of Audio Implementation

//~ Begin of Collision Implementation
void Collision_MapBorder(Entity* entity);
//~ End of Collision Implementation

//~ Begin of Drop Implementation
void Drop_GenerateDrop(Vector2 pos);
void Drop_ProcessPickUp();
//~ End of Drop Implementation

//~ Begin of Enemy Implementation
Entity Enemy_GenerateEnemy(EnemyType enemyType);
void Enemy_ProcessAllMovement(f32 deltaTime);
//~ End of Enemy Implementation

//~ Begin of Event Implementation
void Event_TriggerEvent(GameEventType type);
//~ End of Event Implementation

//~ Begin of HUD Implementation
void HUD_Init();
void HUD_UpdateData();
void HUD_Draw();
void HUD_DrawInventory();
void HUD_DrawLevelUp();
void HUD_DrawEvent();
void HUD_GenerateLevelUpOptions();
void HUD_DrawPowerUps();
void HUD_DrawPickupNotification();
//~ End of HUD Implementation

//~ Begin of Player Implementation
Camera2D Player_GenerateCamera();
Entity Player_GeneratePlayer();
PlayerStats Player_GeneratePlayerStats();
void Player_ProcessMovement(Entity* player, f32 deltaTime);
void Player_AnimateMovement(Entity* player, f32 deltaTime);
//~ End of Player Implementation

//~ Begin of Popup Implementation
Entity Popup_SpawnDamagePopup(Vector2 pos, f32 amount);
void Popup_UpdateAll(f32 deltaTime);
//~ End of Popup Implementation

//~ Begin of Projectile Implementation
Entity Projectile_Spawn(ProjectileType type, Vector2 pos, Vector2 vel, f32 damage, f32 lifeTime, u8 penetration);
void Projectile_ProcessAllMovement(f32 deltaTime);
//~ End of Projectile Implementation

//~ Begin of Relic Implementation
void Relic_GenerateRelicDefinition();
void Relic_AddRelic(RelicType relicType); //This function also levels up relics
void Relic_ApplyEffects();
const char* Relic_GetRelicName(RelicType relicType);
//~ End of Relic Implementation

//~ Begin of Render Implementation
void Render_DrawMap();
void Render_DrawAllEntitiesSorted();
void Render_DrawEntity(Entity* entity);
//~ End of Render Implementation

//~ Begin of Spawner Implementation
SpawnerData Spawner_GenerateSpawnerData();
void Spawner_ProcessSpawnLogic(f32 deltaTime);
//~ End of Spawner Implementation

//~ Begin of Weapon Implementation
void Weapon_GenerateWeaponLevels();
bool Weapon_AddWeapon(WeaponType weaponType); //This function also levels up weapons
void Weapon_ProcessAttack(f32 deltaTime);
const char* Weapon_GetWeaponName(WeaponType weaponType);
//~ End of Weapon Implementation

//~ Begin of XP Implementation
void XP_GenerateXPCrystal(Vector2 position, f32 amount);
void XP_MoveCrystals(f32 deltaTime);
void XP_GrantXP(f32 amount);
void XP_LevelUp();
//~ End of XP Implementation

//~ Begin of Global Implementation
void Global_UpdateGameTimer(f32 deltaTime);
inline static Entity* Global_GetPlayer()
{
    return &globalVariables.entities[globalVariables.playerIndex];
}
inline static bool Global_AddEntity(Entity* entity)
{
    if (!entity) return false;
    if (globalVariables.lastEntityIndex >= MAX_ENTITIES_AMOUNT) return false;

    entity->id = globalVariables.nextEntityId++;
    globalVariables.entities[globalVariables.lastEntityIndex] = *entity;
    if (entity->type == ENTITY_TYPE_PROJECTILE && entity->projectile.projectileType == PROJECTILE_TYPE_DEATH_AURA) {
        globalVariables.deathAuraIndex = globalVariables.lastEntityIndex;
    }
    globalVariables.lastEntityIndex++;

    return true;
}
inline static bool Global_DestroyEntity(u16 entityIndex)
{
    if (entityIndex >= globalVariables.lastEntityIndex || entityIndex >= MAX_ENTITIES_AMOUNT) return false;

    if (entityIndex == globalVariables.deathAuraIndex) {
        globalVariables.deathAuraIndex = 65535;
    }

    if (globalVariables.lastEntityIndex > 1 && entityIndex < globalVariables.lastEntityIndex - 1) {
        globalVariables.entities[entityIndex] = globalVariables.entities[globalVariables.lastEntityIndex - 1];
        if (globalVariables.deathAuraIndex == globalVariables.lastEntityIndex - 1) {
            globalVariables.deathAuraIndex = entityIndex;
        }
    }
    
    globalVariables.entities[globalVariables.lastEntityIndex - 1].type = ENTITY_TYPE_UNDEFINED;
    globalVariables.lastEntityIndex--;

    return true;
}
inline static void Global_DealDamageToEnemy(i32 enemyIndex, f32 damage, bool bIsAOE)
{
    if (enemyIndex < 0 || enemyIndex >= globalVariables.lastEntityIndex) return;
    Entity* enemy = &globalVariables.entities[enemyIndex];
    if (enemy->type != ENTITY_TYPE_ENEMY) return;

    // Healing should be from what actual damage was dealt
    f32 actualDamageDealt = damage;
    if (actualDamageDealt > enemy->enemyCharacter.health) {
        actualDamageDealt = enemy->enemyCharacter.health;
    }

    enemy->enemyCharacter.health -= damage;
    enemy->enemyCharacter.flashTimer = 0.1f;
    Popup_SpawnDamagePopup(enemy->position, damage);
    Audio_PlaySoundVar(ASSET_SOUND_TYPE_DAMAGE, true);
    
    // Life Steal logic
    Entity* player = Global_GetPlayer();
    if (globalVariables.playerStats.lifeStealMultiplier > 0.0f && actualDamageDealt > 0) {
        f32 steal = actualDamageDealt * globalVariables.playerStats.lifeStealMultiplier;
        
        // AOE reduction
        if (bIsAOE) steal *= 0.4f;

        player->character.health += steal;
        if (player->character.health > player->character.maxHealth) {
            player->character.health = player->character.maxHealth;
        }
    }

    if (enemy->enemyCharacter.health <= 0) {
        XP_GenerateXPCrystal(enemy->position, enemy->enemyCharacter.xpDropAmount);
        Drop_GenerateDrop(enemy->position);
        Global_DestroyEntity(enemyIndex);
    }
}
// ~End of Global Implementation

// ~Begin of Helpers Implementation
inline static f32 Helper_GetRandomf32InRange(f32Range range)
{
    return range.min + (f32)GetRandomValue(0, 10000) / 10000.0f * (range.max - range.min);
}
inline static u16 Helper_GetRandomu16InRange(u16Range range)
{
    return (u16)GetRandomValue(range.min, range.max);
}
// ~End of Helpers Implementation