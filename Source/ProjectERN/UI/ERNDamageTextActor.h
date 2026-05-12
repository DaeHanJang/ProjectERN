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
	// 데미지 값과 색상으로 초기화
	UFUNCTION(BlueprintCallable, Category = "DamageText")
	void Initialize(float InDamage, FLinearColor InColor = FLinearColor::White);

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

	// 경과 시간
	float ElapsedTime = 0.f;

	// 시작 투명도
	float StartOpacity = 1.f;
};
