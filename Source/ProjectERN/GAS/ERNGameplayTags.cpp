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

// 전투 상태
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Attacking, "State.Combat.Attacking")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Blocking, "State.Combat.Blocking")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Rolling, "State.Combat.Rolling")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Jumping, "State.Combat.Jumping")

// 어빌리티
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_Jump, "Ability.Movement.Jump")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_Roll, "Ability.Movement.Roll")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Movement_Sprint, "Ability.Movement.Sprint")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Attack_Light, "Ability.Attack.Light")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Attack_Heavy, "Ability.Attack.Heavy")

// 상태
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Stagger, "State.Stagger")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_SuperArmor, "State.SuperArmor")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_StaggerImmune, "State.StaggerImmune")

// 상태 (움직임)
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Falling, "State.Movement.Falling")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Landing, "State.Movement.Landing")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Sprinting, "State.Movement.Sprinting")
