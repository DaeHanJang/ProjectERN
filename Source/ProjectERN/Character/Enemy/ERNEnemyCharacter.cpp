// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/ERNEnemyCharacter.h"
#include "GAS/ERNAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/BoxComponent.h"
#include "UI/ERNEnemyHealthBarWidget.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Engine/DamageEvents.h"
#include "MotionWarpingComponent.h"

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

	// 체력바 위젯 초기화
	if (HealthBarWidget)
	{
		if (UERNEnemyHealthBarWidget* Widget = Cast<UERNEnemyHealthBarWidget>(HealthBarWidget->GetUserWidgetObject()))
		{
			Widget->InitWidget(this);
		}
	}

	// 경직 저항력 초기값 적용
	if (AttributeSet)
	{
		AttributeSet->InitStaggerResistance(InitialStaggerResistance);
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
	Super::OnDeath();

	if (HasAuthority())
	{
		// 골드 드롭
		SpawnGold();

		// 아이템 드롭
		SpawnDrops();

		// 3초 후 제거
		FTimerHandle DestroyTimer;
		GetWorld()->GetTimerManager().SetTimer(DestroyTimer, [this]()
		{
			Destroy();
		}, 3.0f, false);
	}
}

void AERNEnemyCharacter::SpawnDrops()
{
	// TODO: 드랍 테이블 기반으로 아이템 스폰
	for (const FDropItemInfo& DropInfo : DropTable)
	{
		float RandomValue = FMath::FRand();
		if (RandomValue <= DropInfo.DropChance)
		{
			int32 DropCount = FMath::RandRange(DropInfo.MinCount, DropInfo.MaxCount);
			UE_LOG(LogTemp, Log, TEXT("Dropped %s x%d"), *DropInfo.ItemID.ToString(), DropCount);
		}
	}
}

void AERNEnemyCharacter::SpawnGold()
{
	int32 GoldAmount = FMath::RandRange(MinGold, MaxGold);
	UE_LOG(LogTemp, Log, TEXT("%s dropped %d gold"), *GetName(), GoldAmount);

	// TODO: 골드 스폰
}
