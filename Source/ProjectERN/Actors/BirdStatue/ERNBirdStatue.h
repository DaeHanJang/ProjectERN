// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "ERNBirdStatue.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UWidgetComponent;
class USceneComponent;
class UNiagaraComponent;
class AERNIntroBird;
class AProjectERNCharacter;

/**
 * AERNBirdStatue — 새 조각상 (빠른 이동 거점)
 * 상호작용 시 서버에서 새 스폰 → Approach(플레이어 추격) → Ascend(솟구침) → Forward 비행.
 * 부착/매달림/Steering/자동 점프(Release)는 기존 AERNIntroBird 시스템 재사용.
 */
UCLASS()
class PROJECTERN_API AERNBirdStatue : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AERNBirdStatue();

	// Scene Root (BP에서 StatueMesh 자유 회전 가능하도록)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// 조각상 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StatueMesh;

	// 상호작용 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;

	// 상호작용 프롬프트 위젯
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> InteractionPromptWidget;

	// 조각상 상시 이펙트 (BP에서 NiagaraSystem 에셋 할당)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> AmbientVFX;

	// 새 등장 위치 (BP에서 조각상 뒤쪽 위에 배치)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> BirdSpawnPoint;

	// 스폰할 새 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statue")
	TSubclassOf<AERNIntroBird> BirdClass;

	// === 이 동상에서 스폰되는 새의 비행 파라미터 (동상별 개별 설정) ===

	// 솟구치는 높이 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statue|Flight")
	float AscentHeight = 8000.f;

	// 솟구치는 동안 앞으로 추가 이동 거리 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statue|Flight")
	float AscentForwardDistance = 1500.f;

	// 전진 비행 거리 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statue|Flight")
	float FlightDistance = 100000.f;

	// 전진 비행 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statue|Flight")
	float FlightDuration = 50.f;

	// IInteractable 구현
	virtual void Interact_Implementation(APlayerController* PlayerController) override;
	virtual bool CanInteract_Implementation() const override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;

	virtual void Tick(float DeltaTime) override;

	// 오르락 내리락 애니메이션 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statue|Animation")
	float BobSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statue|Animation")
	float BobHeight = 10.0f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	float RunningTime = 0.0f;
	TMap<USceneComponent*, float> ComponentBaseZMap;
};
