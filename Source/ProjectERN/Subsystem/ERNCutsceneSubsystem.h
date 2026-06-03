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
class AERNCharacterBase;
class AERNIntroBird;

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

	// 컷신 재생 (플레이어 자동 바인딩)
	UFUNCTION(BlueprintCallable, Category = "Cutscene")
	void PlayCutscene(ULevelSequence* Sequence, bool bDisablePlayerInput = true);

	// 컷신 재생 (플레이어 바인딩 없음 - 환경 연출 등)
	UFUNCTION(BlueprintCallable, Category = "Cutscene")
	void PlayCutsceneWithoutPlayers(ULevelSequence* Sequence, bool bDisablePlayerInput = true);

	// 컷신 중단
	UFUNCTION(BlueprintCallable, Category = "Cutscene")
	void StopCutscene();

	// 컷신 재생 중 여부
	UFUNCTION(BlueprintPure, Category = "Cutscene")
	bool IsPlayingCutscene() const { return bIsPlayingCutscene; }

	// ===== 인트로 시퀀스 (새 매달림 → 점프 해제) =====
	// 데이터(IntroBirdClass / FadeInDuration / AutoStart)는 BP_ERNGameInstance에서 설정

	// 인트로 시퀀스 시작 (서버에서만 동작) — 그룹 1개 랜덤 선택 → 새 N마리 스폰 → 플레이어 부착 → 페이드 인
	UFUNCTION(BlueprintCallable, Category = "Intro")
	void StartBirdIntroSequence();

protected:
	// 로딩 화면 콜백 (맵 로딩 완료 시)
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

	// 컷신 종료 콜백
	UFUNCTION()
	void OnCutsceneComplete();

	// 플레이어 입력 차단/복구
	void DisablePlayerInput();
	void EnablePlayerInput();

	// 플레이어 캐릭터들을 시퀀서에 바인딩
	void BindPlayersToSequence(ULevelSequencePlayer* Player, ULevelSequence* Sequence);

	// 컷신 내부 재생 로직 (공통)
	void PlayCutsceneInternal(ULevelSequence* Sequence, bool bDisablePlayerInput, bool bBindPlayers);

private:
	// ===== 에셋 웜업 (PSO/메시 히칭 완화) =====

	// 로딩 화면 동안 카메라 앞에 모든 아이템을 잠깐 spawn하여 메시/PSO를 미리 컴파일
	// PostLoadMap 직후엔 카메라가 아직 폰으로 블렌드되기 전(원점)이라 컬링되므로,
	// 카메라/폰이 준비될 때까지 짧은 간격으로 재시도한 뒤 실제 시야 앞에 스폰
	void WarmUpFieldItems();

	// 웜업용으로 spawn한 임시 아이템 액터 일괄 제거 (몇 프레임 렌더 후 호출)
	void CleanupWarmUpActors();

	// 웜업용으로 spawn한 임시 아이템 액터들
	UPROPERTY()
	TArray<TObjectPtr<AActor>> WarmUpActors;

	// 카메라/폰 준비를 기다리는 재시도 횟수 (0.1초 간격, 최대 WarmUpMaxRetries회)
	int32 WarmUpRetryCount = 0;
	static constexpr int32 WarmUpMaxRetries = 50;

	// ===== 로딩 화면 =====

	// 로딩 위젯 생성 + 뷰포트 부착 (이미 부착되어 있으면 no-op)
	// — 클라이언트 ServerTravel 도중 슬레이트가 무효화되는 케이스 대응용 재부착에도 사용
	void EnsureLoadingWidgetInViewport();

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
