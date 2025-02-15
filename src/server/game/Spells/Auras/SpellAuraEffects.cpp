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

#include "Common.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "ObjectAccessor.h"
#include "Util.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "Battleground.h"
#include "OutdoorPvPMgr.h"
#include "Formulas.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ScriptMgr.h"

class Aura;
//
// EFFECT HANDLER NOTES
//
// in aura handler there should be check for modes:
// AURA_EFFECT_HANDLE_REAL set -  aura mod is just applied/removed on the target
// AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK set - aura is just applied/removed, or aura packet request is made
// AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK set - aura is recalculated or is just applied/removed - need to redo all things related to m_amount
// AURA_EFFECT_HANDLE_CHANGE_AMOUNT_SEND_FOR_CLIENT_MASK - logical or of above conditions
// AURA_EFFECT_HANDLE_STAT - set when stats are reapplied
// such checks will speedup trinity change amount/send for client operations
// because for change amount operation packets will not be send
// aura effect handlers shouldn't contain any AuraEffect or Aura object modifications

pAuraEffectHandler AuraEffectHandler[TOTAL_AURAS]=
{
    &AuraEffect::HandleNULL,                                      //  0 SPELL_AURA_NONE
    &AuraEffect::HandleBindSight,                                 //  1 SPELL_AURA_BIND_SIGHT
    &AuraEffect::HandleModPossess,                                //  2 SPELL_AURA_MOD_POSSESS
    &AuraEffect::HandleNoImmediateEffect,                         //  3 SPELL_AURA_PERIODIC_DAMAGE implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleAuraDummy,                                 //  4 SPELL_AURA_DUMMY
    &AuraEffect::HandleModConfuse,                                //  5 SPELL_AURA_MOD_CONFUSE
    &AuraEffect::HandleModCharm,                                  //  6 SPELL_AURA_MOD_CHARM
    &AuraEffect::HandleModFear,                                   //  7 SPELL_AURA_MOD_FEAR
    &AuraEffect::HandleNoImmediateEffect,                         //  8 SPELL_AURA_PERIODIC_HEAL implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleModAttackSpeed,                            //  9 SPELL_AURA_MOD_ATTACKSPEED
    &AuraEffect::HandleModThreat,                                 // 10 SPELL_AURA_MOD_THREAT
    &AuraEffect::HandleModTaunt,                                  // 11 SPELL_AURA_MOD_TAUNT
    &AuraEffect::HandleAuraModStun,                               // 12 SPELL_AURA_MOD_STUN
    &AuraEffect::HandleModDamageDone,                             // 13 SPELL_AURA_MOD_DAMAGE_DONE
    &AuraEffect::HandleNoImmediateEffect,                         // 14 SPELL_AURA_MOD_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonus and Unit::SpellDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         // 15 SPELL_AURA_DAMAGE_SHIELD    implemented in Unit::DoAttackDamage
    &AuraEffect::HandleModStealth,                                // 16 SPELL_AURA_MOD_STEALTH
    &AuraEffect::HandleModStealthDetect,                          // 17 SPELL_AURA_MOD_DETECT
    &AuraEffect::HandleModInvisibility,                           // 18 SPELL_AURA_MOD_INVISIBILITY
    &AuraEffect::HandleModInvisibilityDetect,                     // 19 SPELL_AURA_MOD_INVISIBILITY_DETECTION
    &AuraEffect::HandleNoImmediateEffect,                         // 20 SPELL_AURA_OBS_MOD_HEALTH implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNoImmediateEffect,                         // 21 SPELL_AURA_OBS_MOD_POWER implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleAuraModResistance,                         // 22 SPELL_AURA_MOD_RESISTANCE
    &AuraEffect::HandleNoImmediateEffect,                         // 23 SPELL_AURA_PERIODIC_TRIGGER_SPELL implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNoImmediateEffect,                         // 24 SPELL_AURA_PERIODIC_ENERGIZE implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleAuraModPacify,                             // 25 SPELL_AURA_MOD_PACIFY
    &AuraEffect::HandleAuraModRoot,                               // 26 SPELL_AURA_MOD_ROOT
    &AuraEffect::HandleAuraModSilence,                            // 27 SPELL_AURA_MOD_SILENCE
    &AuraEffect::HandleNoImmediateEffect,                         // 28 SPELL_AURA_REFLECT_SPELLS        implement in Unit::SpellHitResult
    &AuraEffect::HandleAuraModStat,                               // 29 SPELL_AURA_MOD_STAT
    &AuraEffect::HandleAuraModSkill,                              // 30 SPELL_AURA_MOD_SKILL
    &AuraEffect::HandleAuraModIncreaseSpeed,                      // 31 SPELL_AURA_MOD_INCREASE_SPEED
    &AuraEffect::HandleAuraModIncreaseMountedSpeed,               // 32 SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED
    &AuraEffect::HandleAuraModDecreaseSpeed,                      // 33 SPELL_AURA_MOD_DECREASE_SPEED
    &AuraEffect::HandleAuraModIncreaseHealth,                     // 34 SPELL_AURA_MOD_INCREASE_HEALTH
    &AuraEffect::HandleAuraModIncreaseEnergy,                     // 35 SPELL_AURA_MOD_INCREASE_ENERGY
    &AuraEffect::HandleAuraModShapeshift,                         // 36 SPELL_AURA_MOD_SHAPESHIFT
    &AuraEffect::HandleAuraModEffectImmunity,                     // 37 SPELL_AURA_EFFECT_IMMUNITY
    &AuraEffect::HandleAuraModStateImmunity,                      // 38 SPELL_AURA_STATE_IMMUNITY
    &AuraEffect::HandleAuraModSchoolImmunity,                     // 39 SPELL_AURA_SCHOOL_IMMUNITY
    &AuraEffect::HandleAuraModDmgImmunity,                        // 40 SPELL_AURA_DAMAGE_IMMUNITY
    &AuraEffect::HandleAuraModDispelImmunity,                     // 41 SPELL_AURA_DISPEL_IMMUNITY
    &AuraEffect::HandleNoImmediateEffect,                         // 42 SPELL_AURA_PROC_TRIGGER_SPELL  implemented in Unit::ProcDamageAndSpellFor and Unit::HandleProcTriggerSpell
    &AuraEffect::HandleNoImmediateEffect,                         // 43 SPELL_AURA_PROC_TRIGGER_DAMAGE implemented in Unit::ProcDamageAndSpellFor
    &AuraEffect::HandleAuraTrackCreatures,                        // 44 SPELL_AURA_TRACK_CREATURES
    &AuraEffect::HandleAuraTrackResources,                        // 45 SPELL_AURA_TRACK_RESOURCES
    &AuraEffect::HandleNULL,                                      // 46 SPELL_AURA_46 (used in test spells 54054 and 54058, and spell 48050) (3.0.8a)
    &AuraEffect::HandleAuraModParryPercent,                       // 47 SPELL_AURA_MOD_PARRY_PERCENT
    &AuraEffect::HandleNULL,                                      // 48 SPELL_AURA_48 spell Napalm (area damage spell with additional delayed damage effect)
    &AuraEffect::HandleAuraModDodgePercent,                       // 49 SPELL_AURA_MOD_DODGE_PERCENT
    &AuraEffect::HandleNoImmediateEffect,                         // 50 SPELL_AURA_MOD_CRITICAL_HEALING_AMOUNT implemented in Unit::SpellCriticalHealingBonus
    &AuraEffect::HandleAuraModBlockPercent,                       // 51 SPELL_AURA_MOD_BLOCK_PERCENT
    &AuraEffect::HandleAuraModWeaponCritPercent,                  // 52 SPELL_AURA_MOD_WEAPON_CRIT_PERCENT
    &AuraEffect::HandleNoImmediateEffect,                         // 53 SPELL_AURA_PERIODIC_LEECH implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleModHitChance,                              // 54 SPELL_AURA_MOD_HIT_CHANCE
    &AuraEffect::HandleModSpellHitChance,                         // 55 SPELL_AURA_MOD_SPELL_HIT_CHANCE
    &AuraEffect::HandleAuraTransform,                             // 56 SPELL_AURA_TRANSFORM
    &AuraEffect::HandleModSpellCritChance,                        // 57 SPELL_AURA_MOD_SPELL_CRIT_CHANCE
    &AuraEffect::HandleAuraModIncreaseSwimSpeed,                  // 58 SPELL_AURA_MOD_INCREASE_SWIM_SPEED
    &AuraEffect::HandleNoImmediateEffect,                         // 59 SPELL_AURA_MOD_DAMAGE_DONE_CREATURE implemented in Unit::MeleeDamageBonus and Unit::SpellDamageBonus
    &AuraEffect::HandleAuraModPacifyAndSilence,                   // 60 SPELL_AURA_MOD_PACIFY_SILENCE
    &AuraEffect::HandleAuraModScale,                              // 61 SPELL_AURA_MOD_SCALE
    &AuraEffect::HandleNoImmediateEffect,                         // 62 SPELL_AURA_PERIODIC_HEALTH_FUNNEL implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNULL,                                      // 63 unused (3.2.0) old SPELL_AURA_PERIODIC_MANA_FUNNEL
    &AuraEffect::HandleNoImmediateEffect,                         // 64 SPELL_AURA_PERIODIC_MANA_LEECH implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleModCastingSpeed,                           // 65 SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK
    &AuraEffect::HandleFeignDeath,                                // 66 SPELL_AURA_FEIGN_DEATH
    &AuraEffect::HandleAuraModDisarm,                             // 67 SPELL_AURA_MOD_DISARM
    &AuraEffect::HandleAuraModStalked,                            // 68 SPELL_AURA_MOD_STALKED
    &AuraEffect::HandleNoImmediateEffect,                         // 69 SPELL_AURA_SCHOOL_ABSORB implemented in Unit::CalcAbsorbResist
    &AuraEffect::HandleUnused,                                    // 70 SPELL_AURA_EXTRA_ATTACKS clientside
    &AuraEffect::HandleModSpellCritChanceShool,                   // 71 SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL
    &AuraEffect::HandleModPowerCostPCT,                           // 72 SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT
    &AuraEffect::HandleModPowerCost,                              // 73 SPELL_AURA_MOD_POWER_COST_SCHOOL
    &AuraEffect::HandleNoImmediateEffect,                         // 74 SPELL_AURA_REFLECT_SPELLS_SCHOOL  implemented in Unit::SpellHitResult
    &AuraEffect::HandleNoImmediateEffect,                         // 75 SPELL_AURA_MOD_LANGUAGE
    &AuraEffect::HandleNoImmediateEffect,                         // 76 SPELL_AURA_FAR_SIGHT
    &AuraEffect::HandleModMechanicImmunity,                       // 77 SPELL_AURA_MECHANIC_IMMUNITY
    &AuraEffect::HandleAuraMounted,                               // 78 SPELL_AURA_MOUNTED
    &AuraEffect::HandleModDamagePercentDone,                      // 79 SPELL_AURA_MOD_DAMAGE_PERCENT_DONE
    &AuraEffect::HandleModPercentStat,                            // 80 SPELL_AURA_MOD_PERCENT_STAT
    &AuraEffect::HandleNoImmediateEffect,                         // 81 SPELL_AURA_SPLIT_DAMAGE_PCT implemented in Unit::CalcAbsorbResist
    &AuraEffect::HandleWaterBreathing,                            // 82 SPELL_AURA_WATER_BREATHING
    &AuraEffect::HandleModBaseResistance,                         // 83 SPELL_AURA_MOD_BASE_RESISTANCE
    &AuraEffect::HandleNoImmediateEffect,                         // 84 SPELL_AURA_MOD_REGEN implemented in Player::RegenerateHealth
    &AuraEffect::HandleModPowerRegen,                             // 85 SPELL_AURA_MOD_POWER_REGEN implemented in Player::Regenerate
    &AuraEffect::HandleChannelDeathItem,                          // 86 SPELL_AURA_CHANNEL_DEATH_ITEM
    &AuraEffect::HandleNoImmediateEffect,                         // 87 SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN implemented in Unit::MeleeDamageBonus and Unit::SpellDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         // 88 SPELL_AURA_MOD_HEALTH_REGEN_PERCENT implemented in Player::RegenerateHealth
    &AuraEffect::HandleNoImmediateEffect,                         // 89 SPELL_AURA_PERIODIC_DAMAGE_PERCENT
    &AuraEffect::HandleNULL,                                      // 90 unused (3.0.8a) old SPELL_AURA_MOD_RESIST_CHANCE
    &AuraEffect::HandleNoImmediateEffect,                         // 91 SPELL_AURA_MOD_DETECT_RANGE implemented in Creature::GetAttackDistance
    &AuraEffect::HandlePreventFleeing,                            // 92 SPELL_AURA_PREVENTS_FLEEING
    &AuraEffect::HandleModUnattackable,                           // 93 SPELL_AURA_MOD_UNATTACKABLE
    &AuraEffect::HandleNoImmediateEffect,                         // 94 SPELL_AURA_INTERRUPT_REGEN implemented in Player::Regenerate
    &AuraEffect::HandleAuraGhost,                                 // 95 SPELL_AURA_GHOST
    &AuraEffect::HandleNoImmediateEffect,                         // 96 SPELL_AURA_SPELL_MAGNET implemented in Unit::SelectMagnetTarget
    &AuraEffect::HandleNoImmediateEffect,                         // 97 SPELL_AURA_MANA_SHIELD implemented in Unit::CalcAbsorbResist
    &AuraEffect::HandleAuraModSkill,                              // 98 SPELL_AURA_MOD_SKILL_TALENT
    &AuraEffect::HandleAuraModAttackPower,                        // 99 SPELL_AURA_MOD_ATTACK_POWER
    &AuraEffect::HandleUnused,                                    //100 SPELL_AURA_AURAS_VISIBLE obsolete? all player can see all auras now, but still have spells including GM-spell
    &AuraEffect::HandleModResistancePercent,                      //101 SPELL_AURA_MOD_RESISTANCE_PCT
    &AuraEffect::HandleNoImmediateEffect,                         //102 SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleAuraModTotalThreat,                        //103 SPELL_AURA_MOD_TOTAL_THREAT
    &AuraEffect::HandleAuraWaterWalk,                             //104 SPELL_AURA_WATER_WALK
    &AuraEffect::HandleAuraFeatherFall,                           //105 SPELL_AURA_FEATHER_FALL
    &AuraEffect::HandleAuraHover,                                 //106 SPELL_AURA_HOVER
    &AuraEffect::HandleNoImmediateEffect,                         //107 SPELL_AURA_ADD_FLAT_MODIFIER implemented in AuraEffect::CalculateSpellMod()
    &AuraEffect::HandleNoImmediateEffect,                         //108 SPELL_AURA_ADD_PCT_MODIFIER implemented in AuraEffect::CalculateSpellMod()
    &AuraEffect::HandleNoImmediateEffect,                         //109 SPELL_AURA_ADD_TARGET_TRIGGER
    &AuraEffect::HandleModPowerRegenPCT,                          //110 SPELL_AURA_MOD_POWER_REGEN_PERCENT implemented in Player::Regenerate, Creature::Regenerate
    &AuraEffect::HandleNoImmediateEffect,                         //111 SPELL_AURA_ADD_CASTER_HIT_TRIGGER implemented in Unit::SelectMagnetTarget
    &AuraEffect::HandleNoImmediateEffect,                         //112 SPELL_AURA_OVERRIDE_CLASS_SCRIPTS
    &AuraEffect::HandleNoImmediateEffect,                         //113 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         //114 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         //115 SPELL_AURA_MOD_HEALING                 implemented in Unit::SpellBaseHealingBonusForVictim
    &AuraEffect::HandleNoImmediateEffect,                         //116 SPELL_AURA_MOD_REGEN_DURING_COMBAT
    &AuraEffect::HandleNoImmediateEffect,                         //117 SPELL_AURA_MOD_MECHANIC_RESISTANCE     implemented in Unit::MagicSpellHitResult
    &AuraEffect::HandleNoImmediateEffect,                         //118 SPELL_AURA_MOD_HEALING_PCT             implemented in Unit::SpellHealingBonus
    &AuraEffect::HandleNULL,                                      //119 unused (3.2.0) old SPELL_AURA_SHARE_PET_TRACKING
    &AuraEffect::HandleAuraUntrackable,                           //120 SPELL_AURA_UNTRACKABLE
    &AuraEffect::HandleAuraEmpathy,                               //121 SPELL_AURA_EMPATHY
    &AuraEffect::HandleModOffhandDamagePercent,                   //122 SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT
    &AuraEffect::HandleModTargetResistance,                       //123 SPELL_AURA_MOD_TARGET_RESISTANCE
    &AuraEffect::HandleAuraModRangedAttackPower,                  //124 SPELL_AURA_MOD_RANGED_ATTACK_POWER
    &AuraEffect::HandleNoImmediateEffect,                         //125 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         //126 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         //127 SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleModPossessPet,                             //128 SPELL_AURA_MOD_POSSESS_PET
    &AuraEffect::HandleAuraModIncreaseSpeed,                      //129 SPELL_AURA_MOD_SPEED_ALWAYS
    &AuraEffect::HandleAuraModIncreaseMountedSpeed,               //130 SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS
    &AuraEffect::HandleNoImmediateEffect,                         //131 SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleAuraModIncreaseEnergyPercent,              //132 SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT
    &AuraEffect::HandleAuraModIncreaseHealthPercent,              //133 SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT
    &AuraEffect::HandleAuraModRegenInterrupt,                     //134 SPELL_AURA_MOD_MANA_REGEN_INTERRUPT
    &AuraEffect::HandleModHealingDone,                            //135 SPELL_AURA_MOD_HEALING_DONE
    &AuraEffect::HandleNoImmediateEffect,                         //136 SPELL_AURA_MOD_HEALING_DONE_PERCENT   implemented in Unit::SpellHealingBonus
    &AuraEffect::HandleModTotalPercentStat,                       //137 SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE
    &AuraEffect::HandleModMeleeSpeedPct,                          //138 SPELL_AURA_MOD_MELEE_HASTE
    &AuraEffect::HandleForceReaction,                             //139 SPELL_AURA_FORCE_REACTION
    &AuraEffect::HandleAuraModRangedHaste,                        //140 SPELL_AURA_MOD_RANGED_HASTE
    &AuraEffect::HandleNULL,                                      //141 SPELL_AURA_MOD_RANGED_AMMO_HASTE
    &AuraEffect::HandleAuraModBaseResistancePCT,                  //142 SPELL_AURA_MOD_BASE_RESISTANCE_PCT
    &AuraEffect::HandleAuraModResistanceExclusive,                //143 SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE
    &AuraEffect::HandleNoImmediateEffect,                         //144 SPELL_AURA_SAFE_FALL                         implemented in WorldSession::HandleMovementOpcodes
    &AuraEffect::HandleAuraModPetTalentsPoints,                   //145 SPELL_AURA_MOD_PET_TALENT_POINTS
    &AuraEffect::HandleNoImmediateEffect,                         //146 SPELL_AURA_ALLOW_TAME_PET_TYPE
    &AuraEffect::HandleModStateImmunityMask,                      //147 SPELL_AURA_MECHANIC_IMMUNITY_MASK
    &AuraEffect::HandleAuraRetainComboPoints,                     //148 SPELL_AURA_RETAIN_COMBO_POINTS
    &AuraEffect::HandleNoImmediateEffect,                         //149 SPELL_AURA_REDUCE_PUSHBACK
    &AuraEffect::HandleShieldBlockValue,                          //150 SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT
    &AuraEffect::HandleAuraTrackStealthed,                        //151 SPELL_AURA_TRACK_STEALTHED
    &AuraEffect::HandleNoImmediateEffect,                         //152 SPELL_AURA_MOD_DETECTED_RANGE implemented in Creature::GetAttackDistance
    &AuraEffect::HandleNoImmediateEffect,                         //153 SPELL_AURA_SPLIT_DAMAGE_FLAT
    &AuraEffect::HandleModStealthLevel,                           //154 SPELL_AURA_MOD_STEALTH_LEVEL
    &AuraEffect::HandleNoImmediateEffect,                         //155 SPELL_AURA_MOD_WATER_BREATHING
    &AuraEffect::HandleNoImmediateEffect,                         //156 SPELL_AURA_MOD_REPUTATION_GAIN
    &AuraEffect::HandleNULL,                                      //157 SPELL_AURA_PET_DAMAGE_MULTI
    &AuraEffect::HandleShieldBlockValue,                          //158 SPELL_AURA_MOD_SHIELD_BLOCKVALUE
    &AuraEffect::HandleNoImmediateEffect,                         //159 SPELL_AURA_NO_PVP_CREDIT      only for Honorless Target spell
    &AuraEffect::HandleNoImmediateEffect,                         //160 SPELL_AURA_MOD_AOE_AVOIDANCE                 implemented in Unit::MagicSpellHitResult
    &AuraEffect::HandleNoImmediateEffect,                         //161 SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT
    &AuraEffect::HandleNoImmediateEffect,                         //162 SPELL_AURA_POWER_BURN_MANA implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNoImmediateEffect,                         //163 SPELL_AURA_MOD_CRIT_DAMAGE_BONUS_MELEE
    &AuraEffect::HandleUnused,                                    //164 unused (3.2.0), only one test spell
    &AuraEffect::HandleNoImmediateEffect,                         //165 SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleAuraModAttackPowerPercent,                 //166 SPELL_AURA_MOD_ATTACK_POWER_PCT
    &AuraEffect::HandleAuraModRangedAttackPowerPercent,           //167 SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT
    &AuraEffect::HandleNoImmediateEffect,                         //168 SPELL_AURA_MOD_DAMAGE_DONE_VERSUS            implemented in Unit::SpellDamageBonus, Unit::MeleeDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         //169 SPELL_AURA_MOD_CRIT_PERCENT_VERSUS           implemented in Unit::DealDamageBySchool, Unit::DoAttackDamage, Unit::SpellCriticalBonus
    &AuraEffect::HandleNULL,                                      //170 SPELL_AURA_DETECT_AMORE       various spells that change visual of units for aura target (clientside?)
    &AuraEffect::HandleAuraModIncreaseSpeed,                      //171 SPELL_AURA_MOD_SPEED_NOT_STACK
    &AuraEffect::HandleAuraModIncreaseMountedSpeed,               //172 SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK
    &AuraEffect::HandleNULL,                                      //173 unused (3.2.0) no spells, old SPELL_AURA_ALLOW_CHAMPION_SPELLS  only for Proclaim Champion spell
    &AuraEffect::HandleModSpellDamagePercentFromStat,             //174 SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT  implemented in Unit::SpellBaseDamageBonus
    &AuraEffect::HandleModSpellHealingPercentFromStat,            //175 SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT implemented in Unit::SpellBaseHealingBonus
    &AuraEffect::HandleSpiritOfRedemption,                        //176 SPELL_AURA_SPIRIT_OF_REDEMPTION   only for Spirit of Redemption spell, die at aura end
    &AuraEffect::HandleCharmConvert,                              //177 SPELL_AURA_AOE_CHARM
    &AuraEffect::HandleNoImmediateEffect,                         //178 SPELL_AURA_MOD_DEBUFF_RESISTANCE          implemented in Unit::MagicSpellHitResult
    &AuraEffect::HandleNoImmediateEffect,                         //179 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE implemented in Unit::SpellCriticalBonus
    &AuraEffect::HandleNoImmediateEffect,                         //180 SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS   implemented in Unit::SpellDamageBonus
    &AuraEffect::HandleNULL,                                      //181 unused (3.2.0) old SPELL_AURA_MOD_FLAT_SPELL_CRIT_DAMAGE_VERSUS
    &AuraEffect::HandleAuraModResistenceOfStatPercent,            //182 SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT
    &AuraEffect::HandleNULL,                                      //183 SPELL_AURA_MOD_CRITICAL_THREAT only used in 28746 - miscvalue - spell school
    &AuraEffect::HandleNoImmediateEffect,                         //184 SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE  implemented in Unit::RollMeleeOutcomeAgainst
    &AuraEffect::HandleNoImmediateEffect,                         //185 SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE implemented in Unit::RollMeleeOutcomeAgainst
    &AuraEffect::HandleNoImmediateEffect,                         //186 SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE  implemented in Unit::MagicSpellHitResult
    &AuraEffect::HandleNoImmediateEffect,                         //187 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE  implemented in Unit::GetUnitCriticalChance
    &AuraEffect::HandleNoImmediateEffect,                         //188 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE implemented in Unit::GetUnitCriticalChance
    &AuraEffect::HandleModRating,                                 //189 SPELL_AURA_MOD_RATING
    &AuraEffect::HandleNoImmediateEffect,                         //190 SPELL_AURA_MOD_FACTION_REPUTATION_GAIN     implemented in Player::CalculateReputationGain
    &AuraEffect::HandleAuraModUseNormalSpeed,                     //191 SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED
    &AuraEffect::HandleModMeleeRangedSpeedPct,                    //192 SPELL_AURA_MOD_MELEE_RANGED_HASTE
    &AuraEffect::HandleModCombatSpeedPct,                         //193 SPELL_AURA_MELEE_SLOW (in fact combat (any type attack) speed pct)
    &AuraEffect::HandleNoImmediateEffect,                         //194 SPELL_AURA_MOD_TARGET_ABSORB_SCHOOL implemented in Unit::CalcAbsorbResist
    &AuraEffect::HandleNoImmediateEffect,                         //195 SPELL_AURA_MOD_TARGET_ABILITY_ABSORB_SCHOOL implemented in Unit::CalcAbsorbResist
    &AuraEffect::HandleNULL,                                      //196 SPELL_AURA_MOD_COOLDOWN - flat mod of spell cooldowns
    &AuraEffect::HandleNoImmediateEffect,                         //197 SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE implemented in Unit::SpellCriticalBonus Unit::GetUnitCriticalChance
    &AuraEffect::HandleNULL,                                      //198 unused (3.2.0) old SPELL_AURA_MOD_ALL_WEAPON_SKILLS
    &AuraEffect::HandleNoImmediateEffect,                         //199 SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT  implemented in Unit::MagicSpellHitResult
    &AuraEffect::HandleNoImmediateEffect,                         //200 SPELL_AURA_MOD_XP_PCT implemented in Player::RewardPlayerAndGroupAtKill
    &AuraEffect::HandleAuraAllowFlight,                           //201 SPELL_AURA_FLY                             this aura enable flight mode...
    &AuraEffect::HandleNoImmediateEffect,                         //202 SPELL_AURA_CANNOT_BE_DODGED                implemented in Unit::RollPhysicalOutcomeAgainst
    &AuraEffect::HandleNoImmediateEffect,                         //203 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE  implemented in Unit::CalculateMeleeDamage and Unit::CalculateSpellDamage
    &AuraEffect::HandleNoImmediateEffect,                         //204 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE implemented in Unit::CalculateMeleeDamage and Unit::CalculateSpellDamage
    &AuraEffect::HandleNULL,                                      //205 SPELL_AURA_MOD_SCHOOL_CRIT_DMG_TAKEN
    &AuraEffect::HandleAuraModIncreaseFlightSpeed,                //206 SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED
    &AuraEffect::HandleAuraModIncreaseFlightSpeed,                //207 SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED
    &AuraEffect::HandleAuraModIncreaseFlightSpeed,                //208 SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED
    &AuraEffect::HandleAuraModIncreaseFlightSpeed,                //209 SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS
    &AuraEffect::HandleAuraModIncreaseFlightSpeed,                //210 SPELL_AURA_MOD_VEHICLE_SPEED_ALWAYS
    &AuraEffect::HandleAuraModIncreaseFlightSpeed,                //211 SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK
    &AuraEffect::HandleAuraModRangedAttackPowerOfStatPercent,     //212 SPELL_AURA_MOD_RANGED_ATTACK_POWER_OF_STAT_PERCENT
    &AuraEffect::HandleNoImmediateEffect,                         //213 SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT implemented in Player::RewardRage
    &AuraEffect::HandleNULL,                                      //214 Tamed Pet Passive
    &AuraEffect::HandleArenaPreparation,                          //215 SPELL_AURA_ARENA_PREPARATION
    &AuraEffect::HandleModCastingSpeed,                           //216 SPELL_AURA_HASTE_SPELLS
    &AuraEffect::HandleNULL,                                      //217 69106 - killing spree helper - unknown use
    &AuraEffect::HandleAuraModRangedHaste,                        //218 SPELL_AURA_HASTE_RANGED
    &AuraEffect::HandleModManaRegen,                              //219 SPELL_AURA_MOD_MANA_REGEN_FROM_STAT
    &AuraEffect::HandleModRatingFromStat,                         //220 SPELL_AURA_MOD_RATING_FROM_STAT
    &AuraEffect::HandleNULL,                                      //221 SPELL_AURA_MOD_DETAUNT
    &AuraEffect::HandleUnused,                                    //222 unused (3.2.0) only for spell 44586 that not used in real spell cast
    &AuraEffect::HandleNoImmediateEffect,                         //223 SPELL_AURA_RAID_PROC_FROM_CHARGE
    &AuraEffect::HandleUnused,                                    //224 unused (3.0.8a)
    &AuraEffect::HandleNoImmediateEffect,                         //225 SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE
    &AuraEffect::HandleNoImmediateEffect,                         //226 SPELL_AURA_PERIODIC_DUMMY implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNoImmediateEffect,                         //227 SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNoImmediateEffect,                         //228 SPELL_AURA_DETECT_STEALTH stealth detection
    &AuraEffect::HandleNoImmediateEffect,                         //229 SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE
    &AuraEffect::HandleAuraModIncreaseHealth,                     //230 SPELL_AURA_MOD_INCREASE_HEALTH_2
    &AuraEffect::HandleNoImmediateEffect,                         //231 SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE
    &AuraEffect::HandleNoImmediateEffect,                         //232 SPELL_AURA_MECHANIC_DURATION_MOD           implement in Unit::CalculateSpellDuration
    &AuraEffect::HandleUnused,                                    //233 set model id to the one of the creature with id GetMiscValue() - clientside
    &AuraEffect::HandleNoImmediateEffect,                         //234 SPELL_AURA_MECHANIC_DURATION_MOD_NOT_STACK implement in Unit::CalculateSpellDuration
    &AuraEffect::HandleNoImmediateEffect,                         //235 SPELL_AURA_MOD_DISPEL_RESIST               implement in Unit::MagicSpellHitResult
    &AuraEffect::HandleAuraControlVehicle,                        //236 SPELL_AURA_CONTROL_VEHICLE
    &AuraEffect::HandleModSpellDamagePercentFromAttackPower,      //237 SPELL_AURA_MOD_SPELL_DAMAGE_OF_ATTACK_POWER  implemented in Unit::SpellBaseDamageBonus
    &AuraEffect::HandleModSpellHealingPercentFromAttackPower,     //238 SPELL_AURA_MOD_SPELL_HEALING_OF_ATTACK_POWER implemented in Unit::SpellBaseHealingBonus
    &AuraEffect::HandleAuraModScale,                              //239 SPELL_AURA_MOD_SCALE_2 only in Noggenfogger Elixir (16595) before 2.3.0 aura 61
    &AuraEffect::HandleAuraModExpertise,                          //240 SPELL_AURA_MOD_EXPERTISE
    &AuraEffect::HandleForceMoveForward,                          //241 SPELL_AURA_FORCE_MOVE_FORWARD Forces the caster to move forward
    &AuraEffect::HandleNULL,                                      //242 SPELL_AURA_MOD_SPELL_DAMAGE_FROM_HEALING - 2 test spells: 44183 and 44182
    &AuraEffect::HandleAuraModFaction,                            //243 SPELL_AURA_MOD_FACTION
    &AuraEffect::HandleComprehendLanguage,                        //244 SPELL_AURA_COMPREHEND_LANGUAGE
    &AuraEffect::HandleNoImmediateEffect,                         //245 SPELL_AURA_MOD_AURA_DURATION_BY_DISPEL
    &AuraEffect::HandleNoImmediateEffect,                         //246 SPELL_AURA_MOD_AURA_DURATION_BY_DISPEL_NOT_STACK implemented in Spell::EffectApplyAura
    &AuraEffect::HandleAuraCloneCaster,                           //247 SPELL_AURA_CLONE_CASTER
    &AuraEffect::HandleNoImmediateEffect,                         //248 SPELL_AURA_MOD_COMBAT_RESULT_CHANCE         implemented in Unit::RollMeleeOutcomeAgainst
    &AuraEffect::HandleAuraConvertRune,                           //249 SPELL_AURA_CONVERT_RUNE
    &AuraEffect::HandleAuraModIncreaseHealth,                     //250 SPELL_AURA_MOD_INCREASE_HEALTH_2
    &AuraEffect::HandleNoImmediateEffect,                         //251 SPELL_AURA_MOD_ENEMY_DODGE
    &AuraEffect::HandleModCombatSpeedPct,                         //252 SPELL_AURA_252 Is there any difference between this and SPELL_AURA_MELEE_SLOW ? maybe not stacking mod?
    &AuraEffect::HandleNoImmediateEffect,                         //253 SPELL_AURA_MOD_BLOCK_CRIT_CHANCE  implemented in Unit::isBlockCritical
    &AuraEffect::HandleAuraModDisarm,                             //254 SPELL_AURA_MOD_DISARM_OFFHAND
    &AuraEffect::HandleNoImmediateEffect,                         //255 SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT    implemented in Unit::SpellDamageBonus
    &AuraEffect::HandleNoReagentUseAura,                          //256 SPELL_AURA_NO_REAGENT_USE Use SpellClassMask for spell select
    &AuraEffect::HandleNULL,                                      //257 SPELL_AURA_MOD_TARGET_RESIST_BY_SPELL_CLASS Use SpellClassMask for spell select
    &AuraEffect::HandleNULL,                                      //258 SPELL_AURA_MOD_SPELL_VISUAL
    &AuraEffect::HandleNoImmediateEffect,                         //259 SPELL_AURA_MOD_HOT_PCT implemented in Unit::SpellHealingBonus
    &AuraEffect::HandleNoImmediateEffect,                         //260 SPELL_AURA_SCREEN_EFFECT (miscvalue = id in ScreenEffect.dbc) not required any code
    &AuraEffect::HandlePhase,                                     //261 SPELL_AURA_PHASE undetactable invisibility?     implemented in Unit::isVisibleForOrDetect
    &AuraEffect::HandleNoImmediateEffect,                         //262 SPELL_AURA_ABILITY_IGNORE_AURASTATE implemented in spell::cancast
    &AuraEffect::HandleAuraAllowOnlyAbility,                      //263 SPELL_AURA_ALLOW_ONLY_ABILITY player can use only abilities set in SpellClassMask
    &AuraEffect::HandleUnused,                                    //264 unused (3.2.0)
    &AuraEffect::HandleUnused,                                    //265 unused (3.2.0)
    &AuraEffect::HandleUnused,                                    //266 unused (3.2.0)
    &AuraEffect::HandleNoImmediateEffect,                         //267 SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL         implemented in Unit::IsImmunedToSpellEffect
    &AuraEffect::HandleAuraModAttackPowerOfStatPercent,           //268 SPELL_AURA_MOD_ATTACK_POWER_OF_STAT_PERCENT
    &AuraEffect::HandleNoImmediateEffect,                         //269 SPELL_AURA_MOD_IGNORE_TARGET_RESIST implemented in Unit::CalcAbsorbResist and CalcArmorReducedDamage
    &AuraEffect::HandleNoImmediateEffect,                         //270 SPELL_AURA_MOD_ABILITY_IGNORE_TARGET_RESIST implemented in Unit::CalcAbsorbResist and CalcArmorReducedDamage
    &AuraEffect::HandleNoImmediateEffect,                         //271 SPELL_AURA_MOD_DAMAGE_FROM_CASTER    implemented in Unit::SpellDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         //272 SPELL_AURA_IGNORE_MELEE_RESET
    &AuraEffect::HandleUnused,                                    //273 clientside
    &AuraEffect::HandleNoImmediateEffect,                         //274 SPELL_AURA_CONSUME_NO_AMMO implemented in spell::CalculateDamageDoneForAllTargets
    &AuraEffect::HandleNoImmediateEffect,                         //275 SPELL_AURA_MOD_IGNORE_SHAPESHIFT Use SpellClassMask for spell select
    &AuraEffect::HandleNULL,                                      //276 mod damage % mechanic?
    &AuraEffect::HandleNoImmediateEffect,                         //277 SPELL_AURA_MOD_ABILITY_AFFECTED_TARGETS implemented in spell::settargetmap
    &AuraEffect::HandleAuraModDisarm,                             //278 SPELL_AURA_MOD_DISARM_RANGED disarm ranged weapon
    &AuraEffect::HandleNoImmediateEffect,                         //279 SPELL_AURA_INITIALIZE_IMAGES
    &AuraEffect::HandleNoImmediateEffect,                         //280 SPELL_AURA_MOD_TARGET_ARMOR_PCT
    &AuraEffect::HandleNoImmediateEffect,                         //281 SPELL_AURA_MOD_HONOR_GAIN_PCT implemented in Player::RewardHonor
    &AuraEffect::HandleAuraIncreaseBaseHealthPercent,             //282 SPELL_AURA_INCREASE_BASE_HEALTH_PERCENT
    &AuraEffect::HandleNoImmediateEffect,                         //283 SPELL_AURA_MOD_HEALING_RECEIVED       implemented in Unit::SpellHealingBonus
    &AuraEffect::HandleAuraLinked,                                //284 SPELL_AURA_LINKED
    &AuraEffect::HandleAuraModAttackPowerOfArmor,                 //285 SPELL_AURA_MOD_ATTACK_POWER_OF_ARMOR  implemented in Player::UpdateAttackPowerAndDamage
    &AuraEffect::HandleNoImmediateEffect,                         //286 SPELL_AURA_ABILITY_PERIODIC_CRIT implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNoImmediateEffect,                         //287 SPELL_AURA_DEFLECT_SPELLS             implemented in Unit::MagicSpellHitResult and Unit::MeleeSpellHitResult
    &AuraEffect::HandleNoImmediateEffect,                         //288 SPELL_AURA_IGNORE_HIT_DIRECTION  implemented in Unit::MagicSpellHitResult and Unit::MeleeSpellHitResult Unit::RollMeleeOutcomeAgainst
    &AuraEffect::HandleNULL,                                      //289 unused (3.2.0)
    &AuraEffect::HandleAuraModCritPct,                            //290 SPELL_AURA_MOD_CRIT_PCT
    &AuraEffect::HandleNoImmediateEffect,                         //291 SPELL_AURA_MOD_XP_QUEST_PCT  implemented in Player::RewardQuest
    &AuraEffect::HandleAuraOpenStable,                            //292 SPELL_AURA_OPEN_STABLE
    &AuraEffect::HandleAuraOverrideSpells,                        //293 auras which probably add set of abilities to their target based on it's miscvalue
    &AuraEffect::HandleNoImmediateEffect,                         //294 SPELL_AURA_PREVENT_REGENERATE_POWER implemented in Player::Regenerate(Powers power)
    &AuraEffect::HandleNULL,                                      //295 0 spells in 3.3.5
    &AuraEffect::HandleAuraSetVehicle,                            //296 SPELL_AURA_SET_VEHICLE_ID sets vehicle on target
    &AuraEffect::HandleNULL,                                      //297 Spirit Burst spells
    &AuraEffect::HandleNULL,                                      //298 70569 - Strangulating, maybe prevents talk or cast
    &AuraEffect::HandleNULL,                                      //299 unused
    &AuraEffect::HandleNoImmediateEffect,                         //300 SPELL_AURA_SHARE_DAMAGE_PCT implemented in Unit::DealDamage
    &AuraEffect::HandleNoImmediateEffect,                         //301 SPELL_AURA_SCHOOL_HEAL_ABSORB implemented in Unit::CalcHealAbsorb
    &AuraEffect::HandleNULL,                                      //302 0 spells in 3.3.5
    &AuraEffect::HandleNoImmediateEffect,                         //303 SPELL_AURA_MOD_DAMAGE_DONE_VERSUS_AURASTATE implemented in Unit::SpellDamageBonus, Unit::MeleeDamageBonus
    &AuraEffect::HandleAuraModFakeInebriation,                    //304 SPELL_AURA_MOD_DRUNK
    &AuraEffect::HandleAuraModIncreaseSpeed,                      //305 SPELL_AURA_MOD_MINIMUM_SPEED
    &AuraEffect::HandleNULL,                                      //306 0 spells in 3.3.5
    &AuraEffect::HandleNULL,                                      //307 0 spells in 3.3.5
    &AuraEffect::HandleNULL,                                      //308 new aura for hunter traps
    &AuraEffect::HandleNULL,                                      //309 0 spells in 3.3.5
    &AuraEffect::HandleNoImmediateEffect,                         //310 SPELL_AURA_MOD_CREATURE_AOE_DAMAGE_AVOIDANCE implemented in Spell::CalculateDamageDone
    &AuraEffect::HandleNULL,                                      //311 0 spells in 3.3.5
    &AuraEffect::HandleNULL,                                      //312 0 spells in 3.3.5
    &AuraEffect::HandleNULL,                                      //313 0 spells in 3.3.5
    &AuraEffect::HandleNoImmediateEffect,                         //314 SPELL_AURA_PREVENT_RESSURECTION todo
    &AuraEffect::HandleNoImmediateEffect,                         //315 SPELL_AURA_UNDERWATER_WALKING todo
    &AuraEffect::HandleNoImmediateEffect,                         //316 SPELL_AURA_PERIODIC_HASTE implemented in AuraEffect::CalculatePeriodic
    &AuraEffect::HandleNULL,                                      //317
    &AuraEffect::HandleNULL,                                      //318
    &AuraEffect::HandleNULL,                                      //319
    &AuraEffect::HandleNULL,                                      //320
    &AuraEffect::HandleNULL,                                      //321
    &AuraEffect::HandleNULL,                                      //322
    &AuraEffect::HandleNULL,                                      //323
    &AuraEffect::HandleNULL,                                      //324
    &AuraEffect::HandleNULL,                                      //325
    &AuraEffect::HandleNULL,                                      //326
    &AuraEffect::HandleNULL,                                      //327
    &AuraEffect::HandleNULL,                                      //328
    &AuraEffect::HandleNULL,                                      //329
    &AuraEffect::HandleNULL,                                      //330
    &AuraEffect::HandleNULL,                                      //331
    &AuraEffect::HandleNULL,                                      //332
    &AuraEffect::HandleNULL,                                      //333
    &AuraEffect::HandleNULL,                                      //334
    &AuraEffect::HandleNULL,                                      //335
    &AuraEffect::HandleNULL,                                      //336
    &AuraEffect::HandleNULL,                                      //337
    &AuraEffect::HandleNULL,                                      //338
    &AuraEffect::HandleNULL,                                      //339
    &AuraEffect::HandleNULL,                                      //340
    &AuraEffect::HandleNULL,                                      //341
    &AuraEffect::HandleNULL,                                      //342
    &AuraEffect::HandleNULL,                                      //343
    &AuraEffect::HandleNULL,                                      //344
    &AuraEffect::HandleNULL,                                      //345
    &AuraEffect::HandleNULL,                                      //346
    &AuraEffect::HandleNULL,                                      //347
    &AuraEffect::HandleNULL,                                      //348
    &AuraEffect::HandleNULL,                                      //349
    &AuraEffect::HandleNULL,                                      //350
    &AuraEffect::HandleNULL,                                      //351
    &AuraEffect::HandleNULL,                                      //352
    &AuraEffect::HandleNULL,                                      //353
};

AuraEffect::AuraEffect(Aura * base, uint8 effIndex, int32 *baseAmount, Unit * caster):
m_base(base), m_spellProto(base->GetSpellProto()), m_effIndex(effIndex),
m_baseAmount(baseAmount ? *baseAmount : m_spellProto->GetSpellEffectBasePoints(m_effIndex)),
m_canBeRecalculated(true), m_spellmod(NULL), m_isPeriodic(false),
m_periodicTimer(0), m_tickNumber(0)
{
    CalculatePeriodic(caster, true);

    m_amount = CalculateAmount(caster);

    CalculateSpellMod();
}

AuraEffect::~AuraEffect()
{
    delete m_spellmod;
}

void AuraEffect::GetTargetList(std::list<Unit *> & targetList) const
{
    Aura::ApplicationMap const & targetMap = GetBase()->GetApplicationMap();
    // remove all targets which were not added to new list - they no longer deserve area aura
    for (Aura::ApplicationMap::const_iterator appIter = targetMap.begin(); appIter != targetMap.end(); ++appIter)
    {
        if (appIter->second->HasEffect(GetEffIndex()))
            targetList.push_back(appIter->second->GetTarget());
    }
}

void AuraEffect::GetApplicationList(std::list<AuraApplication *> & applicationList) const
{
    Aura::ApplicationMap const & targetMap = GetBase()->GetApplicationMap();
    // remove all targets which were not added to new list - they no longer deserve area aura
    for (Aura::ApplicationMap::const_iterator appIter = targetMap.begin(); appIter != targetMap.end(); ++appIter)
    {
        if (appIter->second->HasEffect(GetEffIndex()))
            applicationList.push_back(appIter->second);
    }
}

int32 AuraEffect::CalculateAmount(Unit * caster)
{
    int32 amount;
    // default amount calculation
    amount = SpellMgr::CalculateSpellEffectAmount(m_spellProto, m_effIndex, caster, &m_baseAmount, NULL);

    // check item enchant aura cast
    if (!amount && caster)
        if (uint64 itemGUID = GetBase()->GetCastItemGUID())
            if (Player *playerCaster = dynamic_cast<Player*>(caster))
                if (Item *castItem = playerCaster->GetItemByGuid(itemGUID))
                    if (castItem->GetItemSuffixFactor())
                    {
                        ItemRandomSuffixEntry const *item_rand_suffix = sItemRandomSuffixStore.LookupEntry(abs(castItem->GetItemRandomPropertyId()));
                        if (item_rand_suffix)
                        {
                            for (int k = 0; k < MAX_ITEM_ENCHANTMENT_EFFECTS; k++)
                            {
                                SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(item_rand_suffix->enchant_id[k]);
                                if (pEnchant)
                                {
                                    for (int t = 0; t < MAX_ITEM_ENCHANTMENT_EFFECTS; t++)
                                        if (pEnchant->spellid[t] == m_spellProto->Id)
                                    {
                                        amount = uint32((item_rand_suffix->prefix[k]*castItem->GetItemSuffixFactor()) / 10000);
                                        break;
                                    }
                                }

                                if (amount)
                                    break;
                            }
                        }
                    }

    float DoneActualBenefit = 0.0f;

    // custom amount calculations go here
    switch(GetAuraType())
    {
        // crowd control auras
        case SPELL_AURA_MOD_CONFUSE:
        case SPELL_AURA_MOD_FEAR:
        case SPELL_AURA_MOD_STUN:
        case SPELL_AURA_MOD_ROOT:
        case SPELL_AURA_TRANSFORM:
            m_canBeRecalculated = false;
            if (!m_spellProto->GetProcFlags())
                break;
            amount = int32(GetBase()->GetUnitOwner()->CountPctFromMaxHealth(10));
            if (caster)
            {
                // Glyphs increasing damage cap
                Unit::AuraEffectList const& overrideClassScripts = caster->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                for (Unit::AuraEffectList::const_iterator itr = overrideClassScripts.begin(); itr != overrideClassScripts.end(); ++itr)
                {
                    if ((*itr)->IsAffectedOnSpell(m_spellProto))
                    {
                        // Glyph of Fear, Glyph of Frost nova and similar auras
                        if ((*itr)->GetMiscValue() == 7801)
                        {
                            AddPctN(amount, (*itr)->GetAmount());
                            break;
                        }
                    }
                }
            }
            break;
        case SPELL_AURA_SCHOOL_ABSORB:
            m_canBeRecalculated = false;
            if (!caster)
                break;
            switch(GetSpellProto()->GetSpellFamilyName())
            {
                case SPELLFAMILY_MAGE:
                    // Ice Barrier
                    if (GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[1] & 0x1 && GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[2] & 0x8)
                    {
                        // +80.68% from sp bonus
                        DoneActualBenefit += caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.8068f;
                        // Glyph of Ice Barrier: its weird having a SPELLMOD_ALL_EFFECTS here but its blizzards doing :)
                        // Glyph of Ice Barrier is only applied at the spell damage bonus because it was already applied to the base value in CalculateSpellDamage
                        DoneActualBenefit = caster->ApplyEffectModifiers(GetSpellProto(), m_effIndex, DoneActualBenefit);
                    }
                    // Fire Ward
                    else if(GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x8 && GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[2] & 0x8)
                    {
                        // +80.68% from sp bonus
                        DoneActualBenefit += caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.8068f;
                    }
                    // Frost Ward
                    else if(GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x100 && GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[2] & 0x8)
                    {
                        // +80.68% from sp bonus
                        DoneActualBenefit += caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.8068f;
                    }
                    break;
                case SPELLFAMILY_WARLOCK:
                    // Shadow Ward
                    if (m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[2] & 0x40)
                    {
                        // +80.68% from sp bonus
                        DoneActualBenefit += caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.8068f;
                    }
                    break;
                case SPELLFAMILY_PRIEST:
                    // Power Word: Shield
                    if (GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x1 && GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[2] & 0x400)
                    {
                        // +80.68% from sp bonus
                        float bonus = 0.8068f;

                        // Borrowed Time
                        if (AuraEffect const* pAurEff = caster->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_PRIEST, 2899, 1))
                            bonus += CalculatePctN(1.0f, pAurEff->GetAmount());

                        DoneActualBenefit += caster->SpellBaseHealingBonus(GetSpellSchoolMask(m_spellProto)) * bonus;
                        // Improved PW: Shield: its weird having a SPELLMOD_ALL_EFFECTS here but its blizzards doing :)
                        // Improved PW: Shield is only applied at the spell healing bonus because it was already applied to the base value in CalculateSpellDamage
                        DoneActualBenefit = caster->ApplyEffectModifiers(GetSpellProto(), m_effIndex, DoneActualBenefit);
                        DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

                        amount += int32(DoneActualBenefit);

                        // Twin Disciplines
                        if (AuraEffect const* pAurEff = caster->GetAuraEffect(SPELL_AURA_ADD_PCT_MODIFIER, SPELLFAMILY_PRIEST, 0x400000, 0, 0, caster->GetGUID()))
                            AddPctN(amount, pAurEff->GetAmount());

                        // Focused Power
                        // Reuse variable, not sure if this code below can be moved before Twin Disciplines
                        DoneActualBenefit = float(amount);
                        DoneActualBenefit *= caster->GetTotalAuraMultiplier(SPELL_AURA_MOD_HEALING_DONE_PERCENT);
                        amount = int32(DoneActualBenefit);

                        return amount;
                    }
                    break;
                case SPELLFAMILY_PALADIN:
                    // Sacred Shield
                    if (m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[1] & 0x80000)
                    {
                        //+75.00% from sp bonus
                        float bonus = 0.75f;

                        DoneActualBenefit += caster->SpellBaseHealingBonus(GetSpellSchoolMask(m_spellProto)) * bonus;
                        // Divine Guardian is only applied at the spell healing bonus because it was already applied to the base value in CalculateSpellDamage
                        DoneActualBenefit = caster->ApplyEffectModifiers(GetSpellProto(), m_effIndex, DoneActualBenefit);
                        DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

                        amount += (int32)DoneActualBenefit;

                        // Arena - Dampening
                        if (AuraEffect const* pAurEff = caster->GetAuraEffect(74410, 0))
                            AddPctN(amount, pAurEff->GetAmount());
                        // Battleground - Dampening
                        else if (AuraEffect const* pAurEff = caster->GetAuraEffect(74411, 0))
                            AddPctN(amount, pAurEff->GetAmount());

                        return amount;
                    }
                    break;
                default:
                    break;
            }
            break;
        case SPELL_AURA_MANA_SHIELD:
            m_canBeRecalculated = false;
            if (!caster)
                break;
            // Mana Shield
            if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_MAGE && GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x8000 && m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[2] & 0x8)
            {
                // +80.53% from +spd bonus
                DoneActualBenefit += caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.8053f;;
            }
            break;
        case SPELL_AURA_DUMMY:
            if (!caster)
                break;
            // Earth Shield
            if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_SHAMAN && m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[1] & 0x400)
                amount = caster->SpellHealingBonus(GetBase()->GetUnitOwner(), GetSpellProto(), amount, SPELL_DIRECT_DAMAGE);
            break;
        case SPELL_AURA_DAMAGE_SHIELD:
            if (!caster)
                break;
            // Thorns
            if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_DRUID && m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x100)
                // 3.3% from sp bonus
                DoneActualBenefit = caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.033f;
            break;
        case SPELL_AURA_PERIODIC_DAMAGE:
            if (!caster)
                break;
            // Rupture
            if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_ROGUE && m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x100000)
            {
                m_canBeRecalculated = false;
                if (caster->GetTypeId() != TYPEID_PLAYER)
                    break;
                //1 point : ${($m1+$b1*1+0.015*$AP)*4} damage over 8 secs
                //2 points: ${($m1+$b1*2+0.024*$AP)*5} damage over 10 secs
                //3 points: ${($m1+$b1*3+0.03*$AP)*6} damage over 12 secs
                //4 points: ${($m1+$b1*4+0.03428571*$AP)*7} damage over 14 secs
                //5 points: ${($m1+$b1*5+0.0375*$AP)*8} damage over 16 secs
                float AP_per_combo[6] = {0.0f, 0.015f, 0.024f, 0.03f, 0.03428571f, 0.0375f};
                uint8 cp = caster->ToPlayer()->GetComboPoints();
                if (cp > 5) cp = 5;
                amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * AP_per_combo[cp]);
            }
            // Rip
            else if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_DRUID && m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x00800000 && GetAuraType() == SPELL_AURA_PERIODIC_DAMAGE)
            {
                m_canBeRecalculated = false;
                // 0.01*$AP*cp
                if (caster->GetTypeId() != TYPEID_PLAYER)
                    break;

                uint8 cp = caster->ToPlayer()->GetComboPoints();

                // Idol of Feral Shadows. Cant be handled as SpellMod in SpellAura:Dummy due its dependency from CPs
                if (AuraEffect const * aurEff = caster->GetAuraEffect(34241, 0))
                    amount += cp * aurEff->GetAmount();

                amount += uint32(CalculatePctU(caster->GetTotalAttackPowerValue(BASE_ATTACK), cp));
            }
            // Rend
            else if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_WARRIOR && GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x20)
            {
                m_canBeRecalculated = false;
                // $0.2 * (($MWB + $mwb) / 2 + $AP / 14 * $MWS) bonus per tick
                float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
                int32 mws = caster->GetAttackTime(BASE_ATTACK);
                float mwb_min = caster->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE);
                float mwb_max = caster->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE);
                float mwb = ((mwb_min + mwb_max) / 2 + ap * mws / 14000) * 0.2f;
                amount += int32(caster->ApplyEffectModifiers(m_spellProto, m_effIndex, mwb));
                // "If used while your target is above 75% health, Rend does 35% more damage."
                // as for 3.1.3 only ranks above 9 (wrong tooltip?)
                if (sSpellMgr->GetSpellRank(m_spellProto->Id) >= 9)
                {
                    if (GetBase()->GetUnitOwner()->HasAuraState(AURA_STATE_HEALTH_ABOVE_75_PERCENT, m_spellProto, caster))
                        AddPctN(amount, SpellMgr::CalculateSpellEffectAmount(m_spellProto, 2, caster));
                }
            }
            // Unholy Blight damage over time effect
            else if (GetId() == 50536)
            {
                m_canBeRecalculated = false;
                // we're getting total damage on aura apply, change it to be damage per tick
                amount = int32((float)amount / GetTotalTicks());
            }
            break;
        case SPELL_AURA_PERIODIC_ENERGIZE:
            if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_GENERIC)
            {
                // Replenishment (0.25% from max)
                // Infinite Replenishment
                if (m_spellProto->SpellIconID == 3184 && m_spellProto->SpellVisual[0] == 12495)
                    amount = GetBase()->GetUnitOwner()->GetMaxPower(POWER_MANA) * 25 / 10000;
            }
            // Innervate
            else if (m_spellProto->Id == 29166)
                ApplyPctF(amount, float(GetBase()->GetUnitOwner()->GetCreatePowers(POWER_MANA)) / GetTotalTicks());
            // Owlkin Frenzy
            else if (m_spellProto->Id == 48391)
                ApplyPctU(amount, GetBase()->GetUnitOwner()->GetCreatePowers(POWER_MANA));
            break;
        case SPELL_AURA_PERIODIC_HEAL:
            if (!caster)
                break;
            // Lightwell Renew
            if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_PRIEST && m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[2] & 0x4000)
            {
                if (caster->GetTypeId() == TYPEID_PLAYER)
                // Bonus from Glyph of Lightwell
                if (AuraEffect* modHealing = caster->GetAuraEffect(55673, 0))
                    AddPctN(amount, modHealing->GetAmount());
            }
            break;
        case SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN:
            if (!caster)
                break;
            // Icebound Fortitude
            if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_DEATHKNIGHT && m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x00100000)
            {
                if (caster->GetTypeId() == TYPEID_PLAYER)
                {
                    int32 value = (-1 * amount) - 10;
                    uint32 defva = uint32(caster->ToPlayer()->GetSkillValue(SKILL_DEFENSE) + caster->ToPlayer()->GetRatingBonusValue(CR_DEFENSE_SKILL));

                    if (defva > 400)
                        value += int32((defva - 400) * 0.15);

                    // Glyph of Icebound Fortitude
                    if (AuraEffect const * aurEff = caster->GetAuraEffect(58625, 0))
                    {
                        int32 valMax = aurEff->GetAmount();
                        if (value < valMax)
                            value = valMax;
                    }
                    amount = -value;
                }
            }
            // Hand of Salvation
            else if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_PALADIN && GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x00000100)
            {
                //Glyph of Salvation
                if (caster->GetGUID() == GetBase()->GetUnitOwner()->GetGUID())
                    if (AuraEffect const * aurEff = caster->GetAuraEffect(63225, 0))
                        amount = -aurEff->GetAmount();
            }
            break;
        case SPELL_AURA_MOD_THREAT:
        {
            uint8 level_diff = 0;
            float multiplier = 0.0;
            switch (GetId())
            {
                // Arcane Shroud
                case 26400:
                    level_diff = GetBase()->GetUnitOwner()->getLevel() - 60;
                    multiplier = 2;
                    break;
                // The Eye of Diminution
                case 28862:
                    level_diff = GetBase()->GetUnitOwner()->getLevel() - 60;
                    multiplier = 1;
                    break;
            }
            if (level_diff > 0)
                amount += int32(multiplier * level_diff);
            break;
        }
        case SPELL_AURA_MOD_INCREASE_HEALTH:
            // Vampiric Blood
            if (GetId() == 55233)
                amount = GetBase()->GetUnitOwner()->CountPctFromMaxHealth(amount);
            break;
        case SPELL_AURA_MOD_INCREASE_ENERGY:
            // Hymn of Hope
            if (GetId() == 64904)
                ApplyPctU(amount, GetBase()->GetUnitOwner()->GetMaxPower(GetBase()->GetUnitOwner()->getPowerType()));
            break;
        case SPELL_AURA_MOD_INCREASE_SPEED:
            // Dash - do not set speed if not in cat form
            if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_DRUID && GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[2] & 0x00000008)
                amount = GetBase()->GetUnitOwner()->GetShapeshiftForm() == FORM_CAT ? amount : 0;
            break;
        default:
            break;
    }
    if (DoneActualBenefit != 0.0f)
    {
        DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());
        amount += (int32)DoneActualBenefit;
    }

    GetBase()->CallScriptEffectCalcAmountHandlers(const_cast<AuraEffect const *>(this), amount, m_canBeRecalculated);
    amount *= GetBase()->GetStackAmount();
    return amount;
}

void AuraEffect::CalculatePeriodic(Unit * caster, bool create)
{
    m_amplitude = m_spellProto->GetEffectAmplitude(m_effIndex);

    // prepare periodics
    switch (GetAuraType())
    {
        case SPELL_AURA_OBS_MOD_POWER:
            // 3 spells have no amplitude set
            if (!m_amplitude)
                m_amplitude = 1 * IN_MILLISECONDS;
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_PERIODIC_ENERGIZE:
        case SPELL_AURA_OBS_MOD_HEALTH:
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
        case SPELL_AURA_PERIODIC_MANA_LEECH:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        case SPELL_AURA_POWER_BURN_MANA:
            m_isPeriodic = true;
            break;
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
            if (GetId() == 51912)
            {
                m_amplitude = 3000;
            }
            m_isPeriodic = true;
            break;
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
        case SPELL_AURA_PERIODIC_DUMMY:
            m_isPeriodic = true;
            break;
        case SPELL_AURA_DUMMY:
            // Haunting Spirits - perdiodic trigger demon
            if (GetId() == 7057)
            {
                m_isPeriodic = true;
                m_amplitude = irand (0, 60) + 30;
                m_amplitude *= IN_MILLISECONDS;
            }
            break;
        default:
            break;
    }

    GetBase()->CallScriptEffectCalcPeriodicHandlers(const_cast<AuraEffect const *>(this), m_isPeriodic, m_amplitude);

    if (!m_isPeriodic)
        return;

    Player* modOwner = caster ? caster->GetSpellModOwner() : NULL;

    // Apply casting time mods
    if (m_amplitude)
    {
        // Apply periodic time mod
        if (modOwner)
            modOwner->ApplySpellMod(GetId(), SPELLMOD_ACTIVATION_TIME, m_amplitude);

        if (caster)
        {
            // Haste modifies periodic time of channeled spells
            if (IsChanneledSpell(m_spellProto))
                caster->ModSpellCastTime(m_spellProto, m_amplitude);
            // and periodic time of auras affected by SPELL_AURA_PERIODIC_HASTE
            if (caster->HasAuraTypeWithAffectMask(SPELL_AURA_PERIODIC_HASTE, m_spellProto) || m_spellProto->AttributesEx5 & SPELL_ATTR5_HASTE_AFFECT_DURATION)
                m_amplitude = int32(m_amplitude * caster->GetFloatValue(UNIT_MOD_CAST_SPEED));
        }
    }

    if (create)
    {
        // Start periodic on next tick or at aura apply
        if (m_amplitude && !(m_spellProto->AttributesEx5 & SPELL_ATTR5_START_PERIODIC_AT_APPLY))
            m_periodicTimer += m_amplitude;
    }
    else if (m_amplitude) // load aura from character_aura
    {
        m_tickNumber = GetBase()->GetDuration() / m_amplitude;
        m_periodicTimer = GetBase()->GetDuration() % m_amplitude;
        if (m_spellProto->AttributesEx5 & SPELL_ATTR5_START_PERIODIC_AT_APPLY)
            ++m_tickNumber;
    }
}

void AuraEffect::CalculateSpellMod()
{
    switch (GetAuraType())
    {
        case SPELL_AURA_DUMMY:
            switch(GetSpellProto()->GetSpellFamilyName())
            {
                case SPELLFAMILY_PRIEST:
                    // Pain and Suffering
                    if (m_spellProto->SpellIconID == 2874)
                    {
                        if (!m_spellmod)
                        {
                            m_spellmod = new SpellModifier(GetBase());
                            m_spellmod->op = SPELLMOD_DOT;
                            m_spellmod->type = SPELLMOD_PCT;
                            m_spellmod->spellId = GetId();
                            m_spellmod->mask[1] = 0x00002000;
                        }
                        m_spellmod->value = GetAmount();
                    }
                    break;
                case SPELLFAMILY_DRUID:
                    switch (GetId())
                    {
                        case 34246:                                 // Idol of the Emerald Queen
                        case 60779:                                 // Idol of Lush Moss
                        {
                            if (!m_spellmod)
                            {
                                m_spellmod = new SpellModifier(GetBase());
                                m_spellmod->op = SPELLMOD_DOT;
                                m_spellmod->type = SPELLMOD_FLAT;
                                m_spellmod->spellId = GetId();
                                m_spellmod->mask[1] = 0x0010;
                            }
                            m_spellmod->value = GetAmount()/7;
                        }
                        break;
                    }
                    break;
                default:
                    break;
            }
        case SPELL_AURA_MOD_SPELL_CRIT_CHANCE:
            switch(GetId())
            {
                case 51466: // Elemental oath
                case 51470: // Elemental oath
                    // "while Clearcasting from Elemental Focus is active, you deal 5%/10% more spell damage."
                    if (!m_spellmod)
                    {
                        m_spellmod = new SpellModifier(GetBase());
                        m_spellmod->op = SPELLMOD_EFFECT2;
                        m_spellmod->type = SPELLMOD_FLAT;
                        m_spellmod->spellId = GetId();
                        m_spellmod->mask[1] = 0x0004000;
                    }
                    m_spellmod->value = GetBase()->GetUnitOwner()->CalculateSpellDamage(GetBase()->GetUnitOwner(), GetSpellProto(), 1);
                    break;
                default:
                    break;
            }
            break;
        case SPELL_AURA_ADD_FLAT_MODIFIER:
        case SPELL_AURA_ADD_PCT_MODIFIER:
            if (!m_spellmod)
            {
                m_spellmod = new SpellModifier(GetBase());
                m_spellmod->op = SpellModOp(GetMiscValue());
                ASSERT(m_spellmod->op < MAX_SPELLMOD);

                m_spellmod->type = SpellModType(GetAuraType());    // SpellModType value == spell aura types
                m_spellmod->spellId = GetId();
                //m_spellmod->mask = GetBase()->GetSpellProto()->GetEffectSpellClassMask(GetEffIndex());
                m_spellmod->charges = GetBase()->GetCharges();
            }
            m_spellmod->value = GetAmount();
            break;
        default:
            break;
    }
    GetBase()->CallScriptEffectCalcSpellModHandlers(const_cast<AuraEffect const *>(this), m_spellmod);
}

void AuraEffect::ChangeAmount(int32 newAmount, bool mark)
{
    //Unit * caster = GetCaster();
    // Reapply if amount change
    if (newAmount != GetAmount())
    {
        UnitList targetList;
        GetTargetList(targetList);
        for (UnitList::iterator aurEffTarget = targetList.begin(); aurEffTarget != targetList.end(); ++aurEffTarget)
        {
            HandleEffect(*aurEffTarget, AURA_EFFECT_HANDLE_CHANGE_AMOUNT, false);
        }
        if (!mark)
            m_amount = newAmount;
        else
            SetAmount(newAmount);
        CalculateSpellMod();
        for (UnitList::iterator aurEffTarget = targetList.begin(); aurEffTarget != targetList.end(); ++aurEffTarget)
        {
            HandleEffect(*aurEffTarget, AURA_EFFECT_HANDLE_CHANGE_AMOUNT, true);
        }
    }
}

void AuraEffect::HandleEffect(AuraApplication const * aurApp, uint8 mode, bool apply)
{
    // check if call is correct
    ASSERT(!mode
        || mode == AURA_EFFECT_HANDLE_REAL
        || mode == AURA_EFFECT_HANDLE_SEND_FOR_CLIENT
        || mode == AURA_EFFECT_HANDLE_CHANGE_AMOUNT
        || mode == AURA_EFFECT_HANDLE_STAT);

    // real aura apply/remove, handle modifier
    if (mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK)
        ApplySpellMod(aurApp->GetTarget(), apply);

    bool prevented = false;
    if (apply)
        prevented = GetBase()->CallScriptEffectApplyHandlers(const_cast<AuraEffect const *>(this), aurApp, (AuraEffectHandleModes)mode);
    else
        prevented = GetBase()->CallScriptEffectRemoveHandlers(const_cast<AuraEffect const *>(this), aurApp, (AuraEffectHandleModes)mode);

    // check if script events have removed the aura or if default effect prevention was requested
    if ((apply && aurApp->GetRemoveMode()) || prevented)
        return;

    (*this.*AuraEffectHandler [GetAuraType()])(aurApp, mode, apply);
}

void AuraEffect::HandleEffect(Unit * target, uint8 mode, bool apply)
{
    AuraApplication const * aurApp = GetBase()->GetApplicationOfTarget(target->GetGUID());
    ASSERT(aurApp);
    HandleEffect(aurApp, mode, apply);
}

void AuraEffect::ApplySpellMod(Unit * target, bool apply)
{
    if (!m_spellmod || target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->ToPlayer()->AddSpellMod(m_spellmod, apply);

    // Auras with charges do not mod amount of passive auras
    if (GetBase()->GetCharges())
        return;
    // reapply some passive spells after add/remove related spellmods
    // Warning: it is a dead loop if 2 auras each other amount-shouldn't happen
    switch (GetMiscValue())
    {
        case SPELLMOD_ALL_EFFECTS:
        case SPELLMOD_EFFECT1:
        case SPELLMOD_EFFECT2:
        case SPELLMOD_EFFECT3:
        {
            uint64 guid = target->GetGUID();
            Unit::AuraApplicationMap & auras = target->GetAppliedAuras();
            for (Unit::AuraApplicationMap::iterator iter = auras.begin(); iter != auras.end(); ++iter)
            {
                Aura * aura = iter->second->GetBase();
                // only passive auras-active auras should have amount set on spellcast and not be affected
                // if aura is casted by others, it will not be affected
                if ((aura->IsPassive() || aura->GetSpellProto()->AttributesEx2 & SPELL_ATTR2_ALWAYS_APPLY_MODIFIERS) && aura->GetCasterGUID() == guid && sSpellMgr->IsAffectedByMod(aura->GetSpellProto(), m_spellmod))
                {
                    if (GetMiscValue() == SPELLMOD_ALL_EFFECTS)
                    {
                        for (uint8 i = 0; i<MAX_SPELL_EFFECTS; ++i)
                        {
                            if (AuraEffect * aurEff = aura->GetEffect(i))
                                aurEff->RecalculateAmount();
                        }
                    }
                    else if (GetMiscValue() == SPELLMOD_EFFECT1)
                    {
                       if (AuraEffect * aurEff = aura->GetEffect(0))
                            aurEff->RecalculateAmount();
                    }
                    else if (GetMiscValue() == SPELLMOD_EFFECT2)
                    {
                       if (AuraEffect * aurEff = aura->GetEffect(1))
                            aurEff->RecalculateAmount();
                    }
                    else //if (modOp == SPELLMOD_EFFECT3)
                    {
                       if (AuraEffect * aurEff = aura->GetEffect(2))
                            aurEff->RecalculateAmount();
                    }
                }
            }
        }
        default:
            break;
    }
}

void AuraEffect::Update(uint32 diff, Unit * caster)
{
    if (m_isPeriodic && (GetBase()->GetDuration() >=0 || GetBase()->IsPassive() || GetBase()->IsPermanent()))
    {
        if (m_periodicTimer > int32(diff))
            m_periodicTimer -= diff;
        else // tick also at m_periodicTimer == 0 to prevent lost last tick in case max m_duration == (max m_periodicTimer)*N
        {
            ++m_tickNumber;

            // update before tick (aura can be removed in TriggerSpell or PeriodicTick calls)
            m_periodicTimer += m_amplitude - diff;
            UpdatePeriodic(caster);

            std::list<AuraApplication*> effectApplications;
            GetApplicationList(effectApplications);
            // tick on targets of effects
            for (std::list<AuraApplication*>::iterator apptItr = effectApplications.begin(); apptItr != effectApplications.end(); ++apptItr)
                PeriodicTick(*apptItr, caster);
        }
    }
}

void AuraEffect::UpdatePeriodic(Unit * caster)
{
    switch(GetAuraType())
    {
        case SPELL_AURA_DUMMY:
            // Haunting Spirits
            if (GetId() == 7057)
            {
                m_amplitude = irand (0 , 60) + 30;
                m_amplitude *= IN_MILLISECONDS;
            }
            break;
        case SPELL_AURA_PERIODIC_DUMMY:
            switch(GetSpellProto()->GetSpellFamilyName())
            {
                case SPELLFAMILY_GENERIC:
                    switch(GetId())
                    {
                        // Drink
                        case 430:
                        case 431:
                        case 432:
                        case 1133:
                        case 1135:
                        case 1137:
                        case 10250:
                        case 22734:
                        case 27089:
                        case 34291:
                        case 43182:
                        case 43183:
                        case 46755:
                        case 49472: // Drink Coffee
                        case 57073:
                        case 61830:
                            if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                                return;
                            // Get SPELL_AURA_MOD_POWER_REGEN aura from spell
                            if (AuraEffect * aurEff = GetBase()->GetEffect(0))
                            {
                                if (aurEff->GetAuraType() != SPELL_AURA_MOD_POWER_REGEN)
                                {
                                    m_isPeriodic = false;
                                    sLog->outError("Aura %d structure has been changed - first aura is no longer SPELL_AURA_MOD_POWER_REGEN", GetId());
                                }
                                else
                                {
                                    // default case - not in arena
                                    if (!caster->ToPlayer()->InArena())
                                    {
                                        aurEff->ChangeAmount(GetAmount());
                                        m_isPeriodic = false;
                                    }
                                    else
                                    {
                                        // **********************************************
                                        // This feature uses only in arenas
                                        // **********************************************
                                        // Here need increase mana regen per tick (6 second rule)
                                        // on 0 tick -   0  (handled in 2 second)
                                        // on 1 tick - 166% (handled in 4 second)
                                        // on 2 tick - 133% (handled in 6 second)

                                        // Apply bonus for 1 - 4 tick
                                        switch (m_tickNumber)
                                        {
                                            case 1:   // 0%
                                                aurEff->ChangeAmount(0);
                                                break;
                                            case 2:   // 166%
                                                aurEff->ChangeAmount(GetAmount() * 5 / 3);
                                                break;
                                            case 3:   // 133%
                                                aurEff->ChangeAmount(GetAmount() * 4 / 3);
                                                break;
                                            default:  // 100% - normal regen
                                                aurEff->ChangeAmount(GetAmount());
                                                // No need to update after 4th tick
                                                m_isPeriodic = false;
                                                break;
                                        }
                                    }
                                }
                            }
                            break;
                        case 58549: // Tenacity
                        case 59911: // Tenacity (vehicle)
                           GetBase()->RefreshDuration();
                           break;
                        case 66823: case 67618: case 67619: case 67620: // Paralytic Toxin
                            // Get 0 effect aura
                            if (AuraEffect *slow = GetBase()->GetEffect(0))
                            {
                                int32 newAmount = slow->GetAmount() - 10;
                                if (newAmount < -100)
                                    newAmount = -100;
                                slow->ChangeAmount(newAmount);
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case SPELLFAMILY_MAGE:
                    if (GetId() == 55342)// Mirror Image
                        m_isPeriodic = false;
                    break;
                case SPELLFAMILY_DEATHKNIGHT:
                    // Chains of Ice
                    if (GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[1] & 0x00004000)
                    {
                        // Get 0 effect aura
                        if (AuraEffect *slow = GetBase()->GetEffect(0))
                        {
                            int32 newAmount = slow->GetAmount() + GetAmount();
                            if (newAmount > 0)
                                newAmount = 0;
                            slow->ChangeAmount(newAmount);
                        }
                        return;
                    }
                    break;
                default:
                    break;
           }
       default:
           break;
    }
    GetBase()->CallScriptEffectUpdatePeriodicHandlers(this);
}

bool AuraEffect::IsPeriodicTickCrit(Unit * target, Unit const * caster) const
{
    ASSERT(caster);
    Unit::AuraEffectList const& mPeriodicCritAuras= caster->GetAuraEffectsByType(SPELL_AURA_ABILITY_PERIODIC_CRIT);
    for (Unit::AuraEffectList::const_iterator itr = mPeriodicCritAuras.begin(); itr != mPeriodicCritAuras.end(); ++itr)
    {
        if ((*itr)->IsAffectedOnSpell(m_spellProto) && caster->isSpellCrit(target, m_spellProto, GetSpellSchoolMask(m_spellProto)))
            return true;
    }
    // Rupture - since 3.3.3 can crit
    if (target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_ROGUE, 0x100000, 0x0, 0x0, caster->GetGUID()))
    {
        if (caster->isSpellCrit(target, m_spellProto, GetSpellSchoolMask(m_spellProto)))
            return true;
    }
    return false;
}

void AuraEffect::SendTickImmune(Unit * target, Unit *caster) const
{
    if (caster)
        caster->SendSpellDamageImmune(target, m_spellProto->Id);
}

void AuraEffect::PeriodicTick(AuraApplication * aurApp, Unit * caster) const
{
    bool prevented = GetBase()->CallScriptEffectPeriodicHandlers(this, aurApp);
    if (prevented)
        return;

    Unit * target = aurApp->GetTarget();

    switch(GetAuraType())
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        {
            if (!caster)
                break;

            if (!target->isAlive())
                return;

            if (target->HasUnitState(UNIT_STAT_ISOLATED))
            {
                SendTickImmune(target, caster);
                return;
            }

            // Consecrate ticks can miss and will not show up in the combat log
            if (GetSpellProto()->GetSpellEffectIdByIndex(GetEffIndex()) == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                caster->SpellHitResult(target, GetSpellProto(), false) != SPELL_MISS_NONE)
                break;

            // Check for immune (not use charges)
            if (target->IsImmunedToDamage(GetSpellProto()))
            {
                SendTickImmune(target, caster);
                break;
            }

            // some auras remove at specific health level or more
            if (GetAuraType() == SPELL_AURA_PERIODIC_DAMAGE)
            {
                switch (GetId())
                {
                    case 43093: case 31956: case 38801:  // Grievous Wound
                    case 35321: case 38363: case 39215:  // Gushing Wound
                        if (target->IsFullHealth())
                        {
                            target->RemoveAurasDueToSpell(GetId());
                            return;
                        }
                        break;
                    case 38772: // Grievous Wound
                    {
                        uint32 percent =
                            GetEffIndex() < 2 && GetSpellProto()->GetSpellEffectIdByIndex(GetEffIndex()) == SPELL_EFFECT_DUMMY ?
                            caster->CalculateSpellDamage(target, GetSpellProto(), GetEffIndex()+1) :
                            100;
                            if (!target->HealthBelowPct(percent))
                        {
                            target->RemoveAurasDueToSpell(GetId());
                            return;
                        }
                        break;
                    }
                }
            }

            uint32 absorb=0;
            uint32 resist=0;
            CleanDamage cleanDamage =  CleanDamage(0, 0, BASE_ATTACK, MELEE_HIT_NORMAL);

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 damage = GetAmount() > 0 ? GetAmount() : 0;

            if (GetAuraType() == SPELL_AURA_PERIODIC_DAMAGE)
            {
                damage = caster->SpellDamageBonus(target, GetSpellProto(), damage, DOT, GetBase()->GetStackAmount());

                // Calculate armor mitigation
                if (Unit::IsDamageReducedByArmor(GetSpellSchoolMask(GetSpellProto()), GetSpellProto(), m_effIndex))
                {
                    uint32 damageReductedArmor = caster->CalcArmorReducedDamage(target, damage, GetSpellProto());
                    cleanDamage.mitigated_damage += damage - damageReductedArmor;
                    damage = damageReductedArmor;
                }

                // Curse of Agony damage-per-tick calculation
                if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_WARLOCK && (GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x400) && GetSpellProto()->SpellIconID == 544)
                {
                    uint32 totalTick = GetTotalTicks();
                    // 1..4 ticks, 1/2 from normal tick damage
                    if (m_tickNumber <= totalTick / 3)
                        damage = damage/2;
                    // 9..12 ticks, 3/2 from normal tick damage
                    else if (m_tickNumber > totalTick * 2 / 3)
                        damage += (damage+1)/2;           // +1 prevent 0.5 damage possible lost at 1..4 ticks
                    // 5..8 ticks have normal tick damage
                }
                // There is a Chance to make a Soul Shard when Drain soul does damage
                if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_WARLOCK && (GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x00004000))
                {
                    if (caster->GetTypeId() == TYPEID_PLAYER && caster->ToPlayer()->isHonorOrXPTarget(target))
                    {
                        if (roll_chance_i(20))
                        {
                            caster->CastSpell(caster, 43836, true, 0, this);
                            // Glyph of Drain Soul - chance to create an additional Soul Shard
                            if (AuraEffect *aur = caster->GetAuraEffect(58070, 0))
                                if (roll_chance_i(aur->GetMiscValue()))
                                    caster->CastSpell(caster, 58068, true, 0, aur);
                        }
                    }
                }
                if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_GENERIC)
                {
                    switch (GetId())
                    {
                        case 70911: // Unbound Plague
                        case 72854: // Unbound Plague
                        case 72855: // Unbound Plague
                        case 72856: // Unbound Plague
                            damage *= uint32(pow(1.25f, int32(m_tickNumber)));
                            break;
                        default:
                            break;
                    }
                }
            }
            else
                damage = uint32(target->CountPctFromMaxHealth(damage));

            bool crit = IsPeriodicTickCrit(target, caster);
            if (crit)
                damage = caster->SpellCriticalDamageBonus(m_spellProto, damage, target);

            int32 dmg = damage;
            caster->ApplyResilience(target, NULL, &dmg, crit, CR_CRIT_TAKEN_SPELL);
            damage = dmg;

            caster->CalcAbsorbResist(target, GetSpellSchoolMask(GetSpellProto()), DOT, damage, &absorb, &resist, m_spellProto);

            sLog->outDetail("PeriodicTick: %u (TypeId: %u) attacked %u (TypeId: %u) for %u dmg inflicted by %u abs is %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), damage, GetId(), absorb);

            caster->DealDamageMods(target, damage, &absorb);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_DONE_PERIODIC;
            uint32 procVictim   = PROC_FLAG_TAKEN_PERIODIC;
            uint32 procEx = (crit ? PROC_EX_CRITICAL_HIT : PROC_EX_NORMAL_HIT) | PROC_EX_INTERNAL_DOT;
            damage = (damage <= absorb+resist) ? 0 : (damage-absorb-resist);
            if (damage)
                procVictim|=PROC_FLAG_TAKEN_DAMAGE;

            int32 overkill = damage - target->GetHealth();
            if (overkill < 0)
              overkill = 0;

            SpellPeriodicAuraLogInfo pInfo(this, damage, overkill, absorb, resist, 0.0f, crit);
            target->SendPeriodicAuraLog(&pInfo);

            caster->ProcDamageAndSpell(target, procAttacker, procVictim, procEx, damage, BASE_ATTACK, GetSpellProto());

            caster->DealDamage(target, damage, &cleanDamage, DOT, GetSpellSchoolMask(GetSpellProto()), GetSpellProto(), true);
            break;
        }
        case SPELL_AURA_PERIODIC_LEECH:
        {
            if (!caster)
                return;

            if (!caster->isAlive())
                return;

            if (!target->isAlive())
                return;

            if (target->HasUnitState(UNIT_STAT_ISOLATED))
            {
                SendTickImmune(target, caster);
                return;
            }

            if (GetSpellProto()->GetSpellEffectIdByIndex(GetEffIndex()) == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                caster->SpellHitResult(target, GetSpellProto(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune
            if (target->IsImmunedToDamage(GetSpellProto()))
            {
                SendTickImmune(target, caster);
                return;
            }

            uint32 absorb=0;
            uint32 resist=0;
            CleanDamage cleanDamage =  CleanDamage(0, 0, BASE_ATTACK, MELEE_HIT_NORMAL);

            //uint32 damage = GetModifierValuePerStack() > 0 ? GetModifierValuePerStack() : 0;
            uint32 damage = GetAmount() > 0 ? GetAmount() : 0;
            damage = caster->SpellDamageBonus(target, GetSpellProto(), damage, DOT, GetBase()->GetStackAmount());

            bool crit = IsPeriodicTickCrit(target, caster);
            if (crit)
                damage = caster->SpellCriticalDamageBonus(m_spellProto, damage, target);

            // Calculate armor mitigation
            if (Unit::IsDamageReducedByArmor(GetSpellSchoolMask(GetSpellProto()), GetSpellProto(), m_effIndex))
            {
                uint32 damageReductedArmor = caster->CalcArmorReducedDamage(target, damage, GetSpellProto());
                cleanDamage.mitigated_damage += damage - damageReductedArmor;
                damage = damageReductedArmor;
            }

            int32 dmg = damage;
            caster->ApplyResilience(target, NULL, &dmg, crit, CR_CRIT_TAKEN_SPELL);
            damage = dmg;

            caster->CalcAbsorbResist(target, GetSpellSchoolMask(GetSpellProto()), DOT, damage, &absorb, &resist, m_spellProto);

            if (target->GetHealth() < damage)
                damage = uint32(target->GetHealth());

            sLog->outDetail("PeriodicTick: %u (TypeId: %u) health leech of %u (TypeId: %u) for %u dmg inflicted by %u abs is %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), damage, GetId(), absorb);

            caster->SendSpellNonMeleeDamageLog(target, GetId(), damage, GetSpellSchoolMask(GetSpellProto()), absorb, resist, false, 0, crit);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_DONE_PERIODIC;
            uint32 procVictim   = PROC_FLAG_TAKEN_PERIODIC;
            uint32 procEx = (crit ? PROC_EX_CRITICAL_HIT : PROC_EX_NORMAL_HIT) | PROC_EX_INTERNAL_DOT;
            damage = (damage <= absorb+resist) ? 0 : (damage-absorb-resist);
            if (damage)
                procVictim|=PROC_FLAG_TAKEN_DAMAGE;
            caster->ProcDamageAndSpell(target, procAttacker, procVictim, procEx, damage, BASE_ATTACK, GetSpellProto());
            int32 new_damage = caster->DealDamage(target, damage, &cleanDamage, DOT, GetSpellSchoolMask(GetSpellProto()), GetSpellProto(), false);

            if (!target->isAlive() && caster->IsNonMeleeSpellCasted(false))
                for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
                    if (Spell* spell = caster->GetCurrentSpell(CurrentSpellTypes(i)))
                        if (spell->m_spellInfo->Id == GetId())
                            spell->cancel();

            float gainMultiplier = SpellMgr::CalculateSpellEffectValueMultiplier(GetSpellProto(), GetEffIndex(), caster);

            uint32 heal = uint32(caster->SpellHealingBonus(caster, GetSpellProto(), uint32(new_damage * gainMultiplier), DOT, GetBase()->GetStackAmount()));

            int32 gain = caster->HealBySpell(caster, GetSpellProto(), heal);
            caster->getHostileRefManager().threatAssist(caster, gain * 0.5f, GetSpellProto());
            break;
        }
        case SPELL_AURA_PERIODIC_HEALTH_FUNNEL: // only three spells
        {
            if (!caster || !caster->GetHealth())
                break;

            if (!target->isAlive())
                return;

            if (target->HasUnitState(UNIT_STAT_ISOLATED))
            {
                SendTickImmune(target, caster);
                return;
            }

            uint32 damage = GetAmount();
            // do not kill health donator
            if (caster->GetHealth() < damage)
                damage = caster->GetHealth() - 1;
            if (!damage)
                break;

            //donator->SendSpellNonMeleeDamageLog(donator, GetId(), damage, GetSpellSchoolMask(spellProto), 0, 0, false, 0);
            caster->ModifyHealth(-(int32)damage);
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "PeriodicTick: donator %u target %u damage %u.", target->GetEntry(), target->GetEntry(), damage);

            float gainMultiplier = SpellMgr::CalculateSpellEffectValueMultiplier(GetSpellProto(), GetEffIndex(), caster);

            damage = int32(damage * gainMultiplier);

            caster->HealBySpell(target, GetSpellProto(), damage);
            break;
        }
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_OBS_MOD_HEALTH:
        {
            if (!caster)
                break;

            if (!target->isAlive())
                return;

            if (target->HasUnitState(UNIT_STAT_ISOLATED))
            {
                SendTickImmune(target, caster);
                return;
            }

            // heal for caster damage (must be alive)
            if (target != caster && GetSpellProto()->AttributesEx2 & SPELL_ATTR2_HEALTH_FUNNEL && !caster->isAlive())
                break;

            if (GetBase()->GetDuration() == -1 && target->IsFullHealth())
                break;

            // ignore non positive values (can be result apply spellmods to aura damage
            int32 damage = m_amount > 0 ? m_amount : 0;

            if (GetAuraType() == SPELL_AURA_OBS_MOD_HEALTH)
            {
                // Taken mods
                float TakenTotalMod = 1.0f;

                // Tenacity increase healing % taken
                if (AuraEffect const* Tenacity = target->GetAuraEffect(58549, 0))
                    AddPctN(TakenTotalMod, Tenacity->GetAmount());

                // Healing taken percent
                float minval = (float)target->GetMaxNegativeAuraModifier(SPELL_AURA_MOD_HEALING_PCT);
                if (minval)
                    AddPctF(TakenTotalMod, minval);

                float maxval = (float)target->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HEALING_PCT);
                if (maxval)
                    AddPctF(TakenTotalMod, maxval);

                // Healing over time taken percent
                float minval_hot = (float)target->GetMaxNegativeAuraModifier(SPELL_AURA_MOD_HOT_PCT);
                if (minval_hot)
                    AddPctF(TakenTotalMod, minval_hot);

                float maxval_hot = (float)target->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HOT_PCT);
                if (maxval_hot)
                    AddPctF(TakenTotalMod, maxval_hot);

                TakenTotalMod = std::max(TakenTotalMod, 0.0f);

                damage = uint32(target->CountPctFromMaxHealth(damage));
                damage = uint32(damage * TakenTotalMod);
            }
            else
            {
                // Wild Growth = amount + (6 - 2*doneTicks) * ticks* amount / 100
                if (m_spellProto->GetSpellFamilyName() == SPELLFAMILY_DRUID && m_spellProto->SpellIconID == 2864)
                {
                    damage += int32(float(damage * GetTotalTicks()) * ((6-float(2*(GetTickNumber()-1)))/100));
                }

                damage = caster->SpellHealingBonus(target, GetSpellProto(), damage, DOT, GetBase()->GetStackAmount());
            }

            bool crit = IsPeriodicTickCrit(target, caster);
            if (crit)
                damage = caster->SpellCriticalHealingBonus(m_spellProto, damage, target);

            sLog->outDetail("PeriodicTick: %u (TypeId: %u) heal of %u (TypeId: %u) for %u health inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), damage, GetId());

            uint32 absorb = 0;
            uint32 heal = uint32(damage);
            caster->CalcHealAbsorb(target, GetSpellProto(), heal, absorb);
            int32 gain = caster->DealHeal(target, heal);

            SpellPeriodicAuraLogInfo pInfo(this, damage, damage - gain, absorb, 0, 0.0f, crit);
            target->SendPeriodicAuraLog(&pInfo);

            target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f, GetSpellProto());

            bool haveCastItem = GetBase()->GetCastItemGUID() != 0;

            // Health Funnel
            // damage caster for heal amount
            if (target != caster && GetSpellProto()->AttributesEx2 & SPELL_ATTR2_HEALTH_FUNNEL)
            {
                uint32 damage = SpellMgr::CalculateSpellEffectAmount(GetSpellProto(), 0); // damage is not affected by spell power
                if ((int32)damage > gain)
                    damage = gain;
                uint32 absorb = 0;
                caster->DealDamageMods(caster, damage, &absorb);
                caster->SendSpellNonMeleeDamageLog(caster, GetId(), damage, GetSpellSchoolMask(GetSpellProto()), absorb, 0, false, 0, false);

                CleanDamage cleanDamage =  CleanDamage(0, 0, BASE_ATTACK, MELEE_HIT_NORMAL);
                caster->DealDamage(caster, damage, &cleanDamage, NODAMAGE, GetSpellSchoolMask(GetSpellProto()), GetSpellProto(), true);
            }

            uint32 procAttacker = PROC_FLAG_DONE_PERIODIC;
            uint32 procVictim   = PROC_FLAG_TAKEN_PERIODIC;
            uint32 procEx = (crit ? PROC_EX_CRITICAL_HIT : PROC_EX_NORMAL_HIT) | PROC_EX_INTERNAL_HOT;
            // ignore item heals
            if (!haveCastItem)
                caster->ProcDamageAndSpell(target, procAttacker, procVictim, procEx, damage, BASE_ATTACK, GetSpellProto());
            break;
        }
        case SPELL_AURA_PERIODIC_MANA_LEECH:
        {
            if (GetMiscValue() < 0 || GetMiscValue() >= int8(MAX_POWERS))
                break;

            if (!target->isAlive())
                return;

            if (target->HasUnitState(UNIT_STAT_ISOLATED))
            {
                SendTickImmune(target, caster);
                return;
            }

            Powers power = Powers(GetMiscValue());

            // power type might have changed between aura applying and tick (druid's shapeshift)
            if (target->getPowerType() != power)
                break;

            if (!caster || !caster->isAlive())
                break;

            if (GetSpellProto()->GetSpellEffectIdByIndex(GetEffIndex()) == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                caster->SpellHitResult(target, GetSpellProto(), false) != SPELL_MISS_NONE)
                break;

            // Check for immune (not use charges)
            if (target->IsImmunedToDamage(GetSpellProto()))
            {
                SendTickImmune(target, caster);
                break;
            }

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 damage = m_amount > 0 ? m_amount : 0;

            // Special case: draining x% of mana (up to a maximum of 2*x% of the caster's maximum mana)
            // It's mana percent cost spells, m_amount is percent drain from target
            if (m_spellProto->GetManaCostPercentage())
            {
                // max value
                uint32 maxmana = CalculatePctF(caster->GetMaxPower(power), damage * 2.0f);
                ApplyPctU(damage, target->GetMaxPower(power));
                if (damage > maxmana)
                    damage = maxmana;
            }

            sLog->outDetail("PeriodicTick: %u (TypeId: %u) power leech of %u (TypeId: %u) for %u dmg inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), damage, GetId());

            int32 drain_amount = std::min(target->GetPower(power), damage);

            // resilience reduce mana draining effect at spell crit damage reduction (added in 2.4)
            if (power == POWER_MANA)
                drain_amount -= target->GetSpellCritDamageReduction(drain_amount);

            target->ModifyPower(power, -drain_amount);

            float gain_multiplier = 0.0f;

            if (caster->GetMaxPower(power) > 0)
                gain_multiplier = SpellMgr::CalculateSpellEffectValueMultiplier(GetSpellProto(), GetEffIndex(), caster);

            SpellPeriodicAuraLogInfo pInfo(this, drain_amount, 0, 0, 0, gain_multiplier, false);
            target->SendPeriodicAuraLog(&pInfo);

            int32 gain_amount = int32(drain_amount * gain_multiplier);

            if (gain_amount)
            {
                int32 gain = caster->ModifyPower(power, gain_amount);
                target->AddThreat(caster, float(gain) * 0.5f, GetSpellSchoolMask(GetSpellProto()), GetSpellProto());
            }

            switch(GetId())
            {
                case 31447: // Mark of Kaz'rogal
                    if (target->GetPower(power) == 0)
                    {
                        target->CastSpell(target, 31463, true, 0, this);
                        // Remove aura
                        GetBase()->SetDuration(0);
                    }
                    break;

                case 32960: // Mark of Kazzak
                    int32 modifier = int32(target->GetPower(power) * 0.05f);
                    target->ModifyPower(power, -modifier);

                    if (target->GetPower(power) == 0)
                    {
                        target->CastSpell(target, 32961, true, 0, this);
                        // Remove aura
                        GetBase()->SetDuration(0);
                    }
            }
            // Drain Mana
            if (m_spellProto->GetSpellFamilyName() == SPELLFAMILY_WARLOCK
                && m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x00000010)
            {
                int32 manaFeedVal = 0;
                if (AuraEffect const * aurEff = GetBase()->GetEffect(1))
                    manaFeedVal = aurEff->GetAmount();
                // Mana Feed - Drain Mana
                if (manaFeedVal > 0)
                {
                    ApplyPctN(manaFeedVal, gain_amount);
                    caster->CastCustomSpell(caster, 32554, &manaFeedVal, NULL, NULL, true, NULL, this);
                }
            }
            break;
        }
        case SPELL_AURA_OBS_MOD_POWER:
        {
            if (GetMiscValue() < 0)
                return;

            if (!target->isAlive())
                return;

            if (target->HasUnitState(UNIT_STAT_ISOLATED))
            {
                SendTickImmune(target, caster);
                return;
            }

            Powers power;
            if (GetMiscValue() == POWER_ALL)
                power = target->getPowerType();
            else
                power = Powers(GetMiscValue());

            if (target->GetMaxPower(power) == 0)
                return;

            if (GetBase()->GetDuration() == -1 && target->GetPower(power) == target->GetMaxPower(power))
                return;

            uint32 amount = m_amount * target->GetMaxPower(power) /100;
            sLog->outDetail("PeriodicTick: %u (TypeId: %u) energize %u (TypeId: %u) for %u dmg inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), amount, GetId());

            SpellPeriodicAuraLogInfo pInfo(this, amount, 0, 0, 0, 0.0f, false);
            target->SendPeriodicAuraLog(&pInfo);

            int32 gain = target->ModifyPower(power, amount);

            if (caster)
                target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f, GetSpellProto());
            break;
        }
        case SPELL_AURA_PERIODIC_ENERGIZE:
        {
            // ignore non positive values (can be result apply spellmods to aura damage
            if (m_amount < 0 || GetMiscValue() >= int8(MAX_POWERS))
                return;

            if (!target->isAlive())
                return;

            if (target->HasUnitState(UNIT_STAT_ISOLATED))
            {
                SendTickImmune(target, caster);
                return;
            }

            Powers power = Powers(GetMiscValue());

            if (target->GetMaxPower(power) == 0)
                return;

            if (GetBase()->GetDuration() == -1 && target->GetPower(power) == target->GetMaxPower(power))
                return;

            uint32 amount = m_amount;

            SpellPeriodicAuraLogInfo pInfo(this, amount, 0, 0, 0, 0.0f, false);
            target->SendPeriodicAuraLog(&pInfo);

            sLog->outDetail("PeriodicTick: %u (TypeId: %u) energize %u (TypeId: %u) for %u dmg inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), amount, GetId());

            int32 gain = target->ModifyPower(power, amount);

            if (caster)
                target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f, GetSpellProto());
            break;
        }
        case SPELL_AURA_POWER_BURN_MANA:
        {
            if (!caster)
                return;

            if (!target->isAlive())
                return;

            if (target->HasUnitState(UNIT_STAT_ISOLATED))
            {
                SendTickImmune(target, caster);
                return;
            }

            // Check for immune (not use charges)
            if (target->IsImmunedToDamage(GetSpellProto()))
            {
                SendTickImmune(target, caster);
                return;
            }

            int32 damage = m_amount > 0 ? m_amount : 0;

            Powers powerType = Powers(GetMiscValue());

            if (!target->isAlive() || target->getPowerType() != powerType)
                return;

            // resilience reduce mana draining effect at spell crit damage reduction (added in 2.4)
            if (powerType == POWER_MANA)
                damage -= target->GetSpellCritDamageReduction(damage);

            uint32 gain = uint32(-target->ModifyPower(powerType, -damage));

            float dmgMultiplier = SpellMgr::CalculateSpellEffectValueMultiplier(GetSpellProto(), GetEffIndex(), caster);

            SpellEntry const* spellProto = GetSpellProto();
            //maybe has to be sent different to client, but not by SMSG_PERIODICAURALOG
            SpellNonMeleeDamage damageInfo(caster, target, spellProto->Id, spellProto->SchoolMask);
            //no SpellDamageBonus for burn mana
            caster->CalculateSpellDamageTaken(&damageInfo, int32(gain * dmgMultiplier), spellProto);

            caster->DealDamageMods(damageInfo.target, damageInfo.damage, &damageInfo.absorb);

            caster->SendSpellNonMeleeDamageLog(&damageInfo);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_DONE_PERIODIC;
            uint32 procVictim   = PROC_FLAG_TAKEN_PERIODIC;
            uint32 procEx       = createProcExtendMask(&damageInfo, SPELL_MISS_NONE) | PROC_EX_INTERNAL_DOT;
            if (damageInfo.damage)
                procVictim|=PROC_FLAG_TAKEN_DAMAGE;

            caster->ProcDamageAndSpell(damageInfo.target, procAttacker, procVictim, procEx, damageInfo.damage, BASE_ATTACK, spellProto);

            caster->DealSpellDamage(&damageInfo, true);
            break;
        }
        case SPELL_AURA_DUMMY:
            // Haunting Spirits
            if (GetId() == 7057)
                target->CastSpell((Unit*)NULL , GetAmount() , true);
            break;
        case SPELL_AURA_PERIODIC_DUMMY:
            PeriodicDummyTick(target, caster);
            break;
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
            TriggerSpell(target, caster);
            break;
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
            TriggerSpellWithValue(target, caster);
            break;
        default:
            break;
    }
}

void AuraEffect::PeriodicDummyTick(Unit * target, Unit * caster) const
{
    switch (GetSpellProto()->GetSpellFamilyName())
    {
        case SPELLFAMILY_GENERIC:
        switch (GetId())
        {
            // Forsaken Skills
            case 7054:
            {
                // Possibly need cast one of them (but
                // 7038 Forsaken Skill: Swords
                // 7039 Forsaken Skill: Axes
                // 7040 Forsaken Skill: Daggers
                // 7041 Forsaken Skill: Maces
                // 7042 Forsaken Skill: Staves
                // 7043 Forsaken Skill: Bows
                // 7044 Forsaken Skill: Guns
                // 7045 Forsaken Skill: 2H Axes
                // 7046 Forsaken Skill: 2H Maces
                // 7047 Forsaken Skill: 2H Swords
                // 7048 Forsaken Skill: Defense
                // 7049 Forsaken Skill: Fire
                // 7050 Forsaken Skill: Frost
                // 7051 Forsaken Skill: Holy
                // 7053 Forsaken Skill: Shadow
                return;
            }
            case 54798: // FLAMING Arrow Triggered Effect
            {
                if (!target || !target->ToCreature() || !caster->IsVehicle())
                    return;

                Unit *rider = caster->GetVehicleKit()->GetPassenger(0);
                if (!rider)
                    return;

                // set ablaze
                if (target->HasAuraEffect(54683, EFFECT_0))
                    return;
                else
                    target->CastSpell(target, 54683, true);

                // Credit Frostworgs
                if (target->GetEntry() == 29358)
                    rider->CastSpell(rider, 54896, true);
                // Credit Frost Giants
                else if (target->GetEntry() == 29351)
                    rider->CastSpell(rider, 54893, true);

                break;
            }
            case 62292: // Blaze (Pool of Tar)
                // should we use custom damage?
                target->CastSpell((Unit*)NULL, m_spellProto->GetEffectTriggerSpell(m_effIndex), true);
                break;
            case 62399: // Overload Circuit
                if (target->GetMap()->IsDungeon() && int(target->GetAppliedAuras().count(62399)) >= (target->GetMap()->IsHeroic() ? 4 : 2))
                {
                     target->CastSpell(target, 62475, true); // System Shutdown
                     if (Unit *veh = target->GetVehicleBase())
                         veh->CastSpell(target, 62475, true);
                }
                break;
            case 64821: // Fuse Armor (Razorscale)
                if (GetBase()->GetStackAmount() == GetSpellProto()->GetStackAmount())
                {
                    target->CastSpell(target, 64774, true, NULL, NULL, GetCasterGUID());
                    target->RemoveAura(64821);
                }
                break;
        }
        break;
        case SPELLFAMILY_MAGE:
        {
            // Mirror Image
            if (GetId() == 55342)
                // Set name of summons to name of caster
                target->CastSpell((Unit *)NULL, m_spellProto->GetEffectTriggerSpell(m_effIndex), true);
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            switch (GetSpellProto()->Id)
            {
                // Demonic Circle
                case 48018:
                    if (GameObject* obj = target->GetGameObject(GetSpellProto()->Id))
                    {
                        if (target->IsWithinDist(obj, GetSpellMaxRange(48020, true)))
                        {
                            if (!target->HasAura(62388))
                                target->CastSpell(target, 62388, true);
                        }
                        else
                            target->RemoveAura(62388);
                    }
                    break;
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            switch (GetSpellProto()->Id)
            {
                // Frenzied Regeneration
                case 22842:
                {
                    // Converts up to 10 rage per second into health for $d.  Each point of rage is converted into ${$m2/10}.1% of max health.
                    // Should be manauser
                    if (target->getPowerType() != POWER_RAGE)
                        break;
                    uint32 rage = target->GetPower(POWER_RAGE);
                    // Nothing todo
                    if (rage == 0)
                        break;
                    int32 mod = (rage < 100) ? rage : 100;
                    int32 points = target->CalculateSpellDamage(target, GetSpellProto(), 1);
                    int32 regen = target->GetMaxHealth() * (mod * points / 10) / 1000;
                    target->CastCustomSpell(target, 22845, &regen, 0, 0, true, 0, this);
                    target->SetPower(POWER_RAGE, rage-mod);
                    break;
                }
                // Force of Nature
                case 33831:
                    break;
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            switch (GetSpellProto()->Id)
            {
                // Master of Subtlety
                case 31666:
                    if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                        target->RemoveAurasDueToSpell(31665);
                    break;
                // Killing Spree
                case 51690:
                {
                    // TODO: this should use effect[1] of 51690
                    UnitList targets;
                    {
                        // eff_radius == 0
                        float radius = GetSpellMaxRange(GetSpellProto(), false);

                        CellPair p(Trinity::ComputeCellPair(caster->GetPositionX(), caster->GetPositionY()));
                        Cell cell(p);
                        cell.data.Part.reserved = ALL_DISTRICT;

                        Trinity::AnyUnfriendlyVisibleUnitInObjectRangeCheck u_check(caster, caster, radius);
                        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyVisibleUnitInObjectRangeCheck> checker(caster, targets, u_check);

                        TypeContainerVisitor<Trinity::UnitListSearcher<Trinity::AnyUnfriendlyVisibleUnitInObjectRangeCheck>, GridTypeMapContainer > grid_object_checker(checker);
                        TypeContainerVisitor<Trinity::UnitListSearcher<Trinity::AnyUnfriendlyVisibleUnitInObjectRangeCheck>, WorldTypeMapContainer > world_object_checker(checker);

                        cell.Visit(p, grid_object_checker,  *GetBase()->GetOwner()->GetMap(), *caster, radius);
                        cell.Visit(p, world_object_checker, *GetBase()->GetOwner()->GetMap(), *caster, radius);
                    }

                    if (targets.empty())
                        return;

                    UnitList::const_iterator itr = targets.begin();
                    std::advance(itr, rand()%targets.size());
                    Unit* spellTarget = *itr;

                    target->CastSpell(spellTarget, 57840, true);
                    target->CastSpell(spellTarget, 57841, true);
                    break;
                }
                // Overkill
                case 58428:
                    if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                        target->RemoveAurasDueToSpell(58427);
                    break;
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Explosive Shot
            if (GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[1] & 0x80000000)
            {
                if (caster)
                    caster->CastCustomSpell(53352, SPELLVALUE_BASE_POINT0, m_amount, target, true, NULL, this);
                break;
            }
            switch (GetSpellProto()->Id)
            {
                // Feeding Frenzy Rank 1
                case 53511:
                    if (target->getVictim() && target->getVictim()->HealthBelowPct(35))
                        target->CastSpell(target, 60096, true, 0, this);
                    return;
                // Feeding Frenzy Rank 2
                case 53512:
                    if (target->getVictim() && target->getVictim()->HealthBelowPct(35))
                        target->CastSpell(target, 60097, true, 0, this);
                    return;
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
            if (GetId() == 52179) // Astral Shift
            {
                // Periodic need for remove visual on stun/fear/silence lost
                if (!(target->GetUInt32Value(UNIT_FIELD_FLAGS)&(UNIT_FLAG_STUNNED|UNIT_FLAG_FLEEING|UNIT_FLAG_SILENCED)))
                    target->RemoveAurasDueToSpell(52179);
                break;
            }
            break;
        case SPELLFAMILY_DEATHKNIGHT:
            switch (GetId())
            {
                case 49016: // Hysteria
                    uint32 damage = uint32(target->CountPctFromMaxHealth(1));
                    target->DealDamage(target, damage, NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                    break;
            }
            // Death and Decay
            if (GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x20)
            {
                if (caster)
                    caster->CastCustomSpell(target, 52212, &m_amount, NULL, NULL, true, 0, this);
                break;
            }
            // Blood of the North
            // Reaping
            // Death Rune Mastery
            if (GetSpellProto()->SpellIconID == 3041 || GetSpellProto()->SpellIconID == 22 || GetSpellProto()->SpellIconID == 2622)
            {
                if (target->GetTypeId() != TYPEID_PLAYER)
                    return;
                if (target->ToPlayer()->getClass() != CLASS_DEATH_KNIGHT)
                    return;

                 // timer expired - remove death runes
                target->ToPlayer()->RemoveRunesByAuraEffect(this);
            }
            break;
        default:
            break;
    }
}

Unit* AuraEffect::GetTriggerTarget(Unit * target) const
{
    if (target->GetTypeId() == TYPEID_UNIT)
    {
        if (Unit * trigger = target->ToCreature()->AI()->GetAuraEffectTriggerTarget(GetId(), GetEffIndex()))
            return trigger;
    }
    return target;
}

void AuraEffect::TriggerSpell(Unit * target, Unit * caster) const
{
    if (!caster || !target)
        return;

    Unit* triggerTarget = GetTriggerTarget(target);

    // generic casting code with custom spells and target/caster customs
    uint32 triggerSpellId = GetSpellProto()->GetEffectTriggerSpell(GetEffIndex());

    SpellEntry const *triggeredSpellInfo = sSpellStore.LookupEntry(triggerSpellId);
    SpellEntry const *auraSpellInfo = GetSpellProto();
    uint32 auraId = auraSpellInfo->Id;

    // specific code for cases with no trigger spell provided in field
    if (triggeredSpellInfo == NULL)
    {
        switch (auraSpellInfo->GetSpellFamilyName())
        {
            case SPELLFAMILY_GENERIC:
            {
                switch (auraId)
                {
                    // Thaumaturgy Channel
                    case 9712:
                        triggerSpellId = 21029;
                        break;
                    // Brood Affliction: Bronze
                    case 23170:
                        triggerSpellId = 23171;
                        return;
                    // Restoration
                    case 24379:
                    case 23493:
                    {
                        int32 heal = caster->CountPctFromMaxHealth(10);
                        caster->HealBySpell(target, auraSpellInfo, heal);

                        int32 mana = caster->GetMaxPower(POWER_MANA);
                        if (mana)
                        {
                            mana /= 10;
                            caster->EnergizeBySpell(caster, 23493, mana, POWER_MANA);
                        }
                        return;
                    }
                    // Nitrous Boost
                    case 27746:
                        if (target->GetPower(POWER_MANA) >= 10)
                        {
                            target->ModifyPower(POWER_MANA, -10);
                            target->SendEnergizeSpellLog(caster, 27746, 10, POWER_MANA);
                        }
                        else
                            target->RemoveAurasDueToSpell(27746);
                        return;
                    // Frost Blast
                    case 27808:
                        caster->CastCustomSpell(29879, SPELLVALUE_BASE_POINT0, int32(target->CountPctFromMaxHealth(21)), target, true, NULL, this);
                        return;
                    // Detonate Mana
                    case 27819:
                        if (int32 mana = (int32)(target->GetMaxPower(POWER_MANA) / 10))
                        {
                            mana = target->ModifyPower(POWER_MANA, -mana);
                            target->CastCustomSpell(27820, SPELLVALUE_BASE_POINT0, -mana*10, target, true, NULL, this);
                        }
                        return;
                    // Inoculate Nestlewood Owlkin
                    case 29528:
                        if (triggerTarget->GetTypeId() != TYPEID_UNIT)// prevent error reports in case ignored player target
                            return;
                        break;
                    // Feed Captured Animal
                    case 29917:
                        triggerSpellId = 29916;
                        break;
                    // Extract Gas
                    case 30427:
                    {
                        // move loot to player inventory and despawn target
                        if (caster->GetTypeId() == TYPEID_PLAYER &&
                                triggerTarget->GetTypeId() == TYPEID_UNIT &&
                                triggerTarget->ToCreature()->GetCreatureInfo()->type == CREATURE_TYPE_GAS_CLOUD)
                        {
                            Player* player = (Player*)caster;
                            Creature* creature = triggerTarget->ToCreature();
                            // missing lootid has been reported on startup - just return
                            if (!creature->GetCreatureInfo()->SkinLootId)
                                return;

                            player->AutoStoreLoot(creature->GetCreatureInfo()->SkinLootId, LootTemplates_Skinning, true);

                            creature->DespawnOrUnsummon();
                        }
                        return;
                    }
                    // Quake
                    case 30576: triggerSpellId = 30571; break;
                    // Doom
                    // TODO: effect trigger spell may be independant on spell targets, and executed in spell finish phase
                    // so instakill will be naturally done before trigger spell
                    case 31347:
                    {
                        target->CastSpell(target, 31350, true, NULL, this);
                        target->Kill(target);
                        return;
                    }
                    // Spellcloth
                    case 31373:
                    {
                        // Summon Elemental after create item
                        target->SummonCreature(17870, 0, 0, 0, target->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
                        return;
                    }
                    // Flame Quills
                    case 34229:
                    {
                        // cast 24 spells 34269-34289, 34314-34316
                        for (uint32 spell_id = 34269; spell_id != 34290; ++spell_id)
                            caster->CastSpell(target, spell_id, true, NULL, this);
                        for (uint32 spell_id = 34314; spell_id != 34317; ++spell_id)
                            caster->CastSpell(target, spell_id, true, NULL, this);
                        return;
                    }
                    // Remote Toy
                    case 37027: triggerSpellId = 37029; break;
                    // Eye of Grillok
                    case 38495:
                    {
                        target->CastSpell(target, 38530, true, NULL, this);
                        return;
                    }
                    // Absorb Eye of Grillok (Zezzak's Shard)
                    case 38554:
                    {
                        if (target->GetTypeId() != TYPEID_UNIT)
                            return;

                        caster->CastSpell(caster, 38495, true, NULL, this);

                        Creature* creatureTarget = target->ToCreature();

                        creatureTarget->DespawnOrUnsummon();
                        return;
                    }
                    // Tear of Azzinoth Summon Channel - it's not really supposed to do anything, and this only prevents the console spam
                    case 39857:
                        triggerSpellId = 39856;
                        break;
                    // Personalized Weather
                    case 46736:
                        triggerSpellId = 46737;
                        break;
                    // Shield Level 1
                    case 63130:
                    // Shield Level 2
                    case 63131:
                    // Shield Level 3
                    case 63132:
                    // Ball of Flames Visual
                    case 71706:
                        return;
                }
                break;
            }
            case SPELLFAMILY_MAGE:
            {
                switch(auraId)
                {
                    // Invisibility
                    case 66:
                    // Here need periodic triger reducing threat spell (or do it manually)
                        return;
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                switch(auraId)
                {
                    // Cat Form
                    // triggerSpellId not set and unknown effect triggered in this case, ignoring for while
                    case 768:
                        return;
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                switch(auraId)
                {
                    // Lightning Shield (The Earthshatterer set trigger after cast Lighting Shield)
                    case 28820:
                    {
                        // Need remove self if Lightning Shield not active
                        if (!target->GetAuraEffect(SPELL_AURA_PROC_TRIGGER_SPELL, SPELLFAMILY_SHAMAN, 0x400))
                            target->RemoveAurasDueToSpell(28820);
                        return;
                    }
                    // Totemic Mastery (Skyshatter Regalia (Shaman Tier 6) - bonus)
                    case 38443:
                    {
                        bool all = true;
                        for (int i = SUMMON_SLOT_TOTEM; i < MAX_TOTEM_SLOT; ++i)
                        {
                            if (!target->m_SummonSlot[i])
                            {
                                all = false;
                                break;
                            }
                        }

                        if (all)
                            caster->CastSpell(target, 38437, true, NULL, this);
                        else
                            target->RemoveAurasDueToSpell(38437);
                        return;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    else
    {
        // Spell exist but require custom code
        switch (auraId)
        {
            // Pursuing Spikes (Anub'arak)
            case 65920:
            case 65922:
            case 65923:
                {
                    Unit *permafrostCaster = NULL;
                    if (caster->HasAura(66193)) permafrostCaster = caster->GetAura(66193)->GetCaster();
                    if (caster->HasAura(67855)) permafrostCaster = caster->GetAura(67855)->GetCaster();
                    if (caster->HasAura(67856)) permafrostCaster = caster->GetAura(67856)->GetCaster();
                    if (caster->HasAura(67857)) permafrostCaster = caster->GetAura(67857)->GetCaster();

                    if (permafrostCaster)
                    {
                        if (Creature *permafrostCasterAsCreature = permafrostCaster->ToCreature())
                            permafrostCasterAsCreature->DespawnOrUnsummon(3000);

                        caster->CastSpell(caster, 66181, false);
                        caster->RemoveAllAuras();
                        if (Creature *casterAsCreature = caster->ToCreature())
                            casterAsCreature->DisappearAndDie();
                    }
                }
                break;
            // Mana Tide
            case 16191:
                target->CastCustomSpell(triggerTarget, triggerSpellId, &m_amount, NULL, NULL, true, NULL, this);
                return;
            // Negative Energy Periodic
            case 46284:
                caster->CastCustomSpell(triggerSpellId, SPELLVALUE_MAX_TARGETS, m_tickNumber / 10 + 1, NULL, true, NULL, this);
                return;
            // Poison (Grobbulus)
            case 28158:
            case 54362:
            // Slime Pool (Dreadscale & Acidmaw)
            case 66882:
                target->CastCustomSpell(triggerSpellId, SPELLVALUE_RADIUS_MOD, (int32)((((float)m_tickNumber / 60) * 0.9f + 0.1f) * 10000 * 2 / 3), NULL, true, NULL, this);
                return;
            // Beacon of Light
            case 53563:
            {
                Unit * triggerCaster = (Unit *)(GetBase()->GetOwner());
                triggerCaster->CastSpell(triggerTarget, triggeredSpellInfo, true, 0, this, triggerCaster->GetGUID());
                return;
            }
            // Slime Spray - temporary here until preventing default effect works again
            // added on 9.10.2010
            case 69508:
            {
                caster->CastSpell(triggerTarget, triggerSpellId, true, NULL, NULL, caster->GetGUID());
                return;
            }
        }
    }

    // Reget trigger spell proto
    triggeredSpellInfo = sSpellStore.LookupEntry(triggerSpellId);

    if (triggeredSpellInfo)
    {
        Unit * triggerCaster = GetTriggeredSpellCaster(triggeredSpellInfo, caster, triggerTarget);
        triggerCaster->CastSpell(triggerTarget, triggeredSpellInfo, true, NULL, this);
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AuraEffect::TriggerSpell: Spell %u Trigger %u", GetId(), triggeredSpellInfo->Id);
    }
    /*else
    {
        Creature* c = triggerTarget->ToCreature();
        if (!c || (c && !sScriptMgr->OnDummyEffect(caster, GetId(), SpellEffIndex(GetEffIndex()), triggerTarget->ToCreature())) ||
            (c && !c->AI()->sOnDummyEffect(caster, GetId(), SpellEffIndex(GetEffIndex()))))
            sLog->outError("AuraEffect::TriggerSpell: Spell %u has value 0 in EffectTriggered[%d] and is therefor not handled. Define as custom case?", GetId(), GetEffIndex());
    }*/
}

void AuraEffect::TriggerSpellWithValue(Unit * target, Unit * caster) const
{
    if (!caster || !target)
        return;

    Unit* triggerTarget = GetTriggerTarget(target);

    uint32 triggerSpellId = GetSpellProto()->GetEffectTriggerSpell(m_effIndex);
    SpellEntry const *triggeredSpellInfo = sSpellStore.LookupEntry(triggerSpellId);
    if (triggeredSpellInfo)
    {
        Unit * triggerCaster = GetTriggeredSpellCaster(triggeredSpellInfo, caster, triggerTarget);

        // generic casting code with custom spells and target/caster customs
        int32  basepoints0 = GetAmount();
        triggerCaster->CastCustomSpell(triggerTarget, triggerSpellId, &basepoints0, 0, 0, true, 0, this);
    }
    else
        sLog->outError("AuraEffect::TriggerSpellWithValue: Spell %u has value 0 in EffectTriggered[%d] and is therefor not handled. Define as custom case?", GetId(), GetEffIndex());
}

bool AuraEffect::IsAffectedOnSpell(SpellEntry const *spell) const
{
    if (!spell)
        return false;
    // Check family name
    if (spell->GetSpellFamilyName() != m_spellProto->GetSpellFamilyName())
        return false;

    // Check EffectClassMask
    if (m_spellProto->GetEffectSpellClassMask(m_effIndex) && spell->GetSpellFamilyFlags())
        return true;
    return false;
}

void AuraEffect::CleanupTriggeredSpells(Unit * target)
{
    uint32 tSpellId = m_spellProto->GetEffectTriggerSpell(GetEffIndex());
    if (!tSpellId)
        return;

    SpellEntry const* tProto = sSpellStore.LookupEntry(tSpellId);
    if (!tProto)
        return;

    if (GetSpellDuration(tProto) != -1)
        return;

    // needed for spell 43680, maybe others
    // TODO: is there a spell flag, which can solve this in a more sophisticated way?
    if (m_spellProto->GetEffectApplyAuraNameByIndex(GetEffIndex()) == SPELL_AURA_PERIODIC_TRIGGER_SPELL &&
        uint32(GetSpellDuration(m_spellProto)) == m_spellProto->GetEffectAmplitude(GetEffIndex()))
        return;

    target->RemoveAurasDueToSpell(tSpellId, GetCasterGUID());
}

void AuraEffect::HandleShapeshiftBoosts(Unit * target, bool apply) const
{
    uint32 spellId = 0;
    uint32 spellId2 = 0;
    //uint32 spellId3 = 0;
    uint32 HotWSpellId = 0;

    switch(GetMiscValue())
    {
        case FORM_CAT:
            spellId = 3025;
            HotWSpellId = 24900;
            break;
        case FORM_TREE:
            spellId = 34123;
            break;
        case FORM_TRAVEL:
            spellId = 5419;
            break;
        case FORM_AQUA:
            spellId = 5421;
            break;
        case FORM_BEAR:
            spellId = 1178;
            spellId2 = 21178;
            HotWSpellId = 24899;
            break;
        case FORM_DIREBEAR:
            spellId = 9635;
            spellId2 = 21178;
            HotWSpellId = 24899;
            break;
        case FORM_BATTLESTANCE:
            spellId = 21156;
            break;
        case FORM_DEFENSIVESTANCE:
            spellId = 7376;
            break;
        case FORM_BERSERKERSTANCE:
            spellId = 7381;
            break;
        case FORM_MOONKIN:
            spellId = 24905;
            spellId2 = 69366;
            break;
        case FORM_FLIGHT:
            spellId = 33948;
            spellId2 = 34764;
            break;
        case FORM_FLIGHT_EPIC:
            spellId  = 40122;
            spellId2 = 40121;
            break;
        case FORM_METAMORPHOSIS:
            spellId  = 54817;
            spellId2 = 54879;
            break;
        case FORM_SPIRITOFREDEMPTION:
            spellId  = 27792;
            spellId2 = 27795;                               // must be second, this important at aura remove to prevent to early iterator invalidation.
            break;
        case FORM_SHADOW:
            spellId = 49868;
            spellId2 = 71167;
            break;
        case FORM_GHOSTWOLF:
            spellId = 67116;
            break;
        case FORM_GHOUL:
        case FORM_AMBIENT:
        case FORM_STEALTH:
        case FORM_CREATURECAT:
        case FORM_CREATUREBEAR:
            break;
        default:
            break;
    }

    if (apply)
    {
        // Remove cooldown of spells triggered on stance change - they may share cooldown with stance spell
        if (spellId)
        {
            if (target->GetTypeId() == TYPEID_PLAYER)
                target->ToPlayer()->RemoveSpellCooldown(spellId);
            target->CastSpell(target, spellId, true, NULL, this);
        }

        if (spellId2)
        {
            if (target->GetTypeId() == TYPEID_PLAYER)
                target->ToPlayer()->RemoveSpellCooldown(spellId2);
            target->CastSpell(target, spellId2, true, NULL, this);
        }

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            const PlayerSpellMap& sp_list = target->ToPlayer()->GetSpellMap();
            for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
            {
                if (itr->second->state == PLAYERSPELL_REMOVED || itr->second->disabled) continue;
                if (itr->first == spellId || itr->first == spellId2) continue;
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                if (!spellInfo || !(spellInfo->Attributes & (SPELL_ATTR0_PASSIVE | SPELL_ATTR0_UNK7))) continue;
                if (spellInfo->GetStances() & (1<<(GetMiscValue()-1)))
                    target->CastSpell(target, itr->first, true, NULL, this);
            }
            // Leader of the Pack
            if (target->ToPlayer()->HasSpell(17007))
            {
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(24932);
                if (spellInfo && spellInfo->GetStances() & (1<<(GetMiscValue()-1)))
                    target->CastSpell(target, 24932, true, NULL, this);
            }
            // Improved Barkskin - apply/remove armor bonus due to shapeshift
            if (target->ToPlayer()->HasSpell(63410) || target->ToPlayer()->HasSpell(63411))
            {
                target->RemoveAurasDueToSpell(66530);
                if (GetMiscValue() == FORM_TRAVEL || GetMiscValue() == FORM_NONE) // "while in Travel Form or while not shapeshifted"
                    target->CastSpell(target, 66530, true);
            }
            // Heart of the Wild
            if (HotWSpellId)
            {   // hacky, but the only way as spell family is not SPELLFAMILY_DRUID
                Unit::AuraEffectList const& mModTotalStatPct = target->GetAuraEffectsByType(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE);
                for (Unit::AuraEffectList::const_iterator i = mModTotalStatPct.begin(); i != mModTotalStatPct.end(); ++i)
                {
                    // Heart of the Wild
                    if ((*i)->GetSpellProto()->SpellIconID == 240 && (*i)->GetMiscValue() == 3)
                    {
                        int32 HotWMod = (*i)->GetAmount();

                        target->CastCustomSpell(target, HotWSpellId, &HotWMod, NULL, NULL, true, NULL, this);
                        break;
                    }
                }
            }
            switch(GetMiscValue())
            {
                case FORM_CAT:
                    // Savage Roar
                    if (target->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_DRUID, 0 , 0x10000000, 0))
                        target->CastSpell(target, 62071, true);
                    // Nurturing Instinct
                    if (AuraEffect const * aurEff = target->GetAuraEffect(SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT, SPELLFAMILY_DRUID, 2254, 0))
                    {
                        uint32 spellId = 0;
                        switch (aurEff->GetId())
                        {
                        case 33872:
                            spellId = 47179;
                            break;
                        case 33873:
                            spellId = 47180;
                            break;
                        }
                        target->CastSpell(target, spellId, true, NULL, this);
                    }
                    // Master Shapeshifter - Cat
                    if (AuraEffect const * aurEff = target->GetDummyAuraEffect(SPELLFAMILY_GENERIC, 2851, 0))
                    {
                        int32 bp = aurEff->GetAmount();
                        target->CastCustomSpell(target, 48420, &bp, NULL, NULL, true);
                    }
                break;
                case FORM_DIREBEAR:
                case FORM_BEAR:
                    // Master Shapeshifter - Bear
                    if (AuraEffect const * aurEff = target->GetDummyAuraEffect(SPELLFAMILY_GENERIC, 2851, 0))
                    {
                        int32 bp = aurEff->GetAmount();
                        target->CastCustomSpell(target, 48418, &bp, NULL, NULL, true);
                    }
                    // Survival of the Fittest
                    if (AuraEffect const * aurEff = target->GetAuraEffect(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE, SPELLFAMILY_DRUID, 961, 0))
                    {
                        int32 bp = 100 + SpellMgr::CalculateSpellEffectAmount(aurEff->GetSpellProto(), 2);
                        target->CastCustomSpell(target, 62069, &bp, NULL, NULL, true, 0, this);
                    }
                break;
                case FORM_MOONKIN:
                    // Master Shapeshifter - Moonkin
                    if (AuraEffect const * aurEff = target->GetDummyAuraEffect(SPELLFAMILY_GENERIC, 2851, 0))
                    {
                        int32 bp = aurEff->GetAmount();
                        target->CastCustomSpell(target, 48421, &bp, NULL, NULL, true);
                    }
                break;
                    // Master Shapeshifter - Tree of Life
                case FORM_TREE:
                    if (AuraEffect const * aurEff = target->GetDummyAuraEffect(SPELLFAMILY_GENERIC, 2851, 0))
                    {
                        int32 bp = aurEff->GetAmount();
                        target->CastCustomSpell(target, 48422, &bp, NULL, NULL, true);
                    }
                break;
            }
        }
    }
    else
    {
        if (spellId)
            target->RemoveAurasDueToSpell(spellId);
        if (spellId2)
            target->RemoveAurasDueToSpell(spellId2);

        // Improved Barkskin - apply/remove armor bonus due to shapeshift
        if (Player *pl=target->ToPlayer())
        {
            if (pl->HasSpell(63410) || pl->HasSpell(63411))
            {
                target->RemoveAurasDueToSpell(66530);
                target->CastSpell(target, 66530, true);
            }
        }

        Unit::AuraApplicationMap& tAuras = target->GetAppliedAuras();
        for (Unit::AuraApplicationMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
        {
            if (itr->second->GetBase()->IsRemovedOnShapeLost(target))
                target->RemoveAura(itr);
            else
                ++itr;
        }
    }
}

/*********************************************************/
/***               AURA EFFECT HANDLERS                ***/
/*********************************************************/

/**************************************/
/***       VISIBILITY & PHASES      ***/
/**************************************/

void AuraEffect::HandleModInvisibilityDetect(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();
    InvisibilityType type = InvisibilityType(GetMiscValue());

    if (apply)
    {
        target->m_invisibilityDetect.AddFlag(type);
        target->m_invisibilityDetect.AddValue(type, GetAmount());
    }
    else
    {
        if (!target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY_DETECT))
            target->m_invisibilityDetect.DelFlag(type);

        target->m_invisibilityDetect.AddValue(type, -GetAmount());
    }

    target->UpdateObjectVisibility();
}

void AuraEffect::HandleModInvisibility(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();
    InvisibilityType type = InvisibilityType(GetMiscValue());

    if (apply)
    {
        if (mode & AURA_EFFECT_HANDLE_REAL)
        {
            // drop flag at invisibiliy in bg
            target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
        }

        // apply glow vision
        if (target->GetTypeId() == TYPEID_PLAYER)
            target->SetByteFlag(PLAYER_FIELD_BYTES2, 3, PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

        target->m_invisibility.AddFlag(type);
        target->m_invisibility.AddValue(type, GetAmount());
    }
    else
    {
        if (!target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
        {
            // if not have different invisibility auras.
            // remove glow vision
            if (target->GetTypeId() == TYPEID_PLAYER)
                target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 3, PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

            target->m_invisibility.DelFlag(type);
        }
        else
        {
            bool found = false;
            Unit::AuraEffectList const& invisAuras = target->GetAuraEffectsByType(SPELL_AURA_MOD_INVISIBILITY);
            for (Unit::AuraEffectList::const_iterator i = invisAuras.begin(); i != invisAuras.end(); ++i)
            {
                if (GetMiscValue() == (*i)->GetMiscValue())
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                target->m_invisibility.DelFlag(type);
        }

        target->m_invisibility.AddValue(type, -GetAmount());
    }

    target->UpdateObjectVisibility();
}

void AuraEffect::HandleModStealthDetect(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();
    StealthType type = StealthType(GetMiscValue());

    if (apply)
    {
        target->m_stealthDetect.AddFlag(type);
        target->m_stealthDetect.AddValue(type, GetAmount());
    }
    else
    {
        if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH_DETECT))
            target->m_stealthDetect.DelFlag(type);

        target->m_stealthDetect.AddValue(type, -GetAmount());
    }

    target->UpdateObjectVisibility();
}

void AuraEffect::HandleModStealth(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();
    StealthType type = StealthType(GetMiscValue());

    if (apply)
    {
        if (mode & AURA_EFFECT_HANDLE_REAL)
        {
            // drop flag at stealth in bg
            target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
        }

        // stop handling the effect if it was removed by linked event
        if (aurApp->GetRemoveMode())
            return;

        target->m_stealth.AddFlag( type);
        target->m_stealth.AddValue(type, GetAmount());

        target->SetStandFlags(UNIT_STAND_FLAGS_CREEP);
        if (target->GetTypeId() == TYPEID_PLAYER)
            target->SetByteFlag(PLAYER_FIELD_BYTES2, 3, PLAYER_FIELD_BYTE2_STEALTH);
    }
    else
    {
        target->m_stealth.AddValue(type, -GetAmount());

        if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH)) // if last SPELL_AURA_MOD_STEALTH
        {
            target->m_stealth.DelFlag(type);

            target->RemoveStandFlags(UNIT_STAND_FLAGS_CREEP);
            if (target->GetTypeId() == TYPEID_PLAYER)
                target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 3, PLAYER_FIELD_BYTE2_STEALTH);
        }
    }

    target->UpdateObjectVisibility();
}

void AuraEffect::HandleModStealthLevel(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();
    StealthType type = StealthType(GetMiscValue());

    if (apply)
        target->m_stealth.AddValue(type, GetAmount());
    else
        target->m_stealth.AddValue(type, -GetAmount());

    target->UpdateObjectVisibility();
}

void AuraEffect::HandleSpiritOfRedemption(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // prepare spirit state
    if (apply)
    {
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            // disable breath/etc timers
            target->ToPlayer()->StopMirrorTimers();

            // set stand state (expected in this form)
            if (!target->IsStandState())
                target->SetStandState(UNIT_STAND_STATE_STAND);
        }

        target->SetHealth(1);
    }
    // die at aura end
    else
        target->setDeathState(JUST_DIED);
}

void AuraEffect::HandleAuraGhost(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
    {
        target->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
        target->m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_GHOST);
        target->m_serverSideVisibilityDetect.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_GHOST);
    }
    else
    {
        if (target->HasAuraType(SPELL_AURA_GHOST))
            return;

        target->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
        target->m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE);
        target->m_serverSideVisibilityDetect.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE);
    }
}

void AuraEffect::HandlePhase(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    // no-phase is also phase state so same code for apply and remove
    uint32 newPhase = 0;
    Unit::AuraEffectList const& phases = target->GetAuraEffectsByType(SPELL_AURA_PHASE);
    if (!phases.empty())
        for (Unit::AuraEffectList::const_iterator itr = phases.begin(); itr != phases.end(); ++itr)
            newPhase |= (*itr)->GetMiscValue();

    // phase auras normally not expected at BG but anyway better check
    if (Player* player = target->ToPlayer())
    {
        if (!newPhase)
            newPhase = PHASEMASK_NORMAL;

        // drop flag at invisible in bg
        if (player->InBattleground())
            if (Battleground *bg = player->GetBattleground())
                bg->EventPlayerDroppedFlag(player);

        // stop handling the effect if it was removed by linked event
        if (apply && aurApp->GetRemoveMode())
            return;

        // GM-mode have mask 0xFFFFFFFF
        if (player->isGameMaster())
            newPhase = 0xFFFFFFFF;

        player->SetPhaseMask(newPhase, false);
        player->GetSession()->SendSetPhaseShift(newPhase);
    }
    else
    {
        if (!newPhase)
        {
            newPhase = PHASEMASK_NORMAL;
            if (Creature* creature = target->ToCreature())
                if (CreatureData const* data = sObjectMgr->GetCreatureData(creature->GetDBTableGUIDLow()))
                    newPhase = data->phaseMask;
        }

        target->SetPhaseMask(newPhase, false);
    }

    // need triggering visibility update base at phase update of not GM invisible (other GMs anyway see in any phases)
    if (!target->IsVisible())
        target->UpdateObjectVisibility();
}

/**********************/
/***   UNIT MODEL   ***/
/**********************/

void AuraEffect::HandleAuraModShapeshift(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    uint32 modelid = 0;
    Powers PowerType = POWER_MANA;
    ShapeshiftForm form = ShapeshiftForm(GetMiscValue());

    switch (form)
    {
        case FORM_CAT:                                      // 0x01
        case FORM_GHOUL:                                    // 0x07
            PowerType = POWER_ENERGY;
            break;

        case FORM_BEAR:                                     // 0x05
        case FORM_DIREBEAR:                                 // 0x08

        case FORM_BATTLESTANCE:                             // 0x11
        case FORM_DEFENSIVESTANCE:                          // 0x12
        case FORM_BERSERKERSTANCE:                          // 0x13
            PowerType = POWER_RAGE;
            break;

        case FORM_TREE:                                     // 0x02
        case FORM_TRAVEL:                                   // 0x03
        case FORM_AQUA:                                     // 0x04
        case FORM_AMBIENT:                                  // 0x06

        case FORM_STEVES_GHOUL:                             // 0x09
        case FORM_THARONJA_SKELETON:                        // 0x0A
        case FORM_TEST_OF_STRENGTH:                         // 0x0B
        case FORM_BLB_PLAYER:                               // 0x0C
        case FORM_SHADOW_DANCE:                             // 0x0D
        case FORM_CREATUREBEAR:                             // 0x0E
        case FORM_CREATURECAT:                              // 0x0F
        case FORM_GHOSTWOLF:                                // 0x10

        case FORM_TEST:                                     // 0x14
        case FORM_ZOMBIE:                                   // 0x15
        case FORM_METAMORPHOSIS:                            // 0x16
        case FORM_UNDEAD:                                   // 0x19
        case FORM_MASTER_ANGLER:                            // 0x1A
        case FORM_FLIGHT_EPIC:                              // 0x1B
        case FORM_SHADOW:                                   // 0x1C
        case FORM_FLIGHT:                                   // 0x1D
        case FORM_STEALTH:                                  // 0x1E
        case FORM_MOONKIN:                                  // 0x1F
        case FORM_SPIRITOFREDEMPTION:                       // 0x20
            break;
        default:
            sLog->outError("Auras: Unknown Shapeshift Type: %u", GetMiscValue());
    }

    modelid = target->GetModelForForm(form);

    if (apply)
    {
        // remove polymorph before changing display id to keep new display id
        switch (form)
        {
            case FORM_CAT:
            case FORM_TREE:
            case FORM_TRAVEL:
            case FORM_AQUA:
            case FORM_BEAR:
            case FORM_DIREBEAR:
            case FORM_FLIGHT_EPIC:
            case FORM_FLIGHT:
            case FORM_MOONKIN:
            {
                // remove movement affects
                target->RemoveMovementImpairingAuras();

                // and polymorphic affects
                if (target->IsPolymorphed())
                    target->RemoveAurasDueToSpell(target->getTransForm());
                break;
            }
            default:
               break;
        }

        // remove other shapeshift before applying a new one
        target->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT, 0, GetBase());

        // stop handling the effect if it was removed by linked event
        if (aurApp->GetRemoveMode())
            return;

        if (modelid > 0)
            target->SetDisplayId(modelid);

        if (PowerType != POWER_MANA)
        {
            uint32 oldPower = target->GetPower(PowerType);
            // reset power to default values only at power change
            if (target->getPowerType() != PowerType)
                target->setPowerType(PowerType);

            switch (form)
            {
                case FORM_CAT:
                case FORM_BEAR:
                case FORM_DIREBEAR:
                {
                    // get furor proc chance
                    uint32 FurorChance = 0;
                    if (AuraEffect const *dummy = target->GetDummyAuraEffect(SPELLFAMILY_DRUID, 238, 0))
                        FurorChance = std::max(dummy->GetAmount(), 0);

                    switch (GetMiscValue())
                    {
                        case FORM_CAT:
                        {
                            int32 basePoints = int32(std::min(oldPower, FurorChance));
                            target->SetPower(POWER_ENERGY, 0);
                            target->CastCustomSpell(target, 17099, &basePoints, NULL, NULL, true, NULL, this);
                        }
                        break;
                        case FORM_BEAR:
                        case FORM_DIREBEAR:
                        if (urand(0, 99) < FurorChance)
                            target->CastSpell(target, 17057, true);
                        default:
                        {
                            uint32 newEnergy = std::min(target->GetPower(POWER_ENERGY), FurorChance);
                            target->SetPower(POWER_ENERGY, newEnergy);
                        }
                        break;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        // stop handling the effect if it was removed by linked event
        if (aurApp->GetRemoveMode())
            return;

        target->SetShapeshiftForm(form);
    }
    else
    {
        // reset model id if no other auras present
        // may happen when aura is applied on linked event on aura removal
        if (!target->HasAuraType(SPELL_AURA_MOD_SHAPESHIFT))
        {
            target->SetShapeshiftForm(FORM_NONE);
            if (target->getClass() == CLASS_DRUID)
            {
                target->setPowerType(POWER_MANA);
                // Remove movement impairing effects also when shifting out
                target->RemoveMovementImpairingAuras();
            }
        }

        if (modelid > 0)
            target->RestoreDisplayId();

        switch (form)
        {
            // Nordrassil Harness - bonus
            case FORM_BEAR:
            case FORM_DIREBEAR:
            case FORM_CAT:
                if (AuraEffect* dummy = target->GetAuraEffect(37315, 0))
                    target->CastSpell(target, 37316, true, NULL, dummy);
                break;
            // Nordrassil Regalia - bonus
            case FORM_MOONKIN:
                if (AuraEffect* dummy = target->GetAuraEffect(37324, 0))
                    target->CastSpell(target, 37325, true, NULL, dummy);
                break;
            case FORM_BATTLESTANCE:
            case FORM_DEFENSIVESTANCE:
            case FORM_BERSERKERSTANCE:
            {
                uint32 Rage_val = 0;
                // Defensive Tactics
                if (form == FORM_DEFENSIVESTANCE)
                {
                    if (AuraEffect const * aurEff = target->IsScriptOverriden(m_spellProto, 831))
                        Rage_val += aurEff->GetAmount() * 10;
                }
                // Stance mastery + Tactical mastery (both passive, and last have aura only in defense stance, but need apply at any stance switch)
                if (target->GetTypeId() == TYPEID_PLAYER)
                {
                    PlayerSpellMap const& sp_list = target->ToPlayer()->GetSpellMap();
                    for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
                    {
                        if (itr->second->state == PLAYERSPELL_REMOVED || itr->second->disabled) continue;
                        SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                        if (spellInfo && spellInfo->GetSpellFamilyName() == SPELLFAMILY_WARRIOR && spellInfo->SpellIconID == 139)
                            Rage_val += target->CalculateSpellDamage(target, spellInfo, 0) * 10;
                    }
                }
                if (target->GetPower(POWER_RAGE) > Rage_val)
                    target->SetPower(POWER_RAGE, Rage_val);
                break;
            }
            default:
                break;
        }
    }

    // adding/removing linked auras
    // add/remove the shapeshift aura's boosts
    HandleShapeshiftBoosts(target, apply);

    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->InitDataForForm();

    if (target->getClass() == CLASS_DRUID)
    {
        // Dash
        if (AuraEffect * aurEff =target->GetAuraEffect(SPELL_AURA_MOD_INCREASE_SPEED, SPELLFAMILY_DRUID, 0, 0, 0x8))
            aurEff->RecalculateAmount();

        // Disarm handling
        // If druid shifts while being disarmed we need to deal with that since forms aren't affected by disarm
        // and also HandleAuraModDisarm is not triggered
        if(!target->CanUseAttackType(BASE_ATTACK))
        {
            if (Item *pItem = target->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
            {
                    target->ToPlayer()->_ApplyWeaponDamage(EQUIPMENT_SLOT_MAINHAND, pItem->GetTemplate(), NULL, apply);
            }
        }
    }

    // stop handling the effect if it was removed by linked event
    if (apply && aurApp->GetRemoveMode())
        return;

    //if (target->GetTypeId() == TYPEID_PLAYER)
    //{
    //    SpellShapeshiftEntry const *shapeInfo = sSpellShapeshiftStore.LookupEntry(form);
    //    // Learn spells for shapeshift form - no need to send action bars or add spells to spellbook
    //    for (uint8 i = 0; i<MAX_SHAPESHIFT_SPELLS; ++i)
    //    {
    //        if (!shapeInfo->stanceSpell[i])
    //            continue;
    //        if (apply)
    //            target->ToPlayer()->AddTemporarySpell(shapeInfo->stanceSpell[i]);
    //        else
    //            target->ToPlayer()->RemoveTemporarySpell(shapeInfo->stanceSpell[i]);
    //    }
    //}
}

void AuraEffect::HandleAuraTransform(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
    {
        // update active transform spell only when transform or shapeshift not set or not overwriting negative by positive case
        if (!target->GetModelForForm(target->GetShapeshiftForm()) || !IsPositiveSpell(GetId()))
        {
            // special case (spell specific functionality)
            if (GetMiscValue() == 0)
            {
                switch (GetId())
                {
                    // Orb of Deception
                    case 16739:
                    {
                        uint32 orb_model = target->GetNativeDisplayId();
                        switch (orb_model)
                        {
                            // Troll Female
                            case 1479: target->SetDisplayId(10134); break;
                            // Troll Male
                            case 1478: target->SetDisplayId(10135); break;
                            // Tauren Male
                            case 59:   target->SetDisplayId(10136); break;
                            // Human Male
                            case 49:   target->SetDisplayId(10137); break;
                            // Human Female
                            case 50:   target->SetDisplayId(10138); break;
                            // Orc Male
                            case 51:   target->SetDisplayId(10139); break;
                            // Orc Female
                            case 52:   target->SetDisplayId(10140); break;
                            // Dwarf Male
                            case 53:   target->SetDisplayId(10141); break;
                            // Dwarf Female
                            case 54:   target->SetDisplayId(10142); break;
                            // NightElf Male
                            case 55:   target->SetDisplayId(10143); break;
                            // NightElf Female
                            case 56:   target->SetDisplayId(10144); break;
                            // Undead Female
                            case 58:   target->SetDisplayId(10145); break;
                            // Undead Male
                            case 57:   target->SetDisplayId(10146); break;
                            // Tauren Female
                            case 60:   target->SetDisplayId(10147); break;
                            // Gnome Male
                            case 1563: target->SetDisplayId(10148); break;
                            // Gnome Female
                            case 1564: target->SetDisplayId(10149); break;
                            // BloodElf Female
                            case 15475: target->SetDisplayId(17830); break;
                            // BloodElf Male
                            case 15476: target->SetDisplayId(17829); break;
                            // Dranei Female
                            case 16126: target->SetDisplayId(17828); break;
                            // Dranei Male
                            case 16125: target->SetDisplayId(17827); break;
                            default: break;
                        }
                        break;
                    }
                    // Murloc costume
                    case 42365: target->SetDisplayId(21723); break;
                    // Pygmy Oil
                    case 53806: target->SetDisplayId(22512); break;
                    default: break;
                }
            }
            else
            {
                CreatureTemplate const * ci = sObjectMgr->GetCreatureTemplate(GetMiscValue());
                if (!ci)
                {
                    target->SetDisplayId(16358);              // pig pink ^_^
                    sLog->outError("Auras: unknown creature id = %d (only need its modelid) From Spell Aura Transform in Spell ID = %d", GetMiscValue(), GetId());
                }
                else
                {
                    uint32 model_id = 0;

                    if (uint32 modelid = ci->GetRandomValidModelId())
                        model_id = modelid;                     // Will use the default model here

                    // Polymorph (sheep)
                    if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_MAGE && GetSpellProto()->SpellIconID == 82 && GetSpellProto()->SpellVisual[0] == 12978)
                        if (Unit * caster = GetCaster())
                            if (caster->HasAura(52648))         // Glyph of the Penguin
                                model_id = 26452;

                    target->SetDisplayId(model_id);

                    // Dragonmaw Illusion (set mount model also)
                    if (GetId() == 42016 && target->GetMountID() && !target->GetAuraEffectsByType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED).empty())
                        target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 16314);
                }
            }
        }

        // update active transform spell only when transform or shapeshift not set or not overwriting negative by positive case
        if (!target->getTransForm() || !IsPositiveSpell(GetId()) || IsPositiveSpell(target->getTransForm()))
            target->setTransForm(GetId());

        // polymorph case
        if ((mode & AURA_EFFECT_HANDLE_REAL) && target->GetTypeId() == TYPEID_PLAYER && target->IsPolymorphed())
        {
            // for players, start regeneration after 1s (in polymorph fast regeneration case)
            // only if caster is Player (after patch 2.4.2)
            if (IS_PLAYER_GUID(GetCasterGUID()))
                target->ToPlayer()->setRegenTimerCount(1*IN_MILLISECONDS);

            //dismount polymorphed target (after patch 2.4.2)
            if (target->IsMounted())
                target->RemoveAurasByType(SPELL_AURA_MOUNTED);
        }
    }
    else
    {
        // HandleEffect(this, AURA_EFFECT_HANDLE_SEND_FOR_CLIENT, true) will reapply it if need
        if (target->getTransForm() == GetId())
            target->setTransForm(0);

        target->RestoreDisplayId();

        // Dragonmaw Illusion (restore mount model)
        if (GetId() == 42016 && target->GetMountID() == 16314)
        {
            if (!target->GetAuraEffectsByType(SPELL_AURA_MOUNTED).empty())
            {
                uint32 cr_id = target->GetAuraEffectsByType(SPELL_AURA_MOUNTED).front()->GetMiscValue();
                if (CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(cr_id))
                {
                    uint32 team = 0;
                    if (target->GetTypeId() == TYPEID_PLAYER)
                        team = target->ToPlayer()->GetTeam();

                    uint32 displayID = sObjectMgr->ChooseDisplayId(team, ci);
                    sObjectMgr->GetCreatureModelRandomGender(&displayID);

                    target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, displayID);
                }
            }
        }
    }
}

void AuraEffect::HandleAuraModScale(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    target->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, (float)GetAmount(), apply);
}

void AuraEffect::HandleAuraCloneCaster(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
    {
        Unit * caster = GetCaster();
        if (!caster || caster == target)
            return;
        // What must be cloned? at least display and scale
        target->SetDisplayId(caster->GetDisplayId());
        target->SetCreatorGUID(caster->GetGUID());
        //target->SetFloatValue(OBJECT_FIELD_SCALE_X, caster->GetFloatValue(OBJECT_FIELD_SCALE_X)); // we need retail info about how scaling is handled (aura maybe?)
        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_MIRROR_IMAGE);
    }
    else
    {
        target->SetCreatorGUID(0);
        target->SetDisplayId(target->GetNativeDisplayId());
        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_MIRROR_IMAGE);
    }
}

/************************/
/***      FIGHT       ***/
/************************/

void AuraEffect::HandleFeignDeath(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
    {
        /*
        WorldPacket data(SMSG_FEIGN_DEATH_RESISTED, 9);
        data<<target->GetGUID();
        data<<uint8(0);
        target->SendMessageToSet(&data, true);
        */

        UnitList targets;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(target, target, target->GetMap()->GetVisibilityRange());
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(target, targets, u_check);
        target->VisitNearbyObject(target->GetMap()->GetVisibilityRange(), searcher);
        for (UnitList::iterator iter = targets.begin(); iter != targets.end(); ++iter)
        {
            if (!(*iter)->HasUnitState(UNIT_STAT_CASTING))
                continue;

            for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; i++)
            {
                if ((*iter)->GetCurrentSpell(i)
                && (*iter)->GetCurrentSpell(i)->m_targets.getUnitTargetGUID() == target->GetGUID())
                {
                    (*iter)->InterruptSpell(CurrentSpellTypes(i), false);
                }
            }
        }
        target->CombatStop();
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        // prevent interrupt message
        if (GetCasterGUID() == target->GetGUID() && target->GetCurrentSpell(CURRENT_GENERIC_SPELL))
            target->FinishSpell(CURRENT_GENERIC_SPELL, false);
        target->InterruptNonMeleeSpells(true);
        target->getHostileRefManager().deleteReferences();

        // stop handling the effect if it was removed by linked event
        if (aurApp->GetRemoveMode())
            return;
                                                            // blizz like 2.0.x
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
                                                            // blizz like 2.0.x
        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                                                            // blizz like 2.0.x
        target->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);

        target->AddUnitState(UNIT_STAT_DIED);
    }
    else
    {
        /*
        WorldPacket data(SMSG_FEIGN_DEATH_RESISTED, 9);
        data<<target->GetGUID();
        data<<uint8(1);
        target->SendMessageToSet(&data, true);
        */
                                                            // blizz like 2.0.x
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
                                                            // blizz like 2.0.x
        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                                                            // blizz like 2.0.x
        target->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);

        target->ClearUnitState(UNIT_STAT_DIED);
    }
}

void AuraEffect::HandleModUnattackable(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
    {
        if (mode & AURA_EFFECT_HANDLE_REAL)
        {
            target->CombatStop();
            target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
        }
        // stop handling the effect if it was removed by linked event
        if (aurApp->GetRemoveMode())
            return;
    }
    // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
    else if (target->HasAuraType(SPELL_AURA_MOD_UNATTACKABLE))
        return;

    target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE, apply);
}

void AuraEffect::HandleAuraModDisarm(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    AuraType type = GetAuraType();

    //Prevent handling aura twice
    if ((apply) ? target->GetAuraEffectsByType(type).size() > 1 : target->HasAuraType(type))
        return;

    uint32 field, flag, slot;
    WeaponAttackType attType;
    switch (type)
    {
    case SPELL_AURA_MOD_DISARM:
        field=UNIT_FIELD_FLAGS;
        flag=UNIT_FLAG_DISARMED;
        slot=EQUIPMENT_SLOT_MAINHAND;
        attType=BASE_ATTACK;
        break;
    case SPELL_AURA_MOD_DISARM_OFFHAND:
        field=UNIT_FIELD_FLAGS_2;
        flag=UNIT_FLAG2_DISARM_OFFHAND;
        slot=EQUIPMENT_SLOT_OFFHAND;
        attType=OFF_ATTACK;
        break;
    case SPELL_AURA_MOD_DISARM_RANGED:
        field=UNIT_FIELD_FLAGS_2;
        flag=UNIT_FLAG2_DISARM_RANGED;
        slot=EQUIPMENT_SLOT_RANGED;
        attType=RANGED_ATTACK;
        break;
    default:
        return;
    }

    if (!apply)
        target->RemoveFlag(field, flag);

    if (apply)
        target->SetFlag(field, flag);

    // Handle damage modifcation, shapeshifted druids are not affected
    if (target->GetTypeId() == TYPEID_PLAYER && !target->IsInFeralForm())
    {
        if (Item *pItem = target->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            uint8 attacktype = Player::GetAttackBySlot(slot);

            if (attacktype < MAX_ATTACK)
                target->ToPlayer()->_ApplyWeaponDamage(slot, pItem->GetTemplate(), NULL, !apply);
        }
    }

    if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->GetCurrentEquipmentId())
        target->UpdateDamagePhysical(attType);
}

void AuraEffect::HandleAuraModSilence(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
    {
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
        // Stop cast only spells vs PreventionType == SPELL_PREVENTION_TYPE_SILENCE
        for (uint32 i = CURRENT_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
            if (Spell* spell = target->GetCurrentSpell(CurrentSpellTypes(i)))
                if (spell->m_spellInfo->GetPreventionType() == SPELL_PREVENTION_TYPE_SILENCE)
                    // Stop spells on prepare or casting state
                    target->InterruptSpell(CurrentSpellTypes(i), false);
    }
    else
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(SPELL_AURA_MOD_SILENCE) || target->HasAuraType(SPELL_AURA_MOD_PACIFY_SILENCE))
            return;

        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
    }
}

void AuraEffect::HandleAuraModPacify(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
    else
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(SPELL_AURA_MOD_PACIFY) || target->HasAuraType(SPELL_AURA_MOD_PACIFY_SILENCE))
            return;
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
    }
}

void AuraEffect::HandleAuraModPacifyAndSilence(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    // Vengeance of the Blue Flight (TODO: REMOVE THIS!)
    if (m_spellProto->Id == 45839)
    {
        if (apply)
            target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        else
            target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
    }
    if (!(apply))
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(SPELL_AURA_MOD_PACIFY_SILENCE))
            return;
    }
    HandleAuraModPacify(aurApp, mode, apply);
    HandleAuraModSilence(aurApp, mode, apply);
}

void AuraEffect::HandleAuraAllowOnlyAbility(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        if (apply)
            target->SetFlag(PLAYER_FLAGS, PLAYER_ALLOW_ONLY_ABILITY);
        else
        {
            // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
            if (target->HasAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY))
                return;
            target->RemoveFlag(PLAYER_FLAGS, PLAYER_ALLOW_ONLY_ABILITY);
        }
    }
}

/****************************/
/***      TRACKING        ***/
/****************************/

void AuraEffect::HandleAuraTrackCreatures(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->SetUInt32Value(PLAYER_TRACK_CREATURES, (apply) ? ((uint32)1)<<(GetMiscValue()-1) : 0);
}

void AuraEffect::HandleAuraTrackResources(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->SetUInt32Value(PLAYER_TRACK_RESOURCES, (apply) ? ((uint32)1)<<(GetMiscValue()-1): 0);
}

void AuraEffect::HandleAuraTrackStealthed(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    if (!(apply))
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
    }
    target->ApplyModFlag(PLAYER_FIELD_BYTES, PLAYER_FIELD_BYTE_TRACK_STEALTHED, apply);
}

void AuraEffect::HandleAuraModStalked(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    // used by spells: Hunter's Mark, Mind Vision, Syndicate Tracker (MURP) DND
    if (apply)
        target->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
    else
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (!target->HasAuraType(GetAuraType()))
            target->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
    }

    target->UpdateObjectVisibility();
}

void AuraEffect::HandleAuraUntrackable(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
        target->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
    else
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
        target->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
    }
}

/****************************/
/***  SKILLS & TALENTS    ***/
/****************************/

void AuraEffect::HandleAuraModPetTalentsPoints(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate pet talent points
    if (Pet *pet = target->ToPlayer()->GetPet())
        pet->InitTalentForLevel();
}

void AuraEffect::HandleAuraModSkill(AuraApplication const * aurApp, uint8 /*mode*/, bool apply) const
{
    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 prot = GetMiscValue();
    int32 points = GetAmount();

    target->ToPlayer()->ModifySkillBonus(prot, ((apply) ? points: -points), GetAuraType() == SPELL_AURA_MOD_SKILL_TALENT);
    if (prot == SKILL_DEFENSE)
        target->ToPlayer()->UpdateDefenseBonusesMod();
}

/****************************/
/***       MOVEMENT       ***/
/****************************/

void AuraEffect::HandleAuraMounted(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
    {
        uint32 creatureEntry = GetMiscValue();

        // Festive Holiday Mount
        if (target->HasAura(62061))
        {
            if (GetBase()->HasEffectType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED))
                creatureEntry = 24906;
            else
                creatureEntry = 15665;
        }

        CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(creatureEntry);
        if (!ci)
        {
            sLog->outErrorDb("AuraMounted: `creature_template`='%u' not found in database (only need its modelid)", GetMiscValue());
            return;
        }

        uint32 team = 0;
        if (target->GetTypeId() == TYPEID_PLAYER)
            team = target->ToPlayer()->GetTeam();

        uint32 displayID = sObjectMgr->ChooseDisplayId(team, ci);
        sObjectMgr->GetCreatureModelRandomGender(&displayID);

        //some spell has one aura of mount and one of vehicle
        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (GetSpellProto()->GetSpellEffectIdByIndex(i) == SPELL_EFFECT_SUMMON
                && GetSpellProto()->GetEffectMiscValue(i) == GetMiscValue())
                displayID = 0;

        target->Mount(displayID, ci->VehicleId, GetMiscValue());
    }
    else
    {
        target->Unmount();
        //some mounts like Headless Horseman's Mount or broom stick are skill based spell
        // need to remove ALL arura related to mounts, this will stop client crash with broom stick
        // and never endless flying after using Headless Horseman's Mount
        if (mode & AURA_EFFECT_HANDLE_REAL)
            target->RemoveAurasByType(SPELL_AURA_MOUNTED);
    }
}

void AuraEffect::HandleAuraAllowFlight(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (!apply)
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()) || target->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED))
            return;
    }

    if (target->GetTypeId() == TYPEID_UNIT)
        target->SetFlying(apply);

    if (Player *plr = target->m_movedPlayer)
    {
        // allow fly
        WorldPacket data;
        if (apply)
            data.Initialize(SMSG_MOVE_SET_CAN_FLY, 12);
        else
            data.Initialize(SMSG_MOVE_UNSET_CAN_FLY, 12);
        data.append(target->GetPackGUID());
        data << uint32(0);                                      // unk
        plr->SendDirectMessage(&data);
    }
}

void AuraEffect::HandleAuraWaterWalk(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (!(apply))
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
    }

    WorldPacket data;
    if (apply)
        data.Initialize(SMSG_MOVE_WATER_WALK, 8+4);
    else
        data.Initialize(SMSG_MOVE_LAND_WALK, 8+4);
    data.append(target->GetPackGUID());
    data << uint32(0);
    target->SendMessageToSet(&data, true);
}

void AuraEffect::HandleAuraFeatherFall(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (!(apply))
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
    }

    WorldPacket data;
    if (apply)
        data.Initialize(SMSG_MOVE_FEATHER_FALL, 8+4);
    else
        data.Initialize(SMSG_MOVE_NORMAL_FALL, 8+4);
    data.append(target->GetPackGUID());
    data << uint32(0);
    target->SendMessageToSet(&data, true);

    // start fall from current height
    if (!apply && target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->SetFallInformation(0, target->GetPositionZ());
}

void AuraEffect::HandleAuraHover(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (!(apply))
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
    }

    WorldPacket data;
    if (apply)
        data.Initialize(SMSG_MOVE_SET_HOVER, 8+4);
    else
        data.Initialize(SMSG_MOVE_UNSET_HOVER, 8+4);
    data.append(target->GetPackGUID());
    data << uint32(0);
    target->SendMessageToSet(&data, true);
}

void AuraEffect::HandleWaterBreathing(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    // update timers in client
    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->UpdateMirrorTimers();
}

void AuraEffect::HandleForceMoveForward(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVE);
    else
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVE);
    }
}

/****************************/
/***        THREAT        ***/
/****************************/

void AuraEffect::HandleModThreat(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit * target = aurApp->GetTarget();
    for (int32 i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if (GetMiscValue() & (1 << i))
            ApplyPercentModFloatVar(target->m_threatModifier[i], float(GetAmount()), apply);
}

void AuraEffect::HandleAuraModTotalThreat(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (!target->isAlive() || target->GetTypeId() != TYPEID_PLAYER)
        return;

    Unit * caster = GetCaster();
    if (!caster || !caster->isAlive())
        return;

    target->getHostileRefManager().addTempThreat((float)GetAmount(), apply);
}

void AuraEffect::HandleModTaunt(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (!target->isAlive() || !target->CanHaveThreatList())
        return;

    Unit * caster = GetCaster();
    if (!caster || !caster->isAlive())
        return;

    if (apply)
        target->TauntApply(caster);
    else
    {
        // When taunt aura fades out, mob will switch to previous target if current has less than 1.1 * secondthreat
        target->TauntFadeOut(caster);
    }
}

/*****************************/
/***        CONTROL        ***/
/*****************************/

void AuraEffect::HandleModConfuse(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    target->SetControlled(apply, UNIT_STAT_CONFUSED);
}

void AuraEffect::HandleModFear(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    target->SetControlled(apply, UNIT_STAT_FLEEING);
}

void AuraEffect::HandleAuraModStun(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    target->SetControlled(apply, UNIT_STAT_STUNNED);
}

void AuraEffect::HandleAuraModRoot(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    target->SetControlled(apply, UNIT_STAT_ROOT);
}

void AuraEffect::HandlePreventFleeing(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    Unit::AuraEffectList const& fearAuras = target->GetAuraEffectsByType(SPELL_AURA_MOD_FEAR);
    if (!fearAuras.empty())
        target->SetControlled(!(apply), UNIT_STAT_FLEEING);
}

/***************************/
/***        CHARM        ***/
/***************************/

void AuraEffect::HandleModPossess(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    Unit * caster = GetCaster();
    if (caster && caster->GetTypeId() == TYPEID_UNIT)
    {
        HandleModCharm(aurApp, mode, apply);
        return;
    }

    if (apply)
        target->SetCharmedBy(caster, CHARM_TYPE_POSSESS, aurApp);
    else
        target->RemoveCharmedBy(caster);
}

// only one spell has this aura
void AuraEffect::HandleModPossessPet(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* caster = GetCaster();
    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    //seems it may happen that when removing it is no longer owner's pet
    //if (caster->ToPlayer()->GetPet() != target)
    //    return;

    Unit* target = aurApp->GetTarget();
    if (target->GetTypeId() != TYPEID_UNIT || !target->ToCreature()->isPet())
        return;

    Pet* pet = target->ToPet();

    if (apply)
    {
        if (caster->ToPlayer()->GetPet() != pet)
            return;

        pet->SetCharmedBy(caster, CHARM_TYPE_POSSESS, aurApp);
    }
    else
    {
        pet->RemoveCharmedBy(caster);

        if (!pet->IsWithinDistInMap(caster, pet->GetMap()->GetVisibilityRange()))
            pet->Remove(PET_SAVE_NOT_IN_SLOT, true);
        else
        {
            // Reinitialize the pet bar and make the pet come back to the owner
            caster->ToPlayer()->PetSpellInitialize();
            if (!pet->getVictim())
            {
                pet->GetMotionMaster()->MoveFollow(caster, PET_FOLLOW_DIST, pet->GetFollowAngle());
                //if (target->GetCharmInfo())
                //    target->GetCharmInfo()->SetCommandState(COMMAND_FOLLOW);
            }
        }
    }
}

void AuraEffect::HandleModCharm(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    Unit * caster = GetCaster();

    if (apply)
        target->SetCharmedBy(caster, CHARM_TYPE_CHARM, aurApp);
    else
        target->RemoveCharmedBy(caster);
}

void AuraEffect::HandleCharmConvert(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    Unit * caster = GetCaster();

    if (apply)
        target->SetCharmedBy(caster, CHARM_TYPE_CONVERT, aurApp);
    else
        target->RemoveCharmedBy(caster);
}

/**
 * Such auras are applied from a caster(=player) to a vehicle.
 * This has been verified using spell #49256
 */
void AuraEffect::HandleAuraControlVehicle(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (!target->IsVehicle())
        return;

    Unit * caster = GetCaster();

    if (!caster || caster == target)
        return;

    if (apply)
    {
        caster->_EnterVehicle(target->GetVehicleKit(), m_amount - 1, aurApp);
    }
    else
    {
        if (GetId() == 53111) // Devour Humanoid
        {
            target->Kill(caster);
            if (caster->GetTypeId() == TYPEID_UNIT)
                caster->ToCreature()->RemoveCorpse();
        }

        // some SPELL_AURA_CONTROL_VEHICLE auras have a dummy effect on the player - remove them
        caster->RemoveAurasDueToSpell(GetId());
        caster->_ExitVehicle();
    }
}

/*********************************************************/
/***                  MODIFY SPEED                     ***/
/*********************************************************/
void AuraEffect::HandleAuraModIncreaseSpeed(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    target->UpdateSpeed(MOVE_RUN, true);
}

void AuraEffect::HandleAuraModIncreaseMountedSpeed(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    HandleAuraModIncreaseSpeed(aurApp, mode, apply);
}

void AuraEffect::HandleAuraModIncreaseFlightSpeed(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    // Enable Fly mode for flying mounts
    if (GetAuraType() == SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED)
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK && (apply || (!target->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) && !target->HasAuraType(SPELL_AURA_FLY))))
        {
            if (Player *plr = target->m_movedPlayer)
            {
                WorldPacket data;
                if (apply)
                    data.Initialize(SMSG_MOVE_SET_CAN_FLY, 12);
                else
                    data.Initialize(SMSG_MOVE_UNSET_CAN_FLY, 12);
                data.append(plr->GetPackGUID());
                data << uint32(0);                                      // unknown
                plr->SendDirectMessage(&data);
            }
        }

        if (mode & AURA_EFFECT_HANDLE_REAL)
        {
            //Players on flying mounts must be immune to polymorph
            if (target->GetTypeId() == TYPEID_PLAYER)
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, apply);

            // Dragonmaw Illusion (overwrite mount model, mounted aura already applied)
            if (apply && target->HasAuraEffect(42016, 0) && target->GetMountID())
                target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 16314);
        }
    }

    if (mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK)
        target->UpdateSpeed(MOVE_FLIGHT, true);
}

void AuraEffect::HandleAuraModIncreaseSwimSpeed(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    target->UpdateSpeed(MOVE_SWIM, true);
}

void AuraEffect::HandleAuraModDecreaseSpeed(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    target->UpdateSpeed(MOVE_RUN, true);
    target->UpdateSpeed(MOVE_SWIM, true);
    target->UpdateSpeed(MOVE_FLIGHT, true);
    target->UpdateSpeed(MOVE_RUN_BACK, true);
    target->UpdateSpeed(MOVE_SWIM_BACK, true);
    target->UpdateSpeed(MOVE_FLIGHT_BACK, true);
}

void AuraEffect::HandleAuraModUseNormalSpeed(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    target->UpdateSpeed(MOVE_RUN,  true);
    target->UpdateSpeed(MOVE_SWIM, true);
    target->UpdateSpeed(MOVE_FLIGHT,  true);
}

/*********************************************************/
/***                     IMMUNITY                      ***/
/*********************************************************/

void AuraEffect::HandleModStateImmunityMask(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    std::list <AuraType> immunity_list;
    if (GetMiscValue() & (1<<10))
        immunity_list.push_back(SPELL_AURA_MOD_STUN);
    if (GetMiscValue() & (1<<1))
        immunity_list.push_back(SPELL_AURA_TRANSFORM);

    // These flag can be recognized wrong:
    if (GetMiscValue() & (1<<6))
        immunity_list.push_back(SPELL_AURA_MOD_DECREASE_SPEED);
    if (GetMiscValue() & (1<<0))
        immunity_list.push_back(SPELL_AURA_MOD_ROOT);
    if (GetMiscValue() & (1<<2))
        immunity_list.push_back(SPELL_AURA_MOD_CONFUSE);
    if (GetMiscValue() & (1<<9))
        immunity_list.push_back(SPELL_AURA_MOD_FEAR);

    // must be last due to Bladestorm
    if (GetMiscValue() & (1<<7))
        immunity_list.push_back(SPELL_AURA_MOD_DISARM);

    // TODO: figure out a better place to put this...
    // Patch 3.0.3 Bladestorm now breaks all snares and roots on the warrior when activated.
    // however not all mechanic specified in immunity
    if (apply && GetId() == 46924)
    {
        immunity_list.pop_back(); // delete Disarm
        target->RemoveAurasByType(SPELL_AURA_MOD_ROOT);
        target->RemoveAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
    }

    if (apply && GetSpellProto()->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY)
        for (std::list <AuraType>::iterator iter = immunity_list.begin(); iter != immunity_list.end(); ++iter)
            target->RemoveAurasByType(*iter);

    // stop handling the effect if it was removed by linked event
    if (apply && aurApp->GetRemoveMode())
        return;

    for (std::list <AuraType>::iterator iter = immunity_list.begin(); iter != immunity_list.end(); ++iter)
        target->ApplySpellImmune(GetId(), IMMUNITY_STATE, *iter, apply);
}

void AuraEffect::HandleModMechanicImmunity(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();
    uint32 mechanic;

    switch (GetId())
    {
        case 42292: // PvP trinket
        case 59752: // Every Man for Himself
            mechanic = IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;
            // Actually we should apply immunities here, too, but the aura has only 100 ms duration, so there is practically no point
            break;
        case 54508: // Demonic Empowerment
            mechanic = (1 << MECHANIC_SNARE) | (1 << MECHANIC_ROOT);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SNARE, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_ROOT, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
            break;
        case 34471: // The Beast Within
        case 19574: // Bestial Wrath
            mechanic = (1 << MECHANIC_SNARE) | (1 << MECHANIC_ROOT) | (1 << MECHANIC_FEAR) | (1 << MECHANIC_STUN) | (1 << MECHANIC_SLEEP) | (1 << MECHANIC_CHARM) | (1 << MECHANIC_SAPPED) | (1 << MECHANIC_HORROR) | (1 << MECHANIC_POLYMORPH) | (1 << MECHANIC_DISORIENTED) | (1 << MECHANIC_FREEZE) | (1 << MECHANIC_TURN);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SNARE, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_ROOT, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FEAR, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SLEEP, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_CHARM, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SAPPED, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_HORROR, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FREEZE, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_TURN, apply);
            break;
        default:
            if (GetMiscValue() < 1)
                return;
            mechanic = 1 << GetMiscValue();
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, GetMiscValue(), apply);
            break;
    }

    if (apply && GetSpellProto()->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY)
        target->RemoveAurasWithMechanic(mechanic, AURA_REMOVE_BY_DEFAULT, GetId());
}

void AuraEffect::HandleAuraModEffectImmunity(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    // when removing flag aura, handle flag drop
    if (!(apply) && target->GetTypeId() == TYPEID_PLAYER
        && (GetSpellProto()->GetAuraInterruptFlags() & AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION))
    {
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            if (target->ToPlayer()->InBattleground())
            {
                if (Battleground *bg = target->ToPlayer()->GetBattleground())
                    bg->EventPlayerDroppedFlag(target->ToPlayer());
            }
            else
                sOutdoorPvPMgr->HandleDropFlag((Player*)target, GetSpellProto()->Id);
        }
    }
    // stop handling the effect if it was removed by linked event
    if (apply && aurApp->GetRemoveMode())
        return;

    target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, GetMiscValue(), apply);
}

void AuraEffect::HandleAuraModStateImmunity(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if ((apply) && GetSpellProto()->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY)
        target->RemoveAurasByType(AuraType(GetMiscValue()), 0 , GetBase());

    // stop handling the effect if it was removed by linked event
    if (apply && aurApp->GetRemoveMode())
        return;

    target->ApplySpellImmune(GetId(), IMMUNITY_STATE, GetMiscValue(), apply);
}

void AuraEffect::HandleAuraModSchoolImmunity(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if ((apply) && GetMiscValue() == SPELL_SCHOOL_MASK_NORMAL)
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

    // stop handling the effect if it was removed by linked event
    if (apply && aurApp->GetRemoveMode())
        return;

    target->ApplySpellImmune(GetId(), IMMUNITY_SCHOOL, GetMiscValue(), (apply));

    // remove all flag auras (they are positive, but they must be removed when you are immune)
    if (GetSpellProto()->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY
        && GetSpellProto()->AttributesEx2 & SPELL_ATTR2_DAMAGE_REDUCED_SHIELD)
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

    // TODO: optimalize this cycle - use RemoveAurasWithInterruptFlags call or something else
    if ((apply)
        && GetSpellProto()->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY
        && IsPositiveSpell(GetId()))                       //Only positive immunity removes auras
    {
        uint32 school_mask = GetMiscValue();
        Unit::AuraApplicationMap& Auras = target->GetAppliedAuras();
        for (Unit::AuraApplicationMap::iterator iter = Auras.begin(); iter != Auras.end();)
        {
            SpellEntry const *spell = iter->second->GetBase()->GetSpellProto();
            if ((GetSpellSchoolMask(spell) & school_mask)//Check for school mask
                && CanSpellDispelAura(GetSpellProto(), spell)
                && !iter->second->IsPositive()          //Don't remove positive spells
                && spell->Id != GetId())               //Don't remove self
            {
                target->RemoveAura(iter);
            }
            else
                ++iter;
        }
    }

    // stop handling the effect if it was removed by linked event
    if (apply && aurApp->GetRemoveMode())
        return;

    if (GetSpellProto()->GetMechanic() == MECHANIC_BANISH)
    {
        if (apply)
            target->AddUnitState(UNIT_STAT_ISOLATED);
        else
        {
            bool banishFound = false;
            Unit::AuraEffectList const& banishAuras = target->GetAuraEffectsByType(GetAuraType());
            for (Unit::AuraEffectList::const_iterator i = banishAuras.begin(); i !=  banishAuras.end(); ++i)
                if ((*i)->GetSpellProto()->GetMechanic() == MECHANIC_BANISH)
                {
                    banishFound = true;
                    break;
                }
            if (!banishFound)
                target->ClearUnitState(UNIT_STAT_ISOLATED);
        }
    }
}

void AuraEffect::HandleAuraModDmgImmunity(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    target->ApplySpellImmune(GetId(), IMMUNITY_DAMAGE, GetMiscValue(), apply);
}

void AuraEffect::HandleAuraModDispelImmunity(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    target->ApplySpellDispelImmunity(m_spellProto, DispelType(GetMiscValue()), (apply));
}

/*********************************************************/
/***                  MODIFY STATS                     ***/
/*********************************************************/

/********************************/
/***        RESISTANCE        ***/
/********************************/

void AuraEffect::HandleAuraModResistanceExclusive(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; x++)
    {
        if (GetMiscValue() & int32(1<<x))
        {
            int32 amount = target->GetMaxPositiveAuraModifierByMiscMask(SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE, 1<<x, this);
            if (amount < GetAmount())
            {
                target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, float(GetAmount() - amount), apply);
                if (target->GetTypeId() == TYPEID_PLAYER)
                    target->ApplyResistanceBuffModsMod(SpellSchools(x), aurApp->IsPositive(), float(GetAmount() - amount), apply);
            }
        }
    }
}

void AuraEffect::HandleAuraModResistance(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; x++)
    {
        if (GetMiscValue() & int32(1<<x))
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), TOTAL_VALUE, float(GetAmount()), apply);
            if (target->GetTypeId() == TYPEID_PLAYER || target->ToCreature()->isPet())
                target->ApplyResistanceBuffModsMod(SpellSchools(x), GetAmount() > 0, (float)GetAmount(), apply);
        }
    }
}

void AuraEffect::HandleAuraModBaseResistancePCT(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    // only players have base stats
    if (target->GetTypeId() != TYPEID_PLAYER)
    {
        //pets only have base armor
        if (target->ToCreature()->isPet() && (GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL))
            target->HandleStatModifier(UNIT_MOD_ARMOR, BASE_PCT, float(GetAmount()), apply);
    }
    else
    {
        for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; x++)
        {
            if (GetMiscValue() & int32(1<<x))
                target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_PCT, float(GetAmount()), apply);
        }
    }
}

void AuraEffect::HandleModResistancePercent(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    for (int8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
    {
        if (GetMiscValue() & int32(1<<i))
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_PCT, float(GetAmount()), apply);
            if (target->GetTypeId() == TYPEID_PLAYER || target->ToCreature()->isPet())
            {
                target->ApplyResistanceBuffModsPercentMod(SpellSchools(i), true, (float)GetAmount(), apply);
                target->ApplyResistanceBuffModsPercentMod(SpellSchools(i), false, (float)GetAmount(), apply);
            }
        }
    }
}

void AuraEffect::HandleModBaseResistance(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    // only players have base stats
    if (target->GetTypeId() != TYPEID_PLAYER)
    {
        //only pets have base stats
        if (target->ToCreature()->isPet() && (GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL))
            target->HandleStatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, float(GetAmount()), apply);
    }
    else
    {
        for (int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
            if (GetMiscValue() & (1<<i))
                target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(GetAmount()), apply);
    }
}

void AuraEffect::HandleModTargetResistance(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    // applied to damage as HandleNoImmediateEffect in Unit::CalcAbsorbResist and Unit::CalcArmorReducedDamage

    // show armor penetration
    if (target->GetTypeId() == TYPEID_PLAYER && (GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL))
        target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE, GetAmount(), apply);

    // show as spell penetration only full spell penetration bonuses (all resistances except armor and holy
    if (target->GetTypeId() == TYPEID_PLAYER && (GetMiscValue() & SPELL_SCHOOL_MASK_SPELL) == SPELL_SCHOOL_MASK_SPELL)
        target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE, GetAmount(), apply);
}

/********************************/
/***           STAT           ***/
/********************************/

void AuraEffect::HandleAuraModStat(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (GetMiscValue() < -2 || GetMiscValue() > 4)
    {
        sLog->outError("WARNING: Spell %u effect %u has an unsupported misc value (%i) for SPELL_AURA_MOD_STAT ", GetId(), GetEffIndex(), GetMiscValue());
        return;
    }

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        // -1 or -2 is all stats (misc < -2 checked in function beginning)
        if (GetMiscValue() < 0 || GetMiscValue() == i)
        {
            //target->ApplyStatMod(Stats(i), m_amount, apply);
            target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, float(GetAmount()), apply);
            if (target->GetTypeId() == TYPEID_PLAYER || target->ToCreature()->isPet())
                target->ApplyStatBuffMod(Stats(i), (float)GetAmount(), apply);
        }
    }
}

void AuraEffect::HandleModPercentStat(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (GetMiscValue() < -1 || GetMiscValue() > 4)
    {
        sLog->outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    // only players have base stats
    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        if (GetMiscValue() == i || GetMiscValue() == -1)
            target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), BASE_PCT, float(m_amount), apply);
    }
}

void AuraEffect::HandleModSpellDamagePercentFromStat(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Magic damage modifiers implemented in Unit::SpellDamageBonus
    // This information for client side use only
    // Recalculate bonus
    target->ToPlayer()->UpdateSpellDamageAndHealingBonus();
}

void AuraEffect::HandleModSpellHealingPercentFromStat(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    target->ToPlayer()->UpdateSpellDamageAndHealingBonus();
}

void AuraEffect::HandleModSpellDamagePercentFromAttackPower(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Magic damage modifiers implemented in Unit::SpellDamageBonus
    // This information for client side use only
    // Recalculate bonus
    target->ToPlayer()->UpdateSpellDamageAndHealingBonus();
}

void AuraEffect::HandleModSpellHealingPercentFromAttackPower(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    target->ToPlayer()->UpdateSpellDamageAndHealingBonus();
}

void AuraEffect::HandleModHealingDone(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;
    // implemented in Unit::SpellHealingBonus
    // this information is for client side only
    target->ToPlayer()->UpdateSpellDamageAndHealingBonus();
}

void AuraEffect::HandleModTotalPercentStat(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (GetMiscValue() < -1 || GetMiscValue() > 4)
    {
        sLog->outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    // save current health state
    float healthPct = target->GetHealthPct();
    bool alive = target->isAlive();

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        if (GetMiscValue() == i || GetMiscValue() == -1)
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(GetAmount()), apply);
            if (target->GetTypeId() == TYPEID_PLAYER || target->ToCreature()->isPet())
                target->ApplyStatPercentBuffMod(Stats(i), float(GetAmount()), apply);
        }
    }

    //recalculate current HP/MP after applying aura modifications (only for spells with SPELL_ATTR0_UNK4 0x00000010 flag)
    if (GetMiscValue() == STAT_STAMINA && (m_spellProto->Attributes & SPELL_ATTR0_UNK4))
        target->SetHealth(std::max<uint32>(uint32(healthPct * target->GetMaxHealth() * 0.01f), (alive ? 1 : 0)));
}

void AuraEffect::HandleAuraModResistenceOfStatPercent(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    if (GetMiscValue() != SPELL_SCHOOL_MASK_NORMAL)
    {
        // support required adding replace UpdateArmor by loop by UpdateResistence at intellect update
        // and include in UpdateResistence same code as in UpdateArmor for aura mod apply.
        sLog->outError("Aura SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT(182) does not work for non-armor type resistances!");
        return;
    }

    // Recalculate Armor
    target->UpdateArmor();
}

void AuraEffect::HandleAuraModExpertise(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->ToPlayer()->UpdateExpertise(BASE_ATTACK);
    target->ToPlayer()->UpdateExpertise(OFF_ATTACK);
}

/********************************/
/***      HEAL & ENERGIZE     ***/
/********************************/
void AuraEffect::HandleModPowerRegen(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Update manaregen value
    if (GetMiscValue() == POWER_MANA)
        target->ToPlayer()->UpdateManaRegen();
    // other powers are not immediate effects - implemented in Player::Regenerate, Creature::Regenerate
}

void AuraEffect::HandleModPowerRegenPCT(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    HandleModPowerRegen(aurApp, mode, apply);
}

void AuraEffect::HandleModManaRegen(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    //Note: an increase in regen does NOT cause threat.
    target->ToPlayer()->UpdateManaRegen();
}

void AuraEffect::HandleAuraModIncreaseHealth(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
    {
        target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(GetAmount()), apply);
        target->ModifyHealth(GetAmount());
    }
    else
    {
        if (int32(target->GetHealth()) > GetAmount())
            target->ModifyHealth(-GetAmount());
        else
            target->SetHealth(1);
        target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(GetAmount()), apply);
    }
}

void AuraEffect::HandleAuraModIncreaseMaxHealth(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    uint32 oldhealth = target->GetHealth();
    double healthPercentage = (double)oldhealth / (double)target->GetMaxHealth();

    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(GetAmount()), apply);

    // refresh percentage
    if (oldhealth > 0)
    {
        uint32 newhealth = uint32(ceil((double)target->GetMaxHealth() * healthPercentage));
        if (newhealth == 0)
            newhealth = 1;

        target->SetHealth(newhealth);
    }
}

void AuraEffect::HandleAuraModIncreaseEnergy(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    Powers powerType = target->getPowerType();
    if (int32(powerType) != GetMiscValue())
        return;

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    // Special case with temporary increase max/current power (percent)
    if (GetId() == 64904)                                     // Hymn of Hope
    {
        if (mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK)
        {
            int32 change = target->GetPower(powerType) + (apply ? GetAmount() : -GetAmount());
            if (change < 0)
                change = 0;
            target->SetPower(powerType, change);
        }
    }

    // generic flat case
    target->HandleStatModifier(unitMod, TOTAL_VALUE, float(GetAmount()), apply);
}

void AuraEffect::HandleAuraModIncreaseEnergyPercent(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    Powers powerType = target->getPowerType();
    if (int32(powerType) != GetMiscValue())
        return;

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    target->HandleStatModifier(unitMod, TOTAL_PCT, float(GetAmount()), apply);
}

void AuraEffect::HandleAuraModIncreaseHealthPercent(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    // Unit will keep hp% after MaxHealth being modified if unit is alive.
    float percent = target->GetHealthPct();
    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, float(GetAmount()), apply);
    if (target->isAlive())
        target->SetHealth(target->CountPctFromMaxHealth(int32(percent)));
}

void AuraEffect::HandleAuraIncreaseBaseHealthPercent(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    target->HandleStatModifier(UNIT_MOD_HEALTH, BASE_PCT, float(GetAmount()), apply);
}

/********************************/
/***          FIGHT           ***/
/********************************/

void AuraEffect::HandleAuraModParryPercent(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->ToPlayer()->UpdateParryPercentage();
}

void AuraEffect::HandleAuraModDodgePercent(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->ToPlayer()->UpdateDodgePercentage();
}

void AuraEffect::HandleAuraModBlockPercent(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->ToPlayer()->UpdateBlockPercentage();
}

void AuraEffect::HandleAuraModRegenInterrupt(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    HandleModManaRegen(aurApp, mode, apply);
}

void AuraEffect::HandleAuraModWeaponCritPercent(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int i = 0; i < MAX_ATTACK; ++i)
        if (Item* pItem = target->ToPlayer()->GetWeaponForAttack(WeaponAttackType(i), true))
            target->ToPlayer()->_ApplyWeaponDependentAuraCritMod(pItem, WeaponAttackType(i), this, apply);

    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // GetMiscValue() comparison with item generated damage types

    if (GetSpellProto()->GetEquippedItemClass() == -1)
    {
        target->ToPlayer()->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float (GetAmount()), apply);
        target->ToPlayer()->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float (GetAmount()), apply);
        target->ToPlayer()->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float (GetAmount()), apply);
    }
    else
    {
        // done in Player::_ApplyWeaponDependentAuraMods
    }
}

void AuraEffect::HandleModHitChance(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        target->ToPlayer()->UpdateMeleeHitChances();
        target->ToPlayer()->UpdateRangedHitChances();
    }
    else
    {
        target->m_modMeleeHitChance += (apply) ? GetAmount() : (-GetAmount());
        target->m_modRangedHitChance += (apply) ? GetAmount() : (-GetAmount());
    }
}

void AuraEffect::HandleModSpellHitChance(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->UpdateSpellHitChances();
    else
        target->m_modSpellHitChance += (apply) ? GetAmount(): (-GetAmount());
}

void AuraEffect::HandleModSpellCritChance(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->UpdateAllSpellCritChances();
    else
        target->m_baseSpellCritChance += (apply) ? GetAmount():-GetAmount();
}

void AuraEffect::HandleModSpellCritChanceShool(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int school = SPELL_SCHOOL_NORMAL; school < MAX_SPELL_SCHOOL; ++school)
        if (GetMiscValue() & (1<<school))
            target->ToPlayer()->UpdateSpellCritChance(school);
}

void AuraEffect::HandleAuraModCritPct(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
    {
        target->m_baseSpellCritChance += (apply) ? GetAmount():-GetAmount();
        return;
    }

    target->ToPlayer()->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float (GetAmount()), apply);
    target->ToPlayer()->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float (GetAmount()), apply);
    target->ToPlayer()->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float (GetAmount()), apply);

    // included in Player::UpdateSpellCritChance calculation
    target->ToPlayer()->UpdateAllSpellCritChances();
}

/********************************/
/***         ATTACK SPEED     ***/
/********************************/

void AuraEffect::HandleModCastingSpeed(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    target->ApplyCastTimePercentMod((float)GetAmount(), apply);
}

void AuraEffect::HandleModMeleeRangedSpeedPct(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    target->ApplyAttackTimePercentMod(BASE_ATTACK, (float)GetAmount(), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, (float)GetAmount(), apply);
    target->ApplyAttackTimePercentMod(RANGED_ATTACK, (float)GetAmount(), apply);
}

void AuraEffect::HandleModCombatSpeedPct(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    target->ApplyCastTimePercentMod(float(m_amount), apply);
    target->ApplyAttackTimePercentMod(BASE_ATTACK, float(GetAmount()), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, float(GetAmount()), apply);
    target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(GetAmount()), apply);
}

void AuraEffect::HandleModAttackSpeed(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    target->ApplyAttackTimePercentMod(BASE_ATTACK, (float)GetAmount(), apply);
    target->UpdateDamagePhysical(BASE_ATTACK);
}

void AuraEffect::HandleModMeleeSpeedPct(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    target->ApplyAttackTimePercentMod(BASE_ATTACK,   (float)GetAmount(), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK,    (float)GetAmount(), apply);
}

void AuraEffect::HandleAuraModRangedHaste(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    target->ApplyAttackTimePercentMod(RANGED_ATTACK, (float)GetAmount(), apply);
}

/********************************/
/***       COMBAT RATING      ***/
/********************************/

void AuraEffect::HandleModRating(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
        if (GetMiscValue() & (1 << rating))
            target->ToPlayer()->ApplyRatingMod(CombatRating(rating), GetAmount(), apply);
}

void AuraEffect::HandleModRatingFromStat(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Just recalculate ratings
    for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
        if (GetMiscValue() & (1 << rating))
            target->ToPlayer()->ApplyRatingMod(CombatRating(rating), 0, apply);
}

/********************************/
/***        ATTACK POWER      ***/
/********************************/

void AuraEffect::HandleAuraModAttackPower(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    target->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(GetAmount()), apply);
}

void AuraEffect::HandleAuraModRangedAttackPower(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if ((target->getClassMask() & CLASSMASK_WAND_USERS) != 0)
        return;

    target->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(GetAmount()), apply);
}

void AuraEffect::HandleAuraModAttackPowerPercent(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    //UNIT_FIELD_ATTACK_POWER_MULTIPLIER = multiplier - 1
    target->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, float(GetAmount()), apply);
}

void AuraEffect::HandleAuraModRangedAttackPowerPercent(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    if ((target->getClassMask() & CLASSMASK_WAND_USERS) != 0)
        return;

    //UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER = multiplier - 1
    target->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT, float(GetAmount()), apply);
}

void AuraEffect::HandleAuraModRangedAttackPowerOfStatPercent(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    // Recalculate bonus
    if (target->GetTypeId() == TYPEID_PLAYER && !(target->getClassMask() & CLASSMASK_WAND_USERS))
        target->ToPlayer()->UpdateAttackPowerAndDamage(true);
}

void AuraEffect::HandleAuraModAttackPowerOfStatPercent(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    HandleAuraModAttackPowerOfArmor(aurApp, mode, apply);
}

void AuraEffect::HandleAuraModAttackPowerOfArmor(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    // Recalculate bonus
    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->UpdateAttackPowerAndDamage(false);
}
/********************************/
/***        DAMAGE BONUS      ***/
/********************************/
void AuraEffect::HandleModDamageDone(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    // apply item specific bonuses for already equipped weapon
    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
            if (Item* pItem = target->ToPlayer()->GetWeaponForAttack(WeaponAttackType(i), true))
                target->ToPlayer()->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
    }

    // GetMiscValue() is bitmask of spell schools
    // 1 (0-bit) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wands
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // GetMiscValue() comparison with item generated damage types

    if ((GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->GetEquippedItemClass() == -1 || target->GetTypeId() != TYPEID_PLAYER)
        {
            target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(GetAmount()), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(GetAmount()), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(GetAmount()), apply);

            if (target->GetTypeId() == TYPEID_PLAYER)
            {
                if (GetAmount() > 0)
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS, GetAmount(), apply);
                else
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG, GetAmount(), apply);
            }
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }
    }

    // Skip non magic case for speedup
    if ((GetMiscValue() & SPELL_SCHOOL_MASK_MAGIC) == 0)
        return;

    if (GetSpellProto()->GetEquippedItemClass() != -1 || GetSpellProto()->GetEquippedItemInventoryTypeMask() != 0)
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage modifiers implemented in Unit::SpellDamageBonus
    // This information for client side use only
    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        if (GetAmount() > 0)
        {
            for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; i++)
            {
                if ((GetMiscValue() & (1<<i)) != 0)
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS+i, GetAmount(), apply);
            }
        }
        else
        {
            for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; i++)
            {
                if ((GetMiscValue() & (1<<i)) != 0)
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG+i, GetAmount(), apply);
            }
        }
        if (Guardian* pet = target->ToPlayer()->GetGuardianPet())
            pet->UpdateAttackPowerAndDamage();
    }
}

void AuraEffect::HandleModDamagePercentDone(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Player* target = aurApp->GetTarget()->ToPlayer();
    if (!target)
        return;

    if (target->HasItemFitToSpellRequirements(GetSpellProto()))
        aurApp->GetTarget()->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT, GetAmount()/100.0f, apply);
}

void AuraEffect::HandleModOffhandDamagePercent(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(GetAmount()), apply);
}

void AuraEffect::HandleShieldBlockValue(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit * target = aurApp->GetTarget();

    BaseModType modType = FLAT_MOD;
    if (GetAuraType() == SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT)
        modType = PCT_MOD;

    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->HandleBaseModValue(SHIELD_BLOCK_VALUE, modType, float(GetAmount()), apply);
}

/********************************/
/***        POWER COST        ***/
/********************************/

void AuraEffect::HandleModPowerCostPCT(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    float amount = CalculatePctN(1.0f, GetAmount());
    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if (GetMiscValue() & (1 << i))
            target->ApplyModSignedFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + i, amount, apply);
}

void AuraEffect::HandleModPowerCost(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if (GetMiscValue() & (1<<i))
            target->ApplyModInt32Value(UNIT_FIELD_POWER_COST_MODIFIER+i, GetAmount(), apply);
}

void AuraEffect::HandleArenaPreparation(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION);
    else
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION);
    }
}

void AuraEffect::HandleNoReagentUseAura(AuraApplication const * aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    flag96 mask;
    Unit::AuraEffectList const& noReagent = target->GetAuraEffectsByType(SPELL_AURA_NO_REAGENT_USE);
  /*      for (Unit::AuraEffectList::const_iterator i = noReagent.begin(); i !=  noReagent.end(); ++i)
            mask |= (*i)->m_spellProto->GetEffectSpellClassMask((*i)->m_effIndex);*/

    target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1  , mask[0]);
    target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1+1, mask[1]);
    target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1+2, mask[2]);
}

void AuraEffect::HandleAuraRetainComboPoints(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // combo points was added in SPELL_EFFECT_ADD_COMBO_POINTS handler
    // remove only if aura expire by time (in case combo points amount change aura removed without combo points lost)
    if (!(apply) && GetBase()->GetDuration() == 0 && target->ToPlayer()->GetComboTarget())
        if (Unit* unit = ObjectAccessor::GetUnit(*target, target->ToPlayer()->GetComboTarget()))
            target->ToPlayer()->AddComboPoints(unit, -GetAmount());
}

/*********************************************************/
/***                    OTHERS                         ***/
/*********************************************************/

void AuraEffect::HandleAuraDummy(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    Unit * caster = GetCaster();

    if (mode & AURA_EFFECT_HANDLE_REAL)
    {
        // AT APPLY
        if (apply)
        {
            // Overpower
            if (caster && m_spellProto->GetSpellFamilyName() == SPELLFAMILY_WARRIOR &&
                m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x4)
            {
                // In addition, if you strike a player..
                if (target->GetTypeId() != TYPEID_PLAYER)
                    return;
                //  ..while they are casting
                if (target->IsNonMeleeSpellCasted(false, false, true, false, false))
                    if (AuraEffect * aurEff = caster->GetAuraEffect(SPELL_AURA_ADD_FLAT_MODIFIER, SPELLFAMILY_WARRIOR, 2775, 0))
                        switch (aurEff->GetId())
                        {
                            // Unrelenting Assault, rank 1
                            case 46859:
                                target->CastSpell(target, 64849, true, NULL, aurEff);
                                break;
                            // Unrelenting Assault, rank 2
                            case 46860:
                                target->CastSpell(target, 64850, true, NULL, aurEff);
                                break;
                        }
            }
            switch(GetId())
            {
                case 1515:                                      // Tame beast
                    // FIX_ME: this is 2.0.12 threat effect replaced in 2.1.x by dummy aura, must be checked for correctness
                    if (caster && target->CanHaveThreatList())
                        target->AddThreat(caster, 10.0f);
                    break;
                case 13139:                                     // net-o-matic
                    // root to self part of (root_target->charge->root_self sequence
                    if (caster)
                        caster->CastSpell(caster, 13138, true, NULL, this);
                    break;
                case 34026:   // kill command
                {
                    Unit * pet = target->GetGuardianPet();
                    if (!pet)
                        break;

                    target->CastSpell(target, 34027, true, NULL, this);

                    // set 3 stacks and 3 charges (to make all auras not disappear at once)
                    Aura * owner_aura = target->GetAura(34027, GetCasterGUID());
                    Aura * pet_aura  = pet->GetAura(58914, GetCasterGUID());
                    if (owner_aura)
                    {
                        owner_aura->SetStackAmount(owner_aura->GetSpellProto()->GetStackAmount());
                    }
                    if (pet_aura)
                    {
                        pet_aura->SetCharges(0);
                        pet_aura->SetStackAmount(owner_aura->GetSpellProto()->GetStackAmount());
                    }
                    break;
                }
                case 37096:                                     // Blood Elf Illusion
                {
                    if (caster)
                    {
                        switch(caster->getGender())
                        {
                            case GENDER_FEMALE:
                                caster->CastSpell(target, 37095, true, NULL, this); // Blood Elf Disguise
                                break;
                            case GENDER_MALE:
                                caster->CastSpell(target, 37093, true, NULL, this);
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                }
                case 55198:   // Tidal Force
                {
                    target->CastSpell(target, 55166, true, NULL, this);
                    // set 3 stacks and 3 charges (to make all auras not disappear at once)
                    Aura * owner_aura = target->GetAura(55166, GetCasterGUID());
                    if (owner_aura)
                    {
                        // This aura lasts 2 sec, need this hack to properly proc spells
                        // TODO: drop aura charges for ApplySpellMod in ProcDamageAndSpell
                        GetBase()->SetDuration(owner_aura->GetDuration());
                        // Make aura be not charged-this prevents removing charge on not crit spells
                        owner_aura->SetCharges(0);
                        owner_aura->SetStackAmount(owner_aura->GetSpellProto()->GetStackAmount());
                    }
                    break;
                }
                case 39850:                                     // Rocket Blast
                    if (roll_chance_i(20))                       // backfire stun
                        target->CastSpell(target, 51581, true, NULL, this);
                    break;
                case 43873:                                     // Headless Horseman Laugh
                    target->PlayDistanceSound(11965);
                    break;
                case 46354:                                     // Blood Elf Illusion
                    if (caster)
                    {
                        switch(caster->getGender())
                        {
                            case GENDER_FEMALE:
                                caster->CastSpell(target, 46356, true, NULL, this);
                                break;
                            case GENDER_MALE:
                                caster->CastSpell(target, 46355, true, NULL, this);
                                break;
                        }
                    }
                    break;
                case 46361:                                     // Reinforced Net
                    if (caster)
                    {
                        float currentGroundLevel = target->GetBaseMap()->GetHeight(target->GetPositionX(), target->GetPositionY(), MAX_HEIGHT);
                        if (target->GetPositionZ() > currentGroundLevel)
                            target->GetMotionMaster()->MoveFall(currentGroundLevel);
                    }
                    break;
                case 49028:
                    if (caster)
                        if (AuraEffect *aurEff = caster->GetAuraEffect(63330, 0)) // glyph of Dancing Rune Weapon
                            GetBase()->SetDuration(GetBase()->GetDuration() + aurEff->GetAmount());
                    break;
                case 52916: // Honor Among Thieves
                    if (target->GetTypeId() == TYPEID_PLAYER)
                        if (Unit * spellTarget = ObjectAccessor::GetUnit(*target, target->ToPlayer()->GetComboTarget()))
                            target->CastSpell(spellTarget, 51699, true);
                   break;
                case 28832: // Mark of Korth'azz
                case 28833: // Mark of Blaumeux
                case 28834: // Mark of Rivendare
                case 28835: // Mark of Zeliek
                    if (caster) // actually we can also use cast(this, originalcasterguid)
                    {
                        int32 damage;
                        switch(GetBase()->GetStackAmount())
                        {
                            case 1: damage = 0;     break;
                            case 2: damage = 500;   break;
                            case 3: damage = 1000;  break;
                            case 4: damage = 1500;  break;
                            case 5: damage = 4000;  break;
                            case 6: damage = 12000; break;
                            default:damage = 20000 + 1000 * (GetBase()->GetStackAmount() - 7); break;
                        }
                        if (damage)
                            caster->CastCustomSpell(28836, SPELLVALUE_BASE_POINT0, damage, target);
                    }
                    break;
                case 63322: // Saronite Vapors
                {
                    int32 mana = int32(GetAmount() * pow(2.0f, GetBase()->GetStackAmount())); // mana restore - bp * 2^stackamount
                    int32 damage = mana * 2; // damage
                    caster->CastCustomSpell(target, 63337, &mana, NULL, NULL, true);
                    caster->CastCustomSpell(target, 63338, &damage, NULL, NULL, true);
                    break;
                }
                case 71563:
                    if (Aura* newAura = target->AddAura(71564, target))
                        newAura->SetStackAmount(newAura->GetSpellProto()->GetStackAmount());
            }
        }
        // AT REMOVE
        else
        {
            if ((IsQuestTameSpell(GetId())) && caster && caster->isAlive() && target->isAlive())
            {
                uint32 finalSpelId = 0;
                switch(GetId())
                {
                    case 19548: finalSpelId = 19597; break;
                    case 19674: finalSpelId = 19677; break;
                    case 19687: finalSpelId = 19676; break;
                    case 19688: finalSpelId = 19678; break;
                    case 19689: finalSpelId = 19679; break;
                    case 19692: finalSpelId = 19680; break;
                    case 19693: finalSpelId = 19684; break;
                    case 19694: finalSpelId = 19681; break;
                    case 19696: finalSpelId = 19682; break;
                    case 19697: finalSpelId = 19683; break;
                    case 19699: finalSpelId = 19685; break;
                    case 19700: finalSpelId = 19686; break;
                    case 30646: finalSpelId = 30647; break;
                    case 30653: finalSpelId = 30648; break;
                    case 30654: finalSpelId = 30652; break;
                    case 30099: finalSpelId = 30100; break;
                    case 30102: finalSpelId = 30103; break;
                    case 30105: finalSpelId = 30104; break;
                }

                if (finalSpelId)
                    caster->CastSpell(target, finalSpelId, true, NULL, this);
            }

            switch(m_spellProto->GetSpellFamilyName())
            {
                case SPELLFAMILY_GENERIC:
                    switch(GetId())
                    {
                        case 2584: // Waiting to Resurrect
                            // Waiting to resurrect spell cancel, we must remove player from resurrect queue
                            if (target->GetTypeId() == TYPEID_PLAYER)
                                if (Battleground *bg = target->ToPlayer()->GetBattleground())
                                    bg->RemovePlayerFromResurrectQueue(target->GetGUID());
                            break;
                        case 36730:                                     // Flame Strike
                        {
                            target->CastSpell(target, 36731, true, NULL, this);
                            break;
                        }
                        case 44191:                                     // Flame Strike
                        {
                            if (target->GetMap()->IsDungeon())
                            {
                                uint32 spellId = target->GetMap()->IsHeroic() ? 46163 : 44190;

                                target->CastSpell(target, spellId, true, NULL, this);
                            }
                            break;
                        }
                        case 43681: // Inactive
                        {
                            if (!target || target->GetTypeId() != TYPEID_PLAYER || aurApp->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
                                return;

                            if (target->GetMap()->IsBattleground())
                                target->ToPlayer()->LeaveBattleground();
                            break;
                        }
                        case 42783: // Wrath of the Astromancer
                            target->CastSpell(target, GetAmount(), true, NULL, this);
                            break;
                        case 46308: // Burning Winds casted only at creatures at spawn
                            target->CastSpell(target, 47287, true, NULL, this);
                            break;
                        case 52172:  // Coyote Spirit Despawn Aura
                        case 60244:  // Blood Parrot Despawn Aura
                            target->CastSpell((Unit*)NULL, GetAmount(), true, NULL, this);
                            break;
                        case 58600: // Restricted Flight Area
                            if (aurApp->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                                target->CastSpell(target, 58601, true);
                            break;
                    }
                    break;
                case SPELLFAMILY_MAGE:
                    // Living Bomb
                    if (m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[1] & 0x20000)
                    {
                        AuraRemoveMode mode = aurApp->GetRemoveMode();
                        if (caster && (mode == AURA_REMOVE_BY_ENEMY_SPELL || mode == AURA_REMOVE_BY_EXPIRE))
                            caster->CastSpell(target, GetAmount(), true);
                    }
                    break;
                case SPELLFAMILY_WARLOCK:
                    // Haunt
                    if (m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[1] & 0x40000)
                    {
                        if (caster)
                            caster->CastCustomSpell(caster, 48210, &m_amount, 0, 0, true, NULL, this);
                    }
                    break;
                case SPELLFAMILY_DRUID:
                    // Lifebloom
                    if (GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[1] & 0x10)
                    {
                        // Final heal only on duration end
                        if (aurApp->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
                            return;

                        // final heal
                        int32 stack = GetBase()->GetStackAmount();
                        target->CastCustomSpell(target, 33778, &m_amount, &stack, NULL, true, NULL, this, GetCasterGUID());

                        // restore mana
                        if (caster)
                        {
                            int32 returnmana = CalculatePctU(caster->GetCreateMana(), GetSpellProto()->GetManaCostPercentage()) * stack / 2;
                            caster->CastCustomSpell(caster, 64372, &returnmana, NULL, NULL, true, NULL, this, GetCasterGUID());
                        }
                    }
                    break;
                case SPELLFAMILY_PRIEST:
                    // Vampiric Touch
                    if (m_spellProto->GetSpellClassOptions()->SpellFamilyFlags[1] & 0x0400 && aurApp->GetRemoveMode() == AURA_REMOVE_BY_ENEMY_SPELL && GetEffIndex() == 0)
                    {
                        if (AuraEffect const * aurEff = GetBase()->GetEffect(1))
                        {
                            int32 damage = aurEff->GetAmount() * 8;
                            // backfire damage
                            target->CastCustomSpell(target, 64085, &damage, NULL, NULL, true, NULL, NULL, GetCasterGUID());
                        }
                    }
                    break;
                case SPELLFAMILY_HUNTER:
                    // Misdirection
                    if (GetId() == 34477)
                        target->SetReducedThreatPercent(0, 0);
                    break;
                case SPELLFAMILY_DEATHKNIGHT:
                    // Summon Gargoyle (will start feeding gargoyle)
                    if (GetId() == 61777)
                        target->CastSpell(target, m_spellProto->GetEffectTriggerSpell(m_effIndex), true);
                    break;
                default:
                    break;
            }
        }
    }

    // AT APPLY & REMOVE

    switch(m_spellProto->GetSpellFamilyName())
    {
        case SPELLFAMILY_GENERIC:
        {
            if (!(mode & AURA_EFFECT_HANDLE_REAL))
                break;
            switch(GetId())
            {
                // Recently Bandaged
                case 11196:
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, GetMiscValue(), apply);
                    break;
                // Unstable Power
                case 24658:
                {
                    uint32 spellId = 24659;
                    if (apply && caster)
                    {
                        SpellEntry const * spell = sSpellStore.LookupEntry(spellId);

                        for (uint32 i = 0; i < spell->GetStackAmount(); ++i)
                            caster->CastSpell(target, spell->Id, true, NULL, NULL, GetCasterGUID());
                        break;
                    }
                    target->RemoveAurasDueToSpell(spellId);
                    break;
                }
                // Restless Strength
                case 24661:
                {
                    uint32 spellId = 24662;
                    if (apply && caster)
                    {
                        SpellEntry const * spell = sSpellStore.LookupEntry(spellId);
                        for (uint32 i = 0; i < spell->GetStackAmount(); ++i)
                            caster->CastSpell(target, spell->Id, true, NULL, NULL, GetCasterGUID());
                        break;
                    }
                    target->RemoveAurasDueToSpell(spellId);
                    break;
                }
                // Tag Murloc
                case 30877:
                {
                    // Tag/untag Blacksilt Scout
                    target->SetEntry(apply ? 17654 : 17326);
                    break;
                }
                //Summon Fire Elemental
                case 40133:
                {
                    if (!caster)
                        break;

                    Unit *owner = caster->GetOwner();
                    if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (apply)
                            owner->CastSpell(owner, 8985, true);
                        else
                            owner->ToPlayer()->RemovePet(NULL, PET_SAVE_NOT_IN_SLOT, true);
                    }
                    break;
                }
                //Summon Earth Elemental
                case 40132 :
                {
                    if (!caster)
                        break;

                    Unit *owner = caster->GetOwner();
                    if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (apply)
                            owner->CastSpell(owner, 19704, true);
                        else
                            owner->ToPlayer()->RemovePet(NULL, PET_SAVE_NOT_IN_SLOT, true);
                    }
                    break;
                }
                case 57723: // Exhaustion
                case 57724: // Sated
                {
                    switch(GetId())
                    {
                        case 57723: target->ApplySpellImmune(GetId(), IMMUNITY_ID, 32182, apply); break; // Heroism
                        case 57724: target->ApplySpellImmune(GetId(), IMMUNITY_ID, 2825, apply);  break; // Bloodlust
                    }
                    break;
                }
                case 57819: // Argent Champion
                case 57820: // Ebon Champion
                case 57821: // Champion of the Kirin Tor
                case 57822: // Wyrmrest Champion
                {
                    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                        break;

                    uint32 FactionID = 0;

                    if (apply)
                    {
                        switch(m_spellProto->Id)
                        {
                            case 57819: FactionID = 1106; break; // Argent Crusade
                            case 57820: FactionID = 1098; break; // Knights of the Ebon Blade
                            case 57821: FactionID = 1090; break; // Kirin Tor
                            case 57822: FactionID = 1091; break; // The Wyrmrest Accord
                        }
                    }
                    caster->ToPlayer()->SetChampioningFaction(FactionID);
                    break;
                }
                // LK Intro VO (1)
                case 58204:
                    if (target->GetTypeId() == TYPEID_PLAYER)
                    {
                        // Play part 1
                        if (apply)
                            target->PlayDirectSound(14970, target->ToPlayer());
                        // continue in 58205
                        else
                            target->CastSpell(target, 58205, true);
                    }
                    break;
                // LK Intro VO (2)
                case 58205:
                    if (target->GetTypeId() == TYPEID_PLAYER)
                    {
                        // Play part 2
                        if (apply)
                            target->PlayDirectSound(14971, target->ToPlayer());
                        // Play part 3
                        else
                            target->PlayDirectSound(14972, target->ToPlayer());
                    }
                    break;
                case 62061: // Festive Holiday Mount
                    if (target->HasAuraType(SPELL_AURA_MOUNTED))
                    {
                        uint32 creatureEntry = 0;
                        if (apply)
                        {
                            if (target->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED))
                                creatureEntry = 24906;
                            else
                                creatureEntry = 15665;
                        }
                        else
                            creatureEntry = target->GetAuraEffectsByType(SPELL_AURA_MOUNTED).front()->GetMiscValue();

                        if (CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(creatureEntry))
                        {
                            uint32 team = 0;
                            if (target->GetTypeId() == TYPEID_PLAYER)
                                team = target->ToPlayer()->GetTeam();

                            uint32 displayID = sObjectMgr->ChooseDisplayId(team, creatureInfo);
                            sObjectMgr->GetCreatureModelRandomGender(&displayID);

                            target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, displayID);
                        }
                    }
                    break;
            }

            break;
        }
        case SPELLFAMILY_MAGE:
        {
            //if (!(mode & AURA_EFFECT_HANDLE_REAL))
                //break;
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            //if (!(mode & AURA_EFFECT_HANDLE_REAL))
                //break;
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
                break;
            switch(GetId())
            {
                case 52610:                                 // Savage Roar
                {
                    uint32 spellId = 62071;
                    if (apply)
                    {
                        if (target->GetShapeshiftForm() != FORM_CAT)
                            break;

                        target->CastSpell(target, spellId, true, NULL, NULL, GetCasterGUID());
                        break;
                    }
                    target->RemoveAurasDueToSpell(spellId);
                    break;
                }
                case 61336:                                 // Survival Instincts
                {
                    if (!(mode & AURA_EFFECT_HANDLE_REAL))
                        break;

                    if (apply)
                    {
                        if (!target->IsInFeralForm())
                            break;

                        int32 bp0 = int32(target->CountPctFromMaxHealth(GetAmount()));
                        target->CastCustomSpell(target, 50322, &bp0, NULL, NULL, true);
                    }
                    else
                        target-> RemoveAurasDueToSpell(50322);
                    break;
                }
            }
            // Predatory Strikes
            if (target->GetTypeId() == TYPEID_PLAYER && GetSpellProto()->SpellIconID == 1563)
            {
                target->ToPlayer()->UpdateAttackPowerAndDamage();
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            if (!(mode & AURA_EFFECT_HANDLE_REAL))
                break;
            // Sentry Totem
            if (GetId() == 6495 && caster->GetTypeId() == TYPEID_PLAYER)
            {
                if (apply)
                {
                    uint64 guid = caster->m_SummonSlot[3];
                    if (guid)
                    {
                        Creature *totem = caster->GetMap()->GetCreature(guid);
                        if (totem && totem->isTotem())
                            caster->ToPlayer()->CastSpell(totem, 6277, true);
                    }
                }
                else
                    caster->ToPlayer()->StopCastingBindSight();
                return;
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
            // if (!(mode & AURA_EFFECT_HANDLE_REAL))
            //    break;
            break;
    }

    // stop handling the effect if it was removed by linked event
    if (apply && aurApp->GetRemoveMode())
        return;

    if (mode & AURA_EFFECT_HANDLE_REAL)
    {
        // pet auras
        if (PetAura const* petSpell = sSpellMgr->GetPetAura(GetId(), m_effIndex))
        {
            if (apply)
                target->AddPetAura(petSpell);
            else
                target->RemovePetAura(petSpell);
        }
    }
}

void AuraEffect::HandleChannelDeathItem(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    if (!(apply))
    {
        Unit * caster = GetCaster();

        if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
            return;

        Player *plCaster = caster->ToPlayer();
        Unit *target = aurApp->GetTarget();

        if (target->getDeathState() != JUST_DIED)
            return;

        // Item amount
        if (GetAmount() <= 0)
            return;

        if (GetSpellProto()->GetEffectItemType(m_effIndex) == 0)
            return;

        // Soul Shard
        if (GetSpellProto()->GetEffectItemType(m_effIndex) == 6265)
        {
            // Soul Shard only from units that grant XP or honor
            if (!plCaster->isHonorOrXPTarget(target) ||
                (target->GetTypeId() == TYPEID_UNIT && !target->ToCreature()->isTappedBy(plCaster)))
                return;

            // If this is Drain Soul, check for Glyph of Drain Soul
            if (GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_WARLOCK && (GetSpellProto()->GetSpellClassOptions()->SpellFamilyFlags[0] & 0x00004000))
            {
                // Glyph of Drain Soul - chance to create an additional Soul Shard
                if (AuraEffect *aur = caster->GetAuraEffect(58070, 0))
                    if (roll_chance_i(aur->GetMiscValue()))
                        caster->CastSpell(caster, 58068, true, 0, aur); // We _could_ simply do ++count here, but Blizz does it this way :)
            }
        }

        //Adding items
        uint32 noSpaceForCount = 0;
        uint32 count = m_amount;

        ItemPosCountVec dest;
        InventoryResult msg = plCaster->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, GetSpellProto()->GetEffectItemType(m_effIndex), count, &noSpaceForCount);
        if (msg != EQUIP_ERR_OK)
        {
            count-=noSpaceForCount;
            plCaster->SendEquipError(msg, NULL, NULL, GetSpellProto()->GetEffectItemType(m_effIndex));
            if (count == 0)
                return;
        }

        Item* newitem = plCaster->StoreNewItem(dest, GetSpellProto()->GetEffectItemType(m_effIndex), true);
        if (!newitem)
        {
            plCaster->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
            return;
        }
        plCaster->SendNewItem(newitem, count, true, true);
    }
}

void AuraEffect::HandleBindSight(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    Unit * caster = GetCaster();

    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    caster->ToPlayer()->SetViewpoint(target, (apply));
}

void AuraEffect::HandleForceReaction(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = (Player*)target;

    uint32 faction_id = GetMiscValue();
    ReputationRank faction_rank = ReputationRank(m_amount);

    player->GetReputationMgr().ApplyForceReaction(faction_id, faction_rank, apply);
    player->GetReputationMgr().SendForceReactions();

    // stop fighting if at apply forced rank friendly or at remove real rank friendly
    if ((apply && faction_rank >= REP_FRIENDLY) || (!apply && player->GetReputationRank(faction_id) >= REP_FRIENDLY))
        player->StopAttackFaction(faction_id);
}

void AuraEffect::HandleAuraEmpathy(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_UNIT)
        return;

    if (!(apply))
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
    }

    CreatureTemplate const * ci = sObjectMgr->GetCreatureTemplate(target->GetEntry());
    if (ci && ci->type == CREATURE_TYPE_BEAST)
        target->ApplyModUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_SPECIALINFO, apply);
}

void AuraEffect::HandleAuraModFaction(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
    {
        target->setFaction(GetMiscValue());
        if (target->GetTypeId() == TYPEID_PLAYER)
            target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    }
    else
    {
        target->RestoreFaction();
        if (target->GetTypeId() == TYPEID_PLAYER)
            target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    }
}

void AuraEffect::HandleComprehendLanguage(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_COMPREHEND_LANG);
    else
    {
        if (target->HasAuraType(GetAuraType()))
            return;

        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_COMPREHEND_LANG);
    }
}

void AuraEffect::HandleAuraConvertRune(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *plr = (Player*)target;

    if (plr->getClass() != CLASS_DEATH_KNIGHT)
        return;

    uint32 runes = m_amount;
    // convert number of runes specified in aura amount of rune type in miscvalue to runetype in miscvalueb
    if (apply)
    {
        for (uint32 i = 0; i < MAX_RUNES && runes; ++i)
        {
            if (GetMiscValue() != plr->GetCurrentRune(i))
                continue;
            if (!plr->GetRuneCooldown(i))
            {
                plr->AddRuneByAuraEffect(i, RuneType(GetMiscValueB()), this);
                --runes;
            }
        }
    }
    else
        plr->RemoveRunesByAuraEffect(this);
}

void AuraEffect::HandleAuraLinked(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (apply)
    {
        Unit * caster = GetTriggeredSpellCaster(m_spellProto, GetCaster(), target);

        if (!caster)
            return;
        // If amount avalible cast with basepoints (Crypt Fever for example)
        if (GetAmount())
            caster->CastCustomSpell(target, m_spellProto->GetEffectTriggerSpell(m_effIndex), &m_amount, NULL, NULL, true, NULL, this);
        else
            caster->CastSpell(target, m_spellProto->GetEffectTriggerSpell(m_effIndex), true, NULL, this);
    }
    else
        target->RemoveAura(m_spellProto->GetEffectTriggerSpell(m_effIndex), GetCasterGUID(), 0, AuraRemoveMode(aurApp->GetRemoveMode()));
}

void AuraEffect::HandleAuraOpenStable(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER || !target->IsInWorld())
        return;

    if (apply)
        target->ToPlayer()->GetSession()->SendStablePet(target->GetGUID());

     // client auto close stable dialog at !apply aura
}

void AuraEffect::HandleAuraModFakeInebriation(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
    {
        target->m_invisibilityDetect.AddFlag( INVISIBILITY_DRUNK);
        target->m_invisibilityDetect.AddValue(INVISIBILITY_DRUNK, GetAmount());

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            int32 oldval = target->ToPlayer()->GetInt32Value(PLAYER_FAKE_INEBRIATION);
            target->ToPlayer()->SetInt32Value(PLAYER_FAKE_INEBRIATION, oldval + GetAmount());
        }
    }
    else
    {
        bool removeDetect = !target->HasAuraType(SPELL_AURA_MOD_FAKE_INEBRIATE);

        target->m_invisibilityDetect.AddValue(INVISIBILITY_DRUNK, -GetAmount());

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            int32 oldval = target->ToPlayer()->GetInt32Value(PLAYER_FAKE_INEBRIATION);
            target->ToPlayer()->SetInt32Value(PLAYER_FAKE_INEBRIATION, oldval - GetAmount());

            if (removeDetect)
                removeDetect = !target->ToPlayer()->GetDrunkValue();
        }

        if (removeDetect)
            target->m_invisibilityDetect.DelFlag(INVISIBILITY_DRUNK);
    }

    target->UpdateObjectVisibility();
}

void AuraEffect::HandleAuraOverrideSpells(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Player * target = aurApp->GetTarget()->ToPlayer();

    if (!target || !target->IsInWorld())
        return;

    uint32 overrideId = uint32(GetMiscValue());

    if (apply)
    {
        target->SetUInt16Value(PLAYER_FIELD_BYTES2, 0, overrideId);
        if (OverrideSpellDataEntry const* overrideSpells = sOverrideSpellDataStore.LookupEntry(overrideId))
            for (uint8 i = 0; i < MAX_OVERRIDE_SPELL; ++i)
                if (uint32 spellId = overrideSpells->spellId[i])
                    target->AddTemporarySpell(spellId);
    }
    else
    {
        target->SetUInt16Value(PLAYER_FIELD_BYTES2, 0, 0);
        if (OverrideSpellDataEntry const* overrideSpells = sOverrideSpellDataStore.LookupEntry(overrideId))
            for (uint8 i = 0; i < MAX_OVERRIDE_SPELL; ++i)
                if (uint32 spellId = overrideSpells->spellId[i])
                    target->RemoveTemporarySpell(spellId);
    }
}

void AuraEffect::HandleAuraSetVehicle(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit * target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER || !target->IsInWorld())
        return;

    uint32 vehicleId = GetMiscValue();

    if (apply)
    {
        if (!target->CreateVehicleKit(vehicleId, 0))
            return;
    }
    else if (target->GetVehicleKit())
        target->RemoveVehicleKit();

    WorldPacket data(SMSG_PLAYER_VEHICLE_DATA, target->GetPackGUID().size()+4);
    data.appendPackGUID(target->GetGUID());
    data << uint32(apply ? vehicleId : 0);
    target->SendMessageToSet(&data, true);

    if (apply)
    {
        data.Initialize(SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA, 0);
        target->ToPlayer()->GetSession()->SendPacket(&data);
    }
}
