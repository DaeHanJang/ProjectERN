// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/WeaponSkill/ERNGA_WeaponSkill_Buff.h"

#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Combat/Weapons/ERNMeleeWeapon.h"
#include "Components/BoxComponent.h"
#include "Inventory/Components/ERNEquipmentComponent.h"

UERNGA_WeaponSkill_Buff::UERNGA_WeaponSkill_Buff()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UERNGA_WeaponSkill_Buff::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bBuffAppliedThisActivation = false;
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UERNGA_WeaponSkill_Buff::ApplyBuffFromNotify(USkeletalMeshComponent* MeshComp)
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!AvatarActor || !AvatarActor->HasAuthority())
	{
		return;
	}

	if (MeshComp && MeshComp->GetOwner() != AvatarActor)
	{
		return;
	}

	if (bApplyBuffOnlyOncePerActivation && bBuffAppliedThisActivation)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (BuffEffectClass)
	{
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle =
			ASC->MakeOutgoingSpec(BuffEffectClass, BuffEffectLevel, EffectContext);

		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	// SpawnCastEffect(MeshComp);
	ExecuteCastGameplayCue(MeshComp);

	bBuffAppliedThisActivation = true;
}

/*
void UERNGA_WeaponSkill_Buff::SpawnCastEffect(USkeletalMeshComponent* MeshComp) const
{
	if (!CastEffectData.bUseCastEffect || !CastEffectData.CastEffect || !MeshComp)
	{
		return;
	}

	FTransform SpawnTransform;
	if (!GetCastEffectTransform(MeshComp, SpawnTransform))
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		MeshComp->GetWorld(),
		CastEffectData.CastEffect,
		SpawnTransform.GetLocation(),
		SpawnTransform.GetRotation().Rotator(),
		CastEffectData.Scale);
}

bool UERNGA_WeaponSkill_Buff::GetCastEffectTransform(USkeletalMeshComponent* MeshComp, FTransform& OutTransform) const
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

	if (CastEffectData.OriginMode == EWeaponSkillAreaOriginMode::WeaponHitbox)
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
					OutTransform.SetLocation(
						OutTransform.TransformPosition(CastEffectData.LocationOffset));
					OutTransform.ConcatenateRotation(
						CastEffectData.RotationOffset.Quaternion());
					return true;
				}
			}
		}
	}

	if (CastEffectData.OriginMode == EWeaponSkillAreaOriginMode::MeshSocket)
	{
		if (CastEffectData.MeshSocketName != NAME_None &&
			MeshComp->DoesSocketExist(CastEffectData.MeshSocketName))
		{
			OutTransform = MeshComp->GetSocketTransform(CastEffectData.MeshSocketName);
			OutTransform.SetLocation(
				OutTransform.TransformPosition(CastEffectData.LocationOffset));
			OutTransform.ConcatenateRotation(
				CastEffectData.RotationOffset.Quaternion());
			return true;
		}
	}

	OutTransform = FTransform(
		OwnerActor->GetActorRotation(),
		OwnerActor->GetActorLocation()
		+ OwnerActor->GetActorForwardVector() * CastEffectData.LocationOffset.X
		+ OwnerActor->GetActorRightVector() * CastEffectData.LocationOffset.Y
		+ OwnerActor->GetActorUpVector() * CastEffectData.LocationOffset.Z,
		FVector::OneVector);

	OutTransform.ConcatenateRotation(CastEffectData.RotationOffset.Quaternion());
	return true;
}
*/

void UERNGA_WeaponSkill_Buff::ExecuteCastGameplayCue(USkeletalMeshComponent* MeshComp) const
{
	if (!CastCueData.bUseCastCue || !CastCueData.CastCueTag.IsValid() || !MeshComp)
	{
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!AvatarActor || !AvatarActor->HasAuthority())
	{
		return;
	}

	FTransform CueTransform;
	if (!GetCastCueTransform(MeshComp, CueTransform))
	{
		return;
	}

	FGameplayCueParameters CueParams;
	CueParams.Instigator = AvatarActor;
	CueParams.EffectCauser = AvatarActor;
	CueParams.SourceObject = this;
	CueParams.Location = CueTransform.GetLocation();
	CueParams.Normal = CueTransform.GetRotation().Rotator().Vector();
	CueParams.RawMagnitude = CastCueData.CueScale;

	ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(CastCueData.CastCueTag, CueParams);
}

bool UERNGA_WeaponSkill_Buff::GetCastCueTransform(USkeletalMeshComponent* MeshComp, FTransform& OutTransform) const
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

	if (CastCueData.OriginMode == EWeaponSkillAreaOriginMode::WeaponHitbox)
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
					OutTransform.SetLocation(
						OutTransform.TransformPosition(CastCueData.LocationOffset));
					OutTransform.ConcatenateRotation(
						CastCueData.RotationOffset.Quaternion());
					return true;
				}
			}
		}
	}

	if (CastCueData.OriginMode == EWeaponSkillAreaOriginMode::MeshSocket)
	{
		if (CastCueData.MeshSocketName != NAME_None &&
			MeshComp->DoesSocketExist(CastCueData.MeshSocketName))
		{
			OutTransform = MeshComp->GetSocketTransform(CastCueData.MeshSocketName);
			OutTransform.SetLocation(
				OutTransform.TransformPosition(CastCueData.LocationOffset));
			OutTransform.ConcatenateRotation(
				CastCueData.RotationOffset.Quaternion());
			return true;
		}
	}

	OutTransform = FTransform(
		OwnerActor->GetActorRotation(),
		OwnerActor->GetActorLocation()
		+ OwnerActor->GetActorForwardVector() * CastCueData.LocationOffset.X
		+ OwnerActor->GetActorRightVector() * CastCueData.LocationOffset.Y
		+ OwnerActor->GetActorUpVector() * CastCueData.LocationOffset.Z,
		FVector::OneVector);

	OutTransform.ConcatenateRotation(CastCueData.RotationOffset.Quaternion());
	return true;
}