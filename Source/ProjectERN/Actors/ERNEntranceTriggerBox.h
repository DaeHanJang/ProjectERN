// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERNEntranceTriggerBox.generated.h"

class UBoxComponent;

/**
 * 플레이어가 닿으면 각자 자기 머신에서 진입 안내 위젯을 띄우는 트리거 박스
 * - EntranceText를 위젯에 전달 (장소별로 다른 문구)
 * - 플레이어별로 1회 표시 후 Cooldown 동안 재진입해도 안 뜸
 * - 로컬 UI (서버 RPC 없음, IsLocallyControlled로 자기 폰만 반응)
 */
UCLASS()
class PROJECTERN_API AERNEntranceTriggerBox : public AActor
{
	GENERATED_BODY()

public:
	AERNEntranceTriggerBox();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 트리거 볼륨
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	// 진입 시 위젯에 표시할 문구 (장소별 설정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entrance")
	FText EntranceText;

	// 같은 플레이어가 재진입해도 안 뜨는 쿨다운 (초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entrance", meta = (ClampMin = "0.0"))
	float ReactivateCooldown = 30.f;

private:
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 플레이어별 마지막 표시 시각 (이 트리거 박스 한정)
	TMap<TWeakObjectPtr<APlayerController>, double> LastShownTimes;
};
