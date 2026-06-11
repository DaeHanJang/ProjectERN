// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/ERNBossCharacter.h"
#include "Character/Enemy/AI/ERNBossAIController.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Character/Player/ERNPlayerController.h"
#include "GAS/ERNAttributeSet.h"
#include "GAS/ERNGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "OnlineSubsystemUtils.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/CapsuleComponent.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/ERNCutsceneSubsystem.h"
#include "LevelSequence.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BrainComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Subsystem/ERNSoundSubsystem.h"
#include "Core/ERNGameState.h"
#include "Actors/Portal/ERNBossPortal.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "Combat/Weapons/ERNWeaponBase.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

AERNBossCharacter::AERNBossCharacter()
{
	// 보스용 AI Controller 클래스 설정
	AIControllerClass = AERNBossAIController::StaticClass();

	// 보스 기본 설정
	InitialStaggerResistance = 50.f;  // 보스는 경직 저항 높음
	
	// 피지컬 에셋 히트 판정 활성화
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);    
}

void AERNBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 첫 페이즈 설정
	if (Phases.Num() > 0)
	{
		CurrentPhaseIndex = 0;
	}
}

void AERNBossCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AERNBossCharacter, CurrentPhaseIndex);
	DOREPLIFETIME(AERNBossCharacter, bIsTransitioningPhase);
}

float AERNBossCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 페이즈 전환 중에는 데미지 무시
	if (bIsTransitioningPhase)
	{
		return 0.0f;
	}
	
	// 3페이즈 이상이면 받는 데미지 감소
	if (CurrentPhaseIndex >= 2)
	{
		DamageAmount *= 0.65f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.0f && HasAuthority())
	{
		// 피격 시 보스 남은 체력 + 출력 공격력 배수 로그
		if (AttributeSet)
		{
			UE_LOG(LogTemp, Log, TEXT("[Boss %s] Hit for %.1f -> Health %.1f / %.1f (%.1f%%) | DmgMul x%.2f"),
				*GetName(), ActualDamage,
				AttributeSet->GetHealth(), AttributeSet->GetMaxHealth(),
				AttributeSet->GetMaxHealth() > 0.f ? AttributeSet->GetHealth() / AttributeSet->GetMaxHealth() * 100.f : 0.f,
				OutgoingDamageMultiplier);
		}

		// 데미지 어그로 추가
		if (AERNBossAIController* BossAIC = Cast<AERNBossAIController>(GetController()))
		{
			AActor* AttackerPawn = EventInstigator ? EventInstigator->GetPawn() : DamageCauser;
			if (AttackerPawn)
			{
				BossAIC->AddAggro(AttackerPawn, ActualDamage);

				// 시야 밖 원거리 피격도 비전투 해제되도록 명시 알림
				if (AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(AttackerPawn))
				{
					Player->NotifyDetectedBy(this);
				}
			}
		}

		// 페이즈 전환 체크
		CheckPhaseTransition();

		// 공격자 캐릭터 찾기
		AProjectERNCharacter* AttackerChar = Cast<AProjectERNCharacter>(DamageCauser);
		if (!AttackerChar && EventInstigator)
		{
			AttackerChar = Cast<AProjectERNCharacter>(EventInstigator->GetPawn());
		}

		// 체력 퍼센트 계산
		const float HealthPercent = AttributeSet && AttributeSet->GetMaxHealth() > 0.f
			? AttributeSet->GetHealth() / AttributeSet->GetMaxHealth()
			: 0.f;

		// 모든 플레이어에게 보스 체력바 업데이트
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			AERNPlayerController* PC = Cast<AERNPlayerController>(It->Get());
			if (!PC) continue;

			// 공격자에게는 본인 데미지 누적, 나머지는 0 (체력만 업데이트)
			const bool bIsAttacker = AttackerChar && PC == AttackerChar->GetController();
			const float DamageToAdd = bIsAttacker ? ActualDamage : 0.f;
			PC->Client_UpdateBossHealthBar(HealthPercent, DamageToAdd);
		}
	}

	return ActualDamage;
}

int32 AERNBossCharacter::GetPhaseIndexForCurrentHealth() const
{
	if (!AttributeSet || Phases.Num() == 0)
	{
		return 0;
	}

	const float CurrentHealthRatio = AttributeSet->GetHealth() / AttributeSet->GetMaxHealth();

	// 체력 비율에 맞는 페이즈 찾기 (내림차순으로 정렬되어 있다고 가정)
	for (int32 i = Phases.Num() - 1; i >= 0; --i)
	{
		if (CurrentHealthRatio <= Phases[i].HealthThreshold)
		{
			return i;
		}
	}

	return 0;
}

void AERNBossCharacter::CheckPhaseTransition()
{
	const int32 NewPhaseIndex = GetPhaseIndexForCurrentHealth();

	if (NewPhaseIndex > CurrentPhaseIndex)
	{
		TransitionToPhase(NewPhaseIndex);
	}
}

void AERNBossCharacter::TransitionToPhase(int32 NewPhaseIndex)
{
	if (NewPhaseIndex < 0 || NewPhaseIndex >= Phases.Num() || NewPhaseIndex == CurrentPhaseIndex)
	{
		return;
	}

	bIsTransitioningPhase = true;
	CurrentPhaseIndex = NewPhaseIndex;

	const FBossPhaseInfo& NewPhase = Phases[CurrentPhaseIndex];

	// 페이즈 전환 중 슈퍼아머 적용
	if (NewPhase.bSuperArmorDuringTransition)
	{
		ApplySuperArmor();
	}

	// BT 일시정지
	if (AERNBossAIController* BossAIC = Cast<AERNBossAIController>(GetController()))
	{
		if (UBrainComponent* Brain = BossAIC->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("PhaseTransition"));
		}
	}

	// 현재 몽타주가 재생 중인지 체크
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && AnimInstance->IsAnyMontagePlaying())
	{
		UAnimMontage* CurrentMontage = AnimInstance->GetCurrentActiveMontage();

		if (CurrentMontage)
		{
			// 재생중인 몽타주 종료까지 대기
			FOnMontageEnded OnCurrentMontageEnded;
			OnCurrentMontageEnded.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				PlayPhaseTransitionMontage();
			});

			AnimInstance->Montage_SetEndDelegate(OnCurrentMontageEnded, CurrentMontage);
		}
		else
		{
			// 1프레임 타이밍 이슈: 다음 틱에서 재시도
			GetWorldTimerManager().SetTimerForNextTick(this, &AERNBossCharacter::PlayPhaseTransitionMontage);
		}
	}
	else
	{
		// 재생 중인 몽타주가 없으면 바로 재생
		PlayPhaseTransitionMontage();
	}
}

void AERNBossCharacter::PlayPhaseTransitionMontage()
{
	if (!Phases.IsValidIndex(CurrentPhaseIndex))
	{
		OnPhaseTransitionMontageEnded(nullptr, false);
		return;
	}

	const FBossPhaseInfo& PhaseInfo = Phases[CurrentPhaseIndex];

	// 페이즈 전환 몽타주 재생 — Multicast로 모든 클라이언트에 동기화
	if (PhaseInfo.PhaseTransitionMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		Multicast_PlayPhaseTransitionMontage(PhaseInfo.PhaseTransitionMontage);
	}
	else
	{
		// 몽타주 없으면 바로 완료 처리
		OnPhaseTransitionMontageEnded(nullptr, false);
	}
}

void AERNBossCharacter::Multicast_PlayPhaseTransitionMontage_Implementation(UAnimMontage* Montage)
{
	if (!Montage || !GetMesh() || !GetMesh()->GetAnimInstance())
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->Montage_Play(Montage);

	// 종료 콜백은 서버에서만 바인딩 (페이즈 상태 변경/슈퍼아머 해제 등 서버 권한 로직)
	if (HasAuthority())
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AERNBossCharacter::OnPhaseTransitionMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	}
}

void AERNBossCharacter::OnPhaseTransitionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsTransitioningPhase = false;

	// 슈퍼아머 해제
	RemoveSuperArmor();

	// 페이즈 전환 몽타주 끝난 후 새 BT로 전환
	if (Phases.IsValidIndex(CurrentPhaseIndex))
	{
		const FBossPhaseInfo& CurrentPhase = Phases[CurrentPhaseIndex];
		if (AERNBossAIController* BossAIC = Cast<AERNBossAIController>(GetController()))
		{
			BossAIC->SwitchBehaviorTree(CurrentPhase.PhaseBehaviorTree);
		}
	}
}

void AERNBossCharacter::SetIntroCutsceneLocked(bool bLocked)
{
	if (!HasAuthority()) return;
	bIsIntroLocked = bLocked;

	AAIController* AIC = Cast<AAIController>(GetController());
	if (!AIC) return;

	// BT 정지/재개 → 보스 idle 상태로 가만히
	if (UBrainComponent* Brain = AIC->GetBrainComponent())
	{
		if (bLocked) Brain->StopLogic(TEXT("BossEncounterCutscene"));
		else         Brain->RestartLogic();
	}

	// Perception 활성/비활성 → 감지 콜백 차단 → 체력바 트리거 X
	if (UAIPerceptionComponent* PerceptionComp = AIC->GetPerceptionComponent())
	{
		PerceptionComp->SetActive(!bLocked);
	}
}

void AERNBossCharacter::PlayIntro()
{
	// 인트로 컷신이 있으면 컷신 재생
	if (!IntroCutscene.IsNull())
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UERNCutsceneSubsystem* CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>())
			{
				// 인트로 중 슈퍼아머
				ApplySuperArmor();

				// 컷신 종료 시 슈퍼아머 해제
				CutsceneSubsystem->OnCutsceneFinished.AddDynamic(this, &AERNBossCharacter::OnIntroCutsceneFinished);

				// 컷신 재생 (플레이어 자동 바인딩)
				CutsceneSubsystem->PlayCutscene(IntroCutscene.LoadSynchronous());

				UE_LOG(LogTemp, Log, TEXT("[Boss %s] Playing intro cutscene"), *GetName());
				return;
			}
		}
	}

	// 컷신이 없으면 몽타주로 폴백
	if (IntroMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		// 인트로 중 슈퍼아머
		ApplySuperArmor();

		GetMesh()->GetAnimInstance()->Montage_Play(IntroMontage);

		// 인트로 종료 후 슈퍼아머 해제
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AERNBossCharacter::OnPhaseTransitionMontageEnded);
		GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, IntroMontage);
	}
}

void AERNBossCharacter::ApplyDynamicDifficulty()
{
	if (!HasAuthority() || !AttributeSet || bDynamicDifficultyApplied)
	{
		return;
	}

	AERNGameState* GS = GetWorld() ? GetWorld()->GetGameState<AERNGameState>() : nullptr;
	if (!GS)
	{
		return;
	}

	// 파티 평균 공격력(캐릭터 공격력 + 무기 보너스) / 평균 레벨 집계
	float SumAttack = 0.f;
	float SumLevel = 0.f;
	int32 Count = 0;

	for (APlayerState* PS : GS->PlayerArray)
	{
		AProjectERNCharacter* Player = PS ? Cast<AProjectERNCharacter>(PS->GetPawn()) : nullptr;
		if (!Player)
		{
			continue;
		}

		UERNAttributeSet* PlayerAttr = Player->GetAttributeSet();
		if (!PlayerAttr)
		{
			continue;
		}

		// 현재 장착 무기 공격력 (장착 무기 없으면 0)
		float WeaponDamage = 0.f;
		if (UERNEquipmentComponent* Equip = Player->GetEquipmentComponent())
		{
			if (AERNWeaponBase* Weapon = Equip->CurrentWeapon)
			{
				WeaponDamage = Weapon->LightAttackDamage;
			}
		}

		SumAttack += PlayerAttr->GetAttackPower() + WeaponDamage;
		SumLevel += PlayerAttr->GetLevel();
		++Count;
	}

	if (Count == 0)
	{
		return;
	}

	bDynamicDifficultyApplied = true;

	const float AvgAttack = SumAttack / Count;
	const float AvgLevel = SumLevel / Count;

	// 체력 배율 = 1 + 계수 * (평균 공격력 - 기준치), 클램프
	const float HealthMul = FMath::Clamp(
		1.f + HealthScalePerAttack * (AvgAttack - AttackBaseline),
		1.f, MaxHealthMultiplier);

	// 인원수 배율 = Count / FullPartySize (풀파티=1.0, 인원 적을수록 감소). 풀파티 초과 시 1.0로 캡
	const float PartyScale = FMath::Min(static_cast<float>(Count) / FMath::Max(1, FullPartySize), 1.f);

	const float NewMaxHealth = InitialMaxHealth * HealthMul * PartyScale;
	AttributeSet->SetMaxHealth(NewMaxHealth);
	AttributeSet->SetHealth(NewMaxHealth);   // 조우 시점이므로 풀피로 시작

	// 레벨 기반 공격 배율 = 1 + 계수 * (평균 레벨 - 기준치), 클램프
	const float LevelAttackMul = FMath::Clamp(
		1.f + AttackScalePerLevel * (AvgLevel - LevelBaseline),
		1.f, MaxAttackMultiplier);

	// 인원 부족 공격 배율 = 1 - 감소량 * 부족 인원수, 하한 클램프 (풀파티=1.0, 1명 모자랄 때마다 감소)
	const int32 MissingPlayers = FMath::Max(0, FullPartySize - Count);
	const float AttackPartyScale = FMath::Clamp(
		1.f - AttackReductionPerMissingPlayer * MissingPlayers,
		MinAttackPartyScale, 1.f);

	// 최종 출력 데미지 배율 = 레벨 배율 * 인원 배율
	OutgoingDamageMultiplier = LevelAttackMul * AttackPartyScale;

	UE_LOG(LogTemp, Log, TEXT("[Boss %s] DynamicDifficulty: Players=%d AvgAtk=%.1f AvgLv=%.1f -> MaxHP=%.0f(HMul x%.2f, Party x%.2f) DmgMul=%.2f(Lv x%.2f, Party x%.2f)"),
		*GetName(), Count, AvgAttack, AvgLevel, NewMaxHealth, HealthMul, PartyScale, OutgoingDamageMultiplier, LevelAttackMul, AttackPartyScale);
}

void AERNBossCharacter::ShowHealthBarToAllPlayers()
{
	if (!HasAuthority() || bHealthBarShown)
	{
		return;
	}

	bHealthBarShown = true;

	// 체력바 표시 전에 파티 스탯 기반으로 보스 체력/공격력 조정
	ApplyDynamicDifficulty();

	// 체력 퍼센트 계산
	const float HealthPercent = AttributeSet && AttributeSet->GetMaxHealth() > 0.f
		? AttributeSet->GetHealth() / AttributeSet->GetMaxHealth()
		: 1.f;

	// 모든 플레이어에게 보스 체력바 표시
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(It->Get()))
		{
			PC->Client_ShowBossHealthBar(this);
			PC->Client_UpdateBossHealthBar(HealthPercent, 0.f);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Boss %s] Health bar shown to all players"), *GetName());
}

void AERNBossCharacter::Multicast_PlayBossBGM_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[Boss BGM] Multicast_PlayBossBGM_Implementation called. bBGMStarted=%d, BossBGM=%s"),
		bBGMStarted, BossBGM ? *BossBGM->GetName() : TEXT("NULL"));

	if (bBGMStarted || !BossBGM) return;
	bBGMStarted = true;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UERNSoundSubsystem* SS = GI->GetSubsystem<UERNSoundSubsystem>())
		{
			SS->PlayBGM(BossBGM, BGMFadeInTime);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Boss BGM] SoundSubsystem null"));
		}
	}
}

void AERNBossCharacter::Multicast_StopBossBGM_Implementation()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UERNSoundSubsystem* SS = GI->GetSubsystem<UERNSoundSubsystem>())
		{
			SS->StopBGM(BGMFadeOutTime);
		}
	}
}

void AERNBossCharacter::Multicast_PlayTeleportTrail_Implementation(FVector StartLocation, FVector EndLocation)
{
	if (!TeleportTrailFX)
	{
		return;
	}

	// 시작점에 스폰하고, 빔 양 끝 좌표를 월드 좌표로 직접 지정 (모든 클라 동일 위치)
	UNiagaraComponent* Beam = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(), TeleportTrailFX, StartLocation);

	if (Beam)
	{
		Beam->SetVariableVec3(FName("BeamStart"), StartLocation);
		Beam->SetVariableVec3(FName("BeamEnd"), EndLocation);
	}
}

void AERNBossCharacter::OnDeath()
{
	// 모든 플레이어의 보스 체력바 숨김 + BGM 정지
	if (HasAuthority())
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (AERNPlayerController* PC = Cast<AERNPlayerController>(It->Get()))
			{
				PC->Client_HideBossHealthBar();
			}
		}

		// 보스 BGM 페이드 아웃
		Multicast_StopBossBGM();
	}

	// 최종보스 사망 → 게임 클리어(승리) 처리
	if (HasAuthority() && bIsFinalBoss)
	{
		if (AERNGameState* GS = GetWorld()->GetGameState<AERNGameState>())
		{
			GS->HandleGameClear();
		}
	}

	// 중간보스 사망 → 일정 시간 후 죽은 자리에 보스 포탈 스폰
	// 주의: 부모 OnDeath가 사망 몽타주 길이 후 this를 Destroy하므로,
	//       딜레이가 그보다 길면 this가 무효화됨 → 위치/회전/월드를 값으로 캡처해 this 미참조
	if (HasAuthority() && bIsMidBoss && BossPortalClass)
	{
		UWorld* World = GetWorld();
		const FVector  SpawnLoc = GetActorLocation();
		const FRotator SpawnRot = GetActorRotation();
		TSubclassOf<AERNBossPortal> PortalClass = BossPortalClass;

		FTimerHandle PortalTimer;
		World->GetTimerManager().SetTimer(PortalTimer,
			FTimerDelegate::CreateLambda([World, SpawnLoc, SpawnRot, PortalClass]()
			{
				if (!World) return;

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride =
					ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				World->SpawnActor<AERNBossPortal>(PortalClass, SpawnLoc, SpawnRot, SpawnParams);
			}),
			BossPortalSpawnDelay, false);
	}

	// 부모 사망 처리 (사망 몽타주 재생, 이동/충돌 비활성화, 딜레이 후 Destroy 등)
	Super::OnDeath();
}

void AERNBossCharacter::ApplySuperArmor()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(TAG_State_SuperArmor);
	}
}

void AERNBossCharacter::RemoveSuperArmor()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(TAG_State_SuperArmor);
	}
}

void AERNBossCharacter::OnIntroCutsceneFinished()
{
	// 델리게이트 해제
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UERNCutsceneSubsystem* CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>())
		{
			CutsceneSubsystem->OnCutsceneFinished.RemoveDynamic(this, &AERNBossCharacter::OnIntroCutsceneFinished);
		}
	}

	// 슈퍼아머 해제
	RemoveSuperArmor();
}
