// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ERNCutsceneSubsystem.generated.h"

class UUserWidget;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
class SWidget;

// 로딩 시작/종료 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadingStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadingFinished);

// 컷신 시작/종료 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCutsceneStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCutsceneFinished);

/**
 * 컷신 및 로딩 화면 관리 서브시스템
 * GameInstance 생명주기 동안 유지되어 레벨 전환에도 살아있음
 */
UCLASS()
class PROJECTERN_API UERNCutsceneSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem 인터페이스
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===== 로딩 화면 시스템 =====

	// 로딩 시작 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Loading")
	FOnLoadingStarted OnLoadingStarted;

	// 로딩 종료 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Loading")
	FOnLoadingFinished OnLoadingFinished;

	// 로딩 화면 표시
	UFUNCTION(BlueprintCallable, Category = "Loading")
	void ShowLoadingScreen();

	// 로딩 화면 숨김
	UFUNCTION(BlueprintCallable, Category = "Loading")
	void HideLoadingScreen();

	// 로딩 중 여부
	UFUNCTION(BlueprintPure, Category = "Loading")
	bool IsLoading() const { return bIsLoading; }

	// ===== 컷신 시스템 =====

	// 컷신 시작 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Cutscene")
	FOnCutsceneStarted OnCutsceneStarted;

	// 컷신 종료 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Cutscene")
	FOnCutsceneFinished OnCutsceneFinished;

	// 컷신 재생
	UFUNCTION(BlueprintCallable, Category = "Cutscene")
	void PlayCutscene(ULevelSequence* Sequence, bool bDisablePlayerInput = true);

	// 컷신 중단
	UFUNCTION(BlueprintCallable, Category = "Cutscene")
	void StopCutscene();

	// 컷신 재생 중 여부
	UFUNCTION(BlueprintPure, Category = "Cutscene")
	bool IsPlayingCutscene() const { return bIsPlayingCutscene; }

protected:
	// 로딩 화면 콜백
	void OnPreLoadMap(const FString& MapName);
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

	// 컷신 종료 콜백
	UFUNCTION()
	void OnCutsceneComplete();

	// 플레이어 입력 차단/복구
	void DisablePlayerInput();
	void EnablePlayerInput();

private:
	// ===== 로딩 화면 =====

	// 로딩 중 여부
	bool bIsLoading = false;

	// 현재 로딩 위젯
	UPROPERTY()
	UUserWidget* LoadingWidget = nullptr;

	// Slate 위젯 레퍼런스 (RemoveViewportWidgetContent용)
	TSharedPtr<SWidget> LoadingSlateWidget;

	// ===== 컷신 =====

	// 컷신 재생 중 여부
	bool bIsPlayingCutscene = false;

	// 현재 재생 중인 시퀀스 플레이어
	UPROPERTY()
	ULevelSequencePlayer* CurrentSequencePlayer = nullptr;

	// 현재 시퀀스 액터
	UPROPERTY()
	ALevelSequenceActor* CurrentSequenceActor = nullptr;

	// 컷신 중 입력 차단 여부
	bool bInputDisabledDuringCutscene = false;
};
