// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERNDamageTextActor.generated.h"

class UWidgetComponent;
class UERNDamageTextWidget;

/**
 * AERNDamageTextActor - 데미지 숫자 표시 액터 (로컬 전용)
 */
UCLASS()
class PROJECTERN_API AERNDamageTextActor : public AActor
{
	GENERATED_BODY()

public:
	AERNDamageTextActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// 데미지 값으로 초기화 (색상은 데미지 크기에 따라 자동 결정)
	UFUNCTION(BlueprintCallable, Category = "DamageText")
	void Initialize(float InDamage);

	// 기존 텍스트에 데미지 누적 + 수명/스케일 리셋 (같은 적 연타 시 겹침 대신 합산)
	UFUNCTION(BlueprintCallable, Category = "DamageText")
	void AddDamage(float InDamage);

	// 데미지 텍스트 위젯 클래스 (BP에서 설정)
	UPROPERTY(EditDefaultsOnly, Category = "DamageText")
	TSubclassOf<UERNDamageTextWidget> DamageTextWidgetClass;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DamageText")
	UWidgetComponent* WidgetComponent;

	// 위로 떠오르는 속도 (cm/s)
	UPROPERTY(EditDefaultsOnly, Category = "DamageText")
	float FloatSpeed = 100.f;

	// 지속 시간
	UPROPERTY(EditDefaultsOnly, Category = "DamageText")
	float Duration = 1.5f;

	// 시작 스케일 (처음에 크게 떴다가)
	UPROPERTY(EditDefaultsOnly, Category = "DamageText", meta = (ClampMin = "0.0"))
	float StartScale = 1.6f;

	// 끝 스케일 (점점 작아짐)
	UPROPERTY(EditDefaultsOnly, Category = "DamageText", meta = (ClampMin = "0.0"))
	float EndScale = 1.0f;

	// 기본 색상 (주황)
	UPROPERTY(EditDefaultsOnly, Category = "DamageText")
	FLinearColor NormalColor = FLinearColor(1.f, 0.45f, 0.f, 1.f);

	// 고데미지 색상 (빨강)
	UPROPERTY(EditDefaultsOnly, Category = "DamageText")
	FLinearColor HighDamageColor = FLinearColor(1.f, 0.f, 0.f, 1.f);

	// 이 데미지 이상이면 고데미지 색상 적용
	UPROPERTY(EditDefaultsOnly, Category = "DamageText", meta = (ClampMin = "0.0"))
	float HighDamageThreshold = 1000.f;

	// 경과 시간
	float ElapsedTime = 0.f;

	// 시작 투명도
	float StartOpacity = 1.f;

	// 누적 데미지 (합산 표시용)
	float AccumulatedDamage = 0.f;

private:
	// 누적 데미지로 색/스케일/텍스트 갱신
	void UpdateDisplay();
};
