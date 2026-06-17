// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_Ult_Mage.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

UERNGA_Ult_Mage::UERNGA_Ult_Mage()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UERNGA_Ult_Mage::FireProjectileFromNotify(USkeletalMeshComponent* MeshComp, const FVector& SpawnLocationOffset)
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

	// 기본 Transform: 캐릭터 전방
	FTransform SpawnTransform(
		Character->GetActorRotation(),
		Character->GetActorLocation()
		+ Character->GetActorForwardVector() * ProjectileData.CharacterForwardDistance,
		FVector::OneVector);

	// 발사 지점 선택
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

	// BP에서 설정한 Location 오프셋 적용
	SpawnTransform.SetLocation(SpawnTransform.TransformPosition(ProjectileData.SpawnOffset));

	// 노티파이가 지정한 추가 오프셋 (스폰 기준점 로컬 공간)
	SpawnTransform.SetLocation(SpawnTransform.TransformPosition(SpawnLocationOffset));

	// BP에서 설정한 Rotation 적용
	const FRotator SpawnRotation = ProjectileData.bUseSourceRotation
		? SpawnTransform.GetRotation().Rotator()
		: Character->GetActorForwardVector().Rotation();

	// 소유권 — Owner/Instigator를 캐릭터로 (팀/데미지 판정 기준)
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Character->GetWorld()->SpawnActor<AERNProjectileBase>(
		ProjectileData.ProjectileClass,
		SpawnTransform.GetLocation(),
		SpawnRotation,
		SpawnParams);
}

void UERNGA_Ult_Mage::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	ExecuteCastGameplayCue();

	UAbilityTask_PlayMontageAndWait* MontageTask = PlayConfiguredMontage(0);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UERNGA_Ult_Mage::FinishSkill);
	MontageTask->OnBlendOut.AddDynamic(this, &UERNGA_Ult_Mage::FinishSkill);
	MontageTask->OnInterrupted.AddDynamic(this, &UERNGA_Ult_Mage::FinishSkill);
	MontageTask->OnCancelled.AddDynamic(this, &UERNGA_Ult_Mage::FinishSkill);
}

void UERNGA_Ult_Mage::FinishSkill()
{
	if (IsActive())
	{
		K2_EndAbility();
	}
}
