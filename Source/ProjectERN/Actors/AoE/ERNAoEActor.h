// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERNAoEActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UPrimitiveComponent;
class USceneComponent;

UCLASS(Abstract)
class PROJECTERN_API AERNAoEActor : public AActor
{
	GENERATED_BODY()
	
public:
	AERNAoEActor();

	// SpawnActorDeferred 이후, FinishSpawningActor 전에 호출하는 초기화 함수
	UFUNCTION(BlueprintCallable, Category="ERN|AoE")
	void InitializeAoE(AActor* InSourceActor);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 파생 클래스에서 대상 조건을 더 구체화할 수 있도록 열어둔 함수
	virtual bool IsValidAoETarget(AActor* TargetActor, UPrimitiveComponent* OverlappedComponent) const;

	// 파생 클래스가 실제 효과 적용 로직을 구현하는 함수
	virtual void ApplyAoEToTarget(AActor* TargetActor);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ERN|AoE")
	TObjectPtr<USceneComponent> SceneRoot;

	// 나이아가라 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ERN|AoE|VFX")
	TObjectPtr<UNiagaraComponent> AoENiagaraComponent;

	// 장판으로 적용할 나이아가라
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|AoE|VFX")
	TObjectPtr<UNiagaraSystem> AoENiagaraSystem;
	
	// AoE를 생성한 Actor. 효과 적용 시 Instigator/Source로 사용한다.
	UPROPERTY(BlueprintReadOnly, Category="ERN|AoE")
	TObjectPtr<AActor> SourceActor;
	
	// 적용 Radius
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ERN|AoE", meta=(ClampMin="0.0"))
	float Radius = 600.f;

	// 지속 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ERN|AoE", meta=(ClampMin="0.0"))
	float Duration = 10.f;

	// 적용 간격
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ERN|AoE", meta=(ClampMin="0.05"))
	float TickInterval = 1.f;

	// 등장 하자마자 적용 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ERN|AoE")
	bool bApplyFirstTickImmediately = true;

	// 디버그 사용 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ERN|AoE|Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|AoE|Debug", meta=(EditCondition="bDrawDebug", EditConditionHides))
	FColor DebugColor = FColor::Green;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|AoE|Debug", meta=(EditCondition="bDrawDebug", EditConditionHides, ClampMin="4"))
	int32 DebugSegments = 32;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|AoE|Debug", meta=(EditCondition="bDrawDebug", EditConditionHides, ClampMin="0.0"))
	float DebugThickness = 2.f;

private:
	FTimerHandle AoETickTimerHandle;

	// 값 세팅이 잘 되어있는지 확인
	bool IsAoEConfigured() const;
	// 타이머 시작
	void StartAoETick();
	// 타이머 종료
	void StopAoETick();
	// 효과 적용
	void ApplyAoE();
};
