// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Weapons/ERNMeleeWeapon.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/DamageEvents.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Player/ERNPlayerController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/ERNAttributeSet.h"
#include "GAS/ERNGameplayTags.h"

AERNMeleeWeapon::AERNMeleeWeapon()
{
	HitboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxComponent"));
	HitboxComponent->SetupAttachment(RootComponent);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitboxComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	HitboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitboxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	HitboxComponent->OnComponentBeginOverlap.AddDynamic(this, &AERNMeleeWeapon::OnHitboxOverlap);
}

void AERNMeleeWeapon::EnableHitbox()
{
	HitActors.Empty();
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AERNMeleeWeapon::DisableHitbox()
{
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitActors.Empty();
}

void AERNMeleeWeapon::OnHitboxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	if (!OtherActor || OtherActor == GetOwner() || HitActors.Contains(OtherActor))
	{
		return;
	}

	// 적 캐릭터에게만 데미지 적용
	if (!OtherActor->IsA<AERNEnemyCharacter>())
	{
		return;
	}

	HitActors.Add(OtherActor);

	// 데미지 적용
	AController* InstigatorController = GetOwner()
		? Cast<ACharacter>(GetOwner())->GetController()
		: nullptr;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerCharacter);

	/*
	// 태그로 강공 / 약공 구분
	const bool bIsHeavy = ASC && ASC->HasMatchingGameplayTag(TAG_Ability_Attack_Heavy);
	const float DamageToApply = bIsHeavy ? HeavyAttackDamage : LightAttackDamage;
	const float StaggerPower = bIsHeavy ? HeavyAttackStaggerPower : LightAttackStaggerPower;
	*/
	
	float CharacterAttackPower = 0.f;
	float AttackPowerBonus = 0.f;

	if (ASC)
	{
		CharacterAttackPower =
			ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerAttribute());

		AttackPowerBonus =
			ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerBonusAttribute());
	}
	
	const float DamageToApply = LightAttackDamage + CharacterAttackPower + AttackPowerBonus;
	const float StaggerPower = LightAttackStaggerPower;
	
	OtherActor->TakeDamage(DamageToApply, FDamageEvent(), InstigatorController, GetOwner());

	if (AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(OtherActor))
	{
		Enemy->TryApplyStagger(StaggerPower);
	}

	// 히트 이펙트 - 모든 클라이언트에 멀티캐스트
	if (HitEffect)
	{
		Multicast_PlayHitEffect(SweepResult.ImpactPoint, SweepResult.ImpactNormal.Rotation());
	}

	// 공격자에게 Hit Confirmation 카메라 흔들림 (Client RPC → 공격자 본인만 흔들림)
	if (HitConfirmShakeClass)
	{
		ACharacter* OwnerCharForShake = Cast<ACharacter>(GetOwner());
		if (OwnerCharForShake)
		{
			AERNPlayerController* PC = Cast<AERNPlayerController>(OwnerCharForShake->GetController());
			if (PC)
			{
				PC->Client_PlayCameraShake(HitConfirmShakeClass, HitConfirmShakeScale);
			}
		}
	}
}

void AERNMeleeWeapon::Multicast_PlayHitEffect_Implementation(FVector Location, FRotator Rotation)
{
	if (HitEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitEffect, Location, Rotation);
	}
}
