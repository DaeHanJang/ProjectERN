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
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_UseConsumable, "Input.UseConsumable")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_NormalSkill, "Input.NormalSkill")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_UltimateSkill, "Input.UltimateSkill")

// 전투 상태
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Attacking, "State.Combat.Attacking")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Blocking, "State.Combat.Blocking")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Rolling, "State.Combat.Rolling")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Jumping, "State.Combat.Jumping")

// 어빌리티
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_Jump, "Ability.Movement.Jump")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_WallJump, "Ability.Movement.WallJump")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_Roll, "Ability.Movement.Roll")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_Sprint, "Ability.Movement.Sprint")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_UseConsumable, "Ability.Movement.UseConsumable")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Attack_Light, "Ability.Attack.Light")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Attack_Heavy, "Ability.Attack.Heavy")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Skill_Normal, "Ability.Skill.Normal")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Skill_Ultimate, "Ability.Skill.Ultimate")

// 상태
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Stagger, "State.Stagger")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_SuperArmor, "State.SuperArmor")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_StaggerImmune, "State.StaggerImmune")

// 상태 (움직임)
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Falling, "State.Movement.Falling")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Landing, "State.Movement.Landing")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Sprinting, "State.Movement.Sprinting")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_WallJumpUsed, "State.Movement.WallJumpUsed")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_DashSkill, "State.Movement.DashSkill")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Immunity_Damage, "State.Immunity.Damage")

// 자원 관련
UE_DEFINE_GAMEPLAY_TAG(TAG_Data_Cost_Stamina, "Data.Cost.Stamina")
UE_DEFINE_GAMEPLAY_TAG(TAG_Data_Cost_Mana, "Data.Cost.Mana")
UE_DEFINE_GAMEPLAY_TAG(TAG_Data_Cooldown, "Data.Cooldown")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Regen_StaminaBlocked, "State.Regen.StaminaBlocked")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Regen_ManaBlocked, "State.Regen.ManaBlocked")

// 버프
UE_DEFINE_GAMEPLAY_TAG(TAG_Buff_AttackPower, "Buff.AttackPower")

// 캐릭터 스킬 쿨다운 상태
UE_DEFINE_GAMEPLAY_TAG(TAG_Cooldown_Skill_Normal, "Cooldown.Skill.Normal")
UE_DEFINE_GAMEPLAY_TAG(TAG_Cooldown_Skill_Ultimate, "Cooldown.Skill.Ultimate")

// GameplayCue 태그
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Cast_Buff_AttackPower, "GameplayCue.Cast.Buff.AttackPower")