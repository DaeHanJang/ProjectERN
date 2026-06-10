// Copyright Epic Games, Inc. All Rights Reserved.

#include "GAS/ERNGameplayTags.h"

// 캐릭터 입력 태그
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Move, "Input.Move")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Look, "Input.Look")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_MouseLook, "Input.MouseLook")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Jump, "Input.Jump")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Roll, "Input.Roll")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_LightAttack, "Input.LightAttack")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_HeavyAttack, "Input.HeavyAttack")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_LockOn, "Input.LockOn")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Sprint, "Input.Sprint")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Flask, "Input.Flask")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_UseConsumable, "Input.UseConsumable")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_NormalSkill, "Input.NormalSkill")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_UltimateSkill, "Input.UltimateSkill")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Consumable, "Input.Consumable")

// 전투 상태
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Attacking, "State.Combat.Attacking")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Blocking, "State.Combat.Blocking")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Rolling, "State.Combat.Rolling")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Jumping, "State.Combat.Jumping")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_UsingSkill, "State.Combat.UsingSkill")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_UsingSkill_Normal, "State.Combat.UsingSkill.Normal")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_UsingSkill_Ultimate, "State.Combat.UsingSkill.Ultimate")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_OutOfCombat, "State.OutOfCombat")

// 어빌리티
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_Jump, "Ability.Movement.Jump")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_WallJump, "Ability.Movement.WallJump")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_Roll, "Ability.Movement.Roll")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_Sprint, "Ability.Movement.Sprint")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_Flask, "Ability.Movement.Flask")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_Consumable, "Ability.Movement.Consumable")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Attack_Light, "Ability.Attack.Light")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Attack_Heavy, "Ability.Attack.Heavy")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Skill_Normal, "Ability.Skill.Normal")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Skill_Ultimate, "Ability.Skill.Ultimate")

// 상태
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Stagger, "State.Stagger")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_SuperArmor, "State.SuperArmor")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_StaggerImmune, "State.StaggerImmune")

// 플레이어 생명 상태
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Life_Downed, "State.Life.Downed")

// 상태 (움직임)
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Falling, "State.Movement.Falling")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Landing, "State.Movement.Landing")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Sprinting, "State.Movement.Sprinting")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_WallJumpUsed, "State.Movement.WallJumpUsed")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Flask, "State.Movement.Flask")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Consumable, "State.Movement.Consumable")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_DashSkill, "State.Movement.DashSkill")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Immunity_Damage, "State.Immunity.Damage")

// 자원 관련
UE_DEFINE_GAMEPLAY_TAG(TAG_Data_Cost_Stamina, "Data.Cost.Stamina")
UE_DEFINE_GAMEPLAY_TAG(TAG_Data_Cost_Mana, "Data.Cost.Mana")
UE_DEFINE_GAMEPLAY_TAG(TAG_Data_Cooldown, "Data.Cooldown")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Regen_StaminaBlocked, "State.Regen.StaminaBlocked")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Regen_ManaBlocked, "State.Regen.ManaBlocked")
UE_DEFINE_GAMEPLAY_TAG(TAG_Data_Shield_Amount, "Data.Shield.Amount")
UE_DEFINE_GAMEPLAY_TAG(TAG_Data_Shield_Duration, "Data.Shield.Duration")
UE_DEFINE_GAMEPLAY_TAG(TAG_Data_Heal_Amount, "Data.Heal.Amount")

// 버프
UE_DEFINE_GAMEPLAY_TAG(TAG_Buff_AttackPower, "Buff.AttackPower")
UE_DEFINE_GAMEPLAY_TAG(TAG_Buff_Shield, "Buff.Shield")

// 캐릭터 스킬 쿨다운 상태
UE_DEFINE_GAMEPLAY_TAG(TAG_Cooldown_Skill_Normal, "Cooldown.Skill.Normal")
UE_DEFINE_GAMEPLAY_TAG(TAG_Cooldown_Skill_Ultimate, "Cooldown.Skill.Ultimate")

// GameplayCue 태그
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Cast_Buff_AttackPower, "GameplayCue.Cast.Buff.AttackPower")
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Buff_Shield_Aura, "GameplayCue.Buff.Shield.Aura")
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Ultimate_CrimsonJudgment_Cast,"GameplayCue.Ultimate.CrimsonJudgment.Cast")
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Ultimate_CrimsonJudgment_Explosion,"GameplayCue.Ultimate.CrimsonJudgment.Explosion")
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Ultimate_Mage_Cast,"GameplayCue.Ultimate.Mage.Cast")
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Ultimate_Mage_Explosion,"GameplayCue.Ultimate.Mage.Explosion")
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Ultimate_Sanctuary_Cast, "GameplayCue.Ultimate.Sanctuary.Cast")
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Ultimate_Sanctuary_Field, "GameplayCue.Ultimate.Sanctuary.Field")

// 이벤트
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Consumable_Throw, "Event.Consumable.Throw")
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Movement_JumpLaunch, "Event.Movement.JumpLaunch")