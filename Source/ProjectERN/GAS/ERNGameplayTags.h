// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// 캐릭터 입력 태그
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Move)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Look)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_MouseLook)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Jump)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Roll)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_LightAttack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_HeavyAttack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_LockOn)

// 전투 상태
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Attacking)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Blocking)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Rolling)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Jumping)

// 어빌리티
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Movement_Jump)		// 점프
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Movement_Roll)		// 회피(구르기)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Attack_Light)	// 일반 공격
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Attack_Heavy)	// 무기 스킬

// 경직 상태
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Stagger)			// 현재 경직 중
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_SuperArmor)		// 슈퍼아머 (경직 무시)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_StaggerImmune)		// 무적 프레임

// 상태(움직임)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Movement_Falling)	// 공중 상태
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Movement_Landing)	// 착지 모션 진행중
