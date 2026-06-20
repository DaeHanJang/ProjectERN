// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/ERNEnemyCharacter.h"
#include "GAS/ERNAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UI/ERNEnemyHealthBarWidget.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Core/ERNGameInstance.h"
#include "Character/Player/ERNPlayerController.h"
#include "Engine/DamageEvents.h"
#include "MotionWarpingComponent.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Character/Player/ERNPlayerState.h"
#include "Inventory/Item/ERNItemActor.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "EngineUtils.h"
#include "Actors/Portal/ERNInstancePortal.h"
#include "Actors/Portal/ERNPortalDestinationPoint.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

AERNEnemyCharacter::AERNEnemyCharacter()
{
	// AI가 자동으로 빙의하도록 설정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 200.0f, 0.0f);

	// 모든 적의 Capsule/Mesh는 Camera 채널과 충돌하지 않도록 (카메라 SpringArm trace 등에 잡히지 않음)
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	// 머리 위 체력바 위젯 컴포넌트
	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidget->SetupAttachment(RootComponent);
	HealthBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidget->SetDrawSize(FVector2D(150.0f, 20.0f));
	HealthBarWidget->SetVisibility(false);

	// 모션 워핑 컴포넌트
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
}

void AERNEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 GAS 초기화
	if (HasAuthority() && AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	// 초기 스탯 적용
	if (AttributeSet)
	{
		AttributeSet->InitMaxHealth(InitialMaxHealth);
		AttributeSet->InitHealth(InitialMaxHealth);
		AttributeSet->InitMaxMana(InitialMaxMana);
		AttributeSet->InitMana(InitialMaxMana);
		AttributeSet->InitMaxStamina(InitialMaxStamina);
		AttributeSet->InitStamina(InitialMaxStamina);
		AttributeSet->InitAttackPower(InitialAttackPower);
		AttributeSet->InitDefense(InitialDefense);
		AttributeSet->InitStaggerResistance(InitialStaggerResistance);
	}

	// 체력바 위젯 초기화 (스탯 적용 후)
	if (HealthBarWidget)
	{
		if (UERNEnemyHealthBarWidget* Widget = Cast<UERNEnemyHealthBarWidget>(HealthBarWidget->GetUserWidgetObject()))
		{
			Widget->InitWidget(this);
		}
	}

	// 서버에서만 히트박스 Overlap 바인딩
	if (HasAuthority())
	{
		BindHitboxOverlaps();
	}

	// 초기 DrawSize 캐싱 (거리 스케일링의 기준)
	if (HealthBarWidget)
	{
		InitialHealthBarDrawSize = HealthBarWidget->GetDrawSize();
	}

	// 체력바 거리 기반 스케일링 타이머 (로컬 - 모든 머신에서 실행)
	GetWorldTimerManager().SetTimer(
		HealthBarScaleTimerHandle,
		this,
		&AERNEnemyCharacter::UpdateHealthBarScale,
		HealthBarScaleUpdateInterval,
		true);
}

void AERNEnemyCharacter::SetPatrolPoints(const TArray<AActor*>& InPatrolPoints)
{
	// AI/BT는 서버에서만 동작
	if (!HasAuthority())
	{
		return;
	}

	PatrolPoints.Reset();
	for (AActor* Point : InPatrolPoints)
	{
		if (IsValid(Point))
		{
			PatrolPoints.Add(Point);
		}
	}
}

void AERNEnemyCharacter::BindHitboxOverlaps()
{
	TArray<UBoxComponent*> Boxes;
	GetComponents<UBoxComponent>(Boxes);

	for (UBoxComponent* Box : Boxes)
	{
		// 히트박스 초기 상태를 NoCollision으로 설정
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// Projectile 채널은 무시 - 히트박스 활성화 중에도 투사체가 막히지 않도록
		Box->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
		Box->OnComponentBeginOverlap.AddDynamic(this, &AERNEnemyCharacter::OnHitboxOverlap);
	}
}

void AERNEnemyCharacter::OnHitboxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 플레이어만 대상, 중복 히트 방지
	AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(OtherActor);
	if (!Player || Player->GetRootComponent() != OtherComp) return;
	if (!Player || HitActors.Contains(OtherActor)) return;

	HitActors.Add(OtherActor);

	// 태그로 데미지/경직력/사운드/체력비례 검색
	float DamageToApply = 10.f;
	float StaggerPowerToApply = 10.f;
	bool bUseMaxHealthPercent = false;
	float MaxHealthPercentToApply = 0.f;
	USoundBase* HitSoundToPlay = nullptr;
	for (const FEnemyHitboxConfig& Config : HitboxConfigs)
	{
		if (OverlappedComp->ComponentHasTag(Config.Tag))
		{
			DamageToApply = Config.Damage;
			StaggerPowerToApply = Config.StaggerPower;
			bUseMaxHealthPercent = Config.bAddMaxHealthPercentDamage;
			MaxHealthPercentToApply = Config.MaxHealthPercentDamage;
			HitSoundToPlay = Config.HitSound;
			break;
		}
	}

	// NotifyState가 설정한 스테이트별 오버라이드가 있으면 Config 값을 덮어씀
	if (bActiveDamageOverride)
	{
		DamageToApply = ActiveDamageOverride;
	}
	if (bActiveStaggerOverride)
	{
		StaggerPowerToApply = ActiveStaggerOverride;
	}
	if (bActiveMaxHealthPercentOverride)
	{
		bUseMaxHealthPercent = true;
		MaxHealthPercentToApply = ActiveMaxHealthPercentOverride;
	}

	// 동적 난이도 출력 배율 적용 (잡몹은 1.0이라 무영향)
	DamageToApply *= OutgoingDamageMultiplier;

	// 체력비례 추가 데미지 합산 (배율과 분리 — 대상 플레이어 최대HP 기준 고정)
	if (bUseMaxHealthPercent && MaxHealthPercentToApply > 0.f)
	{
		if (const UERNAttributeSet* TargetAttr = Player->GetAttributeSet())
		{
			DamageToApply += TargetAttr->GetMaxHealth() * MaxHealthPercentToApply;
		}
	}

	FDamageEvent DamageEvent;
	const float ActualDamage = Player->TakeDamage(DamageToApply, DamageEvent, GetController(), this);
	// 적 본체 위치를 HitOrigin으로 전달 → 플레이어가 적 방향 기준 4방향 경직 재생
	Player->TryApplyStagger(StaggerPowerToApply, GetActorLocation());

	// 실제 데미지가 들어간 경우에만 히트 사운드 재생 (무적/구르기 등으로 TakeDamage가 0 반환 시 무음)
	if (ActualDamage > 0.f && HitSoundToPlay)
	{
		Multicast_PlayHitSound(HitSoundToPlay, Player->GetActorLocation());
	}
}

void AERNEnemyCharacter::SetHitboxOverride(bool bDamage, float Damage, bool bStagger, float StaggerPower, bool bMaxHealthPercent, float MaxHealthPercent)
{
	bActiveDamageOverride = bDamage;
	ActiveDamageOverride = Damage;
	bActiveStaggerOverride = bStagger;
	ActiveStaggerOverride = StaggerPower;
	bActiveMaxHealthPercentOverride = bMaxHealthPercent;
	ActiveMaxHealthPercentOverride = MaxHealthPercent;
}

void AERNEnemyCharacter::ClearHitboxOverride()
{
	bActiveDamageOverride = false;
	ActiveDamageOverride = 0.f;
	bActiveStaggerOverride = false;
	ActiveStaggerOverride = 0.f;
	bActiveMaxHealthPercentOverride = false;
	ActiveMaxHealthPercentOverride = 0.f;
}

void AERNEnemyCharacter::StartDamageAccumulation()
{
	bTrackAccumulatedDamage = true;
	AccumulatedDamage = 0.f;
}

void AERNEnemyCharacter::StopDamageAccumulation()
{
	bTrackAccumulatedDamage = false;
	AccumulatedDamage = 0.f;
}

void AERNEnemyCharacter::Multicast_PlayHitSound_Implementation(USoundBase* Sound, FVector Location)
{
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Location);
	}
}

void AERNEnemyCharacter::Multicast_PlayCounterChargeFX_Implementation(UNiagaraSystem* System, USoundBase* Sound, FVector Location)
{
	if (System)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, System, Location);
	}
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Location);
	}
}

float AERNEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 무적이면 데미지 무시 (Mob/Boss/TrainingDummy 모두 이 함수를 경유)
	if (bIsImmortal)
	{
		return 0.0f;
	}

	// 하드모드: 플레이어가 주는 피해 20% 감소 (서버 권위)
	if (HasAuthority())
	{
		if (const UERNGameInstance* GI = Cast<UERNGameInstance>(GetGameInstance()))
		{
			if (GI->IsHardModeEnabled())
			{
				AProjectERNCharacter* AttackerChar = Cast<AProjectERNCharacter>(DamageCauser);
				if (!AttackerChar && EventInstigator)
				{
					AttackerChar = Cast<AProjectERNCharacter>(EventInstigator->GetPawn());
				}
				if (AttackerChar)
				{
					DamageAmount *= 0.8f;
				}
			}
		}
	}

	// 최종 데미지에 ±편차 랜덤 적용 (서버 권한에서만 굴려 체력 변경값이 리플리케이션으로 동기화되도록 함)
	if (HasAuthority() && DamageVariance > 0.f && DamageAmount > 0.f)
	{
		DamageAmount *= FMath::FRandRange(1.f - DamageVariance, 1.f + DamageVariance);
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.0f)
	{
		Multicast_ShowHealthBar();

		// NotifyState 구간 데미지 누적 (임계치 도달 시 반격 투사체 발사용)
		if (HasAuthority() && bTrackAccumulatedDamage)
		{
			AccumulatedDamage += ActualDamage;
		}

		// 공격자 클라이언트에게 데미지 텍스트 표시 요청 + 누적 데미지(전과)
		if (HasAuthority())
		{
			AProjectERNCharacter* AttackerChar = Cast<AProjectERNCharacter>(DamageCauser);
			if (!AttackerChar && EventInstigator)
			{
				AttackerChar = Cast<AProjectERNCharacter>(EventInstigator->GetPawn());
			}
			if (AttackerChar)
			{
				if (AERNPlayerController* PC = Cast<AERNPlayerController>(AttackerChar->GetController()))
				{
					PC->Client_ShowDamageText(this, GetActorLocation(), ActualDamage);
				}
				if (!bExcludeFromCombatStats)
				{
					if (AERNPlayerState* AttackerPS = AttackerChar->GetPlayerState<AERNPlayerState>())
					{
						AttackerPS->TotalDamageDealt += ActualDamage;
					}
				}
			}
		}
	}

	return ActualDamage;
}

void AERNEnemyCharacter::Multicast_ShowHealthBar_Implementation()
{
	if (!HealthBarWidget) return;

	bHealthBarIntendedVisible = true;

	// 거리에 맞는 크기를 먼저 계산/적용 → 원래 크기로 잠깐 나왔다 줄어드는 팝핑 방지
	// (UpdateHealthBarScale가 SetDrawSize + SetVisibility(true)까지 처리)
	UpdateHealthBarScale();

	// 기존 타이머 초기화 후 재시작
	GetWorld()->GetTimerManager().SetTimer(
		HealthBarHideTimerHandle,
		this,
		&AERNEnemyCharacter::Multicast_HideHealthBar,
		HealthBarHideDelay,
		false
	);
}

void AERNEnemyCharacter::Multicast_HideHealthBar_Implementation()
{
	if (!HealthBarWidget) return;

	bHealthBarIntendedVisible = false;
	HealthBarWidget->SetVisibility(false);
}

void AERNEnemyCharacter::UpdateHealthBarScale()
{
	if (!HealthBarWidget || !bHealthBarIntendedVisible)
	{
		return;
	}

	APlayerCameraManager* CamMgr = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!CamMgr)
	{
		return;
	}

	const float Dist = FVector::Dist(CamMgr->GetCameraLocation(), GetActorLocation());

	// 최대 표시 거리 초과 시 숨김 (의도 플래그는 유지 → 재진입 시 다시 보이게)
	if (HealthBarMaxDisplayDistance > 0.f && Dist > HealthBarMaxDisplayDistance)
	{
		HealthBarWidget->SetVisibility(false);
		return;
	}

	// 거리 안에 들어오면 다시 표시
	HealthBarWidget->SetVisibility(true);

	// 거리에 반비례한 스케일 (멀수록 작게) + Min/Max 클램프
	const float RawScale = (Dist > 0.f) ? (HealthBarReferenceDistance / Dist) : HealthBarMaxScale;
	const float Scale = FMath::Clamp(RawScale, HealthBarMinScale, HealthBarMaxScale);

	// Screen-space는 DrawSize(픽셀)로 직접 크기 조정
	HealthBarWidget->SetDrawSize(InitialHealthBarDrawSize * Scale);
}

void AERNEnemyCharacter::Multicast_PlayAttackMontage_Implementation(UAnimMontage* Montage)
{
	if (!Montage || !GetMesh()) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && !AnimInstance->Montage_IsPlaying(Montage))
	{
		AnimInstance->Montage_Play(Montage);
	}
}

void AERNEnemyCharacter::OnDeath()
{
	// 막타 크레딧 — 마지막 유효타를 넣은 플레이어의 KillCount 증가
	if (HasAuthority() && !bExcludeFromCombatStats)
	{
		if (AController* Killer = LastHitInstigator.Get())
		{
			if (AERNPlayerState* KillerPS = Killer->GetPlayerState<AERNPlayerState>())
			{
				KillerPS->KillCount++;
			}
		}
	}

	// BT 중지 — 사망 후 BT가 다른 몽타주를 트리거해 사망 몽타주를 덮어쓰는 것 방지
	if (HasAuthority())
	{
		if (AAIController* AIC = Cast<AAIController>(GetController()))
		{
			if (UBrainComponent* Brain = AIC->GetBrainComponent())
			{
				Brain->StopLogic(TEXT("Death"));
			}

			// 자기가 시야로 감지 중이던 플레이어들에게 감지 상실 통지 (비전투 그레이스 타이머 트리거)
			if (UAIPerceptionComponent* PerceptionComp = AIC->GetPerceptionComponent())
			{
				TArray<AActor*> Perceived;
				PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Perceived);
				for (AActor* A : Perceived)
				{
					if (AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(A))
					{
						Player->NotifyLostBy(this);
					}
				}
			}
		}
	}

	// 체력바 즉시 숨김 (모든 클라이언트 동기화)
	Multicast_HideHealthBar();

	Super::OnDeath();

	if (HasAuthority())
	{
		// 몽타주 종료 직전에 정리 (T-pose로 돌아가기 직전 숨김 처리)
		const float CleanupDelay = DeathMontage
			? FMath::Max(DeathMontage->GetPlayLength() - DeathCleanupLeadTime, 0.0f)
			: 0.0f;

		FTimerHandle CleanupTimer;
		GetWorld()->GetTimerManager().SetTimer(CleanupTimer, [this]()
		{
			// 모든 클라이언트에서 메시 숨김
			Multicast_HideOnDeath();

			// 골드 드롭
			SpawnGold();

			// 아이템 드롭
			SpawnDrops();

			// 인스턴스 포탈 스폰 (확률 + 빈 지점 있을 때만)
			TrySpawnInstancePortal();

			// 액터 제거
			Destroy();
		}, CleanupDelay, false);
	}
}

void AERNEnemyCharacter::Multicast_HideOnDeath_Implementation()
{
	SetActorHiddenInGame(true);
}

void AERNEnemyCharacter::SpawnDrops()
{
	if (!DropTable)
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("In AERNEnemyCharacter::SpawnDrops"));
	
	if (UItemManagerSubsystem* ItemManager = GetGameInstance()->GetSubsystem<UItemManagerSubsystem>())
	{
		int32 DropIndex = 0;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (const AERNPlayerState* PS = PC->GetPlayerState<AERNPlayerState>())
			{
				FItemRuntimeState ItemRuntimeState;
				AActor* Item = nullptr;
				const FVector DropLocation = GetActorLocation() + FVector::UpVector * 100.0f * ++DropIndex;

				switch (PS->CharacterType)
				{
				case ECharacterType::Warrior:
					if (ItemManager->RollItemFromDropTable(DropTable, ItemRuntimeState, EDropItemType::Sword))
					{
						Item = ItemManager->SpawnItem(ItemRuntimeState, DropLocation, GetActorForwardVector().Rotation());
					}
					break;
				case ECharacterType::Mage:
					if (ItemManager->RollItemFromDropTable(DropTable, ItemRuntimeState, EDropItemType::Staff))
					{
						Item = ItemManager->SpawnItem(ItemRuntimeState, DropLocation, GetActorForwardVector().Rotation());
					}
					break;
				case ECharacterType::Support:
					if (ItemManager->RollItemFromDropTable(DropTable, ItemRuntimeState, EDropItemType::Polearm))
					{
						Item = ItemManager->SpawnItem(ItemRuntimeState, DropLocation, GetActorForwardVector().Rotation());
					}
					break;
				default:
					Item = nullptr;
					break;
				}
				
				if (Item)
				{
					UE_LOG(LogTemp, Warning, TEXT("Spawn %s Item"), *GetNameSafe(Item));
					
					Item->SetOwner(Cast<AActor>(PC));
					Item->bOnlyRelevantToOwner = true;
					if (AERNItemActor* ERNItem = Cast<AERNItemActor>(Item))
					{
						ERNItem->Launch(GetActorForwardVector());
						ERNItem->UpdateOwnerOnlyVisibility();
					}
				}
			}
		}
	}
}

void AERNEnemyCharacter::SpawnGold()
{
	const int32 MinGold = BasicRewordGold - RewordGoldVariance / 100.0f * BasicRewordGold;
	const int32 MaxGold = BasicRewordGold + RewordGoldVariance / 100.0f * BasicRewordGold;
	const int32 RewordGold = FMath::RandRange(MinGold, MaxGold);
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}
		
		const AERNCharacterBase* PlayerCharacter = Cast<AERNCharacterBase>(PC->GetPawn());
		if (!PlayerCharacter)
		{
			continue;
		}
		
		float GoldReward = RewordGold + PlayerCharacter->GetGoldWeight() + PlayerCharacter->GetAccountGoldWeight();

		// 하드모드: 골드 획득 30% 감소
		if (const UERNGameInstance* GI = Cast<UERNGameInstance>(GetGameInstance()))
		{
			if (GI->IsHardModeEnabled())
			{
				GoldReward *= 0.7f;
			}
		}

		PlayerCharacter->AddGold(GoldReward);
	}
}

void AERNEnemyCharacter::TrySpawnInstancePortal()
{
	if (!HasAuthority())
	{
		return;
	}

	// 확률/클래스 미설정 시 스폰 안 함
	if (InstancePortalClass == nullptr || InstancePortalSpawnChance <= 0.f)
	{
		return;
	}

	// 몬스터별 확률 판정
	if (FMath::FRand() > InstancePortalSpawnChance)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 빈 도착 지점 후보 수집 (NightRainZoneManager::CollectZoneCenter 와 동일하게 TActorIterator 사용)
	TArray<AERNPortalDestinationPoint*> Candidates;
	for (TActorIterator<AERNPortalDestinationPoint> It(World); It; ++It)
	{
		AERNPortalDestinationPoint* Point = *It;
		if (IsValid(Point) && Point->bIsUsed == false)
		{
			Candidates.Add(Point);
		}
	}

	// 빈 지점이 없으면 애초에 스폰하지 않음 (생겼다 Destroy 되는 깜빡임 방지)
	if (Candidates.Num() == 0)
	{
		return;
	}

	// 후보 중 랜덤 선택 → 일회용이므로 즉시 점유 (반납 없음)
	AERNPortalDestinationPoint* ChosenPoint = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	ChosenPoint->bIsUsed = true;

	// 포탈은 적이 죽은 자리에 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AERNInstancePortal* Portal = World->SpawnActor<AERNInstancePortal>(
		InstancePortalClass, GetActorLocation(), GetActorRotation(), SpawnParams);

	if (Portal == nullptr)
	{
		return;
	}

	// 선택된 지점을 포탈의 도착 지점으로 주입
	Portal->SetDestinationPoint(ChosenPoint);
}
