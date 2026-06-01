// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotifyState_MeleeTrace.h"

#include "Combat/Weapons/ERNMeleeWeapon.h"
#include "GameFramework/Character.h"
#include "Inventory/Components/ERNEquipmentComponent.h"

void UAnimNotifyState_MeleeTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                              float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AERNMeleeWeapon* Weapon = GetEquippedMeleeWeapon(MeshComp))
	{
		Weapon->BeginAttackTrace();
	}
}

void UAnimNotifyState_MeleeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	
	if (AERNMeleeWeapon* Weapon =
		GetEquippedMeleeWeapon(MeshComp))
	{
		Weapon->TickAttackTrace();
	}
}

void UAnimNotifyState_MeleeTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	if (AERNMeleeWeapon* Weapon = GetEquippedMeleeWeapon(MeshComp))
	{
		Weapon->EndAttackTrace();
	}
}

AERNMeleeWeapon* UAnimNotifyState_MeleeTrace::GetEquippedMeleeWeapon(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return nullptr;
	}

	ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner());

	// 실제 판정은 서버에서만 처리한다.
	if (!Character || !Character->HasAuthority())
	{
		return nullptr;
	}

	// 장착한 장비 확인
	UERNEquipmentComponent* Equipment = Character->FindComponentByClass<UERNEquipmentComponent>();

	if (!Equipment)
	{
		return nullptr;
	}

	// 장착한 장비에 공격 판정 부여
	return Cast<AERNMeleeWeapon>(Equipment->CurrentWeapon);
}
