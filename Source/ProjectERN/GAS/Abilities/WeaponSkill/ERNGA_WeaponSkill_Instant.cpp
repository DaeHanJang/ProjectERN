// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/WeaponSkill/ERNGA_WeaponSkill_Instant.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "Combat/ERNSkillDamageLibrary.h"
#include "Combat/Weapons/ERNMeleeWeapon.h"
#include "Combat/Weapons/ERNRangedWeapon.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "Inventory/Components/ERNEquipmentComponent.h"

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
	AreaDamageElapsedTimes.Add(MeshComp, 0.f);

	PlayInstantNiagaraEffects(EERNWeaponSkillInstantEffectTrigger::AreaBegin, MeshComp);
}

void UERNGA_WeaponSkill_Instant::TickAreaDamage(USkeletalMeshComponent* MeshComp)
{
	if (!AreaDamageData.bUseAreaDamage || !MeshComp)
	{
		return;
	}

	// 누적 시간 계산
	float& ElapsedTime = AreaDamageElapsedTimes.FindOrAdd(MeshComp);
	ElapsedTime += GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;

	const bool bUseDamageWindow = AreaDamageData.DamageEndTime > AreaDamageData.DamageStartTime;
	if (bUseDamageWindow)
	{
		if (ElapsedTime < AreaDamageData.DamageStartTime ||
			ElapsedTime > AreaDamageData.DamageEndTime)
		{
			return;
		}
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

	// 나이아가라 이펙트 적용
	PlayInstantNiagaraEffects(EERNWeaponSkillInstantEffectTrigger::AreaEnd, MeshComp);

	// 이번 AreaDamage 구간의 피격 기록도 제거한다.
	// 다음 스킬 사용 때는 같은 적도 다시 맞을 수 있어야 한다.
	AreaDamageElapsedTimes.Remove(MeshComp);
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

	// 투사체 발사 시 적용할 나이아가라 이펙트
	PlayInstantNiagaraEffects(EERNWeaponSkillInstantEffectTrigger::ProjectileFire, MeshComp);

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

void UERNGA_WeaponSkill_Instant::ExplodeFromNotify(USkeletalMeshComponent* MeshComp)
{
	if (!ExplosionData.bUseExplosion || !MeshComp)
	{
		return;
	}

	// 서버에서만 처리
	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	FTransform ExplosionTransform;
	if (!GetExplosionTransform(MeshComp, ExplosionTransform))
	{
		return;
	}

	// 폭발 나이아가라 적용
	PlayInstantNiagaraEffects(EERNWeaponSkillInstantEffectTrigger::Explosion, MeshComp);
	// 폭발(범위)대미지 적용
	ApplyExplosionDamage(MeshComp, ExplosionTransform.GetLocation());
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

bool UERNGA_WeaponSkill_Instant::GetProjectileSpawnTransform(USkeletalMeshComponent* MeshComp,
                                                             FTransform& OutTransform) const
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

bool UERNGA_WeaponSkill_Instant::GetExplosionTransform(USkeletalMeshComponent* MeshComp, FTransform& OutTransform) const
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
		OwnerActor->GetActorLocation(),
		FVector::OneVector);

	if (ExplosionData.OriginMode == EWeaponSkillAreaOriginMode::WeaponHitbox)
	{
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
	}
	else if (ExplosionData.OriginMode == EWeaponSkillAreaOriginMode::MeshSocket)
	{
		if (ExplosionData.MeshSocketName != NAME_None &&
			MeshComp->DoesSocketExist(ExplosionData.MeshSocketName))
		{
			OutTransform = MeshComp->GetSocketTransform(ExplosionData.MeshSocketName);
		}
	}

	OutTransform.SetLocation(OutTransform.TransformPosition(ExplosionData.OriginOffset));
	return true;
}

void UERNGA_WeaponSkill_Instant::PlayInstantNiagaraEffects(
	EERNWeaponSkillInstantEffectTrigger Trigger,
	USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return;
	}

	// 서버에서만 처리
	AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(MeshComp->GetOwner());
	if (!Character || !Character->HasAuthority())
	{
		return;
	}

	// 이펙트 소환
	TArray<FERNWeaponSkillInstantNiagaraSpawnData> SpawnEffects;

	for (const FERNWeaponSkillInstantNiagaraEffect& EffectData : InstantNiagaraEffects)
	{
		if (!EffectData.bUseEffect || EffectData.Trigger != Trigger || !EffectData.NiagaraSystem)
		{
			continue;
		}

		// 스폰 위치 적용
		FTransform SpawnTransform;
		if (!GetInstantNiagaraEffectTransform(MeshComp, EffectData, SpawnTransform))
		{
			continue;
		}

		FERNWeaponSkillInstantNiagaraSpawnData SpawnData;
		SpawnData.NiagaraSystem = EffectData.NiagaraSystem;
		SpawnData.Location = SpawnTransform.GetLocation();
		SpawnData.Rotation = SpawnTransform.GetRotation().Rotator();
		SpawnData.Scale = EffectData.Scale;
		SpawnData.StartDelay = EffectData.StartDelay;

		SpawnEffects.Add(SpawnData);
	}

	// 스폰할 이펙트가 존재한다면
	if (!SpawnEffects.IsEmpty())
	{
		// 캐릭터로 멀티캐스트
		Character->Multicast_PlayWeaponSkillInstantNiagaraEffects(SpawnEffects);
	}
}

bool UERNGA_WeaponSkill_Instant::GetInstantNiagaraEffectTransform(USkeletalMeshComponent* MeshComp,
                                                                  const FERNWeaponSkillInstantNiagaraEffect& EffectData,
                                                                  FTransform& OutTransform) const
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

	// 무기의 WeaponHitbox에 이펙트 적용
	if (EffectData.OriginMode == EWeaponSkillAreaOriginMode::WeaponHitbox)
	{
		if (const UERNEquipmentComponent* Equipment =
			OwnerActor->FindComponentByClass<UERNEquipmentComponent>())
		{
			if (const AERNMeleeWeapon* MeleeWeapon =
				Cast<AERNMeleeWeapon>(Equipment->CurrentWeapon))
			{
				if (const UBoxComponent* Hitbox = MeleeWeapon->GetHitboxComponent())
				{
					OutTransform = Hitbox->GetComponentTransform();
					OutTransform.SetLocation(OutTransform.TransformPosition(EffectData.LocationOffset));
					OutTransform.ConcatenateRotation(EffectData.RotationOffset.Quaternion());
					return true;
				}
			}
		}
	}

	// 캐릭터의 MeshSocket에 이펙트 적용
	if (EffectData.OriginMode == EWeaponSkillAreaOriginMode::MeshSocket)
	{
		if (EffectData.MeshSocketName != NAME_None &&
			MeshComp->DoesSocketExist(EffectData.MeshSocketName))
		{
			OutTransform = MeshComp->GetSocketTransform(EffectData.MeshSocketName);
			OutTransform.SetLocation(OutTransform.TransformPosition(EffectData.LocationOffset));
			OutTransform.ConcatenateRotation(EffectData.RotationOffset.Quaternion());
			return true;
		}
	}

	OutTransform = FTransform(
		OwnerActor->GetActorRotation(),
		OwnerActor->GetActorLocation()
		+ OwnerActor->GetActorForwardVector() * EffectData.LocationOffset.X
		+ OwnerActor->GetActorRightVector() * EffectData.LocationOffset.Y
		+ OwnerActor->GetActorUpVector() * EffectData.LocationOffset.Z,
		FVector::OneVector);

	OutTransform.ConcatenateRotation(EffectData.RotationOffset.Quaternion());
	return true;
}

void UERNGA_WeaponSkill_Instant::ApplyExplosionDamage(USkeletalMeshComponent* MeshComp, const FVector& Origin)
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

	if (ExplosionData.bDrawDebug)
	{
		DrawDebugSphere(
			World,
			Origin,
			ExplosionData.DamageRadius,
			24,
			FColor::Orange,
			false,
			ExplosionData.DebugDrawTime,
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
		FCollisionShape::MakeSphere(ExplosionData.DamageRadius),
		QueryParams);

	if (!bHit)
	{
		return;
	}

	AController* InstigatorController = nullptr;
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor))
	{
		InstigatorController = OwnerCharacter->GetController();
	}

	TSet<TWeakObjectPtr<AActor>> DamagedActors;

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || HitActor == OwnerActor || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		const EERNSkillHitResult HitResult = UERNSkillDamageLibrary::ApplySkillHit(
			HitActor,
			OwnerActor,
			InstigatorController,
			ExplosionData.DamageData,
			Origin);

		if (HitResult != EERNSkillHitResult::None)
		{
			DamagedActors.Add(HitActor);
		}
	}
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

	TSet<TWeakObjectPtr<AActor>>& HitActors = HitActorsByMesh.FindOrAdd(MeshComp);

	AController* InstigatorController = nullptr;
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor))
	{
		InstigatorController = OwnerCharacter->GetController();
	}

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* HitActor = Result.GetActor();

		if (!HitActor || HitActor == OwnerActor || HitActors.Contains(HitActor))
		{
			continue;
		}

		const EERNSkillHitResult HitResult = UERNSkillDamageLibrary::ApplySkillHit(
			HitActor,
			OwnerActor,
			InstigatorController,
			AreaDamageData.DamageData,
			Origin);

		if (HitResult != EERNSkillHitResult::None)
		{
			HitActors.Add(HitActor);
		}
	}
}