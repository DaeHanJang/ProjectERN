// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERNBossCutsceneTrigger.generated.h"

class UBoxComponent;
class AProjectERNCharacter;
class UPrimitiveComponent;

/**
 * 보스맵에서 보스 조우 컷신을 트리거하는 박스.
 * 첫 플레이어가 트리거에 진입하면 GameState->StartBossEncounterSequence() 호출.
 * 한 번만 발사 (bTriggered 플래그).
 */
UCLASS()
class PROJECTERN_API AERNBossCutsceneTrigger : public AActor
{
	GENERATED_BODY()

public:
	AERNBossCutsceneTrigger();

protected:
	virtual void BeginPlay() override;

	// 오버랩 감지 볼륨
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> TriggerVolume;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	// 한 번만 발사 (서버 권한)
	bool bTriggered = false;
};
