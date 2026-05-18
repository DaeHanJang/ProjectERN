// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Animation/AnimNotify_WorldCameraShake.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

void UAnimNotify_WorldCameraShake::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!ShakeClass || !MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	UWorld* World = Owner->GetWorld();
	if (!World) return;

	const FVector Origin = Owner->GetActorLocation() + OriginOffset;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->PlayerCameraManager) continue;

		const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
		const float Dist = FVector::Dist(CamLoc, Origin);

		float Attenuation = 0.f;
		if (Dist <= InnerRadius)
		{
			Attenuation = 1.f;
		}
		else if (Dist < OuterRadius)
		{
			const float Alpha = (Dist - InnerRadius) / (OuterRadius - InnerRadius);
			Attenuation = FMath::Pow(1.f - Alpha, Falloff);
		}

		if (Attenuation <= 0.f) continue;

		const float FinalScale = Scale * Attenuation;

		if (bOrientShakeTowardsEpicenter)
		{
			const FRotator EpicenterRot = (Origin - CamLoc).Rotation();
			PC->PlayerCameraManager->StartCameraShake(ShakeClass, FinalScale, ECameraShakePlaySpace::UserDefined, EpicenterRot);
		}
		else
		{
			PC->PlayerCameraManager->StartCameraShake(ShakeClass, FinalScale);
		}
	}
}
