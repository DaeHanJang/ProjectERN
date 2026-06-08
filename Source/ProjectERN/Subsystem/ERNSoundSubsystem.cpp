// Fill out your copyright notice in the Description page of Project Settings.

#include "Subsystem/ERNSoundSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "Core/ERNWorldSettings.h"
#include "Subsystem/ERNCutsceneSubsystem.h"

void UERNSoundSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Warning, TEXT("[SoundSubsystem] Initialize"));

	// 맵 로드 완료 시 BGM 자동 재생 처리
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UERNSoundSubsystem::OnPostLoadMapWithWorld);

	// 첫 맵 fallback — PIE에서 맵 직접 실행 시 PostLoadMapWithWorld 콜백이
	// Initialize 전에 발사되어 놓치는 케이스 대응.
	// — Audio Device가 Subsystem Initialize 후에 초기화되므로 짧은 타이머로 지연 호출
	if (UWorld* World = GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SoundSubsystem] World already exists at Initialize — scheduling delayed trigger"));

		FTimerHandle Tmp;
		World->GetTimerManager().SetTimer(
			Tmp,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (UWorld* W = GetWorld())
				{
					OnPostLoadMapWithWorld(W);
				}
			}),
			0.5f,
			false
		);
	}
}

void UERNSoundSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	if (CurrentBGM)
	{
		CurrentBGM->Stop();
		CurrentBGM = nullptr;
	}

	Super::Deinitialize();
}

void UERNSoundSubsystem::PlayBGM(USoundBase* BGM, float FadeInTime)
{
	UE_LOG(LogTemp, Warning, TEXT("[SoundSubsystem] PlayBGM called: BGM=%s, FadeIn=%.2f"),
		BGM ? *BGM->GetName() : TEXT("NULL"), FadeInTime);

	if (!BGM)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SoundSubsystem] PlayBGM aborted: BGM is null"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SoundSubsystem] PlayBGM aborted: World is null"));
		return;
	}

	// 기존 BGM 정지 (AutoDestroy=false라 컴포넌트는 살아있음 → nullptr 처리로 GC)
	if (CurrentBGM)
	{
		// 일시정지된 소스는 FadeOut이 진행되지 않아 보이스를 계속 점유 → 새 BGM이 컬링됨.
		// 따라서 일시정지 상태면 즉시 Stop으로 소스를 해제한다.
		if (FadeInTime > 0.f && !bBGMPaused)
		{
			CurrentBGM->FadeOut(FadeInTime, 0.f);
		}
		else
		{
			CurrentBGM->Stop();
		}
		CurrentBGM = nullptr;
	}
	bBGMPaused = false;

	// 새 BGM AudioComponent 생성 (Play는 아직 안 함)
	CurrentBGM = UGameplayStatics::CreateSound2D(
		World,
		BGM,
		1.f, // 볼륨 배수
		1.f, // 피치
		0.f, // 시작 시간
		nullptr,
		false, // bPersistAcrossLevelTransition
		false  // bAutoDestroy=false — Stop()이 컴포넌트를 즉시 파괴하지 않도록
	);

	if (CurrentBGM)
	{
		if (FadeInTime > 0.f)
		{
			CurrentBGM->FadeIn(FadeInTime, 1.f);
		}
		else
		{
			CurrentBGM->Play();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[SoundSubsystem] PlayBGM done: CurrentBGM=%s"),
		CurrentBGM ? TEXT("VALID") : TEXT("NULL (CreateSound2D failed)"));
}

void UERNSoundSubsystem::StopBGM(float FadeOutTime)
{
	if (!CurrentBGM) return;

	if (FadeOutTime > 0.f)
	{
		CurrentBGM->FadeOut(FadeOutTime, 0.f);
	}
	else
	{
		CurrentBGM->Stop();
	}
	CurrentBGM = nullptr;
	bBGMPaused = false;
}

bool UERNSoundSubsystem::IsBGMPlaying() const
{
	return CurrentBGM && CurrentBGM->IsPlaying();
}

void UERNSoundSubsystem::PauseBGM(bool bPause, float FadeTime)
{
	if (!CurrentBGM) return;

	bBGMPaused = bPause;

	if (bPause)
	{
		// 즉시 일시정지 — 풀볼륨에서 멈춰 소스가 살아있음(위치 보존). 페이드아웃→0은 컬링되어 재개 불가
		CurrentBGM->SetPaused(true);
	}
	else
	{
		// 즉시 풀볼륨 재개. 페이드인을 위해 볼륨을 0으로 떨어뜨리면 그 순간 무음 컬링으로
		// 소스가 죽어 재개에 실패하므로, 안정적 재개를 위해 즉시 1로 복원
		CurrentBGM->SetPaused(false);
		CurrentBGM->SetVolumeMultiplier(1.f);
	}
}

void UERNSoundSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!LoadedWorld)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SoundSubsystem] OnPostLoadMapWithWorld: LoadedWorld is null"));
		return;
	}

	AWorldSettings* WSBase = LoadedWorld->GetWorldSettings();
	UE_LOG(LogTemp, Warning, TEXT("[SoundSubsystem] OnPostLoadMapWithWorld: Map=%s, WS class=%s"),
		*LoadedWorld->GetMapName(),
		WSBase ? *WSBase->GetClass()->GetName() : TEXT("NULL"));

	// WorldSettings에서 MapBGM 가져와 자동 재생
	AERNWorldSettings* WS = Cast<AERNWorldSettings>(WSBase);
	if (!WS)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[SoundSubsystem] WS cast FAILED — Project Settings의 World Settings Class를 ERNWorldSettings로 변경 필요"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SoundSubsystem] WS cast OK. MapBGM=%s, FadeIn=%.2f"),
		WS->MapBGM ? *WS->MapBGM->GetName() : TEXT("NULL"),
		WS->MapBGMFadeInTime);

	if (!WS->MapBGM)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SoundSubsystem] MapBGM not assigned in WorldSettings"));
		return;
	}

	// 로딩 중이면 끝날 때까지 대기, 아니면 즉시 재생
	UERNCutsceneSubsystem* CS = GetGameInstance() ? GetGameInstance()->GetSubsystem<UERNCutsceneSubsystem>() : nullptr;
	if (CS && CS->IsLoading())
	{
		// 중복 바인딩 방어 — 이전 바인딩 클리어 후 재바인딩
		CS->OnLoadingFinished.RemoveDynamic(this, &UERNSoundSubsystem::OnLoadingFinishedForBGM);
		CS->OnLoadingFinished.AddDynamic(this, &UERNSoundSubsystem::OnLoadingFinishedForBGM);
		UE_LOG(LogTemp, Warning, TEXT("[SoundSubsystem] Loading in progress — BGM 재생 대기"));
	}
	else
	{
		// 로딩 화면 없음 → 즉시 재생
		PlayBGM(WS->MapBGM, WS->MapBGMFadeInTime);
	}
}

void UERNSoundSubsystem::OnLoadingFinishedForBGM()
{
	UE_LOG(LogTemp, Warning, TEXT("[SoundSubsystem] OnLoadingFinishedForBGM — 로딩 종료, BGM 재생 시작"));

	// 콜백 해제
	if (UERNCutsceneSubsystem* CS = GetGameInstance() ? GetGameInstance()->GetSubsystem<UERNCutsceneSubsystem>() : nullptr)
	{
		CS->OnLoadingFinished.RemoveDynamic(this, &UERNSoundSubsystem::OnLoadingFinishedForBGM);
	}

	// 현재 World의 WorldSettings에서 BGM 다시 가져와 재생
	if (UWorld* World = GetWorld())
	{
		if (AERNWorldSettings* WS = Cast<AERNWorldSettings>(World->GetWorldSettings()))
		{
			if (WS->MapBGM)
			{
				PlayBGM(WS->MapBGM, WS->MapBGMFadeInTime);
			}
		}
	}
}
