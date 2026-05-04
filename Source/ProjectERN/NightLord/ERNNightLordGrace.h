// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "ERNNightLordGrace.generated.h"

class USphereComponent;
class UWidgetComponent;

UCLASS()
class PROJECTERN_API AERNNightLordGrace : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	AERNNightLordGrace();
	
	// 루트 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grace | Components")
	UStaticMeshComponent* GraceMesh;
	
	// 나중에 UI와 같은 상호작용 시 를 위한 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grace | Components")
	USphereComponent* InteractionComponent;
	
	// E : 휴식 하기 와 같은 월드 스페이스 위젯
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grace | Components")
	UWidgetComponent* InteractionPromptWidget;
	
	// 레벨업 위젯 참조
	UPROPERTY()
	UUserWidget* LevelUpPopupWidget;
	
	// IInteractable 인터페이스 함수 선언
	virtual void Interact_Implementation(APlayerController* PlayerController) override;
	virtual void EndInteract_Implementation(APlayerController* PlayerController) override;

	virtual bool CanInteract_Implementation() const override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;

protected:
	virtual void BeginPlay() override;
	
	// 델리게이트에 바인딩할 함수
	UFUNCTION()
	void HandlePopupClosed();
	
	// 회복을 담당하는 함수
	void RestoreAttributes(AProjectERNCharacter* TargetCharacter);
	
	// 플레이어와 화토불의 충돌을 담당하는 BeginOverlap 충돌판정 
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	// EndOverlap은 범위를 벗어났을때 UI숨기기 및 화토불 효과를 끝내기 위해 사용예정
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

};
