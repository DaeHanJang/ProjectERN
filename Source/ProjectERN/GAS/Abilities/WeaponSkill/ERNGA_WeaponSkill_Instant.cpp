// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/WeaponSkill/ERNGA_WeaponSkill_Instant.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "Combat/Weapons/ERNMeleeWeapon.h"
#include "Combat/Weapons/ERNRangedWeapon.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GAS/ERNAttributeSet.h"

UERNGA_WeaponSkill_Instant::UERNGA_WeaponSkill_Instant()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UERNGA_WeaponSkill_Instant::BeginAreaDamage(USkeletalMeshComponent* MeshComp)
{
	if (!AreaDamageData.bUseAreaDamage || !MeshComp)
	{
		return;
	}

	// 새 AreaDamage 구간이 시작되므로, 이전에 맞은 적 목록을 초기화한다.
	// 이 구간 안에서는 HitActorsByMesh에 들어간 적에게 다시 데미지를 주지 않는다.
	HitActorsByMesh.FindOrAdd(MeshComp).Empty();

	if (AreaDamageData.AreaEffect)
	{
		AActor* OwnerActor = MeshComp->GetOwner();
		if (!OwnerActor)
		{
			return;
		}

		const FVector EffectLocation =
			OwnerActor->GetActorLocation()
			+ OwnerActor->GetActorForwardVector() * AreaDamageData.AreaEffectOffset.X
			+ OwnerActor->GetActorRightVector() * AreaDamageData.AreaEffectOffset.Y
			+ OwnerActor->GetActorUpVector() * AreaDamageData.AreaEffectOffset.Z;

		UNiagaraComponent* EffectComponent =
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				OwnerActor->GetWorld(),
				AreaDamageData.AreaEffect,
				EffectLocation,
				OwnerActor->GetActorRotation(),
				FVector::OneVector,
				false);

		if (EffectComponent)
		{
			// NotifyEnd에서 끄기 위해 생성한 NiagaraComponent를 MeshComp 기준으로 저장한다.
			AreaEffectsByMesh.Add(MeshComp, EffectComponent);
		}
	}
}

void UERNGA_WeaponSkill_Instant::TickAreaDamage(USkeletalMeshComponent* MeshComp)
{
	if (!AreaDamageData.bUseAreaDamage || !MeshComp)
	{
		return;
	}

	const FVector Origin = GetAreaDamageOrigin(MeshComp);
	ApplyAreaDamage(MeshComp, Origin);
}

void UERNGA_WeaponSkill_Instant::EndAreaDamage(USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp)
	{
		return;
	}

	// BeginAreaDamage에서 생성해 저장한 NiagaraComponent를 찾아 종료한다.
	if (TWeakObjectPtr<UNiagaraComponent>* EffectPtr = AreaEffectsByMesh.Find(MeshComp))
	{
		if (EffectPtr->IsValid())
		{
			EffectPtr->Get()->Deactivate();
		}
		
		// 종료된 이펙트 참조는 더 이상 필요 없으므로 제거한다.
		AreaEffectsByMesh.Remove(MeshComp);
	}
	
	// 이번 AreaDamage 구간의 피격 기록도 제거한다.
	// 다음 스킬 사용 때는 같은 적도 다시 맞을 수 있어야 한다.
	HitActorsByMesh.Remove(MeshComp);
}

void UERNGA_WeaponSkill_Instant::FireProjectileFromNotify(USkeletalMeshComponent* MeshComp)
{
	if (!ProjectileData.bUseProjectile || !ProjectileData.ProjectileClass || !MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	FTransform SpawnTransform;
	if (!GetProjectileSpawnTransform(MeshComp, SpawnTransform))
	{
		return;
	}

	const FRotator SpawnRotation = ProjectileData.bUseSourceRotation
		? SpawnTransform.GetRotation().Rotator()
		: OwnerActor->GetActorForwardVector().Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	OwnerActor->GetWorld()->SpawnActor<AERNProjectileBase>(
		ProjectileData.ProjectileClass,
		SpawnTransform.GetLocation(),
		SpawnRotation,
		SpawnParams);
}

FVector UERNGA_WeaponSkill_Instant::GetAreaDamageOrigin(USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return FVector::ZeroVector;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return MeshComp->GetComponentLocation();
	}

	if (AreaDamageData.OriginMode == EWeaponSkillAreaOriginMode::WeaponHitbox)
	{
		if (const UERNEquipmentComponent* Equipment =
			OwnerActor->FindComponentByClass<UERNEquipmentComponent>())
		{
			if (const AERNMeleeWeapon* MeleeWeapon =
				Cast<AERNMeleeWeapon>(Equipment->CurrentWeapon))
			{
				if (const UBoxComponent* Hitbox = MeleeWeapon->GetHitboxComponent())
				{
					return Hitbox->GetComponentTransform().TransformPosition(
						AreaDamageData.OriginOffset);
				}
			}
		}
	}

	if (AreaDamageData.OriginMode == EWeaponSkillAreaOriginMode::MeshSocket)
	{
		if (AreaDamageData.MeshSocketName != NAME_None &&
			MeshComp->DoesSocketExist(AreaDamageData.MeshSocketName))
		{
			return MeshComp
				->GetSocketTransform(AreaDamageData.MeshSocketName)
				.TransformPosition(AreaDamageData.OriginOffset);
		}
	}

	return OwnerActor->GetActorLocation()
		+ OwnerActor->GetActorForwardVector() * AreaDamageData.OriginOffset.X
		+ OwnerActor->GetActorRightVector() * AreaDamageData.OriginOffset.Y
		+ OwnerActor->GetActorUpVector() * AreaDamageData.OriginOffset.Z;
}

bool UERNGA_WeaponSkill_Instant::GetProjectileSpawnTransform(USkeletalMeshComponent* MeshComp, FTransform& OutTransform) const
{
	if (!MeshComp)
	{
		return false;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	OutTransform = FTransform(
		OwnerActor->GetActorRotation(),
		OwnerActor->GetActorLocation()
			+ OwnerActor->GetActorForwardVector() * ProjectileData.CharacterForwardDistance,
		FVector::OneVector);

	switch (ProjectileData.SpawnSource)
	{
	case EWeaponSkillProjectileSpawnSource::CharacterSocket:
		if (ProjectileData.CharacterSocketName != NAME_None &&
			MeshComp->DoesSocketExist(ProjectileData.CharacterSocketName))
		{
			OutTransform = MeshComp->GetSocketTransform(ProjectileData.CharacterSocketName);
		}
		break;

	case EWeaponSkillProjectileSpawnSource::WeaponMuzzle:
		if (const UERNEquipmentComponent* Equipment =
			OwnerActor->FindComponentByClass<UERNEquipmentComponent>())
		{
			if (const AERNRangedWeapon* RangedWeapon =
				Cast<AERNRangedWeapon>(Equipment->CurrentWeapon))
			{
				if (RangedWeapon->MuzzlePoint)
				{
					OutTransform = RangedWeapon->MuzzlePoint->GetComponentTransform();
				}
			}
		}
		break;

	case EWeaponSkillProjectileSpawnSource::WeaponHitbox:
		if (const UERNEquipmentComponent* Equipment =
			OwnerActor->FindComponentByClass<UERNEquipmentComponent>())
		{
			if (const AERNMeleeWeapon* MeleeWeapon =
				Cast<AERNMeleeWeapon>(Equipment->CurrentWeapon))
			{
				if (const UBoxComponent* Hitbox = MeleeWeapon->GetHitboxComponent())
				{
					OutTransform = Hitbox->GetComponentTransform();
				}
			}
		}
		break;

	case EWeaponSkillProjectileSpawnSource::CharacterForward:
	default:
		break;
	}

	OutTransform.SetLocation(
		OutTransform.TransformPosition(ProjectileData.SpawnOffset));

	return true;
}

void UERNGA_WeaponSkill_Instant::ApplyAreaDamage(USkeletalMeshComponent* MeshComp, const FVector& Origin)
{
	AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	UWorld* World = OwnerActor->GetWorld();
	if (!World)
	{
		return;
	}

	if (AreaDamageData.bDrawDebug)
	{
		DrawDebugSphere(
			World,
			Origin,
			AreaDamageData.DamageRadius,
			16,
			FColor::Red,
			false,
			AreaDamageData.DebugDrawTime,
			0,
			2.f);
	}

	TArray<FOverlapResult> OverlapResults;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	const bool bHit = World->OverlapMultiByObjectType(
		OverlapResults,
		Origin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(AreaDamageData.DamageRadius),
		QueryParams);

	if (!bHit)
	{
		return;
	}

	// 이 MeshComp의 현재 AreaDamage 구간에서 이미 맞은 적 목록을 가져온다. (없으면 새로 만듦)
	TSet<TWeakObjectPtr<AActor>>& HitActors = HitActorsByMesh.FindOrAdd(MeshComp);

	AController* InstigatorController = nullptr;
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor))
	{
		InstigatorController = OwnerCharacter->GetController();
	}

	const float DamageToApply = CalculateAreaDamage(OwnerActor);

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* HitActor = Result.GetActor();

		// 이미 맞은 적이면 이번 AreaDamage 구간에서는 다시 데미지를 주지 않는다.
		if (!HitActor || HitActor == OwnerActor || HitActors.Contains(HitActor))
		{
			continue;
		}

		AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(HitActor);
		if (!Enemy)
		{
			continue;
		}

		// 데미지를 적용하기 전에 기록해 중복 피격을 방지한다.
		HitActors.Add(HitActor);

		Enemy->TakeDamage(DamageToApply, FDamageEvent(), InstigatorController, OwnerActor);

		if (AreaDamageData.StaggerPower > 0.f)
		{
			Enemy->TryApplyStagger(AreaDamageData.StaggerPower);
		}
	}
}

float UERNGA_WeaponSkill_Instant::CalculateAreaDamage(AActor* OwnerActor) const
{
	const float WeaponDamage = GetWeaponBaseDamage(OwnerActor);
	const float CharacterAttackPower = GetCharacterAttackPower(OwnerActor);

	return (WeaponDamage + CharacterAttackPower) * AreaDamageData.DamageMultiplier;
}

float UERNGA_WeaponSkill_Instant::GetWeaponBaseDamage(AActor* OwnerActor) const
{
	if (!OwnerActor)
	{
		return 0.f;
	}

	if (const UERNEquipmentComponent* Equipment =
		OwnerActor->FindComponentByClass<UERNEquipmentComponent>())
	{
		if (const AERNWeaponBase* Weapon = Equipment->CurrentWeapon)
		{
			return Weapon->LightAttackDamage;
		}
	}

	return 0.f;
}

float UERNGA_WeaponSkill_Instant::GetCharacterAttackPower(AActor* OwnerActor) const
{
	if (!OwnerActor)
	{
		return 0.f;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);

	if (!ASC)
	{
		return 0.f;
	}

	const float AttackPower = ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerAttribute());

	const float AttackPowerBonus = ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerBonusAttribute());

	return AttackPower + AttackPowerBonus;
}
