// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/ERNEnemyCharacter.h"
#include "GAS/ERNAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/BoxComponent.h"
#include "UI/ERNEnemyHealthBarWidget.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Character/Player/ERNPlayerController.h"
#include "Engine/DamageEvents.h"
#include "MotionWarpingComponent.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Character/Player/ERNPlayerState.h"
#include "Inventory/Item/ERNItemActor.h"

AERNEnemyCharacter::AERNEnemyCharacter()
{
	// AI가 자동으로 빙의하도록 설정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 200.0f, 0.0f);

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
}

void AERNEnemyCharacter::BindHitboxOverlaps()
{
	TArray<UBoxComponent*> Boxes;
	GetComponents<UBoxComponent>(Boxes);

	for (UBoxComponent* Box : Boxes)
	{
		// 히트박스 초기 상태를 NoCollision으로 설정
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Box->OnComponentBeginOverlap.AddDynamic(this, &AERNEnemyCharacter::OnHitboxOverlap);
	}
}

void AERNEnemyCharacter::OnHitboxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 플레이어만 대상, 중복 히트 방지
	AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(OtherActor);
	if (!Player || HitActors.Contains(OtherActor)) return;

	HitActors.Add(OtherActor);

	// 태그로 데미지/경직력 검색
	float DamageToApply = 10.f;
	float StaggerPowerToApply = 10.f;
	for (const FEnemyHitboxConfig& Config : HitboxConfigs)
	{
		if (OverlappedComp->ComponentHasTag(Config.Tag))
		{
			DamageToApply = Config.Damage;
			StaggerPowerToApply = Config.StaggerPower;
			break;
		}
	}

	FDamageEvent DamageEvent;
	Player->TakeDamage(DamageToApply, DamageEvent, GetController(), this);
	Player->TryApplyStagger(StaggerPowerToApply);
}

float AERNEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.0f)
	{
		Multicast_ShowHealthBar();

		// 공격자 클라이언트에게 데미지 텍스트 표시 요청
		if (HasAuthority())
		{
			if (AProjectERNCharacter* AttackerChar = Cast<AProjectERNCharacter>(DamageCauser))
			{
				if (AERNPlayerController* PC = Cast<AERNPlayerController>(AttackerChar->GetController()))
				{
					PC->Client_ShowDamageText(GetActorLocation(), ActualDamage);
				}
			}
		}
	}

	return ActualDamage;
}

void AERNEnemyCharacter::Multicast_ShowHealthBar_Implementation()
{
	if (!HealthBarWidget) return;

	HealthBarWidget->SetVisibility(true);

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

	HealthBarWidget->SetVisibility(false);
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
	// BT 중지 — 사망 후 BT가 다른 몽타주를 트리거해 사망 몽타주를 덮어쓰는 것 방지
	if (HasAuthority())
	{
		if (AAIController* AIC = Cast<AAIController>(GetController()))
		{
			if (UBrainComponent* Brain = AIC->GetBrainComponent())
			{
				Brain->StopLogic(TEXT("Death"));
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
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (const AERNPlayerState* PS = PC->GetPlayerState<AERNPlayerState>())
			{
				FItemRuntimeState ItemRuntimeState;
				AActor* Item = nullptr;

				switch (PS->CharacterType)
				{
				case ECharacterType::Warrior:
					if (ItemManager->RollItemFromDropTable(DropTable, ItemRuntimeState, EDropItemType::Sword))
					{
						Item = ItemManager->SpawnItem(ItemRuntimeState, GetActorLocation() + FVector(0.0f, 0.0f, 50.0f), GetActorRotation());
					}
					break;
				case ECharacterType::Mage:
					if (ItemManager->RollItemFromDropTable(DropTable, ItemRuntimeState, EDropItemType::Staff))
					{
						Item = ItemManager->SpawnItem(ItemRuntimeState, GetActorLocation() + FVector(0.0f, 0.0f, 50.0f), GetActorRotation());
					}
					break;
				case ECharacterType::Support:
					if (ItemManager->RollItemFromDropTable(DropTable, ItemRuntimeState, EDropItemType::Polearm))
					{
						Item = ItemManager->SpawnItem(ItemRuntimeState, GetActorLocation() + FVector(0.0f, 0.0f, 50.0f), GetActorRotation());
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
						ERNItem->UpdateOwnerOnlyVisibility();
					}
				}
			}
		}
	}
}

void AERNEnemyCharacter::SpawnGold()
{
	int32 GoldAmount = FMath::RandRange(MinGold, MaxGold);
	UE_LOG(LogTemp, Log, TEXT("%s dropped %d gold"), *GetName(), GoldAmount);

	// TODO: 골드 스폰
}
