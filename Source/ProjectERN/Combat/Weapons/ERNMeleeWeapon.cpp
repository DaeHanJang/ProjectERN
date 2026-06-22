// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Weapons/ERNMeleeWeapon.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/DamageEvents.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Player/ERNPlayerController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNAttributeSet.h"
#include "Combat/ERNSkillDamageLibrary.h"
#include "Kismet/GameplayStatics.h"

AERNMeleeWeapon::AERNMeleeWeapon()
{
	HitboxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxComponent"));
	HitboxComponent->SetupAttachment(RootComponent);
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitboxComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	HitboxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitboxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	HitboxComponent->OnComponentBeginOverlap.AddDynamic(this, &AERNMeleeWeapon::OnHitboxOverlap);
	
	// 검날 트레이스 기준점
	TraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("TraceStart"));
	TraceStart->SetupAttachment(HitboxComponent);
	TraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("TraceEnd"));
	TraceEnd->SetupAttachment(HitboxComponent);
}

void AERNMeleeWeapon::BeginPlay()
{
	Super::BeginPlay();

	// 검날 연장 복원 기준점 캐시
	if (TraceEnd)
	{
		DefaultTraceEndRelLocation = TraceEnd->GetRelativeLocation();
	}
}

void AERNMeleeWeapon::EnableHitbox()
{
	HitActors.Empty();
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AERNMeleeWeapon::DisableHitbox()
{
	HitboxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitActors.Empty();
}

void AERNMeleeWeapon::OnHitboxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	if (!OtherActor || OtherActor == GetOwner() || HitActors.Contains(OtherActor))
	{
		return;
	}

	// 적 캐릭터에게만 데미지 적용
	if (!OtherActor->IsA<AERNEnemyCharacter>())
	{
		return;
	}

	HitActors.Add(OtherActor);

	// 데미지 적용
	AController* InstigatorController = GetOwner()
		? Cast<ACharacter>(GetOwner())->GetController()
		: nullptr;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerCharacter);

	/*
	// 태그로 강공 / 약공 구분
	const bool bIsHeavy = ASC && ASC->HasMatchingGameplayTag(TAG_Ability_Attack_Heavy);
	const float DamageToApply = bIsHeavy ? HeavyAttackDamage : LightAttackDamage;
	const float StaggerPower = bIsHeavy ? HeavyAttackStaggerPower : LightAttackStaggerPower;
	*/

	float CharacterAttackPower = 0.f;
	float AttackPowerBonus = 0.f;

	if (ASC)
	{
		CharacterAttackPower =
			ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerAttribute());

		AttackPowerBonus =
			ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerBonusAttribute());
	}

	const float DamageToApply = LightAttackDamage + CharacterAttackPower + AttackPowerBonus;
	const float StaggerPower = LightAttackStaggerPower;

	OtherActor->TakeDamage(DamageToApply, FDamageEvent(), InstigatorController, GetOwner());

	if (AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(OtherActor))
	{
		// 공격자(무기 소유자) 위치를 HitOrigin으로 전달 → 적이 4방향 경직
		const FVector HitOrigin = OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector(SweepResult.ImpactPoint);
		Enemy->TryApplyStagger(StaggerPower, HitOrigin);
	}

	// 히트 이펙트 - 모든 클라이언트에 멀티캐스트
	if (HitEffect)
	{
		FRotator Rotation = SweepResult.ImpactNormal.Rotation();
		Rotation.Pitch += FMath::FRandRange(-5.0f , 5.0f);
		Rotation.Yaw += FMath::FRandRange(-5.0f , 5.0f);
		Rotation.Roll += FMath::FRandRange(-5.0f , 5.0f);
		Multicast_PlayHitEffect(SweepResult.ImpactPoint, Rotation);
	}

	// 공격자에게 Hit Confirmation 카메라 흔들림 (Client RPC → 공격자 본인만 흔들림)
	if (HitConfirmShakeClass)
	{
		ACharacter* OwnerCharForShake = Cast<ACharacter>(GetOwner());
		if (OwnerCharForShake)
		{
			AERNPlayerController* PC = Cast<AERNPlayerController>(OwnerCharForShake->GetController());
			if (PC)
			{
				PC->Client_PlayCameraShake(HitConfirmShakeClass, HitConfirmShakeScale);
			}
		}
	}
}

void AERNMeleeWeapon::Multicast_PlayHitEffect_Implementation(FVector Location, FRotator Rotation)
{
	if (HitEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitEffect, Location, Rotation);
	}
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, Location, Rotation);
	}
}

void AERNMeleeWeapon::BeginAttackTrace()
{
	// 트레일은 시각효과이므로 권한과 무관하게 모든 클라이언트에서 켠다.
	StartTrail();

	// 판정은 "소유 클라"에서 수행한다. (서버에 복제된 클라-폰 몽타주는 깨지지만, 클라 본인 몽타주는 정상)
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	bAttackTraceActive = true;
	// HitActors/이전검선 초기화는 TickAttackTrace의 새-스윙 감지(staleness)에서 처리한다.
	// (노티가 매 프레임 토글되면 여기서 리셋 시 스윕이 "현재→현재" 길이 0이 되어 안 맞음)
}

void AERNMeleeWeapon::TickAttackTrace()
{
	// 트레일 끝점 갱신은 모든 클라이언트에서 수행한다.
	UpdateTrail();

	// 판정은 "소유 클라"에서만 (서버의 클라-폰 몽타주가 깨져도 클라 본인 몽타주는 정상)
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled() || !GetWorld() || !TraceStart || !TraceEnd)
	{
		return;
	}

	const FVector CurrentStart = TraceStart->GetComponentLocation();
	const FVector CurrentEnd = TraceEnd->GetComponentLocation();

	// 직전 틱이 오래 전이면 새 스윙 → 기준점/HitActors 초기화 후 이번 프레임은 스윕 생략.
	// (노티 Begin/End가 매 프레임 토글돼도 이전검선을 여기서만 관리해 스윕 길이를 보존)
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastTraceTickTime > 0.1f)
	{
		HitActors.Empty();
		PreviousTraceStart = CurrentStart;
		PreviousTraceEnd = CurrentEnd;
		LastTraceTickTime = Now;
		return;
	}
	LastTraceTickTime = Now;

	// 이전/현재 검선을 동일한 현재 count로 샘플 → 점 개수 항상 일치 (크기 불일치 SKIP 불필요)
	const TArray<FVector> PreviousTracePoints = SampleBladePoints(PreviousTraceStart, PreviousTraceEnd);
	const TArray<FVector> CurrentTracePoints = SampleBladePoints(CurrentStart, CurrentEnd);

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(MeleeAttackTrace),
		false);

	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	// 검날 연장 중에는 별도 반경 사용 (NotifyState에서 전달받은 값)
	const float EffectiveRadius = bBladeExtending ? BladeExtendRadius : TraceRadius;

	for (int32 Index = 0;
		 Index < CurrentTracePoints.Num();
		 ++Index)
	{
		const FVector& PreviousPoint =
			PreviousTracePoints[Index];

		const FVector& CurrentPoint =
			CurrentTracePoints[Index];

		TArray<FHitResult> HitResults;

		const bool bHasHit =
			GetWorld()->SweepMultiByObjectType(
				HitResults,
				PreviousPoint,
				CurrentPoint,
				FQuat::Identity,
				ObjectQuery,
				FCollisionShape::MakeSphere(EffectiveRadius),
				QueryParams);
		
		// 디버그용
		if (bDrawDebugTrace)
		{
			// 적중한 Sweep은 빨간색, 빗나간 Sweep은 초록색
			const FColor TraceColor = bHasHit ? FColor::Red : FColor::Green;

			// 이전 프레임과 현재 프레임 사이의 이동 경로
			DrawDebugLine(
				GetWorld(),
				PreviousPoint,
				CurrentPoint,
				TraceColor,
				false,
				DebugTraceDuration,
				0,
				DebugTraceThickness);

			// Sweep 반경 확인용 구체
			DrawDebugSphere(
				GetWorld(),
				CurrentPoint,
				EffectiveRadius,
				12,
				TraceColor,
				false,
				DebugTraceDuration);
		}

		for (const FHitResult& HitResult : HitResults)
		{
			HandleTraceHit(HitResult);
		}
	}
	
	// 디버그용
	if (bDrawDebugTrace)
	{
		// 현재 프레임에서 검날을 따라 배치된 샘플 지점을 연결
		for (int32 Index = 0; Index + 1 < CurrentTracePoints.Num(); ++Index)
		{
			DrawDebugLine(
				GetWorld(),
				CurrentTracePoints[Index],
				CurrentTracePoints[Index + 1],
				FColor::Cyan,
				false,
				DebugTraceDuration,
				0,
				DebugTraceThickness);
		}
	}

	PreviousTraceStart = CurrentStart;
	PreviousTraceEnd = CurrentEnd;
}

void AERNMeleeWeapon::EndAttackTrace()
{
	// 트레일은 모든 클라이언트에서 끈다.
	StopTrail();

	// 판정은 소유 클라에서 처리한다.
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	bAttackTraceActive = false;
	LastTraceTickTime = 0.f;

	// HitActors 초기화는 다음 스윙의 staleness 감지에서 처리한다.
	// (노티가 매 프레임 토글되면 여기서 매 프레임 비워 같은 적을 중복 타격하게 됨)
}

void AERNMeleeWeapon::BeginBladeExtend(float Multiplier, float GrowDuration, float ExtendRadius)
{
	BladeTargetMultiplier = Multiplier;
	BladeGrowDuration = GrowDuration;
	BladeExtendRadius = ExtendRadius;
	BladeElapsed = 0.f;
	bBladeExtending = true;

	// 즉시 모드(GrowDuration <= 0)는 바로 목표 배수, 점진 모드는 1배에서 시작
	ApplyBladeMultiplier(GrowDuration > 0.f ? 1.f : Multiplier);
}

void AERNMeleeWeapon::TickBladeExtend(float DeltaTime)
{
	// 즉시 모드이거나 성장 중이 아니면 아무것도 안 함 (이미 목표값 적용됨)
	if (!bBladeExtending || BladeGrowDuration <= 0.f)
	{
		return;
	}

	BladeElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(BladeElapsed / BladeGrowDuration, 0.f, 1.f);
	ApplyBladeMultiplier(FMath::Lerp(1.f, BladeTargetMultiplier, Alpha));
}

void AERNMeleeWeapon::EndBladeExtend()
{
	bBladeExtending = false;
	BladeTargetMultiplier = 1.f;	// 점 개수 원복

	// 기본 길이로 복원
	if (TraceEnd)
	{
		TraceEnd->SetRelativeLocation(DefaultTraceEndRelLocation);
	}
}

void AERNMeleeWeapon::ApplyBladeMultiplier(float Multiplier)
{
	if (!TraceStart || !TraceEnd)
	{
		return;
	}

	// TraceStart는 고정, TraceEnd만 Start→기본End 방향으로 Multiplier배 밀어냄
	const FVector StartRel = TraceStart->GetRelativeLocation();
	const FVector NewEndRel = StartRel + (DefaultTraceEndRelLocation - StartRel) * Multiplier;
	TraceEnd->SetRelativeLocation(NewEndRel);
}

TArray<FVector> AERNMeleeWeapon::GetCurrentTracePoints() const
{
	if (!TraceStart || !TraceEnd)
	{
		return TArray<FVector>();
	}

	return SampleBladePoints(TraceStart->GetComponentLocation(), TraceEnd->GetComponentLocation());
}

TArray<FVector> AERNMeleeWeapon::SampleBladePoints(const FVector& Start, const FVector& End) const
{
	TArray<FVector> Points;

	// 검날 연장 시 점 개수도 배수만큼 늘려 점 간격(=빈틈)을 평소와 동일하게 유지
	const int32 Count = FMath::Max(2, FMath::CeilToInt(TraceSampleCount * BladeTargetMultiplier));
	Points.Reserve(Count);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(Count - 1);

		Points.Add(FMath::Lerp(Start, End, Alpha));
	}

	return Points;
}

void AERNMeleeWeapon::HandleTraceHit(const FHitResult& HitResult)
{
	// 이 함수는 이제 "소유 클라"에서 호출된다 (TickAttackTrace가 소유 클라 게이트).
	// 적중만 확정하고 실제 데미지는 서버 RPC로 위임한다.
	AActor* HitActor = HitResult.GetActor();
	if (!IsValid(HitActor))
	{
		return;
	}

	// 무기 소유자 자신과 이미 이번 스윙에 적중한 대상은 제외 (클라에서 중복 방지)
	if (HitActor == GetOwner() || HitActors.Contains(HitActor))
	{
		return;
	}

	HitActors.Add(HitActor);

	// 서버에 적중 보고 → 서버가 적/부활 판정 후 데미지 적용. override 여부는 클라 자신의 상태 기준.
	Server_ApplyMeleeHit(HitActor, bUseBladeDamageOverride);
}

void AERNMeleeWeapon::Server_ApplyMeleeHit_Implementation(AActor* Target, bool bOverride)
{
	if (!IsValid(Target) || Target == GetOwner())
	{
		return;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	AController* InstigatorController = OwnerCharacter->GetController();

	// 평소엔 무기 기본 스펙(0 + AP*1 + APBonus + Weapon*1), 연장 스테이트면 NotifyState가 세팅한 오버라이드 스펙
	const FERNSkillDamageData DefaultData(
		/*BaseDamage*/ 0.f,
		/*AttackPowerScale*/ 1.f,
		/*WeaponDamageScale*/ 1.f,
		/*StaggerPower*/ LightAttackStaggerPower);
	const FERNSkillDamageData& DataToUse = bOverride ? BladeDamageOverride : DefaultData;

	// 데미지/부활/적 판정/경직을 라이브러리로 통합 처리
	const EERNSkillHitResult Result = UERNSkillDamageLibrary::ApplySkillHit(
		Target,
		GetOwner(),
		InstigatorController,
		DataToUse,
		OwnerCharacter->GetActorLocation());

	// 적 타격(Damage)일 때만 히트 이펙트/카메라 셰이크 (부활 히트 제외)
	if (Result == EERNSkillHitResult::Damage)
	{
		if (HitEffect)
		{
			Multicast_PlayHitEffect(Target->GetActorLocation(), OwnerCharacter->GetActorRotation());
		}

		if (HitConfirmShakeClass)
		{
			if (AERNPlayerController* PlayerController = Cast<AERNPlayerController>(OwnerCharacter->GetController()))
			{
				PlayerController->Client_PlayCameraShake(HitConfirmShakeClass, HitConfirmShakeScale);
			}
		}
	}
}

void AERNMeleeWeapon::BeginBladeDamageOverride(const FERNSkillDamageData& Data)
{
	BladeDamageOverride = Data;
	bUseBladeDamageOverride = true;
}

void AERNMeleeWeapon::EndBladeDamageOverride()
{
	bUseBladeDamageOverride = false;
}

void AERNMeleeWeapon::StartTrail()
{
	// 연장 구간이면 오버라이드 트레일, 아니면 기본 트레일 사용
	UNiagaraSystem* SystemToUse = ActiveTrailOverride ? ActiveTrailOverride.Get() : TrailEffect.Get();
	if (!SystemToUse || !TraceStart)
	{
		return;
	}

	// TraceStart에 1회 부착 후 재사용
	if (!TrailComp)
	{
		TrailComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			SystemToUse,
			TraceStart,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false /*bAutoDestroy*/,
			false /*bAutoActivate*/);
	}
	else if (TrailComp->GetAsset() != SystemToUse)
	{
		// 재사용 중인 컴포넌트의 에셋을 교체
		TrailComp->SetAsset(SystemToUse);
	}

	if (TrailComp)
	{
		TrailComp->Activate(true /*bReset*/);
		bTrailRunning = true;
	}
}

void AERNMeleeWeapon::BeginBladeTrailSwap(UNiagaraSystem* NewTrail)
{
	ActiveTrailOverride = NewTrail;	// nullptr이면 기본 트레일 유지

	// 트레일이 진행 중이면 즉시 교체 반영 (노티 시작 순서와 무관하게 동작)
	if (bTrailRunning)
	{
		StartTrail();
	}
}

void AERNMeleeWeapon::EndBladeTrailSwap()
{
	ActiveTrailOverride = nullptr;

	// MeleeTrace가 아직 진행 중일 때만 기본 트레일로 즉시 복귀.
	// 이미 꺼진 상태면(StopTrail 호출됨) 절대 다시 켜지 않는다 (잔류 입자 IsActive 오판 방지).
	if (bTrailRunning)
	{
		StartTrail();
	}
}

void AERNMeleeWeapon::Multicast_SetBladeTrail_Implementation(UNiagaraSystem* Trail)
{
	// 서버가 쏜 교체를 모든 클라(시뮬프록시 포함)에서 동일하게 적용
	if (Trail)
	{
		BeginBladeTrailSwap(Trail);
	}
	else
	{
		EndBladeTrailSwap();
	}
}

void AERNMeleeWeapon::UpdateTrail()
{
	// 검 끝점(TraceEnd)을 나이아가라 User 파라미터로 매 틱 전달
	if (TrailComp && TraceEnd)
	{
		TrailComp->SetVariableVec3(TrailTipParamName, TraceEnd->GetComponentLocation());
	}
}

void AERNMeleeWeapon::StopTrail()
{
	bTrailRunning = false;	// 논리적으로 꺼짐 (재활성 판단 기준)

	if (TrailComp)
	{
		// Deactivate
		TrailComp->Deactivate();
	}
}