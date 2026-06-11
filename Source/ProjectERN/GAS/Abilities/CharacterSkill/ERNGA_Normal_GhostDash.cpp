// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_Normal_GhostDash.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Character/Player/ERNSkillNiagaraComponent.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Combat/Weapons/ERNWeaponBase.h"
#include "Components/CapsuleComponent.h"
#include "GAS/ERNGameplayTags.h"
#include "Inventory/Components/ERNEquipmentComponent.h"

UERNGA_Normal_GhostDash::UERNGA_Normal_GhostDash()
{
	// 어빌리티가 활성화되어 있는 동안 자동으로 부여되는 태그
	// EndAbility가 호출되면 GAS가 자동으로 제거
	ActivationOwnedTags.AddTag(TAG_State_Movement_DashSkill);
}

void UERNGA_Normal_GhostDash::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 충돌 제거
	ApplyGhostDashCollision(Character);
	// 투명도 적용
	ApplyGhostDashVisual(Character);
	// 나이아가라 출력
	StartGhostDashNiagaraEffects(Character);
	// 이동속도를 갱신
	Character->UpdateMovementSpeed();
	Character->UpdateRotationMode();

	// SkillBase에 설정된 몽타주를 재생한다.
	// MontageSections[0]을 Start 섹션으로 두면 된다.
	// 몽타주 내부에서 Start -> Loop로 이어지도록 섹션 연결을 잡는다.
	PlayConfiguredMontage(0);
	
	// 대시 지속시간이 끝나면 어빌리티를 종료한다.
	UAbilityTask_WaitDelay* WaitDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, SkillDuration);
	if (!WaitDelayTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	WaitDelayTask->OnFinish.AddDynamic(this, &UERNGA_Normal_GhostDash::FinishSkill);
	WaitDelayTask->ReadyForActivation();
}

void UERNGA_Normal_GhostDash::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo());
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	// EndAbility 이후 ActivationOwnedTags가 제거되므로, 제거된 상태 기준으로 이동속도를 다시 계산한다.
	if (Character)
	{
		// 충돌 복구
		ClearGhostDashCollision(Character);
		// 나이아가라 이펙트 종료
		StopGhostDashNiagaraEffects(Character);
		// 투명도 복구
		ClearGhostDashVisual();
		// 속도/방향 복구
		Character->UpdateMovementSpeed();
		Character->UpdateRotationMode();
	}
}

void UERNGA_Normal_GhostDash::FinishSkill()
{
	K2_EndAbility();
}

void UERNGA_Normal_GhostDash::ApplyGhostDashVisualToMesh(UMeshComponent* MeshComp)
{
	if (!MeshComp)
	{
		return;
	}

	const int32 MaterialCount = MeshComp->GetNumMaterials();

	for (int32 Index = 0; Index < MaterialCount; ++Index)
	{
		UMaterialInstanceDynamic* DynamicMaterial =
			MeshComp->CreateDynamicMaterialInstance(Index);

		if (!DynamicMaterial)
		{
			continue;
		}

		DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), GhostOpacity);
		GhostDashMaterials.Add(DynamicMaterial);
	}
}

void UERNGA_Normal_GhostDash::ApplyGhostDashVisual(AProjectERNCharacter* Character)
{
	if (!Character)
	{
		return;
	}

	GhostDashMaterials.Reset();

	ApplyGhostDashVisualToMesh(Character->GetMesh());

	if (const UERNEquipmentComponent* EquipmentComponent = Character->GetEquipmentComponent())
	{
		if (AERNWeaponBase* CurrentWeapon = EquipmentComponent->CurrentWeapon)
		{
			ApplyGhostDashVisualToMesh(CurrentWeapon->WeaponMesh);
			ApplyGhostDashVisualToMesh(CurrentWeapon->SkeletalWeaponMesh);
		}
	}
}

void UERNGA_Normal_GhostDash::ClearGhostDashVisual()
{
	for (UMaterialInstanceDynamic* DynamicMaterial : GhostDashMaterials)
	{
		if (DynamicMaterial)
		{
			DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
		}
	}

	GhostDashMaterials.Reset();
}

void UERNGA_Normal_GhostDash::ApplyGhostDashCollision(AProjectERNCharacter* Character)
{
	if (!Character || !Character->GetCapsuleComponent())
	{
		return;
	}

	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();

	// 종료 시 원복하기 위해 기존 응답을 저장한다.
	PreviousCapsuleResponse = Capsule->GetCollisionResponseToChannel(GhostPassThroughChannel);

	// 스킬 사용 중에는 지정 채널과 충돌하지 않게 한다.
	// (투사체 채널은 무시하지 않음 — 프리즈 투사체가 명중해야 하고, 일반 투사체 데미지는 무적 태그가 0 처리)
	Capsule->SetCollisionResponseToChannel(GhostPassThroughChannel, ECR_Ignore);
}

void UERNGA_Normal_GhostDash::ClearGhostDashCollision(AProjectERNCharacter* Character)
{
	if (!Character || !Character->GetCapsuleComponent())
	{
		return;
	}

	// Collision 원상 복구
	Character->GetCapsuleComponent()->SetCollisionResponseToChannel(GhostPassThroughChannel, PreviousCapsuleResponse);
}

void UERNGA_Normal_GhostDash::StartGhostDashNiagaraEffects(AProjectERNCharacter* Character)
{
	if (!Character || GhostDashNiagaraEffects.IsEmpty())
	{
		return;
	}

	if (UERNSkillNiagaraComponent* NiagaraComponent = Character->FindComponentByClass<UERNSkillNiagaraComponent>())
	{
		NiagaraComponent->StartEffects(GhostDashNiagaraEffects);
	}
}

void UERNGA_Normal_GhostDash::StopGhostDashNiagaraEffects(AProjectERNCharacter* Character)
{
	if (!Character || GhostDashNiagaraEffects.IsEmpty())
	{
		return;
	}

	if (UERNSkillNiagaraComponent* NiagaraComponent = Character->FindComponentByClass<UERNSkillNiagaraComponent>())
	{
		NiagaraComponent->StopEffects(GhostDashNiagaraEffects);
	}
}
