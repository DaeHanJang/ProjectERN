// Copyright Epic Games, Inc. All Rights Reserved.

#include "GAS/ERNGameplayTags.h"

// 전투 상태
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Attacking, "State.Combat.Attacking")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Blocking,  "State.Combat.Blocking")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Rolling,   "State.Combat.Rolling")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Jumping,   "State.Combat.Jumping")

// 어빌리티
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Move_Jump, "Ability.Move.Jump")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Move_Roll, "Ability.Move.Roll")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Attack_Light, "Ability.Attack.Light")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Attack_Heavy, "Ability.Attack.Heavy")

// 경직 상태
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Stagger,        "State.Stagger")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_SuperArmor,     "State.SuperArmor")
UE_DEFINE_GAMEPLAY_TAG(TAG_State_StaggerImmune,  "State.StaggerImmune")
