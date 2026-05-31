// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/ERNEntranceTriggerBox.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Character/Player/ERNPlayerController.h"

AERNEntranceTriggerBox::AERNEntranceTriggerBox()
{
	PrimaryActorTick.bCanEverTick = false;

	// 로컬 UI 트리거 — 복제 불필요
	bReplicates = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(200.f, 200.f, 100.f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
}

void AERNEntranceTriggerBox::BeginPlay()
{
	Super::BeginPlay();

	TriggerBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &AERNEntranceTriggerBox::OnTriggerBeginOverlap);
}

void AERNEntranceTriggerBox::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.RemoveDynamic(this, &AERNEntranceTriggerBox::OnTriggerBeginOverlap);
	}

	Super::EndPlay(EndPlayReason);
}

void AERNEntranceTriggerBox::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	// 자기 머신의 로컬 플레이어만 — 각자 자기 화면에서만 표시
	if (!Pawn->IsLocallyControlled())
	{
		return;
	}

	AERNPlayerController* PC = Cast<AERNPlayerController>(Pawn->GetController());
	if (!PC)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();

	// 이 트리거 박스 한정, 이 플레이어의 쿨다운 확인
	if (const double* LastShown = LastShownTimes.Find(PC))
	{
		if (Now - *LastShown < ReactivateCooldown)
		{
			return;
		}
	}

	LastShownTimes.Add(PC, Now);

	PC->ShowEntranceWidget(EntranceText);
}
