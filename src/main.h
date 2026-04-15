#pragma once
#include <stdint.h>
#include "raylib.h"
#include "raymath.h"

// Global Constants
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define MAX_ENTITIES_AMOUNT 20000

#define MAX_SPAWN_DEFINITION 7

#define MAX_WEAPON_CAPACITY 4
#define MAX_WEAPON_LEVEL 15
#define MAX_RELIC_CAPACITY 4
#define MAX_RELIC_LEVEL 15

#define MAP_SIZE 10000
#define MAP_HALF_SIZE 5000

//~ Begin of Utility Structs
typedef struct FloatRange {
    float min;
    float max;
} FloatRange;

typedef struct Uint16Range {
    uint16_t min;
    uint16_t max;
} Uint16Range;
//~ End of Utility Structs

// ~Begin of Enums
typedef enum EntityType : uint8_t {
    ENTITY_TYPE_UNDEFINED = 0,
    ENTITY_TYPE_CHARACTER = 1,
    ENTITY_TYPE_PLAYER = 2,
    ENTITY_TYPE_PROJECTILE = 3,
    ENTITY_TYPE_ENEMY = 4,
    ENTITY_TYPE_DAMAGE_POPUP = 5,
    ENTITY_TYPE_XP_CRYSTAL = 6
} EntityType;

typedef enum ProjectileType : uint8_t {
    PROJECTILE_TYPE_UNDEFINED = 0,
    PROJECTILE_TYPE_CRYSTAL_SHARD = 1,
    PROJECTILE_TYPE_FIREBALL = 2,
    PROJECTILE_TYPE_BOMB = 3,
    PROJECTILE_TYPE_NATURE_SPIKE = 4,
    PROJECTILE_TYPE_DEATH_AURA = 5,
    PROJECTILE_TYPE_EXPLOSION = 6
} ProjectileType;

typedef enum WeaponType : uint8_t {
    WEAPON_TYPE_CRYSTAL_WAND = 0,
    WEAPON_TYPE_FIREBALL_RING = 1,
    WEAPON_TYPE_BOMB_SHOES = 2,
    WEAPON_TYPE_NATURE_SPIKES = 3,
    WEAPON_TYPE_DEATH_AURA = 4,
    WEAPON_TYPE_COUNT = 5
} WeaponType;

typedef enum RelicType : uint8_t {
    RELIC_TYPE_HEALTH = 0,
    RELIC_TYPE_DAMAGE = 1,
    RELIC_TYPE_ATTACK_SPEED = 2,
    RELIC_TYPE_MOVEMENT_SPEED = 3,
    RELIC_TYPE_SIZE = 4,
    RELIC_TYPE_LIFE_STEAL = 5,
    RELIC_TYPE_XP = 6,
    RELIC_TYPE_COUNT = 7
} RelicType;

typedef enum EnemyType : uint8_t {
    ENEMY_TYPE_NORMAL = 0,
    ENEMY_TYPE_FAST = 1,
    ENEMY_TYPE_TANK = 2,
    ENEMY_TYPE_BOSS = 3
} EnemyType;

typedef enum AssetSpriteType : uint8_t {
    ASSET_SPRITE_TYPE_PLAYER = 0,
    ASSET_SPRITE_TYPE_GRASS = 1,
    ASSET_SPRITE_TYPE_BAT = 2,
    ASSET_SPRITE_TYPE_COUNT = 3
} AssetSpriteType;

typedef enum AssetSoundType : uint8_t {
    ASSET_SOUND_TYPE_DAMAGE = 0,
    ASSET_SOUND_TYPE_EXPLOSION = 1,
    ASSET_SOUND_TYPE_LEVEL_UP = 2,
    ASSET_SOUND_TYPE_XP_GAIN = 3,
    ASSET_SOUND_TYPE_PLAYER_DAMAGE = 4,
    ASSET_SOUND_TYPE_COUNT = 5
} AssetSoundType;

typedef enum AssetMusicType : uint8_t {
    ASSET_MUSIC_TYPE_COMBAT = 0,
    ASSET_MUSIC_TYPE_COUNT = 1
} AssetMusicType;

typedef enum VisualType : uint8_t {
    VISUAL_TYPE_NONE = 0, // For entities that have no sprite
    VISUAL_TYPE_SPRITE = 1, // For entities with sprites
    VISUAL_TYPE_ANIMATED_SPRITE = 2, // For entities with animated sprites
    VISUAL_TYPE_ANIMATED_STATIC_SPRITE = 3 // For entities with animated static sprites (e.g. bounce)
} VisualType;

typedef enum SpawnType : uint8_t {
    SPAWN_TYPE_SINGLE = 0,
    SPAWN_TYPE_CLUSTER = 1,
    SPAWN_TYPE_LINE = 2,
    SPAWN_TYPE_AROUND = 3
} SpawnType;
// ~End of Enums

// ~Begin of Structs

typedef struct SpriteData {
    uint8_t spriteID;
    bool flipX;
} SpriteData;

typedef struct AnimatedSpriteData {
    uint8_t spriteID;
    uint8_t frameCount;
    uint8_t currentFrame;
    float frameTimer;
    float frameTime;
    bool flipX;
} AnimatedSpriteData;

typedef struct AnimatedStaticSpriteData {
    uint8_t spriteID;
    float animationDuration;
    float animationTimer;
    bool flipX;
} AnimatedStaticSpriteData;

typedef struct Character{
    float health;
    float maxHealth;
    float speed;
    float flashTimer;
    float invulnerableTimer;
    bool bIsDead;
    float deathFadeTimer;
} Character;

typedef struct EnemyCharacter{
    float health;
    float speed;

    EnemyType enemyType;
    float xpDropAmount;
    float flashTimer;
} EnemyCharacter;

typedef struct Projectile{
    ProjectileType projectileType;
    float damage;
    float lifeTime;
    uint8_t penetration;
    uint16_t ownerID;
    float timer;
    union {
        struct {
            uint16_t hitIds[16];
        } hitTracking;
        struct {
            float explosionRadius;
            float explosionDamageMultiplier;
        } explosive;
    };
} Projectile;

typedef struct DamagePopup {
    float amount;
    float timer;
} DamagePopup;

typedef struct XPCrystal {
    float amount;
    bool bIsMagnetized;
} XPCrystal;

typedef struct Entity{
    EntityType type;
    uint16_t id;
    uint8_t spriteID;
    bool bIsActive;
    Vector2 position;
    Vector2 velocity;
    Vector2 scale;
    float radius;
    union
    {
        Character character;
        EnemyCharacter enemyCharacter;
        Projectile projectile;
        DamagePopup damagePopup;
        XPCrystal xpCrystal;
    };

    // Sprite Data
    VisualType visualType;
    union
    {
        SpriteData sprite;
        AnimatedSpriteData animatedSprite;
        AnimatedStaticSpriteData animatedStaticSprite;
    };
} Entity;

typedef struct Assets{
    Texture2D sprites[ASSET_SPRITE_TYPE_COUNT];
    Sound sounds[ASSET_SOUND_TYPE_COUNT];
    Music musics[ASSET_MUSIC_TYPE_COUNT];
    Shader flashShader;
    int flashIntensityLoc;
} Assets;

typedef struct SpawnDefinition{
    EnemyType enemyType;
    SpawnType spawnType;
    Uint16Range amountToSpawnRange;
    FloatRange distanceToSpawnRange;
    uint16_t chanceToSpawn;
    float Difficulty;
} SpawnDefinition;

typedef struct SpawnerData{
    float delayBetweenSpawns;
    float spawnTimer;
    SpawnDefinition spawnsDefinitions[MAX_SPAWN_DEFINITION];
} SpawnerData;

// Removed duplicate WeaponData

typedef struct WeaponCrystalDefinition {
    uint8_t penetration;
} WeaponCrystalDefinition;

typedef struct WeaponFireballDefinition {
    float explosionRadius;
    float explosionDamageMultipler;
} WeaponFireballDefinition;

typedef struct WeaponBombShoesDefinition {
    float explosionRadius;
    float delayToExplode;
} WeaponBombShoesDefinition;

typedef struct WeaponNatureSpikesDefinition {
    float rangeToSpawn;
    float spikeDuration;
    float spikeMaxDamage;
} WeaponNatureSpikesDefinition;

typedef struct WeaponDeathAuraDefinition {
    float size;
} WeaponDeathAuraDefinition;

typedef struct WeaponDefinition {
    float damage;
    float delayBetweenAttacks;
    uint8_t projectileAmount;
    union
    {
        WeaponCrystalDefinition crystal;
        WeaponFireballDefinition fireball;
        WeaponBombShoesDefinition bombShoes;
        WeaponNatureSpikesDefinition natureSpikes;
        WeaponDeathAuraDefinition deathAura;
    };
} WeaponDefinition;

typedef struct WeaponData {
    WeaponType weaponType;
    uint8_t level;
    float attackTimer;
    uint8_t burstRemaining;
    float burstTimer;
} WeaponData;

typedef struct WeaponLevelsDefinition {
    WeaponDefinition crystalShard[MAX_WEAPON_LEVEL];
    WeaponDefinition fireballRing[MAX_WEAPON_LEVEL];
    WeaponDefinition bombShoes[MAX_WEAPON_LEVEL];
    WeaponDefinition natureSpikes[MAX_WEAPON_LEVEL];
    WeaponDefinition deathAura[MAX_WEAPON_LEVEL];
} WeaponLevelsDefinition;

typedef struct RelicDefinition {
    float multiplier;
} RelicDefinition;

typedef struct RelicData {
    uint8_t level;
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

typedef enum UpgradeType {
    UPGRADE_TYPE_WEAPON,
    UPGRADE_TYPE_RELIC
} UpgradeType;

typedef struct UpgradeOption {
    UpgradeType type;
    union {
        WeaponType weapon;
        RelicType relic;
    };
    uint8_t level;
} UpgradeOption;

typedef struct LevelUpState {
    bool bShowLevelUp;
    UpgradeOption options[3];
    int pendingCount;
    int selectedIndex;
} LevelUpState;

typedef struct PlayerStats{
    float currentXP;
    float nextLevelXP;
    uint16_t level;

    float healthMultiplier;
    float damageMultiplier;
    float attackSpeedMultiplier;
    float movementSpeedMultiplier;
    float sizeMultiplier;
    float lifeStealMultiplier;
    float xpMultiplier;
} PlayerStats;

typedef struct GlobalVariables{
    Assets assets;
    
    Camera2D camera;
    PlayerStats playerStats;

    Entity entities[MAX_ENTITIES_AMOUNT];
    uint16_t lastEntityIndex;

    uint16_t playerIndex;

    Inventory inventory;
    InventoryDefinitions InventoryDefinitions;

    SpawnerData spawnerData;
    float gameTimer;
    uint16_t deathAuraIndex;
    uint16_t nextEntityId;
    bool bShowInventory;
    LevelUpState levelUpState;
} GlobalVariables;

// ~End of Structs

// Declaration of Global Variables
extern GlobalVariables globalVariables;

// The game main loop
int main(void);

//~ Begin of Core Implementation
void Core_InitGame();
void Core_ProcessInput();
void Core_UpdateGame(float deltaTime);
void Core_RenderGraphics();
void Core_CloseGame();

int Core_IsGameReadyToClose();
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
void Audio_Update(float deltaTime);
//~ End of Audio Implementation

//~ Begin of Collision Implementation
void Collision_MapBorder(Entity* entity);
//~ End of Collision Implementation

//~ Begin of Enemy Implementation
Entity Enemy_GenerateEnemy(EnemyType enemyType);
void Enemy_ProcessAllMovement(float deltaTime);
//~ End of Enemy Implementation

//~ Begin of HUD Implementation
void HUD_Init();
void HUD_UpdateData();
void HUD_Draw();
void HUD_DrawInventory();
void HUD_DrawLevelUp();
void HUD_GenerateLevelUpOptions();
//~ End of HUD Implementation

//~ Begin of Player Implementation
Camera2D Player_GenerateCamera();
Entity Player_GeneratePlayer();
PlayerStats Player_GeneratePlayerStats();
void Player_ProcessMovement(Entity* player, float deltaTime);
void Player_AnimateMovement(Entity* player, float deltaTime);
//~ End of Player Implementation

//~ Begin of Popup Implementation
Entity Popup_SpawnDamagePopup(Vector2 pos, float amount);
void Popup_UpdateAll(float deltaTime);
//~ End of Popup Implementation

//~ Begin of Projectile Implementation
Entity Projectile_Spawn(ProjectileType type, Vector2 pos, Vector2 vel, float damage, float lifeTime, uint8_t penetration);
void Projectile_ProcessAllMovement(float deltaTime);
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
void Spawner_ProcessSpawnLogic(float deltaTime);
//~ End of Spawner Implementation

//~ Begin of Weapon Implementation
void Weapon_GenerateWeaponLevels();
bool Weapon_AddWeapon(WeaponType weaponType); //This function also levels up weapons
void Weapon_ProcessAttack(float deltaTime);
const char* Weapon_GetWeaponName(WeaponType weaponType);
//~ End of Weapon Implementation

//~ Begin of XP Implementation
void XP_GenerateXPCrystal(Vector2 position, float amount);
void XP_MoveCrystals(float deltaTime);
void XP_GrantXP(float amount);
void XP_LevelUp();
//~ End of XP Implementation

//~ Begin of Global Implementation
void Global_UpdateGameTimer(float deltaTime);
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
inline static bool Global_DestroyEntity(uint16_t entityIndex)
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
    
    globalVariables.entities[globalVariables.lastEntityIndex - 1].bIsActive = false;
    globalVariables.lastEntityIndex--;

    return true;
}
inline static void Global_DealDamageToEnemy(int enemyIndex, float damage, bool bIsAOE)
{
    if (enemyIndex < 0 || enemyIndex >= globalVariables.lastEntityIndex) return;
    Entity* enemy = &globalVariables.entities[enemyIndex];
    if (!enemy->bIsActive || enemy->type != ENTITY_TYPE_ENEMY) return;

    // Healing should be from what actual damage was dealt
    float actualDamageDealt = damage;
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
        float steal = actualDamageDealt * globalVariables.playerStats.lifeStealMultiplier;
        
        // AOE reduction
        if (bIsAOE) steal *= 0.4f;

        player->character.health += steal;
        if (player->character.health > player->character.maxHealth) {
            player->character.health = player->character.maxHealth;
        }
    }

    if (enemy->enemyCharacter.health <= 0) {
        XP_GenerateXPCrystal(enemy->position, enemy->enemyCharacter.xpDropAmount);
        Global_DestroyEntity(enemyIndex);
    }
}
// ~End of Global Implementation

// ~Begin of Helpers Implementation
inline static float Helper_GetRandomFloatInRange(FloatRange range)
{
    return range.min + (float)GetRandomValue(0, 10000) / 10000.0f * (range.max - range.min);
}
inline static uint16_t Helper_GetRandomUint16InRange(Uint16Range range)
{
    return (uint16_t)GetRandomValue(range.min, range.max);
}
// ~End of Helpers Implementation