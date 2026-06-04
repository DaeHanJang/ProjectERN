// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Weapons/ERNMeleeWeapon.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/DamageEvents.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Player/ERNPlayerController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "SWarningOrErrorBox.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNAttributeSet.h"

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
		Enemy->TryApplyStagger(StaggerPower);
	}

	// 히트 이펙트 - 모든 클라이언트에 멀티캐스트
	if (HitEffect)
	{
		Multicast_PlayHitEffect(SweepResult.ImpactPoint, SweepResult.ImpactNormal.Rotation());
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
}

void AERNMeleeWeapon::BeginAttackTrace()
{
	if (!HasAuthority())
	{
		return;
	}
	
	HitActors.Empty();
	PreviousTracePoints = GetCurrentTracePoints();
	bAttackTraceActive = true;
}

void AERNMeleeWeapon::TickAttackTrace()
{
	if (!HasAuthority() || !bAttackTraceActive || !GetWorld())
	{
		return;
	}

	const TArray<FVector> CurrentTracePoints =
		GetCurrentTracePoints();

	if (PreviousTracePoints.Num() != CurrentTracePoints.Num())
	{
		PreviousTracePoints = CurrentTracePoints;
		return;
	}

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(MeleeAttackTrace),
		false);

	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

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
				FCollisionShape::MakeSphere(TraceRadius),
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
				TraceRadius,
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

	PreviousTracePoints = CurrentTracePoints;
}

void AERNMeleeWeapon::EndAttackTrace()
{
	// 판정은 서버에서만 처리한다.
	if (!HasAuthority())
	{
		return;
	}

	bAttackTraceActive = false;

	// 다음 공격이 이전 프레임 위치를 이어받지 않도록 초기화한다.
	PreviousTracePoints.Empty();

	// 다음 공격에서는 같은 적을 다시 타격할 수 있어야 한다.
	HitActors.Empty();
}

TArray<FVector> AERNMeleeWeapon::GetCurrentTracePoints() const
{
	TArray<FVector> Points;

	if (!TraceStart || !TraceEnd)
	{
		return Points;
	}

	const FVector Start = TraceStart->GetComponentLocation();
	const FVector End = TraceEnd->GetComponentLocation();

	const int32 Count = FMath::Max(TraceSampleCount, 2);
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
	// 대미지 판정은 서버에서만 확정한다.
	if (!HasAuthority())
	{
		return;
	}

	AActor* HitActor = HitResult.GetActor();
	if (!IsValid(HitActor))
	{
		return;
	}
	
	// 무기 소유자 자신과 이미 적중한 대상은 제외한다.
	if (HitActor == GetOwner() || HitActors.Contains(HitActor))
	{
		return;
	}
	
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}
	
	AController* InstigatorController = OwnerCharacter->GetController();
	
	// 부활 공격 적용
	if (AProjectERNCharacter::TryApplyReviveHit(HitActor, InstigatorController))
	{
		HitActors.Add(HitActor);
		return;
	}
	
	// 현재 일반 공격은 적 캐릭터만 타격한다.
	AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(HitActor);
	if (!Enemy)
	{
		return;
	}
	
	// 같은 공격 유효 구간에서는 대상 하나당 한 번만 대미지를 준다.
	HitActors.Add(HitActor);

	UAbilitySystemComponent* ASC =UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerCharacter);

	float CharacterAttackPower = 0.f;
	float AttackPowerBonus = 0.f;

	if (ASC)
	{
		CharacterAttackPower =ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerAttribute());

		AttackPowerBonus = ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerBonusAttribute());
	}

	const float DamageToApply = LightAttackDamage + CharacterAttackPower + AttackPowerBonus;

	const float StaggerPower = LightAttackStaggerPower;

	HitActor->TakeDamage(
		DamageToApply,
		FDamageEvent(),
		InstigatorController,
		OwnerCharacter);

	Enemy->TryApplyStagger(StaggerPower);
	
	// Sweep 결과의 ImpactPoint를 사용하므로 기존 Overlap 방식보다
	// 실제 검 궤적에 가까운 위치에 이펙트를 재생할 수 있다.
	if (HitEffect)
	{
		const FVector ImpactPoint = FVector(HitResult.ImpactPoint);
		const FVector ImpactNormal = FVector(HitResult.ImpactNormal);

		const FVector EffectLocation = ImpactPoint.IsNearlyZero() ? HitActor->GetActorLocation() : ImpactPoint;
		const FRotator EffectRotation = ImpactNormal.IsNearlyZero() ? OwnerCharacter->GetActorRotation() : ImpactNormal.Rotation();

		Multicast_PlayHitEffect(EffectLocation, EffectRotation);
	}

	// 공격을 성공시킨 플레이어에게만 타격 확인 흔들림을 적용한다.
	if (HitConfirmShakeClass)
	{
		AERNPlayerController* PlayerController =
			Cast<AERNPlayerController>(OwnerCharacter->GetController());

		if (PlayerController)
		{
			PlayerController->Client_PlayCameraShake(HitConfirmShakeClass, HitConfirmShakeScale);
		}
	}
}