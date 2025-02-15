/*
 * Copyright (C) 2011 SingularityCore <http://www.singularitycore.org/>
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#ifndef __SPELL_H
#define __SPELL_H

#include "GridDefines.h"
#include "SharedDefines.h"
#include "ObjectMgr.h"

class Unit;
class Player;
class GameObject;
class DynamicObject;
class WorldObject;
class Aura;
class SpellScript;
class ByteBuffer;

struct SpellEntry;

enum SpellCastTargetFlags
{
    TARGET_FLAG_SELF            = 0x00000000,
    TARGET_FLAG_UNUSED1         = 0x00000001,               // not used in any spells as of 3.2.2a (can be set dynamically)
    TARGET_FLAG_UNIT            = 0x00000002,               // pguid
    TARGET_FLAG_UNUSED2         = 0x00000004,               // not used in any spells as of 3.2.2a (can be set dynamically)
    TARGET_FLAG_UNUSED3         = 0x00000008,               // not used in any spells as of 3.2.2a (can be set dynamically)
    TARGET_FLAG_ITEM            = 0x00000010,               // pguid
    TARGET_FLAG_SOURCE_LOCATION = 0x00000020,               // 3 float
    TARGET_FLAG_DEST_LOCATION   = 0x00000040,               // 3 float
    TARGET_FLAG_OBJECT_CASTER   = 0x00000080,               // used in 7 spells only
    TARGET_FLAG_UNIT_CASTER     = 0x00000100,               // looks like self target (480 spells)
    TARGET_FLAG_PVP_CORPSE      = 0x00000200,               // pguid
    TARGET_FLAG_UNIT_CORPSE     = 0x00000400,               // 10 spells (gathering professions)
    TARGET_FLAG_OBJECT          = 0x00000800,               // pguid, 2 spells
    TARGET_FLAG_TRADE_ITEM      = 0x00001000,               // pguid, 0 spells
    TARGET_FLAG_STRING          = 0x00002000,               // string, 0 spells
    TARGET_FLAG_OPEN_LOCK       = 0x00004000,               // 199 spells, opening object/lock
    TARGET_FLAG_CORPSE          = 0x00008000,               // pguid, resurrection spells
    TARGET_FLAG_UNK17           = 0x00010000,               // pguid, not used in any spells as of 3.2.2a (can be set dynamically)
    TARGET_FLAG_GLYPH           = 0x00020000,               // used in glyph spells
    TARGET_FLAG_UNK19           = 0x00040000,               //
    TARGET_FLAG_UNUSED20        = 0x00080000                // uint32 counter, loop { vec3 - screen position (?), guid }, not used so far
};
#define MAX_TARGET_FLAGS 21

enum SpellCastFlags
{
    CAST_FLAG_NONE               = 0x00000000,
    CAST_FLAG_PENDING            = 0x00000001,              // Pending == 1
    CAST_FLAG_UNKNOWN_2          = 0x00000002,
    CAST_FLAG_UNKNOWN_3          = 0x00000004,
    CAST_FLAG_UNKNOWN_4          = 0x00000008,
    CAST_FLAG_UNKNOWN_5          = 0x00000010,
    CAST_FLAG_AMMO               = 0x00000020,              // Projectiles visual
    CAST_FLAG_UNKNOWN_7          = 0x00000040,
    CAST_FLAG_UNKNOWN_8          = 0x00000080,
    CAST_FLAG_UNKNOWN_9          = 0x00000100,
    CAST_FLAG_UNKNOWN_10         = 0x00000200,
    CAST_FLAG_UNKNOWN_11         = 0x00000400,
    CAST_FLAG_POWER_LEFT_SELF    = 0x00000800,
    CAST_FLAG_UNKNOWN_13         = 0x00001000,
    CAST_FLAG_UNKNOWN_14         = 0x00002000,
    CAST_FLAG_UNKNOWN_15         = 0x00004000,
    CAST_FLAG_UNKNOWN_16         = 0x00008000,
    CAST_FLAG_UNKNOWN_17         = 0x00010000,
    CAST_FLAG_UNKNOWN_18         = 0x00020000,
    CAST_FLAG_UNKNOWN_19         = 0x00040000,
    CAST_FLAG_UNKNOWN_20         = 0x00080000,
    CAST_FLAG_UNKNOWN_21         = 0x00100000,
    CAST_FLAG_RUNE_LIST          = 0x00200000,
    CAST_FLAG_UNKNOWN_23         = 0x00400000,
    CAST_FLAG_UNKNOWN_24         = 0x00800000,
    CAST_FLAG_UNKNOWN_25         = 0x01000000,
    CAST_FLAG_UNKNOWN_26         = 0x02000000,
    CAST_FLAG_UNKNOWN_27         = 0x04000000,
    CAST_FLAG_UNKNOWN_28         = 0x08000000,
    CAST_FLAG_UNKNOWN_29         = 0x10000000,
    CAST_FLAG_UNKNOWN_30         = 0x20000000,
    CAST_FLAG_UNKNOWN_31         = 0x40000000,
    CAST_FLAG_UNKNOWN_32         = 0x80000000,
};

enum SpellRangeFlag
{
    SPELL_RANGE_DEFAULT             = 0,
    SPELL_RANGE_MELEE               = 1,     //melee
    SPELL_RANGE_RANGED              = 2,     //hunter range and ranged weapon
};

enum SpellNotifyPushType
{
    PUSH_NONE           = 0,
    PUSH_IN_FRONT,
    PUSH_IN_BACK,
    PUSH_IN_LINE,
    PUSH_IN_THIN_LINE,
    PUSH_SRC_CENTER,
    PUSH_DST_CENTER,
    PUSH_CASTER_CENTER, //this is never used in grid search
    PUSH_CHAIN,
};

bool IsQuestTameSpell(uint32 spellId);

namespace Trinity
{
    struct SpellNotifierCreatureAndPlayer;
}

typedef std::list<Unit*> UnitList;

class SpellCastTargets
{
    public:
        SpellCastTargets();
        ~SpellCastTargets();

        SpellCastTargets& operator=(const SpellCastTargets &target)
        {
            m_unitTarget = target.m_unitTarget;
            m_itemTarget = target.m_itemTarget;
            m_GOTarget   = target.m_GOTarget;

            m_unitTargetGUID   = target.m_unitTargetGUID;
            m_GOTargetGUID     = target.m_GOTargetGUID;
            m_CorpseTargetGUID = target.m_CorpseTargetGUID;
            m_itemTargetGUID   = target.m_itemTargetGUID;

            m_itemTargetEntry  = target.m_itemTargetEntry;

            m_srcTransGUID = target.m_srcTransGUID;
            m_srcTransOffset = target.m_srcTransOffset;
            m_srcPos = target.m_srcPos;

            m_dstTransGUID = target.m_dstTransGUID;
            m_dstTransOffset = target.m_dstTransOffset;
            m_dstPos = target.m_dstPos;

            m_elevation = target.m_elevation;
            m_speed = target.m_speed;

            m_strTarget = target.m_strTarget;

            m_targetMask = target.m_targetMask;

            return *this;
        }
        void read (ByteBuffer & data, Unit * caster);
        void write (ByteBuffer & data);

        uint32 getTargetMask() const { return m_targetMask; }
        void setTargetMask(uint32 newMask) { m_targetMask = newMask; }

        uint64 getUnitTargetGUID() const { return m_unitTargetGUID; }
        Unit *getUnitTarget() const { return m_unitTarget; }
        void setUnitTarget(Unit *target);
        void setSrc(float x, float y, float z);
        void setSrc(Position &pos);
        void setSrc(WorldObject &wObj);
        void modSrc(Position &pos);
        void setDst(float x, float y, float z, float orientation, uint32 mapId = MAPID_INVALID);
        void setDst(Position &pos);
        void setDst(WorldObject &wObj);
        void setDst(SpellCastTargets &spellTargets);
        void modDst(Position &pos);

        uint64 getGOTargetGUID() const { return m_GOTargetGUID; }
        GameObject *getGOTarget() const { return m_GOTarget; }
        void setGOTarget(GameObject *target);

        uint64 getCorpseTargetGUID() const { return m_CorpseTargetGUID; }
        void setCorpseTarget(Corpse* corpse);
        uint64 getItemTargetGUID() const { return m_itemTargetGUID; }
        Item* getItemTarget() const { return m_itemTarget; }
        uint32 getItemTargetEntry() const { return m_itemTargetEntry; }
        void setItemTarget(Item* item);
        void setTradeItemTarget(Player* caster);
        void updateTradeSlotItem()
        {
            if (m_itemTarget && (m_targetMask & TARGET_FLAG_TRADE_ITEM))
            {
                m_itemTargetGUID = m_itemTarget->GetGUID();
                m_itemTargetEntry = m_itemTarget->GetEntry();
            }
        }

        bool IsEmpty() const { return m_GOTargetGUID == 0 && m_unitTargetGUID == 0 && m_itemTarget == 0 && m_CorpseTargetGUID == 0; }
        bool HasSrc() const { return getTargetMask() & TARGET_FLAG_SOURCE_LOCATION; }
        bool HasDst() const { return getTargetMask() & TARGET_FLAG_DEST_LOCATION; }
        bool HasTraj() const { return m_speed != 0; }

        float GetDist2d() const { return m_srcPos.GetExactDist2d(&m_dstPos); }
        float GetSpeedXY() const { return m_speed * cos(m_elevation); }
        float GetSpeedZ() const { return m_speed * sin(m_elevation); }

        void Update(Unit* caster);
        void OutDebug();

        uint64 m_srcTransGUID;
        Position m_srcTransOffset;
        Position m_srcPos;
        uint64 m_dstTransGUID;
        Position m_dstTransOffset;
        WorldLocation m_dstPos;
        float m_elevation, m_speed;
        std::string m_strTarget;

    private:
        uint32 m_targetMask;
        // objects (can be used at spell creating and after Update at casting
        Unit *m_unitTarget;
        GameObject *m_GOTarget;
        Item *m_itemTarget;

        // object GUID/etc, can be used always
        uint64 m_unitTargetGUID;
        uint64 m_GOTargetGUID;
        uint64 m_CorpseTargetGUID;
        uint64 m_itemTargetGUID;
        uint32 m_itemTargetEntry;
};

struct SpellValue
{
    explicit SpellValue(SpellEntry const *proto)
    {
        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            EffectBasePoints[i] = proto->GetSpellEffectBasePoints(i);
        MaxAffectedTargets = proto->GetMaxAffectedTargets();
        RadiusMod = 1.0f;
    }
    int32     EffectBasePoints[3];
    uint32    MaxAffectedTargets;
    float     RadiusMod;
};

enum SpellState
{
    SPELL_STATE_NULL      = 0,
    SPELL_STATE_PREPARING = 1,
    SPELL_STATE_CASTING   = 2,
    SPELL_STATE_FINISHED  = 3,
    SPELL_STATE_IDLE      = 4,
    SPELL_STATE_DELAYED   = 5
};

enum ReplenishType
{
    REPLENISH_UNDEFINED = 0,
    REPLENISH_HEALTH    = 20,
    REPLENISH_MANA      = 21,
    REPLENISH_RAGE      = 22
};

enum SpellTargets
{
    SPELL_TARGETS_NONE      = 0,
    SPELL_TARGETS_ALLY,
    SPELL_TARGETS_ENEMY,
    SPELL_TARGETS_ENTRY,
    SPELL_TARGETS_CHAINHEAL,
    SPELL_TARGETS_ANY,
    SPELL_TARGETS_GO
};

class Spell
{
    friend struct Trinity::SpellNotifierCreatureAndPlayer;
    friend void Unit::SetCurrentCastedSpell(Spell * pSpell);
    friend class SpellScript;
    public:

        void EffectNULL(SpellEffectEntry const* effect);
        void EffectUnused(SpellEffectEntry const* effect);
        void EffectDistract(SpellEffectEntry const* effect);
        void EffectPull(SpellEffectEntry const* effect);
        void EffectSchoolDMG(SpellEffectEntry const* effect);
        void EffectEnvirinmentalDMG(SpellEffectEntry const* effect);
        void EffectInstaKill(SpellEffectEntry const* effect);
        void EffectDummy(SpellEffectEntry const* effect);
        void EffectTeleportUnits(SpellEffectEntry const* effect);
        void EffectApplyAura(SpellEffectEntry const* effect);
        void EffectSendEvent(SpellEffectEntry const* effect);
        void EffectPowerBurn(SpellEffectEntry const* effect);
        void EffectPowerDrain(SpellEffectEntry const* effect);
        void EffectHeal(SpellEffectEntry const* effect);
        void EffectBind(SpellEffectEntry const* effect);
        void EffectHealthLeech(SpellEffectEntry const* effect);
        void EffectQuestComplete(SpellEffectEntry const* effect);
        void EffectCreateItem(SpellEffectEntry const* effect);
        void EffectCreateItem2(SpellEffectEntry const* effect);
        void EffectCreateRandomItem(SpellEffectEntry const* effect);
        void EffectPersistentAA(SpellEffectEntry const* effect);
        void EffectEnergize(SpellEffectEntry const* effect);
        void EffectOpenLock(SpellEffectEntry const* effect);
        void EffectSummonChangeItem(SpellEffectEntry const* effect);
        void EffectProficiency(SpellEffectEntry const* effect);
        void EffectApplyAreaAura(SpellEffectEntry const* effect);
        void EffectSummonType(SpellEffectEntry const* effect);
        void EffectLearnSpell(SpellEffectEntry const* effect);
        void EffectDispel(SpellEffectEntry const* effect);
        void EffectDualWield(SpellEffectEntry const* effect);
        void EffectPickPocket(SpellEffectEntry const* effect);
        void EffectAddFarsight(SpellEffectEntry const* effect);
        void EffectUntrainTalents(SpellEffectEntry const* effect);
        void EffectHealMechanical(SpellEffectEntry const* effect);
        void EffectJump(SpellEffectEntry const* effect);
        void EffectJumpDest(SpellEffectEntry const* effect);
        void EffectLeapBack(SpellEffectEntry const* effect);
        void EffectQuestClear(SpellEffectEntry const* effect);
        void EffectTeleUnitsFaceCaster(SpellEffectEntry const* effect);
        void EffectLearnSkill(SpellEffectEntry const* effect);
        void EffectAddHonor(SpellEffectEntry const* effect);
        void EffectTradeSkill(SpellEffectEntry const* effect);
        void EffectEnchantItemPerm(SpellEffectEntry const* effect);
        void EffectEnchantItemTmp(SpellEffectEntry const* effect);
        void EffectTameCreature(SpellEffectEntry const* effect);
        void EffectSummonPet(SpellEffectEntry const* effect);
        void EffectLearnPetSpell(SpellEffectEntry const* effect);
        void EffectWeaponDmg(SpellEffectEntry const* effect);
        void EffectForceCast(SpellEffectEntry const* effect);
        void EffectForceCastWithValue(SpellEffectEntry const* effect);
        void EffectTriggerSpell(SpellEffectEntry const* effect);
        void EffectTriggerMissileSpell(SpellEffectEntry const* effect);
        void EffectThreat(SpellEffectEntry const* effect);
        void EffectHealMaxHealth(SpellEffectEntry const* effect);
        void EffectInterruptCast(SpellEffectEntry const* effect);
        void EffectSummonObjectWild(SpellEffectEntry const* effect);
        void EffectScriptEffect(SpellEffectEntry const* effect);
        void EffectSanctuary(SpellEffectEntry const* effect);
        void EffectAddComboPoints(SpellEffectEntry const* effect);
        void EffectDuel(SpellEffectEntry const* effect);
        void EffectStuck(SpellEffectEntry const* effect);
        void EffectSummonPlayer(SpellEffectEntry const* effect);
        void EffectActivateObject(SpellEffectEntry const* effect);
        void EffectApplyGlyph(SpellEffectEntry const* effect);
        void EffectEnchantHeldItem(SpellEffectEntry const* effect);
        void EffectSummonObject(SpellEffectEntry const* effect);
        void EffectResurrect(SpellEffectEntry const* effect);
        void EffectParry(SpellEffectEntry const* effect);
        void EffectBlock(SpellEffectEntry const* effect);
        void EffectLeap(SpellEffectEntry const* effect);
        void EffectTransmitted(SpellEffectEntry const* effect);
        void EffectDisEnchant(SpellEffectEntry const* effect);
        void EffectInebriate(SpellEffectEntry const* effect);
        void EffectFeedPet(SpellEffectEntry const* effect);
        void EffectDismissPet(SpellEffectEntry const* effect);
        void EffectReputation(SpellEffectEntry const* effect);
        void EffectForceDeselect(SpellEffectEntry const* effect);
        void EffectSelfResurrect(SpellEffectEntry const* effect);
        void EffectSkinning(SpellEffectEntry const* effect);
        void EffectCharge(SpellEffectEntry const* effect);
        void EffectChargeDest(SpellEffectEntry const* effect);
        void EffectProspecting(SpellEffectEntry const* effect);
        void EffectMilling(SpellEffectEntry const* effect);
        void EffectRenamePet(SpellEffectEntry const* effect);
        void EffectSendTaxi(SpellEffectEntry const* effect);
        void EffectSummonCritter(SpellEffectEntry const* effect);
        void EffectKnockBack(SpellEffectEntry const* effect);
        void EffectPullTowards(SpellEffectEntry const* effect);
        void EffectDispelMechanic(SpellEffectEntry const* effect);
        void EffectSummonDeadPet(SpellEffectEntry const* effect);
        void EffectDestroyAllTotems(SpellEffectEntry const* effect);
        void EffectDurabilityDamage(SpellEffectEntry const* effect);
        void EffectSkill(SpellEffectEntry const* effect);
        void EffectTaunt(SpellEffectEntry const* effect);
        void EffectDurabilityDamagePCT(SpellEffectEntry const* effect);
        void EffectModifyThreatPercent(SpellEffectEntry const* effect);
        void EffectResurrectNew(SpellEffectEntry const* effect);
        void EffectAddExtraAttacks(SpellEffectEntry const* effect);
        void EffectSpiritHeal(SpellEffectEntry const* effect);
        void EffectSkinPlayerCorpse(SpellEffectEntry const* effect);
        void EffectStealBeneficialBuff(SpellEffectEntry const* effect);
        void EffectUnlearnSpecialization(SpellEffectEntry const* effect);
        void EffectHealPct(SpellEffectEntry const* effect);
        void EffectEnergizePct(SpellEffectEntry const* effect);
        void EffectTriggerSpellWithValue(SpellEffectEntry const* effect);
        void EffectTriggerRitualOfSummoning(SpellEffectEntry const* effect);
        void EffectKillCreditPersonal(SpellEffectEntry const* effect);
        void EffectKillCredit(SpellEffectEntry const* effect);
        void EffectQuestFail(SpellEffectEntry const* effect);
        void EffectQuestStart(SpellEffectEntry const* effect);
        void EffectRedirectThreat(SpellEffectEntry const* effect);
        void EffectWMODamage(SpellEffectEntry const* effect);
        void EffectWMORepair(SpellEffectEntry const* effect);
        void EffectWMOChange(SpellEffectEntry const* effect);
        void EffectActivateRune(SpellEffectEntry const* effect);
        void EffectCreateTamedPet(SpellEffectEntry const* effect);
        void EffectDiscoverTaxi(SpellEffectEntry const* effect);
        void EffectTitanGrip(SpellEffectEntry const* effect);
        void EffectEnchantItemPrismatic(SpellEffectEntry const* effect);
        void EffectPlayMusic(SpellEffectEntry const* effect);
        void EffectSpecCount(SpellEffectEntry const* effect);
        void EffectActivateSpec(SpellEffectEntry const* effect);
        void EffectPlayerNotification(SpellEffectEntry const* effect);
        void EffectRemoveAura(SpellEffectEntry const* effect);
        void EffectCastButtons(SpellEffectEntry const* effect);
        void EffectRechargeManaGem(SpellEffectEntry const* effect);

        typedef std::set<Aura *> UsedSpellMods;

        Spell(Unit* Caster, SpellEntry const *info, bool triggered, uint64 originalCasterGUID = 0, bool skipCheck = false);
        ~Spell();

        void prepare(SpellCastTargets const* targets, AuraEffect const * triggeredByAura = NULL);
        void cancel();
        void update(uint32 difftime);
        void cast(bool skipCheck = false);
        void finish(bool ok = true);
        void TakePower();

        void TakeRunePower(bool didHit);
        void TakeReagents();
        void TakeCastItem();

        SpellCastResult CheckCast(bool strict);
        SpellCastResult CheckPetCast(Unit* target);

        // handlers
        void handle_immediate();
        uint64 handle_delayed(uint64 t_offset);
        // handler helpers
        void _handle_immediate_phase();
        void _handle_finish_phase();

        SpellCastResult CheckItems();
        SpellCastResult CheckRange(bool strict);
        SpellCastResult CheckPower();
        SpellCastResult CheckRuneCost(uint32 runeCostID);
        SpellCastResult CheckCasterAuras() const;

        int32 CalculateDamage(uint8 i, Unit* target) { return m_caster->CalculateSpellDamage(target, m_spellInfo, i, &m_spellValue->EffectBasePoints[i]); }

        bool HaveTargetsForEffect(uint8 effect) const;
        void Delayed();
        void DelayedChannel();
        uint32 getState() const { return m_spellState; }
        void setState(uint32 state) { m_spellState = state; }

        void DoCreateItem(uint32 i, uint32 itemtype);
        void WriteSpellGoTargets(WorldPacket * data);

        void SelectSpellTargets();
        void SelectEffectTargets(uint32 i, uint32 cur);
        void SelectTrajTargets();

        template<typename T> WorldObject* FindCorpseUsing();

        bool CheckTarget(Unit* target, uint32 eff);
        bool CanAutoCast(Unit* target);
        void CheckSrc() { if (!m_targets.HasSrc()) m_targets.setSrc(*m_caster); }
        void CheckDst() { if (!m_targets.HasDst()) m_targets.setDst(*m_caster); }

        static void SendCastResult(Player* caster, SpellEntry const* spellInfo, uint8 cast_count, SpellCastResult result, SpellCustomErrors customError = SPELL_CUSTOM_ERROR_NONE);
        void SendCastResult(SpellCastResult result);
        void SendSpellStart();
        void SendSpellGo();
        void SendSpellCooldown();
        void SendLogExecute();
        void ExecuteLogEffectTakeTargetPower(uint8 effIndex, Unit * target, uint32 powerType, uint32 powerTaken, float gainMultiplier);
        void ExecuteLogEffectExtraAttacks(uint8 effIndex, Unit * victim, uint32 attCount);
        void ExecuteLogEffectInterruptCast(uint8 effIndex, Unit * victim, uint32 spellId);
        void ExecuteLogEffectDurabilityDamage(uint8 effIndex, Unit * victim, uint32 itemslot, uint32 damage);
        void ExecuteLogEffectOpenLock(uint8 effIndex, Object * obj);
        void ExecuteLogEffectCreateItem(uint8 effIndex, uint32 entry);
        void ExecuteLogEffectDestroyItem(uint8 effIndex, uint32 entry);
        void ExecuteLogEffectSummonObject(uint8 effIndex, WorldObject * obj);
        void ExecuteLogEffectUnsummonObject(uint8 effIndex, WorldObject * obj);
        void ExecuteLogEffectResurrect(uint8 effIndex, Unit * target);
        void SendInterrupted(uint8 result);
        void SendChannelUpdate(uint32 time);
        void SendChannelStart(uint32 duration);
        void SendResurrectRequest(Player* target);
        void SendPlaySpellVisual(uint32 SpellID);

        void HandleEffects(Unit *pUnitTarget, Item *pItemTarget, GameObject *pGOTarget, uint32 i);
        void HandleThreatSpells(uint32 spellId);

        const SpellEntry * const m_spellInfo;
        SpellInterruptsEntry const* m_spellInterrupts;
        Item* m_CastItem;
        uint64 m_castItemGUID;
        uint8 m_cast_count;
        uint32 m_glyphIndex;
        uint32 m_preCastSpell;
        SpellCastTargets m_targets;
        int8 m_comboPointGain;
        SpellCustomErrors m_customError;

        UsedSpellMods m_appliedMods;

        int32 GetCastTime() const { return m_casttime; }
        bool IsAutoRepeat() const { return m_autoRepeat; }
        void SetAutoRepeat(bool rep) { m_autoRepeat = rep; }
        void ReSetTimer() { m_timer = m_casttime > 0 ? m_casttime : 0; }
        bool IsNextMeleeSwingSpell() const
        {
            return m_spellInfo->Attributes & SPELL_ATTR0_ON_NEXT_SWING;
        }
        bool IsTriggered() const {return m_IsTriggeredSpell;};
        bool IsChannelActive() const { return m_caster->GetUInt32Value(UNIT_CHANNEL_SPELL) != 0; }
        bool IsAutoActionResetSpell() const { return !m_IsTriggeredSpell && (m_spellInfo->GetInterruptFlags() & SPELL_INTERRUPT_FLAG_AUTOATTACK);  }

        bool IsDeletable() const { return !m_referencedFromCurrentSpell && !m_executedCurrently; }
        void SetReferencedFromCurrent(bool yes) { m_referencedFromCurrentSpell = yes; }
        bool IsInterruptable() const { return !m_executedCurrently; }
        void SetExecutedCurrently(bool yes) {m_executedCurrently = yes;}
        uint64 GetDelayStart() const { return m_delayStart; }
        void SetDelayStart(uint64 m_time) { m_delayStart = m_time; }
        uint64 GetDelayMoment() const { return m_delayMoment; }

        bool IsNeedSendToClient() const;

        CurrentSpellTypes GetCurrentContainer();

        Unit* GetCaster() const { return m_caster; }
        Unit* GetOriginalCaster() const { return m_originalCaster; }
        SpellEntry const * GetSpellInfo() const { return m_spellInfo; }
        int32 GetPowerCost() const { return m_powerCost; }

        void UpdatePointers();                              // must be used at call Spell code after time delay (non triggered spell cast/update spell call/etc)

        bool CheckTargetCreatureType(Unit* target) const;

        void CleanupTargetList();

        void SetSpellValue(SpellValueMod mod, int32 value);
    protected:
        bool HasGlobalCooldown();
        void TriggerGlobalCooldown();
        void CancelGlobalCooldown();

        void SendLoot(uint64 guid, LootType loottype);

        Unit* const m_caster;

        SpellValue * const m_spellValue;

        uint64 m_originalCasterGUID;                        // real source of cast (aura caster/etc), used for spell targets selection
                                                            // e.g. damage around area spell trigered by victim aura and damage enemies of aura caster
        Unit* m_originalCaster;                             // cached pointer for m_originalCaster, updated at Spell::UpdatePointers()

        Spell** m_selfContainer;                            // pointer to our spell container (if applicable)

        //Spell data
        SpellSchoolMask m_spellSchoolMask;                  // Spell school (can be overwrite for some spells (wand shoot for example)
        WeaponAttackType m_attackType;                      // For weapon based attack
        int32 m_powerCost;                                  // Calculated spell cost     initialized only in Spell::prepare
        int32 m_casttime;                                   // Calculated spell cast time initialized only in Spell::prepare
        bool m_canReflect;                                  // can reflect this spell?
        bool m_autoRepeat;
        uint8 m_runesState;

        uint8 m_delayAtDamageCount;
        bool isDelayableNoMore()
        {
            if (m_delayAtDamageCount >= 2)
                return true;

            m_delayAtDamageCount++;
            return false;
        }

        // Delayed spells system
        uint64 m_delayStart;                                // time of spell delay start, filled by event handler, zero = just started
        uint64 m_delayMoment;                               // moment of next delay call, used internally
        bool m_immediateHandled;                            // were immediate actions handled? (used by delayed spells only)

        // These vars are used in both delayed spell system and modified immediate spell system
        bool m_referencedFromCurrentSpell;                  // mark as references to prevent deleted and access by dead pointers
        bool m_executedCurrently;                           // mark as executed to prevent deleted and access by dead pointers
        bool m_needComboPoints;
        uint8 m_applyMultiplierMask;
        float m_damageMultipliers[3];

        // Current targets, to be used in SpellEffects (MUST BE USED ONLY IN SPELL EFFECTS)
        Unit* unitTarget;
        Item* itemTarget;
        GameObject* gameObjTarget;
        int32 damage;
        // used in effects handlers
        Aura * m_spellAura;

        // this is set in Spell Hit, but used in Apply Aura handler
        DiminishingLevels m_diminishLevel;
        DiminishingGroup m_diminishGroup;

        // -------------------------------------------
        GameObject* focusObject;

        // Damage and healing in effects need just calculate
        int32 m_damage;           // Damge   in effects count here
        int32 m_healing;          // Healing in effects count here

        // ******************************************
        // Spell trigger system
        // ******************************************
        uint32 m_procAttacker;                // Attacker trigger flags
        uint32 m_procVictim;                  // Victim   trigger flags
        uint32 m_procEx;
        void   prepareDataForTriggerSystem(AuraEffect const * triggeredByAura);

        // *****************************************
        // Spell target subsystem
        // *****************************************
        // Targets store structures and data
        struct TargetInfo
        {
            uint64 targetGUID;
            uint64 timeDelay;
            SpellMissInfo missCondition:8;
            SpellMissInfo reflectResult:8;
            uint8  effectMask:8;
            bool   processed:1;
            bool   alive:1;
            bool   crit:1;
            bool   scaleAura:1;
            int32  damage;
        };
        std::list<TargetInfo> m_UniqueTargetInfo;
        uint8 m_channelTargetEffectMask;                        // Mask req. alive targets

        struct GOTargetInfo
        {
            uint64 targetGUID;
            uint64 timeDelay;
            uint8  effectMask:8;
            bool   processed:1;
        };
        std::list<GOTargetInfo> m_UniqueGOTargetInfo;

        struct ItemTargetInfo
        {
            Item  *item;
            uint8 effectMask;
        };
        std::list<ItemTargetInfo> m_UniqueItemInfo;

        void AddUnitTarget(Unit* target, uint32 effIndex);
        void AddUnitTarget(uint64 unitGUID, uint32 effIndex);
        void AddGOTarget(GameObject* target, uint32 effIndex);
        void AddGOTarget(uint64 goGUID, uint32 effIndex);
        void AddItemTarget(Item* target, uint32 effIndex);
        void DoAllEffectOnTarget(TargetInfo *target);
        SpellMissInfo DoSpellHitOnUnit(Unit *unit, uint32 effectMask, bool scaleAura);
        void DoTriggersOnSpellHit(Unit *unit);
        void DoAllEffectOnTarget(GOTargetInfo *target);
        void DoAllEffectOnTarget(ItemTargetInfo *target);
        bool UpdateChanneledTargetList();
        void SearchAreaTarget(std::list<Unit*> &unitList, float radius, SpellNotifyPushType type, SpellTargets TargetType, uint32 entry = 0);
        void SearchGOAreaTarget(std::list<GameObject*> &gobjectList, float radius, SpellNotifyPushType type, SpellTargets TargetType, uint32 entry = 0);
        void SearchChainTarget(std::list<Unit*> &unitList, float radius, uint32 unMaxTargets, SpellTargets TargetType);
        WorldObject* SearchNearbyTarget(float range, SpellTargets TargetType, SpellEffectEntry const* effect);
        bool IsValidSingleTargetEffect(Unit const* target, Targets type) const;
        bool IsValidSingleTargetSpell(Unit const* target) const;
        bool IsValidDeadOrAliveTarget(Unit const* target) const;
        void CalculateDamageDoneForAllTargets();
        int32 CalculateDamageDone(Unit *unit, const uint32 effectMask, float *multiplier);
        void SpellDamageSchoolDmg(SpellEffectEntry const* effect);
        void SpellDamageWeaponDmg(SpellEffectEntry const* effect);
        void SpellDamageHeal(SpellEffectEntry const* effect);

        void PrepareTargetProcessing();
        void FinishTargetProcessing();

        // spell execution log
        void InitEffectExecuteData(uint8 effIndex);
        void CleanupEffectExecuteData();
        void CheckEffectExecuteData();

        // Scripting system
        void LoadScripts();
        SpellCastResult CallScriptCheckCastHandlers();
        void PrepareScriptHitHandlers();
        bool CallScriptEffectHandlers(SpellEffIndex effIndex);
        void CallScriptBeforeHitHandlers();
        void CallScriptOnHitHandlers();
        void CallScriptAfterHitHandlers();
        void CallScriptAfterUnitTargetSelectHandlers(std::list<Unit*>& unitTargets, SpellEffectEntry const* effect);
        std::list<SpellScript *> m_loadedScripts;

        // effect helpers
        void GetSummonPosition(uint32 i, Position &pos, float radius = 0.0f, uint32 count = 0);
        void SummonGuardian(uint32 i, uint32 entry, SummonPropertiesEntry const *properties);
        void CalculateJumpSpeeds(uint8 i, float dist, float & speedxy, float & speedz);

        SpellCastResult CanOpenLock(uint32 effIndex, uint32 lockid, SkillType& skillid, int32& reqSkillValue, int32& skillValue);
        // -------------------------------------------

        //List For Triggered Spells
        typedef std::vector< std::pair<SpellEntry const*, int32> > ChanceTriggerSpells;
        ChanceTriggerSpells m_ChanceTriggerSpells;

        uint32 m_spellState;
        uint32 m_timer;

        bool m_IsTriggeredSpell;

        // if need this can be replaced by Aura copy
        // we can't store original aura link to prevent access to deleted auras
        // and in same time need aura data and after aura deleting.
        SpellEntry const* m_triggeredByAuraSpell;

        uint32 m_customAttr;
        bool m_skipCheck;
        uint32 m_effectMask;
        uint8 m_auraScaleMask;

        ByteBuffer * m_effectExecuteData[MAX_SPELL_EFFECTS];

#ifdef MAP_BASED_RAND_GEN
        int32 irand(int32 min, int32 max)       { return int32 (m_caster->GetMap()->mtRand.randInt(max - min)) + min; }
        uint32 urand(uint32 min, uint32 max)    { return m_caster->GetMap()->mtRand.randInt(max - min) + min; }
        int32 rand32()                          { return m_caster->GetMap()->mtRand.randInt(); }
        double rand_norm()                      { return m_caster->GetMap()->mtRand.randExc(); }
        double rand_chance()                    { return m_caster->GetMap()->mtRand.randExc(100.0); }
#endif
};

namespace Trinity
{
    struct SpellNotifierCreatureAndPlayer
    {
        std::list<Unit*> *i_data;
        SpellNotifyPushType i_push_type;
        float i_radius;
        SpellTargets i_TargetType;
        const Unit * const i_source;
        uint32 i_entry;
        const Position * const i_pos;
        bool i_requireDeadTarget;
        SpellEntry const * i_spellProto;

        SpellNotifierCreatureAndPlayer(Unit *source, std::list<Unit*> &data, float radius, SpellNotifyPushType type,
            SpellTargets TargetType = SPELL_TARGETS_ENEMY, const Position *pos = NULL, uint32 entry = 0, SpellEntry const * spellProto = NULL)
            : i_data(&data), i_push_type(type), i_radius(radius), i_TargetType(TargetType),
            i_source(source), i_entry(entry), i_pos(pos), i_spellProto(spellProto)
        {
            ASSERT(i_source);
        }

        template<class T> inline void Visit(GridRefManager<T>& m)
        {
            i_requireDeadTarget = i_spellProto ? bool(i_spellProto->AttributesEx3 & SPELL_ATTR3_REQUIRE_DEAD_TARGET) : false;

            for (typename GridRefManager<T>::iterator itr = m.begin(); itr != m.end(); ++itr)
            {
                Unit *target = (Unit*)itr->getSource();

                if (!i_source->canSeeOrDetect(target, true))
                    continue;

                switch (i_TargetType)
                {
                    case SPELL_TARGETS_ENEMY:
                        if (target->isTotem())
                            continue;
                        if (!target->isAttackableByAOE(i_spellProto))
                            continue;
                        if (i_source->IsControlledByPlayer())
                        {
                            if (i_source->IsFriendlyTo(target))
                                continue;
                        }
                        else
                        {
                            if (!i_source->IsHostileTo(target))
                                continue;
                        }
                        break;
                    case SPELL_TARGETS_ALLY:
                        if (target->isTotem())
                            continue;
                        if (!i_source->IsFriendlyTo(target))
                            continue;
                        if (target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                            continue;
                        if (target->GetTypeId() == TYPEID_PLAYER && target->ToPlayer()->isGameMaster())
                            continue;
                        if (target->isAlive() == i_requireDeadTarget)
                            continue;
                        break;
                    case SPELL_TARGETS_ENTRY:
                        if (target->GetEntry()!= i_entry)
                            continue;
                        break;
                    case SPELL_TARGETS_ANY:
                    default:
                        break;
                }

                switch(i_push_type)
                {
                    case PUSH_SRC_CENTER:
                    case PUSH_DST_CENTER:
                    case PUSH_CHAIN:
                    default:
                        if (target->IsWithinDist3d(i_pos, i_radius))
                            i_data->push_back(target);
                        break;
                    case PUSH_IN_FRONT:
                        if (i_source->isInFront(target, i_radius, static_cast<float>(M_PI/2)))
                            i_data->push_back(target);
                        break;
                    case PUSH_IN_BACK:
                        if (i_source->isInBack(target, i_radius, static_cast<float>(M_PI/2)))
                            i_data->push_back(target);
                        break;
                    case PUSH_IN_LINE:
                        if (i_source->HasInLine(target, i_radius, i_source->GetObjectSize()))
                            i_data->push_back(target);
                        break;
                    case PUSH_IN_THIN_LINE: // only traj
                        if (i_pos->HasInLine(target, i_radius, 0))
                            i_data->push_back(target);
                        break;
                }
            }
        }

        #ifdef _WIN32
        template<> inline void Visit(CorpseMapType &) {}
        template<> inline void Visit(GameObjectMapType &) {}
        template<> inline void Visit(DynamicObjectMapType &) {}
        #endif
    };

    #ifndef _WIN32
    template<> inline void SpellNotifierCreatureAndPlayer::Visit(CorpseMapType&) {}
    template<> inline void SpellNotifierCreatureAndPlayer::Visit(GameObjectMapType&) {}
    template<> inline void SpellNotifierCreatureAndPlayer::Visit(DynamicObjectMapType&) {}
    #endif
}

typedef void(Spell::*pEffect)(SpellEffectEntry const* spellEffect);

class SpellEvent : public BasicEvent
{
    public:
        SpellEvent(Spell* spell);
        virtual ~SpellEvent();

        virtual bool Execute(uint64 e_time, uint32 p_time);
        virtual void Abort(uint64 e_time);
        virtual bool IsDeletable() const;
    protected:
        Spell* m_Spell;
};
#endif
