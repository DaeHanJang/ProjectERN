// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GameFramework/Actor.h"
#include "ERNNightLordGrace.generated.h"

class USphereComponent;

UCLASS()
class PROJECTERN_API AERNNightLordGrace : public AActor
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
	


protected:
	virtual void BeginPlay() override;
	
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
