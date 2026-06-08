// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ERNSoundSubsystem.generated.h"

class USoundBase;
class UAudioComponent;

/**
 * 게임 사운드 관리 서브시스템 (GameInstance 라이프사이클)
 * - BGM 재생/정지 + 페이드 인/아웃
 * - 맵 로드 시 WorldSettings(AERNWorldSettings)의 MapBGM 자동 재생
 */
UCLASS()
class PROJECTERN_API UERNSoundSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 새 BGM 재생 (기존 BGM 자동 정지)
	UFUNCTION(BlueprintCallable, Category = "Sound|BGM")
	void PlayBGM(USoundBase* BGM, float FadeInTime = 0.f);

	// 현재 BGM 정지 (페이드 아웃)
	UFUNCTION(BlueprintCallable, Category = "Sound|BGM")
	void StopBGM(float FadeOutTime = 0.f);

	// 현재 BGM 일시정지/재개 (재생 위치 보존). 볼륨 페이드 후 SetPaused
	UFUNCTION(BlueprintCallable, Category = "Sound|BGM")
	void PauseBGM(bool bPause, float FadeTime = 0.5f);

	// 현재 BGM 재생 중인지
	UFUNCTION(BlueprintPure, Category = "Sound|BGM")
	bool IsBGMPlaying() const;

protected:
	// 맵 로드 완료 시 WorldSettings의 MapBGM 자동 재생
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

	// 로딩 화면 종료 콜백 — 로딩 종료 후 MapBGM 재생
	UFUNCTION()
	void OnLoadingFinishedForBGM();

private:
	// 현재 재생 중인 BGM AudioComponent
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentBGM;
};
