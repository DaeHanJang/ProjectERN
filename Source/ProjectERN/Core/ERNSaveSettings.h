// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ERNSaveSettings.generated.h"

// 계정 영구 스탯 투자 대상
UENUM(BlueprintType)
enum class EAccountStat : uint8
{
	Health		UMETA(DisplayName = "Health"),
	Mana		UMETA(DisplayName = "Mana"),
	Stamina		UMETA(DisplayName = "Stamina"),
	Defense		UMETA(DisplayName = "Defense"),
	Attack		UMETA(DisplayName = "Attack"),
	Lifesteal	UMETA(DisplayName = "Lifesteal"),
	Gold		UMETA(DisplayName = "Gold")
};

UCLASS()
class PROJECTERN_API UERNSaveSettings : public USaveGame
{
	GENERATED_BODY()

public:
	// Audio (0.0 ~ 1.0)
	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float MasterVolume = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float MusicVolume = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float SFXVolume = 1.0f;

	// Brightness (UGameUserSettings에 없어서 별도 저장, 0 ~ 100)
	UPROPERTY(BlueprintReadWrite, Category = "Video")
	float Brightness = 50.0f;

	// DLSS 사용 여부 (체크박스)
	UPROPERTY(BlueprintReadWrite, Category = "Video")
	bool bDLSSEnabled = false;

	// DLSS 품질 모드 (UDLSSMode 값, 4 = Quality) — 플러그인 의존 피하려고 int32로 저장
	UPROPERTY(BlueprintReadWrite, Category = "Video")
	int32 DLSSMode = 4;

	// 마우스 감도 슬라이더 값 (0.0 ~ 1.0, 0.5 = 기본 1.0배). 실제 배율은 GameInstance에서 매핑
	UPROPERTY(BlueprintReadWrite, Category = "Input")
	float MouseSensitivity = 0.5f;

	// ===== 계정 메타 진행도 (최종보스 처치 시 +1, 포인트로 영구 스탯 투자) =====
	// 기본 1부터 시작 (클리어 0회 = 레벨 1). 보유 포인트는 0 (포인트는 클리어로 획득)
	UPROPERTY(BlueprintReadWrite, Category = "Account")
	int32 AccountLevel = 1;

	// 사용 가능한 미투자 포인트
	UPROPERTY(BlueprintReadWrite, Category = "Account")
	int32 AvailablePoints = 0;

	// 스탯별 투자한 포인트 수 (영구 버프 = 이 값 × 포인트당 증가량)
	UPROPERTY(BlueprintReadWrite, Category = "Account")
	int32 InvHealth = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Account")
	int32 InvMana = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Account")
	int32 InvStamina = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Account")
	int32 InvDefense = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Account")
	int32 InvAttack = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Account")
	int32 InvLifesteal = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Account")
	int32 InvGold = 0;

	// 하드모드 ON/OFF (호스트가 계정 위젯 체크박스로 토글, 계정 세이브에 함께 저장)
	UPROPERTY(BlueprintReadWrite, Category = "Account")
	bool bHardMode = false;
};
