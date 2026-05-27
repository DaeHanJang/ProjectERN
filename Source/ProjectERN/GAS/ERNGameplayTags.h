// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// 캐릭터 입력 태그
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Move)			// 움직임 : WASD
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Look)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_MouseLook)		// 시야 : Mouse
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Jump)			// 점프 : SpaceBar
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Roll)			// 구르기(회피) : LShift
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_LightAttack)	// 일반 공격 : Mouse_LeftClick
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_HeavyAttack)	// 무기 스킬 : Mouse_RightClick
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_LockOn)		// 락온 : Mouse_WheelClick
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Sprint)		// 전력질주 : Alt
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Flask)         // 성배병 : 1
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Consumable)	// 소모품 사용 : 2
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_NormalSkill)	// 일반 스킬 사용 : Q
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_UltimateSkill)	// 궁극기 사용 : R

// 전투 상태
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Attacking)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Blocking)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Rolling)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_Jumping)

// 어빌리티
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Movement_Jump)		// 점프
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Movement_WallJump)	// 벽 점프
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Movement_Roll)		// 회피(구르기)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Movement_Sprint)		// 전력질주
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Attack_Light)		// 일반 공격
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Attack_Heavy)		// 무기 스킬
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Movement_Flask)      // 성배병 사용
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Movement_Consumable) // 소모품 사용
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Skill_Normal)		// 직업 일반 스킬
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Skill_Ultimate)		// 직업 궁극기 스킬

// 경직 상태
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Stagger)			// 현재 경직 중
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_SuperArmor)		// 슈퍼아머 (경직 무시)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_StaggerImmune)		// 피격 무적 프레임

// 상태(움직임)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Movement_Falling)	    // 공중 상태
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Movement_Landing)	    // 착지 모션 진행중
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Movement_Sprinting)	// 전력질주 상태
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Movement_WallJumpUsed)	// 벽점프 사용상태
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Movement_Flask)        // 성배병 상태
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Movement_Consumable)   // 소모품 사용중
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Movement_DashSkill)	// 대시 스킬 사용중
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Immunity_Damage)		// 대미지 면역 상태

// 자원 관련
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Cost_Stamina)	       // 소모할 스태미나
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Cost_Mana)		       // 소모할 마나
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Cooldown)		       // 스킬에 적용할 쿨다운
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Regen_StaminaBlocked) // 스태미나 재생 막기
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Regen_ManaBlocked)	   // 마나 재생 막기
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Shield_Amount)		   // 적용 실드량
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_Shield_Duration)	   // 실드 지속 시간

// 버프 적용
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Buff_AttackPower) // 공격력 증가 태그
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Buff_Shield)	     // 실드 적용 태그

// 캐릭터 스킬 쿨다운 상태
// 스킬은 캐릭터마다 Normal/Ultimate가 한 개씩 부여되기 때문에 스킬마다가 아닌 Normal/Ultimate Slot별로 쿨타임을 적용.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Skill_Normal)	// Normal 스킬 슬롯 쿨다운
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Skill_Ultimate)	// Ultimate 스킬 슬롯 쿨다운

// GameplayCue 태그
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_Cast_Buff_AttackPower)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_Buff_Shield_Aura)

// 이벤트
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Consumable_Throw)
