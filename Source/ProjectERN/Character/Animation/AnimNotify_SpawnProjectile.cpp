// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Animation/AnimNotify_SpawnProjectile.h"
#include "Combat/Weapons/ERNRangedWeapon.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "GameFramework/Character.h"

void UAnimNotify_SpawnProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	// 캐릭터의 MeshComponent 확인
	ACharacter* Character = MeshComp ? Cast<ACharacter>(MeshComp->GetOwner()) : nullptr;
	// 서버에서만 실행
	if (!Character || !Character->HasAuthority()) return;

	// 장비 존재 여부 확인
	UERNEquipmentComponent* Equipment = Character->FindComponentByClass<UERNEquipmentComponent>();
	if (!Equipment) return;
	
	// 이제 강공격확인 필요하지 않음. 단순 투사체 소환으로 로직 수정
	AERNRangedWeapon* RangedWeapon = Cast<AERNRangedWeapon>(Equipment->CurrentWeapon);
	if (!RangedWeapon) return;
	
	// 투사체 소환
	RangedWeapon->SpawnProjectile(Character, MeshComp);
}
