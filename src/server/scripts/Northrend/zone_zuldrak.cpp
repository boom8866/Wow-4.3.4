/*
 * Copyright (C) 2015 DevKM <http://facebook.com/krizttianmoreno>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "Player.h"
#include "SpellInfo.h"
#include "Vehicle.h"

/*####
## npc_drakuru_shackles
####*/

enum eDrakuruShackles
{
    SPELL_LEFT_CHAIN           = 59951,
    SPELL_RIGHT_CHAIN          = 59952,
    SPELL_UNLOCK_SHACKLE       = 55083,
    SPELL_FREE_RAGECLAW        = 55223,

    NPC_RAGECLAW               = 29686,
    QUEST_TROLLS_IS_GONE_CRAZY = 12861,
};

class npc_drakuru_shackles : public CreatureScript
{
public:
    npc_drakuru_shackles() : CreatureScript("npc_drakuru_shackles") { }

    struct npc_drakuru_shacklesAI : public ScriptedAI
    {
        npc_drakuru_shacklesAI(Creature* creature) : ScriptedAI(creature) {}

        uint64 RageclawGUID;

        void Reset()
        {
            RageclawGUID = 0;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

            float x, y, z;
            me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 0.1f);

            if (Unit* summon = me->SummonCreature(NPC_RAGECLAW, x, y, z, 0, TEMPSUMMON_DEAD_DESPAWN, 1000))
            {
                RageclawGUID = summon->GetGUID();
                LockRageclaw();
            }
        }

        void LockRageclaw()
        {
            Unit* Rageclaw = Unit::GetCreature(*me, RageclawGUID);
            // pointer check not needed
            me->SetInFront(Rageclaw);
            Rageclaw->SetInFront(me);

            DoCast(Rageclaw, SPELL_LEFT_CHAIN, true);
            DoCast(Rageclaw, SPELL_RIGHT_CHAIN, true);
        }

        void UnlockRageclaw(Unit* who)
        {
            if (!who)
                return;

            Creature* Rageclaw = Unit::GetCreature(*me, RageclawGUID);
            // pointer check not needed
            DoCast(Rageclaw, SPELL_FREE_RAGECLAW, true);

            me->setDeathState(DEAD);
        }

        void SpellHit(Unit* pCaster, const SpellInfo* pSpell)
        {
            if (pSpell->Id == SPELL_UNLOCK_SHACKLE)
            {
                if (pCaster->ToPlayer()->GetQuestStatus(QUEST_TROLLS_IS_GONE_CRAZY) == QUEST_STATUS_INCOMPLETE)
                {
                    if (Creature* pRageclaw = Unit::GetCreature(*me, RageclawGUID))
                    {
                        UnlockRageclaw(pCaster);
                        pCaster->ToPlayer()->KilledMonster(pRageclaw->GetCreatureTemplate(), RageclawGUID);
                        me->DisappearAndDie();
                    }
                    else
                        me->setDeathState(JUST_DIED);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_drakuru_shacklesAI(creature);
    }
};

/*####
## npc_captured_rageclaw
####*/

enum eRageclaw
{
    SPELL_UNSHACKLED           = 55085,
    SPELL_KNEEL                = 39656
};

const char* SAY_RAGECLAW_1 =      "I poop on you, trollses!";
const char* SAY_RAGECLAW_2 =      "ARRRROOOOGGGGAAAA!";
const char* SAY_RAGECLAW_3 =      "No more mister nice wolvar!";

#define SAY_RAGECLAW RAND(SAY_RAGECLAW_1, SAY_RAGECLAW_2, SAY_RAGECLAW_3)

class npc_captured_rageclaw : public CreatureScript
{
public:
    npc_captured_rageclaw() : CreatureScript("npc_captured_rageclaw") { }

    struct npc_captured_rageclawAI : public ScriptedAI
    {
        npc_captured_rageclawAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 DespawnTimer;
        bool Despawn;

        void Reset()
        {
            Despawn = false;
            DespawnTimer = 0;
            me->setFaction(35);
            DoCast(me, SPELL_KNEEL, true); // Little Hack for kneel - Thanks Illy :P
        }

        void MoveInLineOfSight(Unit* /*who*/){}

        void SpellHit(Unit* /*pCaster*/, const SpellInfo* pSpell)
        {
            if (pSpell->Id == SPELL_FREE_RAGECLAW)
            {
                me->RemoveAurasDueToSpell(SPELL_LEFT_CHAIN);

                me->RemoveAurasDueToSpell(SPELL_RIGHT_CHAIN);

                me->RemoveAurasDueToSpell(SPELL_KNEEL);

                me->setFaction(me->GetCreatureTemplate()->faction_H);

                DoCast(me, SPELL_UNSHACKLED, true);
                me->MonsterSay(SAY_RAGECLAW, LANG_UNIVERSAL, 0);
                me->GetMotionMaster()->MoveRandom(10);

                DespawnTimer = 10000;
                Despawn = true;
            }
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (UpdateVictim())
            {
                DoMeleeAttackIfReady();
                return;
            }

            if (!Despawn)
                return;

            if (DespawnTimer <= uiDiff)
                me->DisappearAndDie();
            else DespawnTimer -= uiDiff;
       }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_captured_rageclawAI(creature);
    }
};

/*####
## npc_gymer
####*/

#define    GOSSIP_ITEM_G "I'm ready, Gymer. Let's go!"

enum eGymer
{
    QUEST_STORM_KING_VENGEANCE    = 12919,
    SPELL_GYMER                   = 55568
};

class npc_gymer : public CreatureScript
{
public:
    npc_gymer() : CreatureScript("npc_gymer") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        if (player->GetQuestStatus(QUEST_STORM_KING_VENGEANCE) == QUEST_STATUS_INCOMPLETE)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_G, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            player->SEND_GOSSIP_MENU(13640, creature->GetGUID());
        }

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF+1)
        {
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, SPELL_GYMER, true);
        }

        return true;
    }
};

/*####
## npc_gurgthock
####*/

enum eGurgthock
{
    QUEST_AMPHITHEATER_ANGUISH_TUSKARRMAGEDDON    = 12935,
    QUEST_AMPHITHEATER_ANGUISH_KORRAK_BLOODRAGER  = 12936,
    QUEST_AMPHITHEATER_ANGUISH_YGGDRAS_2          = 12954,
    QUEST_AMPHITHEATER_ANGUISH_YGGDRAS_1          = 12932,
    QUEST_AMPHITHEATER_ANGUISH_MAGNATAUR          = 12933,
    QUEST_AMPHITHEATER_ANGUISH_FROM_BEYOND        = 12934,
    QUEST_CHAMPION_OF_ANGUISH                     = 12948,

    NPC_ORINOKO_TUSKBREAKER                       = 30020,
    NPC_KORRAK_BLOODRAGER                         = 30023,
    NPC_YGGDRAS                                   = 30014,
    NPC_STINKBEARD                                = 30017,
    NPC_AZ_BARIN                                  = 30026, // air
    NPC_DUKE_SINGEN                               = 30019, // fire
    NPC_ERATHIUS                                  = 30025, // earth
    NPC_GARGORAL                                  = 30024, // water
    NPC_FIEND_WATER                               = 30044,
    NPC_FIEND_AIR                                 = 30045,
    NPC_FIEND_FIRE                                = 30042,
    NPC_FIEND_EARTH                               = 30043,
    NPC_VLADOF_THE_BUTCHER                        = 30022,

    SAY_QUEST_ACCEPT_TUSKARRMAGEDON               = 0,
    SAY_QUEST_ACCEPT_KORRAK_1                     = 1,
    SAY_QUEST_ACCEPT_KORRAK_2                     = 2,
    SAY_QUEST_ACCEPT_MAGNATAUR                    = 3,
    EMOTE_YGGDRAS_SPAWN                           = 4,
    SAY_STINKBEARD_SPAWN                          = 5,
    SAY_GURGTHOCK_ELEMENTAL_SPAWN                 = 6,

    SAY_CALL_FOR_HELP                             = 0,
    SAY_RECRUIT                                   = 0,

    SPELL_CRASHING_WAVE                           = 55909, // water
    SPELL_SHOCKWAVE                               = 55918, // earth
    SPELL_BLAST_OF_AIR                            = 55912, // air
    SPELL_MAGMA_WAVE                              = 55916, // fire

    SPELL_ORB_OF_WATER                             = 55888, // fiend of water spell
    SPELL_ORB_OF_STORMS                            = 55882, // fiend of air spell
    SPELL_BOULDER                                  = 55886, // fiend of earth spell
    SPELL_ORB_OF_FLAME                             = 55872, // fiend of fire spell
};

struct BossAndAdd
{
    uint32 uiBoss;
    uint32 uiAdd;
    uint32 uiSpell;
    uint32 uiAddSpell;
};

static BossAndAdd Boss[]=
{
    {NPC_GARGORAL, NPC_FIEND_WATER, SPELL_CRASHING_WAVE, SPELL_ORB_OF_WATER},
    {NPC_AZ_BARIN, NPC_FIEND_AIR, SPELL_BLAST_OF_AIR, SPELL_ORB_OF_STORMS},
    {NPC_DUKE_SINGEN, NPC_FIEND_FIRE, SPELL_MAGMA_WAVE, SPELL_ORB_OF_FLAME},
    {NPC_ERATHIUS, NPC_FIEND_EARTH, SPELL_SHOCKWAVE, SPELL_BOULDER},
};

const Position SpawnPosition[] =
{
    {5754.692f, -2939.46f, 286.276123f, 5.156380f}, // stinkbeard || orinoko || korrak
    {5762.054199f, -2954.385010f, 273.826955f, 5.108289f},  //yggdras
    {5776.855f, -2989.77979f, 272.96814f, 5.194f} // elementals
};

const Position AddSpawnPosition[] =
{
    {5722.487f, -3010.75f, 312.751648f, 0.478f}, // caster location
    {5724.983f, -2969.89551f, 286.359619f, 0.478f},
    {5733.76025f, -3000.34644f, 286.359619f, 0.478f},
    {5739.8125f, -2981.524f, 290.7671f, 0.478f}, // caster location
    {5742.101f, -2950.75586f, 286.2643f, 5.21f},
    {5743.305f, -3011.29736f, 290.7671f, 0.478f}, // caster location
    {5744.417f, -3025.528f, 286.35965f, 0.478f},
    {5763.189f, -3029.67529f, 290.7671f, 0.478f},
    {5769.401f, -2935.121f, 286.335754f, 5.21f},
    {5793.061f, -2934.593f, 286.359619f, 3.53f},
    {5797.32129f, -2955.26855f, 290.7671f, 3.53f}, // caster location
    {5813.94531f, -2956.74683f, 286.359619f, 3.53f},
    {5816.85547f, -2974.476f, 290.7671f, 3.53f}, // caster location
    {5820.30859f, -3002.83716f, 290.7671f, 3.53f}, // caster location
    {5828.50244f, -2981.737f, 286.359619f, 3.53f},
    {5828.899f, -2960.15479f, 312.751648f, 3.53f}, // caster location
};

class npc_gurgthock : public CreatureScript
{
public:
    npc_gurgthock() : CreatureScript("npc_gurgthock") { }

    struct npc_gurgthockAI : public ScriptedAI
    {
        npc_gurgthockAI(Creature* creature) : ScriptedAI(creature) {}

        uint64 SummonGUID;
        uint64 uiPlayerGUID;

        uint32 uiTimer;
        uint32 uiPhase;
        uint32 uiRemoveFlagTimer;
        uint32 uiQuest;
        uint8 uiBossRandom;

        bool bRemoveFlag;

        void Reset()
        {
            SummonGUID = 0;
            uiPlayerGUID = 0;

            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            uiTimer = 0;
            uiPhase = 0;
            uiQuest = 0;
            uiRemoveFlagTimer = 5000;

            uiBossRandom = 0;

            bRemoveFlag = false;
        }

        void SetGUID(uint64 guid, int32 /*id*/)
        {
            uiPlayerGUID = guid;
        }

        void SetData(uint32 uiId, uint32 uiValue)
        {
            bRemoveFlag = true;
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);

            switch (uiId)
            {
                case 1:
                    switch (uiValue)
                    {
                        case QUEST_AMPHITHEATER_ANGUISH_TUSKARRMAGEDDON:
                            Talk(SAY_QUEST_ACCEPT_TUSKARRMAGEDON);
                            uiPhase = 1;
                            uiTimer = 4000;
                            break;
                        case QUEST_AMPHITHEATER_ANGUISH_KORRAK_BLOODRAGER:
                            Talk(SAY_QUEST_ACCEPT_KORRAK_1);
                            uiPhase = 3;
                            uiTimer = 3000;
                            break;
                        case QUEST_AMPHITHEATER_ANGUISH_YGGDRAS_2:
                        case QUEST_AMPHITHEATER_ANGUISH_YGGDRAS_1:
                            uiPhase = 6;
                            uiTimer = 3000;
                            break;
                        case QUEST_AMPHITHEATER_ANGUISH_MAGNATAUR:
                            uiTimer = 5000;
                            uiPhase = 7;
                            break;
                        case QUEST_AMPHITHEATER_ANGUISH_FROM_BEYOND:
                            uiTimer = 2000;
                            uiPhase = 12;
                            break;
                        case QUEST_CHAMPION_OF_ANGUISH:
                            uiTimer = 5000;
                            uiPhase = 15;
                            break;
                   }
                        break;
                }
        }

        void UpdateAI(const uint32 uiDiff)
        {
            ScriptedAI::UpdateAI(uiDiff);

            if (bRemoveFlag)
            {
                if (uiRemoveFlagTimer <= uiDiff)
                {
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    bRemoveFlag = false;

                    uiRemoveFlagTimer = 10000;
                } else uiRemoveFlagTimer -= uiDiff;
            }

            if (uiPhase)
            {
                Player* player = me->GetPlayer(*me, uiPlayerGUID);

                if (uiTimer <= uiDiff)
                {
                    switch (uiPhase)
                    {
                        case 1:
                            if (Creature* summon = me->SummonCreature(NPC_ORINOKO_TUSKBREAKER, SpawnPosition[0], TEMPSUMMON_CORPSE_DESPAWN, 1000))
                                SummonGUID = summon->GetGUID();
                            uiPhase = 2;
                            uiTimer = 4000;
                            break;
                         case 2:
                            if (Creature* summon = Unit::GetCreature(*me, SummonGUID))
                                summon->GetMotionMaster()->MoveJump(5776.319824f, -2981.005371f, 273.100037f, 10.0f, 20.0f);
                            uiPhase = 0;
                            SummonGUID = 0;
                            break;
                        case 3:
                            Talk(SAY_QUEST_ACCEPT_KORRAK_2);
                            uiTimer = 3000;
                            uiPhase = 4;
                            break;
                        case 4:
                            if (Creature* summon = me->SummonCreature(NPC_KORRAK_BLOODRAGER, SpawnPosition[0], TEMPSUMMON_CORPSE_DESPAWN, 1000))
                                SummonGUID = summon->GetGUID();
                            uiTimer = 3000;
                            uiPhase = 0;
                            break;
                        case 6:
                            {
                                if (!player)
                                    return;

                                std::string sText = ("The grand Amphitheater of Anguish awaits, " + std::string(player->GetName()) + ". Remember, once a battle starts you have to stay in the area. WIN OR DIE!");

                                me->MonsterSay(sText.c_str(), LANG_UNIVERSAL, 0);
                                uiTimer = 5000;
                                uiPhase = 9;
                            }
                            break;
                        case 7:
                            {
                               if (!player)
                                   return;

                                std::string sText = ("Prepare to make you stand, " + std::string(player->GetName()) + "! Get in the Amphitheater and stand ready! Remember, you and your opponent must stay in the arena at all times or you will be disqualified!");
                                me->MonsterSay(sText.c_str(), LANG_UNIVERSAL, 0);
                                uiTimer = 3000;
                                uiPhase = 8;
                            }
                            break;
                        case 8:
                            Talk(SAY_QUEST_ACCEPT_MAGNATAUR);
                            uiTimer = 5000;
                            uiPhase = 11;
                            break;
                        case 9:
                            {
                                if (!player)
                                    return;

                                std::string sText = ("Here we are once again, ladies and gentlemen. The epic struggle between life and death in the Amphitheater of Anguish! For this round we have " + std::string(player->GetName()) + " versus the hulking jormungar, Yg... Yggd? Yggdoze? Who comes up with these names?! " + std::string(player->GetName()) + " versus big worm!");
                                me->MonsterYell(sText.c_str(), LANG_UNIVERSAL, 0);
                                uiTimer = 10000;
                                uiPhase = 10;
                            }
                            break;
                        case 10:
                            me->SummonCreature(NPC_YGGDRAS, SpawnPosition[1], TEMPSUMMON_CORPSE_DESPAWN, 1000);
                            Talk(EMOTE_YGGDRAS_SPAWN);
                            uiPhase = 0;
                            break;
                        case 11:
                            if (Creature* creature = me->SummonCreature(NPC_STINKBEARD, SpawnPosition[0], TEMPSUMMON_CORPSE_DESPAWN, 1000))
                                creature->AI()->Talk(SAY_STINKBEARD_SPAWN);
                            uiPhase = 0;
                            break;
                        case 12:
                        {
                            if (!player)
                                return;

                            std::string sText = ("Prepare to make you stand, " + std::string(player->GetName()) + "! Get in the Amphitheater and stand ready! Remember, you and your opponent must stay in the arena at all times or you will be disqualified!");
                            me->MonsterSay(sText.c_str(), LANG_UNIVERSAL, 0);
                            uiTimer = 5000;
                            uiPhase = 13;
                        }
                        break;
                        case 13:
                            Talk(SAY_GURGTHOCK_ELEMENTAL_SPAWN);
                            uiTimer = 3000;
                            uiPhase = 14;
                            break;
                        case 14:
                            uiBossRandom = urand(0, 3);
                            if (Creature* creature = me->SummonCreature(Boss[uiBossRandom].uiBoss, SpawnPosition[2], TEMPSUMMON_CORPSE_DESPAWN, 1000))
                                creature->AI()->SetData(1, uiBossRandom);
                            uiPhase = 0;
                            break;
                        case 15:
                            if (Creature* pSummon = me->SummonCreature(NPC_VLADOF_THE_BUTCHER, SpawnPosition[0], TEMPSUMMON_CORPSE_DESPAWN, 1000))
                            SummonGUID = pSummon->GetGUID();
                            uiTimer = 5000;
                            uiPhase = 16;
                            break;
                        case 16:
                            if (Creature* summon = Unit::GetCreature(*me, SummonGUID))
                            summon->GetMotionMaster()->MoveJump(5776.319824f, -2981.005371f, 273.100037f, 10.0f, 20.0f);
                            uiPhase = 0;
                            SummonGUID = 0;
                            break;
                    }
                }else uiTimer -= uiDiff;
            }
        }
    };

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        switch (quest->GetQuestId())
        {
            case QUEST_AMPHITHEATER_ANGUISH_TUSKARRMAGEDDON:
                creature->AI()->SetData(1, quest->GetQuestId());
                break;
            case QUEST_AMPHITHEATER_ANGUISH_KORRAK_BLOODRAGER:
                creature->AI()->SetData(1, quest->GetQuestId());
                break;
            case QUEST_AMPHITHEATER_ANGUISH_YGGDRAS_2:
            case QUEST_AMPHITHEATER_ANGUISH_YGGDRAS_1:
                creature->AI()->SetData(1, quest->GetQuestId());
                break;
            case QUEST_AMPHITHEATER_ANGUISH_MAGNATAUR:
                creature->AI()->SetData(1, quest->GetQuestId());
                break;
            case QUEST_AMPHITHEATER_ANGUISH_FROM_BEYOND:
                creature->AI()->SetData(1, quest->GetQuestId());
                break;
            case QUEST_CHAMPION_OF_ANGUISH:
                creature->AI()->SetData(1, quest->GetQuestId());
                break;
        }

        creature->AI()->SetGUID(player->GetGUID());

        return false;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_gurgthockAI(creature);
    }
};

/*####
## npc_orinoko_tuskbreaker
####*/

enum eOrinokoTuskbreaker
{
    SPELL_BATTLE_SHOUT      = 32064,
    SPELL_FISHY_SCENT       = 55937,
    SPELL_IMPALE            = 55929,
    SPELL_SUMMON_WHISKER    = 55946,

    NPC_WHISKER             = 30113,
    NPC_HUNGRY_PENGUIN      = 30110
};

class npc_orinoko_tuskbreaker : public CreatureScript
{
public:
    npc_orinoko_tuskbreaker() : CreatureScript("npc_orinoko_tuskbreaker") { }

    struct npc_orinoko_tuskbreakerAI : public ScriptedAI
    {
        npc_orinoko_tuskbreakerAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            me->SetReactState(REACT_PASSIVE);
        }

        bool bSummoned;
        bool bBattleShout;
        bool bFishyScent;

        uint32 uiBattleShoutTimer;
        uint32 uiFishyScentTimer;

        uint64 AffectedGUID;
        uint64 uiWhisker;

        void Reset()
        {
            bSummoned           = false;
            bBattleShout        = false;
            bFishyScent         = false;
            uiBattleShoutTimer  = 0;
            uiFishyScentTimer   = 20000;
            uiWhisker           = 0;
            AffectedGUID        = 0;
        }

        void EnterEvadeMode()
        {
            if (Creature* pWhisker = me->GetCreature(*me, uiWhisker))
                pWhisker->RemoveFromWorld();
        }

        void MovementInform(uint32 type, uint32 /*pointId*/)
        {
            if (type != EFFECT_MOTION_TYPE)
                return;

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            uiBattleShoutTimer  = 7000;
        }

        void EnterCombat(Unit* who)
        {
            DoCast(who, SPELL_IMPALE);
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!UpdateVictim())
                return;

            if (!bBattleShout && uiBattleShoutTimer <= uiDiff)
            {
                DoCast(me, SPELL_BATTLE_SHOUT);
                bBattleShout = true;
            } else uiBattleShoutTimer -= uiDiff;

            if (uiFishyScentTimer <= uiDiff)
            {
                if (Unit* pAffected = SelectTarget(SELECT_TARGET_RANDOM, 0))
                {
                    DoCast(pAffected, SPELL_FISHY_SCENT);
                    AffectedGUID = pAffected->GetGUID();
                }
                uiFishyScentTimer = 20000;
            } else uiFishyScentTimer -= uiDiff;

            if (!bSummoned && !HealthAbovePct(50))
            {
                Talk(SAY_CALL_FOR_HELP);
                //DoCastVictim(SPELL_SUMMON_WHISKER); petai is not working correctly???

                if (Creature* pWhisker = me->SummonCreature(NPC_WHISKER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 0))
                    uiWhisker = pWhisker->GetGUID();
                bSummoned = true;
            }

            DoMeleeAttackIfReady();
        }

        void JustSummoned(Creature* summon)
        {
            switch (summon->GetEntry())
            {
                case NPC_WHISKER:
                    summon->AI()->AttackStart(me->GetVictim());
                    break;
                case NPC_HUNGRY_PENGUIN:
                    if (Unit* pAffected = Unit::GetUnit(*me, AffectedGUID))
                    {
                        if (pAffected->isAlive())
                            summon->AI()->AttackStart(pAffected);
                    }
                    break;
            }
        }

        void JustDied(Unit* killer)
        {
            if (uiWhisker)
                if (Creature* pWhisker = me->GetCreature(*me, uiWhisker))
                    pWhisker->RemoveFromWorld();

            if (killer->GetTypeId() == TYPEID_PLAYER)
                killer->GetCharmerOrOwnerPlayerOrPlayerItself()->GroupEventHappens(QUEST_AMPHITHEATER_ANGUISH_TUSKARRMAGEDDON, killer);

        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_orinoko_tuskbreakerAI(creature);
    }
};

/*####
## npc_korrak_bloodrager
####*/

enum eKorrakBloodrager
{
    SPELL_GROW     = 55948,
    SPELL_CHARGE   = 24193,
    SPELL_UPPERCUT = 30471,
    SPELL_ENRAGE   = 42745
};

class npc_korrak_bloodrager : public CreatureScript
{
public:
    npc_korrak_bloodrager() : CreatureScript("npc_korrak_bloodrager") { }

    struct npc_korrak_bloodragerAI : public npc_escortAI
    {
        npc_korrak_bloodragerAI(Creature* creature) : npc_escortAI(creature)
        {
            Start(true, true, 0, NULL);
            SetDespawnAtEnd(false);
        }

        uint32 uiChargeTimer;
        uint32 uiUppercutTimer;

        bool bEnrage;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            me->SetReactState(REACT_PASSIVE);
            uiChargeTimer = 15000;
            uiUppercutTimer = 12000;
            bEnrage = false;
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 6:
                    me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    me->SetReactState(REACT_AGGRESSIVE);
                    break;
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCast(me, SPELL_GROW);
        }

        void UpdateAI(const uint32 uiDiff)
        {
            npc_escortAI::UpdateAI(uiDiff);

            if (!UpdateVictim())
                return;

            if (uiUppercutTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_NEAREST, 0))
                    DoCast(target, SPELL_UPPERCUT);
                uiUppercutTimer = 12000;
            } else uiUppercutTimer -= uiDiff;

            if (uiChargeTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 0))
                    DoCast(target, SPELL_CHARGE);
                uiChargeTimer = 15000;
            } else uiChargeTimer -= uiDiff;

            if (!bEnrage && !HealthAbovePct(20))
            {
                DoCast(me, SPELL_ENRAGE);
                bEnrage = true;
            }
            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* killer)
        {
            if (Player* player = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
                player->GroupEventHappens(QUEST_AMPHITHEATER_ANGUISH_KORRAK_BLOODRAGER, killer);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_korrak_bloodragerAI(creature);
    }
};

/*####
## npc_vladof_the_butcher
####*/

enum eVladofTheButcher
{
   BLOOD_BOIL        = 55974,
   BLOOD_PLAGUE      = 55973,
   BLOOD_PRESENCE    = 50689,
   HYSTERIA          = 55975,
   SPELL_DEFLECTION  = 55976,
   WHIRLIND          = 55977,
};

class npc_vladof_the_butcher : public CreatureScript
{
    public:
    npc_vladof_the_butcher() : CreatureScript("npc_vladof_the_butcher") { }
    
    struct npc_vladof_the_butcherAI : public ScriptedAI
    {
        npc_vladof_the_butcherAI(Creature* pCreature) : ScriptedAI(pCreature){}
        
        bool bBloodPresence;
        uint32 uiBloodBoilTimer;
        uint32 uiBloodPlagueTimer;
        uint32 uiBloodPresenceTimer;
        uint32 uiHysteriaTimer;
        uint32 uiSpellDeflectionTimer;
        uint32 uiWhirlindTimer;
        
        void Reset()
        {
            bBloodPresence = true;
            uiBloodBoilTimer = 40000;
            uiBloodPlagueTimer = 60000;
            uiBloodPresenceTimer = 2000;
            uiHysteriaTimer = 25000;
            uiSpellDeflectionTimer = 75000;
            uiWhirlindTimer = 12000;
        }
        
        void UpdateAI(const uint32 uiDiff)
        {
            if (!UpdateVictim())
            return;
            
            if (!bBloodPresence && uiBloodPresenceTimer <= uiDiff)
            {
                DoCast(me, BLOOD_PRESENCE);
                bBloodPresence = true;
                uiBloodPresenceTimer = 2000;
            } else uiBloodPresenceTimer -= uiDiff;
            
            if (uiBloodBoilTimer <= uiDiff)
            {
                DoCast(me->GetVictim(), BLOOD_BOIL);
                uiBloodBoilTimer = 40000;
            } else uiBloodBoilTimer -= uiDiff;
            
            if (uiBloodPlagueTimer <= uiDiff)
            {
                DoCast(me->GetVictim(), BLOOD_PLAGUE);
                uiBloodPlagueTimer = 60000;
            } else uiBloodPlagueTimer -= uiDiff;
            
            if (uiHysteriaTimer <= uiDiff)
            {
                DoCast(me->GetVictim(), HYSTERIA);
                uiHysteriaTimer = 25000;
            } else uiHysteriaTimer -= uiDiff;
            
            if (uiSpellDeflectionTimer <= uiDiff)
            {
                DoCast(me->GetVictim(), BLOOD_PLAGUE);
                uiSpellDeflectionTimer = 75000;
            } else uiSpellDeflectionTimer -= uiDiff;
            
            if (uiWhirlindTimer <= uiDiff)
            {
                DoCast(me->GetVictim(), BLOOD_PLAGUE);
                uiWhirlindTimer = 12000;
            } else uiWhirlindTimer -= uiDiff;
            
            DoMeleeAttackIfReady();
        }
        
        void JustDied(Unit* killer)
        {
            if (Player* player = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
                player->GetCharmerOrOwnerPlayerOrPlayerItself()->GroupEventHappens(QUEST_CHAMPION_OF_ANGUISH, killer);
        }
    };
    
    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_vladof_the_butcherAI(creature);
    }
};

/*####
## npc_yggdras
####*/

enum eYggdras
{
    SPELL_CLEAVE            = 40504,
    SPELL_CORRODE_FLESH     = 57076,
    SPELL_JORMUNGAR_SPAWN   = 55859
};

class npc_yggdras : public CreatureScript
{
public:
    npc_yggdras() : CreatureScript("npc_yggdras") { }

    struct npc_yggdrasAI : public ScriptedAI
    {
        npc_yggdrasAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 uiCleaveTimer;
        uint32 uiCorrodeFleshTimer;

        void Reset()
        {
            uiCleaveTimer = 9000;
            uiCorrodeFleshTimer = 6000;
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!UpdateVictim())
                return;

            if (me->GetVictim()->GetPositionZ() >= 286.276f)
            {
                std::list<HostileReference*> t_list = me->getThreatManager().getThreatList();
                for (std::list<HostileReference*>::const_iterator itr = t_list.begin(); itr!= t_list.end(); ++itr)
                {
                    if (Unit* unit = Unit::GetUnit(*me, (*itr)->getUnitGuid()))
                    {
                        if (unit->GetPositionZ() <= 286.276f)
                        {
                            me->getThreatManager().resetAllAggro();
                            me->AddThreat(unit, 5.0f);
                            break;
                        }
                        EnterEvadeMode();
                    }
                }
            }

            if (uiCleaveTimer <= uiDiff)
            {
                DoCastVictim(SPELL_CLEAVE);
                uiCleaveTimer = 9000;
            } else uiCleaveTimer -= uiDiff;

            if (uiCorrodeFleshTimer <= uiDiff)
            {
                DoCastVictim(SPELL_CORRODE_FLESH);
                uiCorrodeFleshTimer = 6000;
            } else uiCorrodeFleshTimer -= uiDiff;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* killer)
        {
            if (Unit* summoner = me->ToTempSummon()->GetSummoner())
            {
                std::string sText = (std::string(killer->GetName()) + " has defeated Yg.. Yggg-really big worm!");
                summoner->MonsterYell(sText.c_str(), LANG_UNIVERSAL, 0);
            }

            if (Player* player = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
            {
                player->GroupEventHappens(QUEST_AMPHITHEATER_ANGUISH_YGGDRAS_1, killer);
                player->GroupEventHappens(QUEST_AMPHITHEATER_ANGUISH_YGGDRAS_2, killer);
            }

            for (uint8 i = 0; i < 3; ++i)
                DoCast(killer, SPELL_JORMUNGAR_SPAWN, true);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_yggdrasAI(creature);
    }
};

/*####
## npc_stinkbeard
####*/

enum eStinkbeard
{
    SPELL_ENRAGE_STINKBEARD = 50420,
    SPELL_KNOCK_AWAY        = 31389,
    SPELL_STINKY_BEARD      = 55867,
    SPELL_THUNDERBLADE      = 55866,
    SPELL_THUNDERCLAP       = 15588
};

class npc_stinkbeard : public CreatureScript
{
public:
    npc_stinkbeard() : CreatureScript("npc_stinkbeard") { }

    struct npc_stinkbeardAI : public npc_escortAI
    {
        npc_stinkbeardAI(Creature* creature) : npc_escortAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            me->SetReactState(REACT_PASSIVE);
            Start(true, true, 0, NULL);
            SetDespawnAtEnd(false);
        }

        uint32 uiKnockAwayTimer;
        uint32 uiStinkyBeardTimer;

        bool bEnrage;
        bool bThunderClap;

        void Reset()
        {
            me->AddAura(SPELL_THUNDERBLADE, me);
            uiKnockAwayTimer   = 10000;
            uiStinkyBeardTimer = 15000;
            bEnrage = false;
            bThunderClap = false;
        }

        void WaypointReached(uint32 waypointId)
        {
            switch (waypointId)
            {
                case 7:
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                    break;
            }
        }

        void UpdateAI(const uint32 uiDiff)
        {
            npc_escortAI::UpdateAI(uiDiff);

            if (!UpdateVictim())
                return;

            if (Unit* victim = me->GetVictim())
            {
                if (victim->GetPositionZ() >= 286.276f)
                {
                    std::list<HostileReference*> t_list = me->getThreatManager().getThreatList();
                    for (std::list<HostileReference*>::const_iterator itr = t_list.begin(); itr!= t_list.end(); ++itr)
                    {
                        if (Unit* unit = Unit::GetUnit(*me, (*itr)->getUnitGuid()))
                        {
                            if (unit->GetPositionZ() <= 286.276f)
                            {
                                me->getThreatManager().resetAllAggro();
                                me->AddThreat(unit, 5.0f);
                                break;
                            }
                            EnterEvadeMode();
                        }
                    }
                }
            }

            if (bThunderClap && !HealthAbovePct(10))
            {
                DoCastAOE(SPELL_THUNDERCLAP);
                bThunderClap = true;
            }

            if (uiKnockAwayTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                {
                    if (target && target->isAlive())
                        DoCast(target, SPELL_KNOCK_AWAY);
                }
                uiKnockAwayTimer = 10000;
            } else uiKnockAwayTimer -= uiDiff;

            if (uiStinkyBeardTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                {
                    if (target && target->isAlive())
                        DoCast(target, SPELL_STINKY_BEARD);
                }
                uiStinkyBeardTimer = 15000;
            } else uiStinkyBeardTimer -= uiDiff;

            if (!bEnrage && !HealthAbovePct(20))
            {
                DoCast(me, SPELL_ENRAGE_STINKBEARD);
                bEnrage = true;
            }
            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* killer)
        {
            if (Player* player = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
                player->GetCharmerOrOwnerPlayerOrPlayerItself()->GroupEventHappens(QUEST_AMPHITHEATER_ANGUISH_MAGNATAUR, killer);

            std::string sText = ("And with AUTHORITY, " + std::string(killer->GetName()) + " dominates the magnataur lord! Stinkbeard's clan is gonna miss him back home in the Dragonblight!");
            me->MonsterYell(sText.c_str(), LANG_UNIVERSAL, 0);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_stinkbeardAI(creature);
    }
};

/*####
## npc_elemental_lord
####*/

class npc_elemental_lord : public CreatureScript
{
public:
    npc_elemental_lord() : CreatureScript("npc_elemental_lord") { }

    struct npc_elemental_lordAI : public ScriptedAI
    {
        npc_elemental_lordAI(Creature* creature) : ScriptedAI(creature) {}

        std::list<uint64> SummonList;

        uint32 uiElementalSpellTimer;

        uint8 uiBossRandom;
        uint32 uiSpellInfo;

        bool bAddAttack;

        void Reset()
        {
            uiBossRandom = 0;
            uiSpellInfo = 0;
            uiElementalSpellTimer = urand(5000, 8000);

            bAddAttack = false;
        }

        void SetData(uint32 uiData, uint32 uiValue)
        {
            if (uiData == 1)
            {
                uiBossRandom = uiValue;
                SummonAdds();
            }
        }

        void SummonAdds()
        {
            if (!Boss[uiBossRandom].uiAdd)
                return;

            SummonList.clear();

            for (uint8 uiI = 0; uiI < 16; uiI++)
            {
                if (Creature* summon = me->SummonCreature(Boss[uiBossRandom].uiAdd, AddSpawnPosition[uiI]))
                {
                    summon->AI()->SetData(1, uiBossRandom);
                    SummonList.push_back(summon->GetGUID());
                }
            }

        }

        void EnterCombat(Unit* unit)
        {
            if (!SummonList.empty())
                for (std::list<uint64>::const_iterator itr = SummonList.begin(); itr != SummonList.end(); ++itr)
                {
                    if (Creature* temp = Unit::GetCreature(*me, *itr))
                    {
                        temp->m_CombatDistance = 100.0f; // ugly hack? we are not in a instance sorry. :(
                        temp->AI()->AttackStart(unit);
                    }
                }
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!UpdateVictim())
                return;

            if (me->GetVictim()->GetPositionZ() >= 286.276f)
            {
                std::list<HostileReference*> t_list = me->getThreatManager().getThreatList();
                for (std::list<HostileReference*>::const_iterator itr = t_list.begin(); itr!= t_list.end(); ++itr)
                {
                    if (Unit* unit = Unit::GetUnit(*me, (*itr)->getUnitGuid()))
                    {
                        if (unit->GetPositionZ() <= 286.276f)
                        {
                            me->getThreatManager().resetAllAggro();
                            me->AddThreat(unit, 5.0f);
                            break;
                        }
                        EnterEvadeMode();
                    }
                }
            }

            if (uiElementalSpellTimer <= uiDiff)
            {
                DoCastVictim(Boss[uiBossRandom].uiSpell);

                uiElementalSpellTimer = urand(5000, 8000);
            } else uiElementalSpellTimer -= uiDiff;

            if (!bAddAttack && !HealthAbovePct(20))
            {
                if (!SummonList.empty())
                    for (std::list<uint64>::const_iterator itr = SummonList.begin(); itr != SummonList.end(); ++itr)
                    {
                        if (Creature* temp = Unit::GetCreature(*me, *itr))
                        {
                            if (temp->GetPositionZ() >= 287.00f)
                                continue;

                            if (temp->GetVictim())
                                temp->GetMotionMaster()->MoveChase(temp->GetVictim());
                        }
                    }

                bAddAttack = true;
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* killer)
        {
            if (!SummonList.empty())
                for (std::list<uint64>::const_iterator itr = SummonList.begin(); itr != SummonList.end(); ++itr)
                    if (Creature* temp = Unit::GetCreature(*me, *itr))
                        temp->DespawnOrUnsummon();

            if (Player* player = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
                player->GetCharmerOrOwnerPlayerOrPlayerItself()->GroupEventHappens(QUEST_AMPHITHEATER_ANGUISH_FROM_BEYOND, killer);

            std::string sText = (std::string(killer->GetName()) + " is victorious once more!");

            if (Unit* summoner = me->ToTempSummon()->GetSummoner())
                summoner->MonsterYell(sText.c_str(), LANG_UNIVERSAL, 0);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_elemental_lordAI(creature);
    }
};

/*####
## npc_fiend_elemental
####*/

class npc_fiend_elemental : public CreatureScript
{
public:
    npc_fiend_elemental() : CreatureScript("npc_fiend_elemental") { }

    struct npc_fiend_elementalAI : public ScriptedAI
    {
        npc_fiend_elementalAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 uiMissleTimer;
        uint32 uiSpell;

        void Reset()
        {
            if (me->GetPositionZ() >= 287.0f)
                me->GetMotionMaster()->MoveIdle();

            uiSpell = 0;
            uiMissleTimer = urand(2000, 7000);
        }

        void AttackStart(Unit* who)
        {
            if (!who)
                return;

            AttackStartNoMove(who);
        }

        void SetData(uint32 uiData, uint32 uiValue)
        {
            if (uiData == 1)
                uiSpell = Boss[uiValue].uiAddSpell;

        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!UpdateVictim())
                return;

            if (me->GetPositionZ() >= 287.0f)
            {
                if (uiMissleTimer <= uiDiff)
                {
                    if (uiSpell) // Sometimes it is 0, why?
                        DoCast(me, uiSpell); // this spell (what spell) is not supported ... YET!
                    uiMissleTimer = urand(2000, 7000);
                } else uiMissleTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_fiend_elementalAI(creature);
    }
};

/*####
## npc_released_offspring_harkoa
####*/

class npc_released_offspring_harkoa : public CreatureScript
{
public:
    npc_released_offspring_harkoa() : CreatureScript("npc_released_offspring_harkoa") { }

    struct npc_released_offspring_harkoaAI : public ScriptedAI
    {
        npc_released_offspring_harkoaAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset()
        {
            float x, y, z;
            me->GetClosePoint(x, y, z, me->GetObjectSize() / 3, 25.0f);
            me->GetMotionMaster()->MovePoint(0, x, y, z);
        }

        void MovementInform(uint32 uiType, uint32 /*uiId*/)
        {
            if (uiType != POINT_MOTION_TYPE)
                return;
            me->DisappearAndDie();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_released_offspring_harkoaAI(creature);
    }
};

/*######
## npc_crusade_recruit
######*/

enum eCrusade_recruit
{
    SPELL_QUEST_CREDIT                            = 50633,

    QUEST_TROLL_PATROL_INTESTINAL_FORTITUDE       = 12509,

    GOSSIP_CRUSADE_TEXT                           = 13069
};

#define GOSSIP_ITEM_1 "Get out there and make those Scourge wish they were never reborn!"

class npc_crusade_recruit : public CreatureScript
{
public:
    npc_crusade_recruit() : CreatureScript("npc_crusade_recruit") { }

    struct npc_crusade_recruitAI : public ScriptedAI
    {
        npc_crusade_recruitAI(Creature* creature) : ScriptedAI(creature) {}

        uint8 m_uiPhase;                  //The current phase we are in
        uint32 m_uiTimer;                 //Timer until phase transition
        float m_heading;                  //Store creature heading

        void Reset()
        {
            m_uiTimer = 0;
            m_uiPhase = 0;
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_COWER);
            m_heading = me->GetOrientation();
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (m_uiPhase)
            {
                if (m_uiTimer <= uiDiff)
                {
                    switch (m_uiPhase)
                    {
                        case 1:
                            // say random text
                            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                            Talk(SAY_RECRUIT);
                            m_uiTimer = 3000;
                            m_uiPhase = 2;
                            break;
                        case 2:
                            // walk forward
                            me->SetWalk(true);
                            me->GetMotionMaster()->MovePoint(0, me->GetPositionX() + (cos(m_heading) * 10), me->GetPositionY() + (sin(m_heading) * 10), me->GetPositionZ());
                            m_uiTimer = 5000;
                            m_uiPhase = 3;
                            break;
                        case 3:
                            // despawn
                            me->DisappearAndDie();
                            m_uiTimer = 0;
                            m_uiPhase = 0;
                            break;
                    }
                }
                else
                m_uiTimer -= uiDiff;
            }
            ScriptedAI::UpdateAI(uiDiff);

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_crusade_recruitAI(creature);
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->GetQuestStatus(QUEST_TROLL_PATROL_INTESTINAL_FORTITUDE) == QUEST_STATUS_INCOMPLETE)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

        player->SEND_GOSSIP_MENU(GOSSIP_CRUSADE_TEXT, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF +1)
        {
            player->CLOSE_GOSSIP_MENU();
            creature->CastSpell(player, SPELL_QUEST_CREDIT, true);
            CAST_AI(npc_crusade_recruit::npc_crusade_recruitAI, (creature->AI()))->m_uiPhase = 1;
            creature->SetFacingToObject(player);
        }

        return true;
    }
};

/*######
## Quest 12916: Our Only Hope!
## go_scourge_enclosure
######*/

enum ScourgeEnclosure
{
    QUEST_OUR_ONLY_HOPE                      = 12916,
    NPC_GYMER_DUMMY                          = 29928, // From quest template
    SPELL_GYMER_LOCK_EXPLOSION               = 55529
};

class go_scourge_enclosure : public GameObjectScript
{
public:
    go_scourge_enclosure() : GameObjectScript("go_scourge_enclosure") { }

    bool OnGossipHello(Player* player, GameObject* go) 
    {
        go->UseDoorOrButton();
        if (player->GetQuestStatus(QUEST_OUR_ONLY_HOPE) == QUEST_STATUS_INCOMPLETE)
        {
            Creature* gymerDummy = go->FindNearestCreature(NPC_GYMER_DUMMY, 20.0f);
            if (gymerDummy)
            {
                player->KilledMonsterCredit(gymerDummy->GetEntry(), gymerDummy->GetGUID());
                gymerDummy->CastSpell(gymerDummy, SPELL_GYMER_LOCK_EXPLOSION, true);
                gymerDummy->DespawnOrUnsummon();
            }
        }
        return true;
    }
};

/*######
## Quest: Troll Patrol: The Alchemist's Apprentice
######*/

enum Finklestein
{
    // Creature
    NPC_FINKLESTEIN                          = 28205,
    // Item
    ITEM_KNOTROOT                            = 38338,
    ITEM_PICKLED_EAGLE_EGG                   = 38341,
    ITEM_SPECKLED_GUANO                      = 38337,
    ITEM_WITHERED_BATWING                    = 38339,
    ITEM_SEASONED_SLIDER_CIDER               = 38381,
    ITEM_PULVERIZED_GARGOYLE_TEETH           = 38384,
    ITEM_MUDDY_MIRE_MAGGOT                   = 38386,
    ITEM_SPIKY_SPIDER_EGG                    = 38393,
    ITEM_HAIRY_HERRING_HEAD                  = 38396,
    ITEM_PUTRID_PIRATE_PERSPIRATION          = 38397,
    ITEM_ICECROWN_BOTTLED_WATER              = 38398,
    ITEM_WASPS_WINGS                         = 38369,
    ITEM_PRISMATIC_MOJO                      = 38343,
    ITEM_RAPTOR_CLAW                         = 38370,
    ITEM_AMBERSEED                           = 38340,
    ITEM_SHRUNKEN_DRAGONS_CLAW               = 38344,
    ITEM_CHILLED_SERPENT_MUCUS               = 38346,
    ITEM_CRYSTALLIZED_HOGSNOT                = 38336,
    ITEM_CRUSHED_BASILISK_CRYSTALS           = 38379,
    ITEM_TROLLBANE                           = 38342,
    ITEM_FROZEN_SPIDER_ICHOR                 = 38345,
    // Quest
    QUEST_THE_ALCHEMIST_APPRENTICE_DAILY     = 12541,
    // Spells
    SPELL_ALCHEMIST_APPRENTICE_INVISBUFF     = 51216,
    SPELL_RANDOM_INGREDIENT_EASY_AURA        = 51015,
    SPELL_RANDOM_INGREDIENT_MEDIUM_AURA      = 51154,
    SPELL_RANDOM_INGREDIENT_HARD_AURA        = 51157,
    SPELL_RANDOM_INGREDIENT_EASY             = 51134,
    SPELL_RANDOM_INGREDIENT_MEDIUM           = 51105,
    SPELL_RANDOM_INGREDIENT_HARD             = 51107,
    SPELL_NEXT_INGREDIENT                    = 51049,
    SPELL_POT_CHECK                          = 51046,
    SPELL_THROW_INGREDIENT                   = 51025,
    SPELL_KILL_CREDIT                        = 51111,
    // Spell Fetch Easy
    SPELL_FETCH_KNOTROOT                     = 51018,
    SPELL_FETCH_PICKLED_EAGLE_EGG            = 51055,
    SPELL_FETCH_SPECKLED_GUANO               = 51057,
    SPELL_FETCH_WITHERED_BATWING             = 51059,
    SPELL_FETCH_SEASONED_SLIDER_CIDER        = 51062,
    SPELL_FETCH_PULVERIZED_GARGOYLE_TEETH    = 51064,
    SPELL_FETCH_MUDDY_MIRE_MAGGOT            = 51067,
    SPELL_FETCH_SPIKY_SPIDER_EGG             = 51069,
    SPELL_FETCH_HAIRY_HERRING_HEAD           = 51072,
    SPELL_FETCH_PUTRID_PIRATE_PERSPIRATION   = 51077,
    SPELL_FETCH_ICECROWN_BOTTLED_WATER       = 51079,
    // Spell Have Easy
    SPELL_HAVE_KNOTROOT                      = 51047,
    SPELL_HAVE_PICKLED_EAGLE_EGG             = 51056,
    SPELL_HAVE_SPECKLED_GUANO                = 51058,
    SPELL_HAVE_WITHERED_BATWING              = 51060,
    SPELL_HAVE_SEASONED_SLIDER_CIDER         = 51063,
    SPELL_HAVE_PULVERIZED_GARGOYLE_TEETH     = 51065,
    SPELL_HAVE_MUDDY_MIRE_MAGGOT             = 51068,
    SPELL_HAVE_SPIKY_SPIDER_EGG              = 51070,
    SPELL_HAVE_HAIRY_HERRING_HEAD            = 51075,
    SPELL_HAVE_PUTRID_PIRATE_PERSPIRATION    = 51078,
    SPELL_HAVE_ICECROWN_BOTTLED_WATER        = 51080,
    // Spell Fetch Medium
    SPELL_FETCH_WASPS_WINGS                  = 51081,
    SPELL_FETCH_PRISMATIC_MOJO               = 51083,
    SPELL_FETCH_RAPTOR_CLAW                  = 51085,
    SPELL_FETCH_AMBERSEED                    = 51087,
    SPELL_FETCH_SHRUNKEN_DRAGONS_CLAW        = 51091,
    // Spell Have Medium
    SPELL_HAVE_WASPS_WINGS                   = 51082,
    SPELL_HAVE_PRISMATIC_MOJO                = 51084,
    SPELL_HAVE_RAPTOR_CLAW                   = 51086,
    SPELL_HAVE_AMBERSEED                     = 51088,
    SPELL_HAVE_SHRUNKEN_DRAGONS_CLAW         = 51092,
    // Spell Fetch Hard
    SPELL_FETCH_CHILLED_SERPENT_MUCUS        = 51093,
    SPELL_FETCH_CRYSTALLIZED_HOGSNOT         = 51095,
    SPELL_FETCH_CRUSHED_BASILISK_CRYSTALS    = 51097,
    SPELL_FETCH_TROLLBANE                    = 51100,
    SPELL_FETCH_FROZEN_SPIDER_ICHOR          = 51102,
    // Spell Have Hard
    SPELL_HAVE_CHILLED_SERPENT_MUCUS         = 51094,
    SPELL_HAVE_CRYSTALLIZED_HOGSNOT          = 51096,
    SPELL_HAVE_CRUSHED_BASILISK_CRYSTALS     = 51098,
    SPELL_HAVE_TROLLBANE                     = 51101,
    SPELL_HAVE_FROZEN_SPIDER_ICHOR           = 51104,
    // Text
    SAY_EASY_123                             = 0,
    SAY_MEDIUM_4                             = 1,
    SAY_MEDIUM_5                             = 2,
    SAY_HARD_6                               = 3,
    SAY_RUINED                               = 4,
    // Text Easy
    SAY_KNOTROOT                             = 5,
    SAY_PICKLED_EAGLE_EGG                    = 6,
    SAY_SPECKLED_GUANO                       = 7,
    SAY_WITHERED_BATWING                     = 8,
    SAY_SEASONED_SLIDER_CIDER                = 9,
    SAY_PULVERIZED_GARGOYLE_TEETH            = 10,
    SAY_MUDDY_MIRE_MAGGOT                    = 11,
    SAY_SPIKY_SPIDER_EGG                     = 12,
    SAY_HAIRY_HERRING_HEAD                   = 13,
    SAY_PUTRID_PIRATE_PERSPIRATION           = 14,
    SAY_ICECROWN_BOTTLED_WATER               = 15,
    // Text Medium
    SAY_WASPS_WINGS                          = 16,
    SAY_PRISMATIC_MOJO                       = 17,
    SAY_RAPTOR_CLAW                          = 18,
    SAY_AMBERSEED                            = 19,
    SAY_SHRUNKEN_DRAGONS_CLAW                = 20,
    // Text Hard
    SAY_CHILLED_SERPENT_MUCUS                = 21,
    SAY_CRYSTALLIZED_HOGSNOT                 = 22,
    SAY_CRUSHED_BASILISK_CRYSTALS            = 23,
    SAY_TROLLBANE                            = 24,
    SAY_FROZEN_SPIDER_ICHOR                  = 25
};

enum FinklesteinEvents
{
    EVENT_TURN_TO_POT                      = 1,
    EVENT_TURN_BACK                        = 2,
    EVENT_EASY_123                         = 3,
    EVENT_MEDIUM_4                         = 4,
    EVENT_MEDIUM_5                         = 5,
    EVENT_HARD_6                           = 6
};

class npc_alchemist_finklestein : public CreatureScript
{
public:
    npc_alchemist_finklestein() : CreatureScript("npc_alchemist_finklestein") { }

        struct npc_alchemist_finklesteinAI : public ScriptedAI
        {
            npc_alchemist_finklesteinAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() 
            {
                _events.ScheduleEvent(EVENT_TURN_TO_POT, urand(15000, 26000));
            }

            void SetData(uint32 Type, uint32 Data) 
            {
                if (Type == 1 && Data == 1)
                    switch (_getingredienttry)
                   {
                        case 2:
                        case 3:
                            _events.ScheduleEvent(EVENT_EASY_123, 100);
                            break;
                        case 4:
                            _events.ScheduleEvent(EVENT_MEDIUM_4, 100);
                            break;
                        case 5:
                            _events.ScheduleEvent(EVENT_MEDIUM_5, 100);
                            break;
                        case 6:
                            _events.ScheduleEvent(EVENT_HARD_6, 100);
                            break;
                        default:
                            break;
                    }
            }

            void UpdateAI(uint32 const diff) 
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_TURN_TO_POT:
                            me->SetFacingTo(6.230825f);
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_USE_STANDING_NO_SHEATHE);
                            _events.ScheduleEvent(EVENT_TURN_BACK, 11000);
                            break;
                        case EVENT_TURN_BACK:
                            me->SetFacingTo(4.886922f);
                            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
                            _events.ScheduleEvent(EVENT_TURN_TO_POT, urand(25000, 41000));
                            break;
                        case EVENT_EASY_123:
                            if (Player* player = Unit::GetPlayer(*me, _playerGUID))
                            {
                                Talk(SAY_EASY_123, _playerGUID);
                                DoCast(player, SPELL_RANDOM_INGREDIENT_EASY_AURA);
                                ++_getingredienttry;
                            }
                            break;
                        case EVENT_MEDIUM_4:
                            if (Player* player = Unit::GetPlayer(*me, _playerGUID))
                            {
                                Talk(SAY_MEDIUM_4, _playerGUID);
                                DoCast(player, SPELL_RANDOM_INGREDIENT_MEDIUM_AURA);
                                ++_getingredienttry;
                            }
                            break;
                        case EVENT_MEDIUM_5:
                            if (Player* player = Unit::GetPlayer(*me, _playerGUID))
                            {
                                Talk(SAY_MEDIUM_5, _playerGUID);
                                DoCast(player, SPELL_RANDOM_INGREDIENT_MEDIUM_AURA);
                                ++_getingredienttry;
                            }
                            break;
                        case EVENT_HARD_6:
                            if (Player* player = Unit::GetPlayer(*me, _playerGUID))
                            {
                                Talk(SAY_HARD_6, _playerGUID);
                                DoCast(player, SPELL_RANDOM_INGREDIENT_HARD_AURA);
                                ++_getingredienttry;
                            }
                            break;
                        default:
                            break;
                    }
                }
            }

            void sGossipSelect(Player* player, uint32 /*sender*/, uint32 /*action*/) 
            {
                player->CLOSE_GOSSIP_MENU();
                DoCast(player, SPELL_ALCHEMIST_APPRENTICE_INVISBUFF);
                _playerGUID = player->GetGUID();
                _getingredienttry = 1;
                _events.ScheduleEvent(EVENT_EASY_123, 100);
            }

        private:
            EventMap _events;
            uint64   _playerGUID;
            uint8    _getingredienttry;
        };

        CreatureAI* GetAI(Creature* creature) const 
        {
            return new npc_alchemist_finklesteinAI(creature);
        }
};

class go_finklesteins_cauldron : public GameObjectScript
{
public:
    go_finklesteins_cauldron() : GameObjectScript("go_finklesteins_cauldron") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) 
    {
        player->CastSpell(player, SPELL_POT_CHECK);
        return true;
    }
};

uint32 const FetchIngredients[21][4] =
{
    { SPELL_FETCH_KNOTROOT,                   SPELL_HAVE_KNOTROOT,                   ITEM_KNOTROOT,                   SAY_KNOTROOT                   },
    { SPELL_FETCH_PICKLED_EAGLE_EGG,          SPELL_HAVE_PICKLED_EAGLE_EGG,          ITEM_PICKLED_EAGLE_EGG,          SAY_PICKLED_EAGLE_EGG          },
    { SPELL_FETCH_SPECKLED_GUANO,             SPELL_HAVE_SPECKLED_GUANO,             ITEM_SPECKLED_GUANO,             SAY_SPECKLED_GUANO             },
    { SPELL_FETCH_WITHERED_BATWING,           SPELL_HAVE_WITHERED_BATWING,           ITEM_WITHERED_BATWING,           SAY_WITHERED_BATWING           },
    { SPELL_FETCH_SEASONED_SLIDER_CIDER,      SPELL_HAVE_SEASONED_SLIDER_CIDER,      ITEM_SEASONED_SLIDER_CIDER,      SAY_SEASONED_SLIDER_CIDER      },
    { SPELL_FETCH_PULVERIZED_GARGOYLE_TEETH,  SPELL_HAVE_PULVERIZED_GARGOYLE_TEETH,  ITEM_PULVERIZED_GARGOYLE_TEETH,  SAY_PULVERIZED_GARGOYLE_TEETH  },
    { SPELL_FETCH_MUDDY_MIRE_MAGGOT,          SPELL_HAVE_MUDDY_MIRE_MAGGOT,          ITEM_MUDDY_MIRE_MAGGOT,          SAY_MUDDY_MIRE_MAGGOT          },
    { SPELL_FETCH_SPIKY_SPIDER_EGG,           SPELL_HAVE_SPIKY_SPIDER_EGG,           ITEM_SPIKY_SPIDER_EGG,           SAY_SPIKY_SPIDER_EGG           },
    { SPELL_FETCH_HAIRY_HERRING_HEAD,         SPELL_HAVE_HAIRY_HERRING_HEAD,         ITEM_HAIRY_HERRING_HEAD,         SAY_HAIRY_HERRING_HEAD         },
    { SPELL_FETCH_PUTRID_PIRATE_PERSPIRATION, SPELL_HAVE_PUTRID_PIRATE_PERSPIRATION, ITEM_PUTRID_PIRATE_PERSPIRATION, SAY_PUTRID_PIRATE_PERSPIRATION },
    { SPELL_FETCH_ICECROWN_BOTTLED_WATER,     SPELL_HAVE_ICECROWN_BOTTLED_WATER,     ITEM_ICECROWN_BOTTLED_WATER,     SAY_ICECROWN_BOTTLED_WATER     },
    { SPELL_FETCH_WASPS_WINGS,                SPELL_HAVE_WASPS_WINGS,                ITEM_WASPS_WINGS,                SAY_WASPS_WINGS                },
    { SPELL_FETCH_PRISMATIC_MOJO,             SPELL_HAVE_PRISMATIC_MOJO,             ITEM_PRISMATIC_MOJO,             SAY_PRISMATIC_MOJO             },
    { SPELL_FETCH_RAPTOR_CLAW,                SPELL_HAVE_RAPTOR_CLAW,                ITEM_RAPTOR_CLAW,                SAY_RAPTOR_CLAW                },
    { SPELL_FETCH_AMBERSEED,                  SPELL_HAVE_AMBERSEED,                  ITEM_AMBERSEED,                  SAY_AMBERSEED                  },
    { SPELL_FETCH_SHRUNKEN_DRAGONS_CLAW,      SPELL_HAVE_SHRUNKEN_DRAGONS_CLAW,      ITEM_SHRUNKEN_DRAGONS_CLAW,      SAY_SHRUNKEN_DRAGONS_CLAW      },
    { SPELL_FETCH_CHILLED_SERPENT_MUCUS,      SPELL_HAVE_CHILLED_SERPENT_MUCUS,      ITEM_CHILLED_SERPENT_MUCUS,      SAY_CHILLED_SERPENT_MUCUS      },
    { SPELL_FETCH_CRYSTALLIZED_HOGSNOT,       SPELL_HAVE_CRYSTALLIZED_HOGSNOT,       ITEM_CRYSTALLIZED_HOGSNOT,       SAY_CRYSTALLIZED_HOGSNOT       },
    { SPELL_FETCH_CRUSHED_BASILISK_CRYSTALS,  SPELL_HAVE_CRUSHED_BASILISK_CRYSTALS,  ITEM_CRUSHED_BASILISK_CRYSTALS,  SAY_CRUSHED_BASILISK_CRYSTALS  },
    { SPELL_FETCH_TROLLBANE,                  SPELL_HAVE_TROLLBANE,                  ITEM_TROLLBANE,                  SAY_TROLLBANE                  },
    { SPELL_FETCH_FROZEN_SPIDER_ICHOR,        SPELL_HAVE_FROZEN_SPIDER_ICHOR,        ITEM_FROZEN_SPIDER_ICHOR,        SAY_FROZEN_SPIDER_ICHOR        }
};

/*#####
# spell_random_ingredient_aura
#####*/

class spell_random_ingredient_aura : public SpellScriptLoader
{
    public: spell_random_ingredient_aura() : SpellScriptLoader("spell_random_ingredient_aura") { }

        class spell_random_ingredient_aura_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_random_ingredient_aura_AuraScript);

            bool Validate(SpellInfo const* /*spellEntry*/) 
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_RANDOM_INGREDIENT_EASY) || !sSpellMgr->GetSpellInfo(SPELL_RANDOM_INGREDIENT_MEDIUM) || !sSpellMgr->GetSpellInfo(SPELL_RANDOM_INGREDIENT_HARD))
                    return false;
                return true;
            }

            void PeriodicTick(AuraEffect const* /*aurEff*/)
            {
                switch (GetSpellInfo()->Id)
                {
                    case SPELL_RANDOM_INGREDIENT_EASY_AURA:
                        GetTarget()->CastSpell(GetTarget(), SPELL_RANDOM_INGREDIENT_EASY);
                        break;
                    case SPELL_RANDOM_INGREDIENT_MEDIUM_AURA:
                        GetTarget()->CastSpell(GetTarget(), SPELL_RANDOM_INGREDIENT_MEDIUM);
                        break;
                    case SPELL_RANDOM_INGREDIENT_HARD_AURA:
                        GetTarget()->CastSpell(GetTarget(), SPELL_RANDOM_INGREDIENT_HARD);
                        break;
                }
            }

            void Register() 
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_random_ingredient_aura_AuraScript::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const 
        {
            return new spell_random_ingredient_aura_AuraScript();
        }
};

/*#####
# spell_random_ingredient
#####*/

class spell_random_ingredient : public SpellScriptLoader
{
    public: spell_random_ingredient() : SpellScriptLoader("spell_random_ingredient") { }

        class spell_random_ingredient_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_random_ingredient_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/) 
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_FETCH_KNOTROOT) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_PICKLED_EAGLE_EGG) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_SPECKLED_GUANO) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FETCH_WITHERED_BATWING) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_SEASONED_SLIDER_CIDER) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_PULVERIZED_GARGOYLE_TEETH) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FETCH_MUDDY_MIRE_MAGGOT) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_SPIKY_SPIDER_EGG) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_HAIRY_HERRING_HEAD) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FETCH_PUTRID_PIRATE_PERSPIRATION) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_ICECROWN_BOTTLED_WATER) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_WASPS_WINGS) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FETCH_PRISMATIC_MOJO) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_RAPTOR_CLAW) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_AMBERSEED) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FETCH_SHRUNKEN_DRAGONS_CLAW) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_CHILLED_SERPENT_MUCUS) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_CRYSTALLIZED_HOGSNOT) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FETCH_CRUSHED_BASILISK_CRYSTALS) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_TROLLBANE) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_FROZEN_SPIDER_ICHOR))
                    return false;
                return true;
            }

            void HandleScriptEffect(SpellEffIndex /* effIndex */)
            {
                if (Player* player = GetHitPlayer())
                {
                    uint8 ingredient = 0;

                    switch (GetSpellInfo()->Id)
                    {
                        case SPELL_RANDOM_INGREDIENT_EASY:
                            ingredient = urand(0, 10);
                            break;
                        case SPELL_RANDOM_INGREDIENT_MEDIUM:
                            ingredient = urand(11, 15);
                            break;
                        case SPELL_RANDOM_INGREDIENT_HARD:
                            ingredient = urand(16, 20);
                            break;
                    }

                    if (Creature* finklestein = GetClosestCreatureWithEntry(player, NPC_FINKLESTEIN, 25.0f))
                    {
                        finklestein->CastSpell(player, FetchIngredients[ingredient][0], true, NULL);
                        finklestein->AI()->Talk(FetchIngredients[ingredient][3], player->GetGUID());
                    }
                }
            }

            void Register() 
            {
                OnEffectHitTarget += SpellEffectFn(spell_random_ingredient_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
    };

        SpellScript* GetSpellScript() const 
        {
            return new spell_random_ingredient_SpellScript();
        }
};

/*#####
# spell_pot_check
#####*/

class spell_pot_check : public SpellScriptLoader
{
    public: spell_pot_check() : SpellScriptLoader("spell_pot_check") { }

        class spell_pot_check_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pot_check_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/) 
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_FETCH_KNOTROOT) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_PICKLED_EAGLE_EGG) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_SPECKLED_GUANO) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FETCH_WITHERED_BATWING) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_SEASONED_SLIDER_CIDER) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_PULVERIZED_GARGOYLE_TEETH) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FETCH_MUDDY_MIRE_MAGGOT) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_SPIKY_SPIDER_EGG) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_HAIRY_HERRING_HEAD) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FETCH_PUTRID_PIRATE_PERSPIRATION) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_ICECROWN_BOTTLED_WATER) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_WASPS_WINGS) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FETCH_PRISMATIC_MOJO) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_RAPTOR_CLAW) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_AMBERSEED) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FETCH_SHRUNKEN_DRAGONS_CLAW) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_CHILLED_SERPENT_MUCUS) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_CRYSTALLIZED_HOGSNOT) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FETCH_CRUSHED_BASILISK_CRYSTALS) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_TROLLBANE) || !sSpellMgr->GetSpellInfo(SPELL_FETCH_FROZEN_SPIDER_ICHOR) ||
                    !sSpellMgr->GetSpellInfo(SPELL_HAVE_KNOTROOT) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_PICKLED_EAGLE_EGG) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_SPECKLED_GUANO) ||
                    !sSpellMgr->GetSpellInfo(SPELL_HAVE_WITHERED_BATWING) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_SEASONED_SLIDER_CIDER) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_PULVERIZED_GARGOYLE_TEETH) ||
                    !sSpellMgr->GetSpellInfo(SPELL_HAVE_MUDDY_MIRE_MAGGOT) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_SPIKY_SPIDER_EGG) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_HAIRY_HERRING_HEAD) ||
                    !sSpellMgr->GetSpellInfo(SPELL_HAVE_PUTRID_PIRATE_PERSPIRATION) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_ICECROWN_BOTTLED_WATER) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_WASPS_WINGS) ||
                    !sSpellMgr->GetSpellInfo(SPELL_HAVE_PRISMATIC_MOJO) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_RAPTOR_CLAW) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_AMBERSEED) ||
                    !sSpellMgr->GetSpellInfo(SPELL_HAVE_SHRUNKEN_DRAGONS_CLAW) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_CHILLED_SERPENT_MUCUS) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_CRYSTALLIZED_HOGSNOT) ||
                    !sSpellMgr->GetSpellInfo(SPELL_HAVE_CRUSHED_BASILISK_CRYSTALS) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_TROLLBANE) || !sSpellMgr->GetSpellInfo(SPELL_HAVE_FROZEN_SPIDER_ICHOR))
                    return false;
                return true;
            }

        void HandleScriptEffect(SpellEffIndex /* effIndex */)
        {
            if (Player* player = GetHitPlayer())
            {
                for (uint8 i = 0; i < 21; ++i)
                {
                    if (player->HasAura(FetchIngredients[i][0]))
                    {
                        player->CastSpell(player, SPELL_THROW_INGREDIENT);
                        player->RemoveAura(FetchIngredients[i][0]);
                        if (player->HasAura(FetchIngredients[i][1]))
                        {
                            player->RemoveAura(FetchIngredients[i][1]);
                            player->DestroyItemCount(FetchIngredients[i][2], 1, true);
                            if (i < 15)
                            {
                                if (Creature* finklestein = GetClosestCreatureWithEntry(player, NPC_FINKLESTEIN, 25.0f))
                                    finklestein->AI()->SetData(1, 1);
                                return;
                            }
                            else
                            {
                                if (player->GetQuestStatus(QUEST_THE_ALCHEMIST_APPRENTICE_DAILY) == QUEST_STATUS_INCOMPLETE)
                                {
                                    player->RemoveAura(SPELL_ALCHEMIST_APPRENTICE_INVISBUFF);
                                    player->CastSpell(player, SPELL_KILL_CREDIT);
                                }
                            }
                        }
                        else
                        {
                            RemoveItems(player);
                            player->RemoveAura(SPELL_ALCHEMIST_APPRENTICE_INVISBUFF);
                            if (Creature* finklestein = GetClosestCreatureWithEntry(player, NPC_FINKLESTEIN, 25.0f))
                                finklestein->AI()->Talk(SAY_RUINED, player->GetGUID());
                            return;
                        }
                    }
                 }
             }
         }

        void RemoveItems(Player* player)
        {
            for (uint8 i = 0; i < 21; ++i)
                if (player->HasItemCount(FetchIngredients[i][2], 1, true))
                    player->DestroyItemCount(FetchIngredients[i][2], 1, true);
        }

        void Register() 
        {
            OnEffectHitTarget += SpellEffectFn(spell_pot_check_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

        SpellScript* GetSpellScript() const 
        {
            return new spell_pot_check_SpellScript();
        }
};

/*#####
# spell_fetch_ingredient_aura
#####*/

class spell_fetch_ingredient_aura : public SpellScriptLoader
{
    public: spell_fetch_ingredient_aura() : SpellScriptLoader("spell_fetch_ingredient_aura") { }

        class spell_fetch_ingredient_aura_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_fetch_ingredient_aura_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                    if (target->HasAura(SPELL_ALCHEMIST_APPRENTICE_INVISBUFF))
                        if (Creature* finklestein = GetClosestCreatureWithEntry(target, NPC_FINKLESTEIN, 100.0f))
                        {
                            target->RemoveAura(SPELL_ALCHEMIST_APPRENTICE_INVISBUFF);
                            finklestein->AI()->Talk(SAY_RUINED, target->GetGUID());
                        }
            }

            void Register() 
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_fetch_ingredient_aura_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const 
        {
            return new spell_fetch_ingredient_aura_AuraScript();
        }
};

enum StormCloud 
{
    STORM_COULD         = 29939,
    HEALING_WINDS       = 55549,
    STORM_VISUAL        = 55708,
    GYMERS_GRAB         = 55516,
    RIDE_VEHICLE        = 43671
};

class npc_storm_cloud : public CreatureScript
{
public:
    npc_storm_cloud() : CreatureScript("npc_storm_cloud") { }

    struct npc_storm_cloudAI : public ScriptedAI
    {
        npc_storm_cloudAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset() 
        {
            me->CastSpell(me, STORM_VISUAL, true);
        }

        void JustRespawned() 
        {
            Reset();
        }
        
        void SpellHit(Unit* caster, const SpellInfo* spell) 
        {
            if (spell->Id != GYMERS_GRAB)
                return;

            if (Vehicle* veh = caster->GetVehicleKit())
                if (veh->GetAvailableSeatCount() != 0)
            {
                me->CastSpell(caster, RIDE_VEHICLE, true);
                me->CastSpell(caster, HEALING_WINDS, true);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const 
    {
        return new npc_storm_cloudAI(creature);
    }
};

void AddSC_zuldrak()
{
    new npc_drakuru_shackles;
    new npc_captured_rageclaw;
    new npc_gymer;
    new npc_gurgthock;
    new npc_orinoko_tuskbreaker;
    new npc_korrak_bloodrager;
    new npc_vladof_the_butcher;
    new npc_yggdras;
    new npc_stinkbeard;
    new npc_released_offspring_harkoa;
    new npc_crusade_recruit;
    new npc_elemental_lord;
    new npc_fiend_elemental;
    new go_scourge_enclosure;
    new npc_alchemist_finklestein();
    new go_finklesteins_cauldron();
    new spell_random_ingredient_aura();
    new spell_random_ingredient();
    new spell_pot_check();
    new spell_fetch_ingredient_aura();
    new npc_storm_cloud();

}
