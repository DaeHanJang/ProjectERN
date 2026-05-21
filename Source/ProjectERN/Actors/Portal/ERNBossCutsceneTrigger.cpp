// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/Portal/ERNBossCutsceneTrigger.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Core/ERNGameState.h"

AERNBossCutsceneTrigger::AERNBossCutsceneTrigger()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Root: BoxComponent (트리거 볼륨)
	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetBoxExtent(FVector(250.f, 250.f, 100.f));
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerVolume->SetGenerateOverlapEvents(true);
}

void AERNBossCutsceneTrigger::BeginPlay()
{
	Super::BeginPlay();

	// 오버랩 이벤트는 서버에서만 처리
	if (HasAuthority() && TriggerVolume)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AERNBossCutsceneTrigger::OnTriggerBeginOverlap);
	}
}

void AERNBossCutsceneTrigger::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bTriggered) return;

	// 플레이어 캐릭터만 트리거
	if (!Cast<AProjectERNCharacter>(OtherActor)) return;

	bTriggered = true;

	// GameState에 컷신 시작 요청
	if (AERNGameState* GS = GetWorld()->GetGameState<AERNGameState>())
	{
		GS->StartBossEncounterSequence();
		UE_LOG(LogTemp, Log, TEXT("[BossCutsceneTrigger] Triggered by %s"), *OtherActor->GetName());
	}
}
