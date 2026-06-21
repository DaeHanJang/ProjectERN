// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotifyState_BladeExtend.h"

#include "Combat/Weapons/ERNMeleeWeapon.h"
#include "GameFramework/Character.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "NiagaraSystem.h"

void UAnimNotifyState_BladeExtend::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AERNMeleeWeapon* Weapon = GetEquippedMeleeWeapon(MeshComp))
	{
		// GrowDuration < 0 이면 노티 전체 구간에 걸쳐 성장
		const float Duration = (GrowDuration < 0.f) ? TotalDuration : GrowDuration;
		Weapon->BeginBladeExtend(ExtendMultiplier, Duration, BladeExtendTraceRadius);

		// 트레일 교체는 노티(토글)가 아니라 GA에서 멀티캐스트로 처리 → 여기서 안 함

		// 연장 구간 동안 데미지 스펙 교체 (지정된 경우)
		if (bOverrideDamage)
		{
			Weapon->BeginBladeDamageOverride(ExtendDamageData);
		}
	}
}

void UAnimNotifyState_BladeExtend::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (AERNMeleeWeapon* Weapon = GetEquippedMeleeWeapon(MeshComp))
	{
		Weapon->TickBladeExtend(FrameDeltaTime);
	}
}

void UAnimNotifyState_BladeExtend::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AERNMeleeWeapon* Weapon = GetEquippedMeleeWeapon(MeshComp))
	{
		Weapon->EndBladeExtend();

		// 트레일 복귀도 GA에서 처리 → 여기서 안 함

		// 기본 데미지 스펙으로 복귀 (교체했던 경우만)
		if (bOverrideDamage)
		{
			Weapon->EndBladeDamageOverride();
		}
	}
}

AERNMeleeWeapon* UAnimNotifyState_BladeExtend::GetEquippedMeleeWeapon(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return nullptr;
	}

	ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner());

	// 트레일(코스메틱)은 모든 클라에서 늘려야 하므로 권한 게이트를 두지 않는다.
	// 실제 히트 판정은 TraceEnd 위치를 읽는 BeginAttackTrace/TickAttackTrace 내부의
	// HasAuthority() 체크로 서버 전용 유지된다.
	if (!Character)
	{
		return nullptr;
	}

	UERNEquipmentComponent* Equipment = Character->FindComponentByClass<UERNEquipmentComponent>();
	if (!Equipment)
	{
		return nullptr;
	}

	return Cast<AERNMeleeWeapon>(Equipment->CurrentWeapon);
}
