#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"

#include "ERNDayNightCycleConfigDataAsset.generated.h"


UCLASS()
class PROJECTERN_API UERNDayNightCycleConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Time")
	float DayToNightDuration = 900.0f;
	
	// 레벨에 배치된 액터에서 Actor Tag로 검색
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Tags")
	FName SunLightTag = TEXT("DayNight.Sun");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Tags")
	FName SkyLightTag = TEXT("DayNight.SkyLight");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Tags")
	FName HeightFogTag = TEXT("DayNight.Fog");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor Tags")
	FName PostProcessTag = TEXT("DayNight.PostProcess");

	// Alpha 0 = 낮 시작, Alpha 1 = 밤 완료.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
	FRotator DaySunRotation = FRotator(-35.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
	FRotator NightSunRotation = FRotator(-170.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
	TObjectPtr<UCurveFloat> SunIntensityCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sun")
	TObjectPtr<UCurveLinearColor> SunColorCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkyLight")
	TObjectPtr<UCurveFloat> SkyLightIntensityCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fog")
	TObjectPtr<UCurveFloat> FogDensityCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fog")
	TObjectPtr<UCurveLinearColor> FogColorCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PostProcess")
	TObjectPtr<UCurveFloat> ExposureBiasCurve;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkyLight")
	bool bRecaptureSkyLight = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkyLight", meta = (EditCondition = "bRecaptureSkyLight"))
	float SkyLightRecaptureInterval = 5.0f;
};
