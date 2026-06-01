// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/ERNCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GAS/ERNAttributeSet.h"
#include "GAS/ERNGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/ERNCutsceneSubsystem.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Kismet/GameplayStatics.h"

AERNCharacterBase::AERNCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// GAS 컴포넌트 생성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UERNAttributeSet>(TEXT("AttributeSet"));
}

void AERNCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AERNCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AERNCharacterBase, CutsceneSpeed);
}

void AERNCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 서버에서만 컷신 Speed 계산
	if (!HasAuthority())
	{
		return;
	}

	// 컷신 재생 중인지 확인
	UERNCutsceneSubsystem* CutsceneSubsystem = nullptr;
	if (UGameInstance* GI = Cast<UGameInstance>(GetWorld()->GetGameInstance()))
	{
		CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>();
	}

	if (!CutsceneSubsystem || !CutsceneSubsystem->IsPlayingCutscene())
	{
		// 컷신 아니면 리셋
		bCutscenePrevLocationValid = false;
		CutsceneSpeed = 0.f;
		return;
	}

	// 컷신 중 Speed 계산
	FVector CurrentLocation = GetActorLocation();

	if (bCutscenePrevLocationValid && DeltaTime > 0.f)
	{
		FVector PositionDelta = CurrentLocation - CutscenePrevLocation;
		float CalculatedSpeed = PositionDelta.Size() / DeltaTime;

		// 텔레포트 감지
		constexpr float MaxReasonableSpeed = 2000.f;
		if (CalculatedSpeed > MaxReasonableSpeed)
		{
			CalculatedSpeed = 0.f;
		}

		// 미세 떨림 방지
		constexpr float SpeedThreshold = 20.f;
		if (CalculatedSpeed < SpeedThreshold)
		{
			CalculatedSpeed = 0.f;
		}

		CutsceneSpeed = CalculatedSpeed;
	}
	else
	{
		bCutscenePrevLocationValid = true;
	}

	CutscenePrevLocation = CurrentLocation;
}

void AERNCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystemActorInfo();

	// 서버에서만 어빌리티 부여
	if (HasAuthority())
	{
		GiveDefaultAbilities();
	}
}

void AERNCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeAbilitySystemActorInfo();
}

void AERNCharacterBase::InitializeAbilitySystemActorInfo()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void AERNCharacterBase::GiveDefaultAbilities()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
		}
	}
}

void AERNCharacterBase::SetWeaponAbility(TSubclassOf<UGameplayAbility> NewWeaponAbility)
{
	if (!NewWeaponAbility)
	{
		return;
	}
	
	if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return;
	}
	
	if (WeaponAbilityHandle.IsValid())
	{
		if (FGameplayAbilitySpec* OldSpec = AbilitySystemComponent->FindAbilitySpecFromHandle(WeaponAbilityHandle))
		{
			AbilitySystemComponent->CancelAbilityHandle(WeaponAbilityHandle);
			
			AbilitySystemComponent->ClearAbility(WeaponAbilityHandle);
		}
		
		WeaponAbilityHandle = FGameplayAbilitySpecHandle();
	}
	
	FGameplayAbilitySpec NewSpec(NewWeaponAbility, 1, INDEX_NONE, this);
	WeaponAbilityHandle = AbilitySystemComponent->GiveAbility(NewSpec);
}

void AERNCharacterBase::SetConsumableAbility(TSubclassOf<UGameplayAbility> NewConsumableAbility)
{
	if (!NewConsumableAbility)
	{
		return;
	}
	
	if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return;
	}
	
	if (ConsumableAbilityHandle.IsValid())
	{
		if (FGameplayAbilitySpec* OldSpec = AbilitySystemComponent->FindAbilitySpecFromHandle(ConsumableAbilityHandle))
		{
			AbilitySystemComponent->CancelAbilityHandle(ConsumableAbilityHandle);
			
			AbilitySystemComponent->ClearAbility(ConsumableAbilityHandle);
		}
		
		ConsumableAbilityHandle = FGameplayAbilitySpecHandle();
	}
	
	FGameplayAbilitySpec NewSpec(NewConsumableAbility, 1, INDEX_NONE, this);
	ConsumableAbilityHandle = AbilitySystemComponent->GiveAbility(NewSpec);
}

UAbilitySystemComponent* AERNCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float AERNCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead)
	{
		return 0.0f;
	}

	// 대미지 면역 상태
	if (AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Immunity_Damage))
	{
		return 0.0f;
	}
	
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (AttributeSet && ActualDamage > 0.0f)
	{
		float RemainingDamage = ActualDamage;
		// 실드 적용 시 체력 감소 대신 적용
		if (AttributeSet->GetShield() > 0.f)
		{
			// 현재 실드
			const float CurrentShield = AttributeSet->GetShield();
			// 흡수한 대미지
			const float AbsorbedDamage = FMath::Min(CurrentShield, RemainingDamage);
			// 남은 실드
			const float RemainingShield = CurrentShield - AbsorbedDamage;
			
			// 남은 실드 적용
			AttributeSet->SetShield(RemainingShield);
			RemainingDamage -= AbsorbedDamage;
			
			// 실드 모두 소진 시
			if (RemainingShield <= 0.f && AbilitySystemComponent)
			{
				// TAG_Buff_Shield 태그 찾아서 제거
				FGameplayTagContainer ShieldTags;
				ShieldTags.AddTag(TAG_Buff_Shield);
				AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(ShieldTags);
			}
		}
		
		// 디버그 무적: 플레이어가 GodMode면 HP 최소 1로 클램프
		float MinHealth = 0.0f;
		if (const AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(this))
		{
			if (Player->bGodMode)
			{
				MinHealth = 1.0f;
			}
		}
		
		// AttributeSet의 Health 감소
		const float NewHealth = FMath::Max(MinHealth, AttributeSet->GetHealth() - RemainingDamage);
		AttributeSet->SetHealth(NewHealth);

		UE_LOG(LogTemp, Log, TEXT("%s took %.2f damage. Health: %.2f"), *GetName(), RemainingDamage, NewHealth);

		// 막타 크레딧용 — 마지막 유효타를 넣은 컨트롤러 캐싱
		LastHitInstigator = EventInstigator;

		// 사망 체크
		if (NewHealth <= 0.0f)
		{
			OnDeath();
		}
	}

	return ActualDamage;
}

void AERNCharacterBase::TryApplyStagger(float IncomingStaggerPower, const FVector& HitOrigin)
{
	if (!AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	// 이미 사망 처리됐으면 경직/히트리액션 무시 (사망 몽타주 보존)
	if (bIsDead)
	{
		return;
	}

	// 슈퍼아머 또는 무적 프레임이면 경직 무시
	if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Immunity_Damage) ||
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_SuperArmor) ||
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_StaggerImmune))
	{
		return;
	}

	// StaggerPower가 저항력 이상일 때만 경직 적용
	if (IncomingStaggerPower < AttributeSet->GetStaggerResistance())
	{
		return;
	}

	// GE_Stagger 적용
	if (StaggerEffect)
	{
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		AbilitySystemComponent->ApplyGameplayEffectToSelf(
			StaggerEffect->GetDefaultObject<UGameplayEffect>(), 1.f, Context);
	}

	// 재생할 몽타주 결정: 다운 임계값 초과면 DownMontage, 아니면 방향별 몽타주
	UAnimMontage* MontageToPlay = nullptr;

	if (IncomingStaggerPower >= AttributeSet->GetDownResistance() && DownMontage)
	{
		MontageToPlay = DownMontage;
	}
	else
	{
		const EHitDirection Dir = ComputeHitDirection(HitOrigin);
		switch (Dir)
		{
			case EHitDirection::Front: MontageToPlay = HitReactionMontage_Front; break;
			case EHitDirection::Back:  MontageToPlay = HitReactionMontage_Back;  break;
			case EHitDirection::Left:  MontageToPlay = HitReactionMontage_Left;  break;
			case EHitDirection::Right: MontageToPlay = HitReactionMontage_Right; break;
		}
	}

	// 방향별 / 다운 몽타주가 비어 있으면 기본 HitReactionMontage로 fallback
	if (!MontageToPlay)
	{
		MontageToPlay = HitReactionMontage;
	}

	if (MontageToPlay)
	{
		Multicast_PlayHitReaction(MontageToPlay);
	}
}

EHitDirection AERNCharacterBase::ComputeHitDirection(const FVector& HitOrigin) const
{
	// HitOrigin이 ZeroVector면 방향 정보 없음 → Front fallback
	if (HitOrigin.IsNearlyZero())
	{
		return EHitDirection::Front;
	}

	// 공격자 방향 벡터 (Z 무시, 수평면)
	FVector ToAttacker = HitOrigin - GetActorLocation();
	ToAttacker.Z = 0.f;
	if (!ToAttacker.Normalize())
	{
		return EHitDirection::Front;
	}

	// 내 캐릭터 Forward/Right도 수평면 투영
	const FVector Forward = FVector::VectorPlaneProject(GetActorForwardVector(), FVector::UpVector).GetSafeNormal();
	const FVector Right   = FVector::VectorPlaneProject(GetActorRightVector(),   FVector::UpVector).GetSafeNormal();

	const float DotF = FVector::DotProduct(ToAttacker, Forward);
	const float DotR = FVector::DotProduct(ToAttacker, Right);

	if (FMath::Abs(DotF) > FMath::Abs(DotR))
	{
		return DotF > 0.f ? EHitDirection::Front : EHitDirection::Back;
	}
	return DotR > 0.f ? EHitDirection::Right : EHitDirection::Left;
}

void AERNCharacterBase::Multicast_PlayHitReaction_Implementation(UAnimMontage* Montage)
{
	if (!Montage || !GetMesh()) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(Montage);
	}
}

void AERNCharacterBase::Multicast_PlayDeathMontage_Implementation()
{
	if (!DeathMontage || !GetMesh()) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(DeathMontage);
	}
}

void AERNCharacterBase::PlayCutsceneMontage(UAnimMontage* Montage)
{
	if (!Montage || !GetMesh())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(Montage);
	}
}

void AERNCharacterBase::OnDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	UE_LOG(LogTemp, Log, TEXT("%s has died."), *GetName());

	// 이동 비활성화
	GetCharacterMovement()->DisableMovement();

	// 충돌 비활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 사망 몽타주 재생 (모든 클라이언트 동기화)
	Multicast_PlayDeathMontage();

	// 자식 클래스에서 추가 사망 처리 구현 (이펙트, 드롭 등)
}

void AERNCharacterBase::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	
	if (!AbilitySystemComponent)
	{
		return;
	}
	
	// 공중 상태 태그 On/Off
	if (GetCharacterMovement()->MovementMode == MOVE_Falling)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(TAG_State_Movement_Falling, 1);
	}
	else
	{
		// 공중 상태 태그 Off
		AbilitySystemComponent->SetLooseGameplayTagCount(TAG_State_Movement_Falling, 0);
		// 벽 점프 사용상태 Off
		AbilitySystemComponent->SetLooseGameplayTagCount(TAG_State_Movement_WallJumpUsed, 0);
	}
}

void AERNCharacterBase::AddGold(const int32 Amount) const
{
	const int32 CurrentGold = static_cast<int32>(AttributeSet->GetGold());
	const int32 NewGold = CurrentGold + Amount;
	AttributeSet->SetGold(static_cast<float>(NewGold));
	
	UE_LOG(LogTemp, Warning, TEXT("PlayerController: %s, Gold: %d"), *GetNameSafe(Controller), static_cast<int32>(AttributeSet->GetGold()));
}

void AERNCharacterBase::Server_RequestEffectAndSound_Implementation(UNiagaraSystem* Effect, FVector EffectLocation,
	USoundBase* Sound, FVector SoundLocation)
{
	if (!HasAuthority())
	{
		return;
	}
	
	Multicast_PlayEffectAndSound(Effect, EffectLocation, Sound, SoundLocation);
}

void AERNCharacterBase::Multicast_PlayEffectAndSound_Implementation(UNiagaraSystem* Effect, FVector EffectLocation,
	USoundBase* Sound, FVector SoundLocation)
{
	if (!GetWorld())
	{
		return;
	}
	
	if (Effect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Effect, EffectLocation);
	}
	
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, SoundLocation);
	}
}
