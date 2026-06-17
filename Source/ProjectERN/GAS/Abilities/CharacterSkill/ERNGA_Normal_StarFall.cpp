// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_Normal_StarFall.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "Combat/Weapons/ERNRangedWeapon.h"
#include "GameFramework/Character.h"
#include "Inventory/Components/ERNEquipmentComponent.h"

UERNGA_Normal_StarFall::UERNGA_Normal_StarFall()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UERNGA_Normal_StarFall::FireProjectileFromNotify(USkeletalMeshComponent* MeshComp, const FVector& SpawnLocationOffset)
{
	// 투사체 적용 여부
	if (!ProjectileData.bUseProjectile || !ProjectileData.ProjectileClass || !MeshComp)
	{
		return;
	}

	// 서버에서만 실행
	ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner());
	if (!Character || !Character->HasAuthority())
	{
		return;
	}

	// 투사체 Transform 적용
	FTransform SpawnTransform(
		Character->GetActorRotation(),
		Character->GetActorLocation()
		+ Character->GetActorForwardVector() * ProjectileData.CharacterForwardDistance,
		FVector::OneVector);
	
	// 발사 지점 선택(무기 관련 선택은 적용 안됨)
	switch (ProjectileData.SpawnSource)
	{
	case EERNProjectileSpawnSource::CharacterSocket:
		if (ProjectileData.CharacterSocketName != NAME_None &&
			MeshComp->DoesSocketExist(ProjectileData.CharacterSocketName))
		{
			SpawnTransform = MeshComp->GetSocketTransform(ProjectileData.CharacterSocketName);
		}
		break;

	case EERNProjectileSpawnSource::CharacterForward:
	case EERNProjectileSpawnSource::WeaponMuzzle:
	case EERNProjectileSpawnSource::WeaponHitbox:
	default:
		break;
	}

	// BP에서 적용한 Location 적용
	SpawnTransform.SetLocation(SpawnTransform.TransformPosition(ProjectileData.SpawnOffset));

	// 노티파이가 지정한 추가 오프셋 (스폰 기준점 로컬 공간)
	SpawnTransform.SetLocation(SpawnTransform.TransformPosition(SpawnLocationOffset));

	// BP에서 적용한 Rotator 적용
	const FRotator SpawnRotation = ProjectileData.bUseSourceRotation
		? SpawnTransform.GetRotation().Rotator()
		: Character->GetActorForwardVector().Rotation();

	// 투사체 소유권 및 정보 적용
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 스폰
	Character->GetWorld()->SpawnActor<AERNProjectileBase>(
		ProjectileData.ProjectileClass,
		SpawnTransform.GetLocation(),
		SpawnRotation,
		SpawnParams);
}

void UERNGA_Normal_StarFall::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayAbilityActivationInfo ActivationInfo,
                                             const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = PlayConfiguredMontage(0);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UERNGA_Normal_StarFall::FinishSkill);
	MontageTask->OnInterrupted.AddDynamic(this, &UERNGA_Normal_StarFall::FinishSkill);
	MontageTask->OnCancelled.AddDynamic(this, &UERNGA_Normal_StarFall::FinishSkill);
	MontageTask->OnBlendOut.AddDynamic(this, &UERNGA_Normal_StarFall::FinishSkill);
}

void UERNGA_Normal_StarFall::FinishSkill()
{
	if (IsActive())
	{
		K2_EndAbility();
	}
}
