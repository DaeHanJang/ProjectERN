// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Animation/AnimNotifyState_BossVanish.h"
#include "Character/ERNCharacterBase.h"
#include "Subsystem/ERNSoundSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	UERNSoundSubsystem* GetSoundSubsystem(USkeletalMeshComponent* MeshComp)
	{
		UWorld* World = MeshComp ? MeshComp->GetWorld() : nullptr;
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UERNSoundSubsystem>() : nullptr;
	}
}

void UAnimNotifyState_BossVanish::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	// BGM 즉시 정지 (각 클라 로컬)
	if (UERNSoundSubsystem* SoundSubsystem = GetSoundSubsystem(MeshComp))
	{
		SoundSubsystem->PauseBGM(true, 0.f);
	}

	// 무형 상태 진입 (서버에서만 설정 → 리플리케이트)
	AERNCharacterBase* Boss = MeshComp ? Cast<AERNCharacterBase>(MeshComp->GetOwner()) : nullptr;
	if (Boss && Boss->HasAuthority())
	{
		Boss->SetIntangible(true);
	}
}

void UAnimNotifyState_BossVanish::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	// BGM 페이드 인 재개 (각 클라 로컬)
	if (UERNSoundSubsystem* SoundSubsystem = GetSoundSubsystem(MeshComp))
	{
		SoundSubsystem->PauseBGM(false, BGMFadeInTime);
	}

	// 무형 상태 해제 (몽타주 중단 시에도 호출 → 복구 보장)
	AERNCharacterBase* Boss = MeshComp ? Cast<AERNCharacterBase>(MeshComp->GetOwner()) : nullptr;
	if (Boss && Boss->HasAuthority())
	{
		Boss->SetIntangible(false);
	}
}
