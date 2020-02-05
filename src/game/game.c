#include "game.h"
/* OT data (WTF)

class Cylinder{
	// virtual
	// empty
};

class Thing {
  Cylinder* parent;
  int32_t m_refCount;
};

class AutoID {
	// virtual
public:
  AutoID() {
    boost::recursive_mutex::scoped_lock lockClass(autoIDLock);
    count++;
    if(count >= 0xFFFFFF)
      count = 1000;

    while(list.find(count) != list.end()){
      if(count >= 0xFFFFFF)
        count = 1000;
      else
        count++;
    }
    list.insert(count);
    auto_id = count;
  }
  virtual ~AutoID(){
    list_type::iterator it = list.find(auto_id);
    if(it != list.end())
      list.erase(it);
  }

  typedef std::set<uint32_t> list_type;

  uint32_t auto_id;
  static boost::recursive_mutex autoIDLock;

protected:
  static uint32_t count;
  static list_type list;

};

class Creature : public AutoID, virtual public Thing
{
	// virtual
  static const int32_t mapWalkWidth = Map_maxViewportX * 2 + 1;
  static const int32_t mapWalkHeight = Map_maxViewportY * 2 + 1;
  bool localMapCache[mapWalkHeight][mapWalkWidth];

  Tile* _tile;
  uint32_t id;
  bool isInternalRemoved;
  bool isMapLoaded;
  bool isUpdatingPath;
  // The creature onThink event vector this creature belongs to
  // -1 represents that the creature isn't in any vector
  int32_t checkCreatureVectorIndex;
  bool creatureCheck;

  Script::ListenerList registered_listeners;
  StorageMap storageMap;

  int32_t health, healthMax;
  int32_t mana, manaMax;

  OutfitType currentOutfit;
  OutfitType defaultOutfit;

  Position masterPos;
  int32_t masterRadius;
  uint64_t lastStep;
  uint32_t lastStepCost;
  uint32_t baseSpeed;
  int32_t varSpeed;
  bool skillLoss;
  bool lootDrop;
  Direction direction;
  ConditionList conditions;
  LightInfo internalLight;

  //summon variables
  Creature* master;
  std::list<Creature*> summons;

  //follow variables
  Creature* followCreature;
  uint32_t eventWalk;
  std::list<Direction> listWalkDir;
  uint32_t walkUpdateTicks;
  bool hasFollowPath;
  bool forceUpdateFollowPath;

  //combat variables
  Creature* attackedCreature;

  struct CountBlock_t{
    int32_t total;
    int64_t ticks;
    uint32_t hits;
  };

  typedef std::map<uint32_t, CountBlock_t> CountMap;
  CountMap damageMap;
  CountMap healMap;
  CombatType lastDamageSource;
  uint32_t lastHitCreature;
  uint32_t blockCount;
  uint32_t blockTicks;
};

class Player : public Creature, public Cylinder
{
	// virtual
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
  static uint32_t playerCount;
#endif
  static MuteCountMap muteCountMap;
  static int32_t maxMessageBuffer;
  static ChannelStatementMap channelStatementMap;
  static uint32_t channelStatementGuid;
  static AutoList<Player> listPlayer;

  VIPListSet VIPList;
  uint32_t maxVipLimit;

  //items
  ContainerVector containerVec;

  //depots
  DepotMap depots;
  uint32_t maxDepotLimit;

  ProtocolGame* client;

  uint32_t level;
  uint32_t levelPercent;
  uint32_t magLevel;
  uint32_t magLevelPercent;
  int16_t accessLevel;
  int16_t violationLevel;
  std::string groupName;
  uint64_t experience;
  CombatType damageImmunities;
  MechanicType mechanicImmunities;
  MechanicType mechanicSuppressions;
  uint32_t condition;
  int32_t stamina;
  uint32_t manaSpent;
  Vocation* vocation;
  PlayerSex sex;
  int32_t soul, soulMax;
  uint64_t groupFlags;
  uint16_t premiumDays;
  uint32_t MessageBufferTicks;
  int32_t MessageBufferCount;
  uint32_t actionTaskEvent;
  uint32_t nextStepEvent;
  uint32_t walkTaskEvent;
  SchedulerTask* walkTask;

  int32_t idleTime;
  bool idleWarned;

  double inventoryWeight;
  double capacity;

  int64_t last_ping;
  int64_t last_pong;
  int64_t nextAction;

  bool pzLocked;
  bool isConnecting;
  int32_t bloodHitCount;
  int32_t shieldBlockCount;
  BlockType lastAttackBlockType;
  bool addAttackSkillPoint;
  uint64_t lastAttack;

  ChaseMode chaseMode;
  FightMode fightMode;
  bool safeMode;

  // This list is set to the same list as sent by sendOutfitWindow
  // and used when setting the outfit to make sure the client doesn't
  // fool us
  OutfitList validOutfitList;

  ShopItemList validShopList;

  //account variables
  uint32_t accountId;
  std::string accountName;
  std::string password;
  time_t lastLoginSaved;
  time_t lastLogout;
  int64_t lastLoginMs;
  Position loginPosition;
  uint32_t lastip;

  //inventory variables
  Item* inventory[11];
  bool inventoryAbilities[11];

  //player advances variables
  uint32_t skills[SkillType::size][3];

  //extra skill modifiers
  int32_t varSkills[SkillType::size];

  //extra stat modifiers
  int32_t varStats[PlayerStatType::size];

  //loss percent variables
  uint32_t lossPercent[LossType::size];

  //rate value variables
  double rateValue[LevelType::size];

  LearnedInstantSpellList learnedInstantSpellList;

  ConditionList storedConditionList;

  //trade variables
  Player* tradePlayer;
  TradeState tradeState;
  Item* tradeItem;

  //party variables
  Party* party;
  PartyList invitePartyList;

  std::string name;
  std::string nameDescription;
  uint32_t guid;
  uint32_t town;

  //guild variables
  uint32_t guildId;
  std::string guildName;
  std::string guildRank;
  std::string guildNick;
  uint32_t guildLevel;

  LightInfo itemsLight;

  //read/write storage data
  uint32_t windowTextId;
  Item* writeItem;
  uint16_t maxWriteLen;
  House* editHouse;
  uint32_t editListId;

#ifdef __SKULLSYSTEM__
  SkullType skullType;
  int64_t lastSkullTime;
  typedef std::set<uint32_t> AttackedSet;
  AttackedSet attackedSet;
#endif
};

class Actor : public Creature
{
	// virtual
public:
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
  static uint32_t monsterCount;
#endif

  static int32_t despawnRange;
  static int32_t despawnRadius;

  static AutoList<Actor> listMonster;

  CreatureList targetList;
  CreatureList friendList;

  CreatureType cType;

  uint32_t attackTicks;
  uint32_t targetTicks;
  uint32_t targetChangeTicks;
  uint32_t defenseTicks;
  uint32_t yellTicks;
  int32_t targetChangeCooldown;
  bool resetTicks;
  bool isIdle;
  bool extraMeleeAttack;
  bool isMasterInRange;

  Spawn* spawn;
  bool shouldReload_;

  bool alwaysThink_;
  bool onlyThink_;
  bool canTarget_;

  std::string strDescription;
};

class Game
{
  std::vector<Thing*> toReleaseThings;
  std::vector<Position> toIndexTiles;

  uint32_t checkLightEvent;
  uint32_t checkCreatureEvent;
  uint32_t checkDecayEvent;

  //list of items that are in trading state, mapped to the player
  std::map<Item*, uint32_t> tradeItems;

  AutoList<Creature> listCreature;
  size_t checkCreatureLastIndex;
  std::vector<Creature*> checkCreatureVectors[EVENT_CREATURECOUNT];
  std::vector<Creature*> toAddCheckCreatureVector;

  // Script handling
  StorageMap globalStorage;
  Script::Environment* script_environment;
  Script::Manager* script_system;
  uint32_t waitingScriptEvent;

  struct GameEvent
  {
    int64_t  tick;
    int      type;
    void*    data;
  };

  typedef std::list<Item*> DecayList;
  DecayList decayItems[EVENT_DECAY_BUCKETS];
  DecayList toDecayItems;
  size_t last_bucket;

  static const int LIGHT_LEVEL_DAY = 250;
  static const int LIGHT_LEVEL_NIGHT = 40;
  static const int SUNSET = 1305;
  static const int SUNRISE = 430;
  int lightlevel;
  LightState_t light_state;
  int light_hour;
  int light_hour_delta;

  uint32_t maxPlayers;
  uint32_t inFightTicks;
  uint32_t exhaustionTicks;
  uint32_t addExhaustionTicks;
  uint32_t fightExhaustionTicks;
  uint32_t healExhaustionTicks;
  uint32_t stairhopExhaustion;

  GameState gameState;
  WorldType worldType;

  ServiceManager* service_manager;
  Map* map;

  std::vector<std::string> commandTags;
};

*/

bool game_init(void){
	return true;
}

void game_shutdown(void){
}

#include <stdio.h>
void game_run(void){
	//while(1){
		// convert server net input to events (this will come from dispatching work into the game thread)
		// convert scheduler entries to events (if they have expired)
		// process chat events
		// process combat events
		// process move events
		// process item events (use/add/remove items)
		// ... player, npc, monster, all events ...
		// process all events (these may also generate script events)
		// process script events (script callbacks)
		// update whatever is left
		// send output commands to server thread
	//}
	getchar();
}

