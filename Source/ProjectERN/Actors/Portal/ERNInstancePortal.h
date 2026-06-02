// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "ERNInstancePortal.generated.h"

class ANightRainZoneManager;
class AERNPortalDestinationPoint;
class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;
class UWidgetComponent;
class UNiagaraComponent;

/**
 * 같은 레벨 내 이동 포탈
 * 한 명이라도 상호작용하면 모든 플레이어가 DestinationPoints로 함께 이동
 * 이동 시 포탈마다 설정한 채팅 메시지를 전원에게 출력
 */
UCLASS()
class PROJECTERN_API AERNInstancePortal : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AERNInstancePortal();

	// 동적 스폰 시 도착 지점 주입 (적 사망 스폰 경로에서 호출)
	void SetDestinationPoint(AERNPortalDestinationPoint* InDestinationPoint);

	// IInteractable
	virtual void Interact_Implementation(APlayerController* PlayerController) override;
	virtual bool CanInteract_Implementation() const override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 루트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// 포탈 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PortalMesh;

	// 상호작용 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;

	// 상호작용 프롬프트 위젯 (World, 기본 숨김)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> InteractionPromptWidget;

	// 상시 이펙트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> PortalVFX;

	// 도착 지점 액터들 (레벨에 배치 후 드래그, 인원수만큼 권장 — 3명 분산)
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Portal")
	TArray<TObjectPtr<AActor>> DestinationPoints;

	// 도착 지점이 모자랄 때 첫 지점 주위로 흩는 반경
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal", meta = (ClampMin = "0.0"))
	float DestinationSpreadRadius = 150.f;
	
	// 일회용포탈 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	bool bIsreusable = false;

	// 상호작용 안내 문구
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FText PromptText = FText::FromString(TEXT("E키를 눌러 이동"));

	// 이동 시 채팅 알림 (포탈마다 설정 — 입장/탈출 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Chat")
	FString ChatSender = TEXT("System");

	// 던전 입장 시 채팅 메시지 (bIsInInstance == false 인 상태에서 탑승)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Chat")
	FString EnterChatMessage;

	// 던전 복귀 시 채팅 메시지 (bIsInInstance == true 인 상태에서 탑승)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Chat")
	FString ReturnChatMessage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal|Chat")
	FLinearColor ChatColor = FLinearColor::White;

private:
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// PlayerIndex번째 플레이어의 도착 트랜스폼 계산
	FTransform ResolveDestination(int32 PlayerIndex) const;

	// 전원에게 채팅 시스템 메시지 전송 (서버) — 입장/복귀에 따라 다른 문구
	void BroadcastPortalChat(bool bEntering) const;

	// ZoneManager 참조 확보 (에디터 배치 참조가 없으면 런타임에 월드에서 탐색하여 캐싱)
	ANightRainZoneManager* ResolveNightRainZoneManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NightRainZoneManager", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ANightRainZoneManager> NightRainZoneManager;
};
