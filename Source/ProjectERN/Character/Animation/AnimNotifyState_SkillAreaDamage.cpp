// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotifyState_SkillAreaDamage.h"

#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Combat/Weapons/ERNMeleeWeapon.h"
#include "Components/BoxComponent.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "DrawDebugHelpers.h"

void UAnimNotifyState_SkillAreaDamage::NotifyBegin(USkeletalMeshComponent* MeshComp,
                                                   UAnimSequenceBase* Animation,
                                                   float TotalDuration,
                                                   const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (!MeshComp)
	{
		return;
	}

	HitActorsByMesh.FindOrAdd(MeshComp).Empty();
	
	if (NiagaraEffect)
	{
		AActor* OwnerActor = MeshComp->GetOwner();
		if (!OwnerActor)
		{
			return;
		}

		const FVector NiagaraLocation =
			OwnerActor->GetActorLocation()
			+ OwnerActor->GetActorForwardVector() * NiagaraOffset.X
			+ OwnerActor->GetActorRightVector() * NiagaraOffset.Y
			+ OwnerActor->GetActorUpVector() * NiagaraOffset.Z;

		const FRotator NiagaraRotation = OwnerActor->GetActorRotation();

		UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			OwnerActor->GetWorld(),
			NiagaraEffect,
			NiagaraLocation,
			NiagaraRotation,
			FVector(1.f),
			false);

		if (NiagaraComponent)
		{
			NiagaraComponentsByMesh.Add(MeshComp, NiagaraComponent);
		}
	}
}

void UAnimNotifyState_SkillAreaDamage::NotifyTick(USkeletalMeshComponent* MeshComp,
                                                  UAnimSequenceBase* Animation,
                                                  float FrameDeltaTime,
                                                  const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	
	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	// 대미지 부여
	const FVector Origin = GetDamageOrigin(MeshComp);
	ApplyDamageAtOrigin(MeshComp, Origin);
}

void UAnimNotifyState_SkillAreaDamage::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                 const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	
	if (!MeshComp)
	{
		return;
	}

	if (TWeakObjectPtr<UNiagaraComponent>* NiagaraComponentPtr = NiagaraComponentsByMesh.Find(MeshComp))
	{
		if (NiagaraComponentPtr->IsValid())
		{
			NiagaraComponentPtr->Get()->Deactivate();
		}

		NiagaraComponentsByMesh.Remove(MeshComp);
	}

	HitActorsByMesh.Remove(MeshComp);
}

FVector UAnimNotifyState_SkillAreaDamage::GetDamageOrigin(const USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp)
	{
		return FVector::ZeroVector;
	}

	/*
	if (AttachSocketName != NAME_None && MeshComp->DoesSocketExist(AttachSocketName))
	{
		return MeshComp->GetSocketTransform(AttachSocketName).TransformPosition(Offset);
	}

	const AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return MeshComp->GetComponentLocation();
	} 
	*/
	
	const AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return MeshComp->GetComponentLocation();
	}

	if (OriginMode == ESkillAreaDamageOriginMode::WeaponHitbox)
	{
		if (const UERNEquipmentComponent* Equipment =
			OwnerActor->FindComponentByClass<UERNEquipmentComponent>())
		{
			if (const AERNMeleeWeapon* MeleeWeapon =
				Cast<AERNMeleeWeapon>(Equipment->CurrentWeapon))
			{
				if (const UBoxComponent* Hitbox = MeleeWeapon->GetHitboxComponent())
				{
					// 월드 기준 오프셋 적용
					// return Hitbox->GetComponentLocation() + Offset;
					
					// Hitbox 기준 오프셋 적용
					return Hitbox->GetComponentTransform().TransformPosition(Offset);
				}
			}
		}
	}

	if (OriginMode == ESkillAreaDamageOriginMode::MeshSocket)
	{
		if (AttachSocketName != NAME_None && MeshComp->DoesSocketExist(AttachSocketName))
		{
			return MeshComp->GetSocketTransform(AttachSocketName).TransformPosition(Offset);
		}
	}

	
	return OwnerActor->GetActorLocation()
		+ OwnerActor->GetActorForwardVector() * Offset.X
		+ OwnerActor->GetActorRightVector() * Offset.Y
		+ OwnerActor->GetActorUpVector() * Offset.Z;
}

void UAnimNotifyState_SkillAreaDamage::ApplyDamageAtOrigin(USkeletalMeshComponent* MeshComp, const FVector& Origin)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	UWorld* World = OwnerActor->GetWorld();
	if (!World)
	{
		return;
	}

	// 디버그 드로잉
	if (bDrawDebug)
	{
		DrawDebugSphere(
			World,
			Origin,
			Radius,
			16,
			FColor::Red,
			false,
			DebugDrawTime,
			0,
			2.f);
	}
	// 디버그 드로잉 end
	
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
		FCollisionShape::MakeSphere(Radius),
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

		AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(HitActor);
		if (!Enemy)
		{
			continue;
		}

		HitActors.Add(HitActor);

		const float DamageToApply = Damage * DamageMultiplier;
		
		Enemy->TakeDamage(DamageToApply, FDamageEvent(), InstigatorController, OwnerActor);
		
		if (StaggerPower > 0.f)
		{
			Enemy->TryApplyStagger(StaggerPower);
		}
	}
}
