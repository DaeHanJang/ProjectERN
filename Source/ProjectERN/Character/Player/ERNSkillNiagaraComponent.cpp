// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Player/ERNSkillNiagaraComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GAS/Abilities/CharacterSkill/ERNSkillNiagaraTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraComponentPoolMethodEnum.h"
#include "NiagaraFunctionLibrary.h"

// Sets default values for this component's properties
UERNSkillNiagaraComponent::UERNSkillNiagaraComponent()
{
	SetIsReplicatedByDefault(true);
}

void UERNSkillNiagaraComponent::StartEffects(const TArray<FERNSkillAttachedNiagaraEffect>& Effects)
{
	if (Effects.IsEmpty())
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// 서버에서 호출된 경우 바로 Multicast
	if (OwnerActor->HasAuthority())
	{
		MulticastStartEffects(Effects);
		return;
	}

	// 로컬 클라이언트에서 호출된 경우 즉시 로컬 출력 후 서버에 요청
	if (CanSendServerRpc())
	{
		for (const FERNSkillAttachedNiagaraEffect& EffectData : Effects)
		{
			SpawnEffect_Local(EffectData);
		}

		ServerStartEffects(Effects);
	}
}

void UERNSkillNiagaraComponent::StopEffects(const TArray<FERNSkillAttachedNiagaraEffect>& Effects)
{
	if (Effects.IsEmpty())
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (OwnerActor->HasAuthority())
	{
		MulticastStopEffects(Effects);
		return;
	}

	if (CanSendServerRpc())
	{
		for (const FERNSkillAttachedNiagaraEffect& EffectData : Effects)
		{
			StopEffect_Local(EffectData);
		}

		ServerStopEffects(Effects);
	}
}

void UERNSkillNiagaraComponent::StopEffectByKey(FName EffectKey, bool bDestroyOnStop)
{
	if (EffectKey == NAME_None)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (OwnerActor->HasAuthority())
	{
		MulticastStopEffectByKey(EffectKey, bDestroyOnStop);
		return;
	}

	if (CanSendServerRpc())
	{
		StopEffectByKey_Local(EffectKey, bDestroyOnStop);
		ServerStopEffectByKey(EffectKey, bDestroyOnStop);
	}
}

void UERNSkillNiagaraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UERNSkillNiagaraComponent::ServerStartEffects_Implementation(const TArray<FERNSkillAttachedNiagaraEffect>& Effects)
{
	MulticastStartEffects(Effects);
}

void UERNSkillNiagaraComponent::ServerStopEffects_Implementation(const TArray<FERNSkillAttachedNiagaraEffect>& Effects)
{
	MulticastStopEffects(Effects);
}

void UERNSkillNiagaraComponent::ServerStopEffectByKey_Implementation(FName EffectKey, bool bDestroyOnStop)
{
	MulticastStopEffectByKey(EffectKey, bDestroyOnStop);
}

void UERNSkillNiagaraComponent::MulticastStartEffects_Implementation(
	const TArray<FERNSkillAttachedNiagaraEffect>& Effects)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	for (const FERNSkillAttachedNiagaraEffect& EffectData : Effects)
	{
		SpawnEffect_Local(EffectData);
	}
}

void UERNSkillNiagaraComponent::MulticastStopEffects_Implementation(
	const TArray<FERNSkillAttachedNiagaraEffect>& Effects)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	for (const FERNSkillAttachedNiagaraEffect& EffectData : Effects)
	{
		StopEffect_Local(EffectData);
	}
}

void UERNSkillNiagaraComponent::MulticastStopEffectByKey_Implementation(FName EffectKey, bool bDestroyOnStop)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	StopEffectByKey_Local(EffectKey, bDestroyOnStop);
}

void UERNSkillNiagaraComponent::SpawnEffect_Local(const FERNSkillAttachedNiagaraEffect& EffectData)
{
	if (!EffectData.NiagaraSystem)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = GetOwnerMesh();
	if (!MeshComp)
	{
		return;
	}

	const FName EffectKey = ResolveEffectKey(EffectData);
	if (EffectKey == NAME_None)
	{
		return;
	}

	// 같은 키의 이펙트가 이미 살아있으면 중복 생성하지 않음
	if (TObjectPtr<UNiagaraComponent>* ExistingComponentPtr = ActiveEffects.Find(EffectKey))
	{
		if (IsValid(ExistingComponentPtr->Get()))
		{
			return;
		}

		ActiveEffects.Remove(EffectKey);
	}

	// Bone/Socket 이름이 잘못된 경우 생성하지 않음
	if (EffectData.AttachSocketName != NAME_None &&
		!MeshComp->DoesSocketExist(EffectData.AttachSocketName) &&
		MeshComp->GetBoneIndex(EffectData.AttachSocketName) == INDEX_NONE)
	{
		return;
	}

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		EffectData.NiagaraSystem,
		MeshComp,
		EffectData.AttachSocketName,
		EffectData.LocationOffset,
		EffectData.RotationOffset,
		EAttachLocation::KeepRelativeOffset,
		false,
		true,
		ENCPoolMethod::None,
		true);

	if (!NiagaraComponent)
	{
		return;
	}

	NiagaraComponent->SetRelativeScale3D(EffectData.Scale);
	ActiveEffects.Add(EffectKey, NiagaraComponent);
}

void UERNSkillNiagaraComponent::StopEffect_Local(const FERNSkillAttachedNiagaraEffect& EffectData)
{
	StopEffectByKey_Local(ResolveEffectKey(EffectData), EffectData.bDestroyOnStop);
}

void UERNSkillNiagaraComponent::StopEffectByKey_Local(FName EffectKey, bool bDestroyOnStop)
{
	if (EffectKey == NAME_None)
	{
		return;
	}

	TObjectPtr<UNiagaraComponent>* ComponentPtr = ActiveEffects.Find(EffectKey);
	if (!ComponentPtr)
	{
		return;
	}

	UNiagaraComponent* NiagaraComponent = ComponentPtr->Get();
	ActiveEffects.Remove(EffectKey);

	if (!IsValid(NiagaraComponent))
	{
		return;
	}

	if (bDestroyOnStop)
	{
		NiagaraComponent->DestroyComponent();
		return;
	}

	// Trail 잔상이 자연스럽게 사라지도록 비활성화만 함
	NiagaraComponent->SetAutoDestroy(true);
	NiagaraComponent->Deactivate();
}

void UERNSkillNiagaraComponent::StopAllEffects_Local()
{
	for (TPair<FName, TObjectPtr<UNiagaraComponent>>& Pair : ActiveEffects)
	{
		if (UNiagaraComponent* NiagaraComponent = Pair.Value.Get())
		{
			NiagaraComponent->DestroyComponent();
		}
	}

	ActiveEffects.Empty();
}

FName UERNSkillNiagaraComponent::ResolveEffectKey(const FERNSkillAttachedNiagaraEffect& EffectData) const
{
	if (EffectData.EffectKey != NAME_None)
	{
		return EffectData.EffectKey;
	}

	if (EffectData.NiagaraSystem)
	{
		return EffectData.NiagaraSystem->GetFName();
	}

	return NAME_None;
}

USkeletalMeshComponent* UERNSkillNiagaraComponent::GetOwnerMesh() const
{
	const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	return CharacterOwner ? CharacterOwner->GetMesh() : nullptr;
}

bool UERNSkillNiagaraComponent::CanSendServerRpc() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}
