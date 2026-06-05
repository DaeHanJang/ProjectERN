// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Player/ProjectERNCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameplayAbilitySpec.h"
#include "InputActionValue.h"
#include "ProjectERN.h"
#include "AbilitySystemComponent.h"
#include "ERNPlayerController.h"
#include "ERNPlayerStatusTable.h"
#include "ERNSkillNiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/PointLightComponent.h"
#include "Character/Player/ERNPlayerState.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Components/SphereComponent.h"
#include "Inventory/Components/ERNInventoryComponent.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "GAS/ERNGameplayTags.h"
#include "GAS/Abilities/ERNGA_LightAttack.h"
#include "Input/ERNInputComponent.h"
#include "Interfaces/IInteractable.h"
#include "Net/UnrealNetwork.h"
#include "Shop/Components/ERNShopComponent.h"
#include "Enhancement/Components/ERNUpgradeComponent.h"
#include "GAS/ERNAttributeSet.h"
#include "Camera/CameraShakeBase.h"
#include "GAS/Abilities/WeaponSkill/ERNGA_WeaponSkill_Channeling.h"
#include "Actors/Intro/ERNIntroBird.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/ERNDownedComponent.h"
#include "Components/ERNLockOnComponent.h"
#include "Components/PostProcessComponent.h"
#include "Inventory/Item/ERNItemActor.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

void AProjectERNCharacter::TryApplyStagger(float IncomingStaggerPower, const FVector& HitOrigin)
{
	// 살아 있는 상태가 아니라면 경직 면역
	if (!IsAlive())
	{
		return;
	}

	Super::TryApplyStagger(IncomingStaggerPower, HitOrigin);

	if (!AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	// 이미 사망 처리됐으면 경직/히트리액션 무시 (사망 몽타주 보존)
	if (bIsDead)
	{
		return;
	}

	// 무적이면 카메라 흔들림 무시
	if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Immunity_Damage))
	{
		return;
	}

	// 최소 경직을 위한 데미지 DamageShakeThresholdSmall 이상일 때만 흔들림
	if (IncomingStaggerPower >= DamageShakeThresholdSmall && HasAuthority())
	{
		// 경직도에 따른 카메라 흔들림 분류
		TSubclassOf<UCameraShakeBase> ShakeToPlay = nullptr;
		if (IncomingStaggerPower < DamageShakeThresholdMedium)
		{
			ShakeToPlay = TakeDamageShakeClass_Small;
		}
		else if (IncomingStaggerPower < DamageShakeThresholdBig)
		{
			ShakeToPlay = TakeDamageShakeClass_Medium;
		}
		else
		{
			ShakeToPlay = TakeDamageShakeClass_Big;
		}

		if (ShakeToPlay)
		{
			AERNPlayerController* PC = Cast<AERNPlayerController>(GetController());
			if (PC)
			{
				PC->Client_PlayCameraShake(ShakeToPlay, 1.f);
			}
		}
	}
}

AProjectERNCharacter::AProjectERNCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 100.f);
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Create Inventory Component
	InventoryComponent = CreateDefaultSubobject<UERNInventoryComponent>(TEXT("InventoryComponent"));

	// Create Equipment Component
	EquipmentComponent = CreateDefaultSubobject<UERNEquipmentComponent>(TEXT("EquipmentComponent"));

	// Create Shop Component
	ShopComponent = CreateDefaultSubobject<UERNShopComponent>(TEXT("ShopComponent"));

	// Create Upgrade Component
	UpgradeComponent = CreateDefaultSubobject<UERNUpgradeComponent>(TEXT("UpgradeComponent"));

	// Create Interaction Detection Component
	InteractionDetector = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionDetector"));
	InteractionDetector->SetupAttachment(GetRootComponent());
	InteractionDetector->InitSphereRadius(150.0f);
	InteractionDetector->SetCollisionProfileName(TEXT("OverlapAll"));
	InteractionDetector->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	InteractionDetector->SetGenerateOverlapEvents(true);

	// Create Skill Niagara Component
	SkillNiagaraComponent = CreateDefaultSubobject<UERNSkillNiagaraComponent>(TEXT("SkillNiagaraComponent"));

	// Head Light
	HeadLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("HeadLight"));
	HeadLight->SetupAttachment(GetCapsuleComponent());
	HeadLight->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	HeadLight->SetIntensity(5000.f);
	HeadLight->SetAttenuationRadius(800.f);
	HeadLight->SetCastShadows(false);
	HeadLight->SetVisibility(false);

	// Head Light Niagara
	HeadLightFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HeadLightFX"));
	HeadLightFX->SetupAttachment(GetCapsuleComponent());
	HeadLightFX->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	HeadLightFX->bAutoActivate = false;

	// NightRainZone 자기장 밤의비
	NightRainPostProcessComponent = CreateDefaultSubobject<
		UPostProcessComponent>(TEXT("NightRainPostProcessComponent"));
	NightRainPostProcessComponent->SetupAttachment(FollowCamera);
	NightRainPostProcessComponent->bEnabled = true;
	NightRainPostProcessComponent->bUnbound = true;
	NightRainPostProcessComponent->BlendWeight = 0.f;
	NightRainPostProcessComponent->Priority = 100.f;

	// Create LockOn Component
	LockOnComponent = CreateDefaultSubobject<UERNLockOnComponent>(TEXT("LockOnComponent"));

	// Create LockOn Detection Component
	LockOnDetector = CreateDefaultSubobject<USphereComponent>(TEXT("LockOnDetector"));
	LockOnDetector->SetupAttachment(GetRootComponent());

	// Create Downed Component
	DownedComponent = CreateDefaultSubobject<UERNDownedComponent>(TEXT("DownedComponent"));

	// GAS 컴포넌트는 부모 클래스(ERNCharacterBase)에서 생성됨

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// 기본값 설정
	CharacterType = ECharacterType::Warrior;

	// 캐릭터 겹쳤을 때 카메라 조정 방지
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

void AProjectERNCharacter::InitStatus()
{
	if (!HasAuthority() || !StatusCurveTable)
	{
		return;
	}

	const FERNPlayerStatusTable* Row = StatusCurveTable->FindRow<FERNPlayerStatusTable>(
		FName("1"), TEXT("StatusCurveContext"));
	if (!Row)
	{
		return;
	}

	AttributeSet->SetMaxHealth(Row->MaxHealth);
	AttributeSet->SetHealth(Row->MaxHealth);
	AttributeSet->SetMaxMana(Row->MaxMana);
	AttributeSet->SetMana(Row->MaxMana);
	AttributeSet->SetManaRegenRate(Row->ManaRegenRate);
	AttributeSet->SetMaxStamina(Row->MaxStamina);
	AttributeSet->SetStamina(Row->MaxStamina);
	AttributeSet->SetStaminaRegenRate(Row->StaminaRegenRate);
	AttributeSet->SetAttackPower(Row->AttackPower);
	AttributeSet->SetDefense(Row->Defense);
	AttributeSet->SetStaggerResistance(Row->StaggerResistance);
	AttributeSet->SetDownResistance(Row->DownResistance);
}

void AProjectERNCharacter::InitStatusForLevel(int32 Level)
{
	if (!HasAuthority() || !StatusCurveTable || !AttributeSet)
	{
		return;
	}

	const FERNPlayerStatusTable* Row = StatusCurveTable->FindRow<FERNPlayerStatusTable>(
		FName(FString::FromInt(Level)), TEXT("InitStatusForLevel"));
	if (!Row)
	{
		return;
	}

	AttributeSet->SetMaxHealth(Row->MaxHealth);
	AttributeSet->SetHealth(Row->MaxHealth);
	AttributeSet->SetMaxMana(Row->MaxMana);
	AttributeSet->SetMana(Row->MaxMana);
	AttributeSet->SetManaRegenRate(Row->ManaRegenRate);
	AttributeSet->SetMaxStamina(Row->MaxStamina);
	AttributeSet->SetStamina(Row->MaxStamina);
	AttributeSet->SetStaminaRegenRate(Row->StaminaRegenRate);
	AttributeSet->SetAttackPower(Row->AttackPower);
	AttributeSet->SetDefense(Row->Defense);
	AttributeSet->SetStaggerResistance(Row->StaggerResistance);
	AttributeSet->SetDownResistance(Row->DownResistance);
	AttributeSet->SetLevel(Level);
}

void AProjectERNCharacter::ApplyRunSnapshot()
{
	if (!HasAuthority())
	{
		return;
	}

	AERNPlayerState* PS = GetPlayerState<AERNPlayerState>();
	UE_LOG(LogTemp, Warning, TEXT("[Snapshot] APPLY called. PS=%s hasSnapshot=%d"),
	       PS ? *PS->GetPlayerName() : TEXT("NULL"), PS ? PS->bHasSnapshot : 0);

	if (!PS || !PS->bHasSnapshot)
	{
		return; // 스냅샷 없으면 기본값 유지 = 초기화
	}

	UE_LOG(LogTemp, Warning, TEXT("[Snapshot] APPLY %s: Level=%d Gold=%d InvItems=%d"),
	       *PS->GetPlayerName(), PS->SavedLevel, PS->SavedGold, PS->SavedInventory.Num());

	// 레벨 기반 스탯 재적용 (+SetLevel) → 그 위에 골드 복원
	InitStatusForLevel(PS->SavedLevel);
	if (AttributeSet)
	{
		AttributeSet->SetGold(PS->SavedGold);
		AttributeSet->SetMaxFlaskQuantity(PS->SavedMaxFlaskQuantity);
		AttributeSet->SetFlaskQuantity(PS->SavedFlaskQuantity);
	}

	// 인벤토리 복원 (강화 수치 포함) + 리슨서버 UI 갱신
	if (InventoryComponent)
	{
		FInventoryList& Inv = InventoryComponent->GetInventory();
		Inv.RestoreFrom(PS->SavedInventory, PS->SavedInventory.Num());
		for (const FInventoryItemEntry& E : Inv.GetItems())
		{
			InventoryComponent->OnInventorySlotChanged.Broadcast(E);
		}
	}

	// 장착 무기 복원 (강화 수치 포함)
	if (EquipmentComponent && PS->SavedWeaponState.IsValid())
	{
		EquipmentComponent->Server_EquipWeaponFromState(PS->SavedWeaponState);
	}

	// 장착 소모품 복원 (종류 + 수량)
	if (EquipmentComponent && PS->SavedConsumableState.IsValid())
	{
		EquipmentComponent->Server_EquipConsumableFromState(PS->SavedConsumableState);
	}
}

void AProjectERNCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 스폰 직후 즉시 OutOfCombat - 적이 감지하면 NotifyDetectedBy로 해제됨
	if (HasAuthority())
	{
		EnterOutOfCombat();
	}
	
	// 부활 완료 바인딩
	if (HasAuthority() && DownedComponent)
	{
		DownedComponent->OnReviveGaugeDepleted.AddUObject(this, &AProjectERNCharacter::CompleteRevive);
	}
	
	if (LockOnComponent && LockOnDetector && FollowCamera)
	{
		LockOnComponent->Initialize(LockOnDetector, FollowCamera);
	}

	// 머리 조명 Niagara 에셋 적용 + 현재 상태 반영
	if (HeadLightFX && HeadLightFXSystem)
	{
		HeadLightFX->SetAsset(HeadLightFXSystem);
	}
	ApplyHeadLightState();

	BindMovementSpeedTagEvents();
	UpdateMovementSpeed();
}

void AProjectERNCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 서버에서만 실행됨
	if (AERNPlayerState* PS = GetPlayerState<AERNPlayerState>())
	{
		// PlayerState의 CharacterType을 캐릭터에 적용 (맵 이동 시 유지)
		// PlayerState가 None이 아니면 그대로 사용, None이면 캐릭터 기본값 사용
		if (PS->CharacterType != ECharacterType::None)
		{
			CharacterType = PS->CharacterType;
		}
		else
		{
			// PlayerState가 None이면 캐릭터의 기본값을 PlayerState에 설정
			PS->CharacterType = CharacterType;
		}
	}

	if (HasAuthority())
	{
		// 재생 GE적용
		ApplyPlayerRegenEffects();
	}

	// GAS 초기화는 부모 클래스에서 처리

	// InteractionDetector 감지 시작
	if (!GetWorldTimerManager().IsTimerActive(DetectionTimerHandle))
	{
		GetWorldTimerManager().SetTimer(DetectionTimerHandle, this, &AProjectERNCharacter::UpdateInteractionDetector,
		                                0.2f, true, 0.0f);
	}
}

void AProjectERNCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	// InteractionDetector 감지 시작
	if (!GetWorldTimerManager().IsTimerActive(DetectionTimerHandle))
	{
		GetWorldTimerManager().SetTimer(DetectionTimerHandle, this, &AProjectERNCharacter::UpdateInteractionDetector,
		                                0.2f, true, 0.0f);
	}
}

void AProjectERNCharacter::UpdateInteractionDetector()
{
	// 감지는 클라이언트에서 하고 상호작용을 서버에 요청
	if (!InteractionDetector || !IsLocallyControlled())
	{
		return;
	}

	AERNPlayerController* ERNController = Cast<AERNPlayerController>(GetController());
	if (!ERNController)
	{
		return;
	}
	
	// 죽었다면 상호작용 불가
	if (!IsAlive())
	{
		AActor* CurrentInteractable = ERNController->GetCurrentInteractable();

		if (IsValid(CurrentInteractable) && CurrentInteractable->Implements<UInteractable>())
		{
			IInteractable::Execute_EndInteract(CurrentInteractable, ERNController);
		}

		ERNController->ClearCurrentInteractable();
		return;
	}

	// 상호작용 감지 콜리전과 겹쳐진 액터 수집
	TArray<AActor*> OverlappingActors;
	InteractionDetector->GetOverlappingActors(OverlappingActors);

	float ClosestDistSq = MAX_FLT;
	AActor* ClosestActor = nullptr;

	// 감지된 액터를 순회하면서 상호작용 가능 액터인 경우 가장 가까운 액터를 선정
	for (AActor* Actor : OverlappingActors)
	{
		if (!IsValid(Actor) || !Actor->Implements<UInteractable>())
		{
			continue;
		}
		
		if (!IInteractable::Execute_CanInteract(Actor))
		{
			continue;
		}

		if (const AERNItemActor* ItemActor = Cast<AERNItemActor>(Actor))
		{
			if (!ItemActor->CanBeInteractedBy(ERNController))
			{
				continue;
			}
		}

		const float DistSq = this->GetSquaredDistanceTo(Actor);

		if (ClosestDistSq > DistSq)
		{
			ClosestDistSq = DistSq;
			ClosestActor = Actor;
		}
	}

	AActor* CurrentInteractable = ERNController->GetCurrentInteractable();
	// 대상이 바뀐 경우 기존 대상 종료
	if (CurrentInteractable != ClosestActor)
	{
		if (IsValid(CurrentInteractable) && CurrentInteractable->Implements<UInteractable>())
		{
			IInteractable::Execute_EndInteract(CurrentInteractable, ERNController);
		}

		ERNController->ClearCurrentInteractable();

		// 현재 상효작용 가능 액터가 존재할 경우 대상이 바뀐 경우 새로 선정
		if (ClosestActor)
		{
			ERNController->SetCurrentInteractable(ClosestActor);
			IInteractable::Execute_ActivateInteract(ClosestActor);
		}
	}
}

void AProjectERNCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 새 매달림 카메라 보간 (소유 클라만 — FOV/ArmLength FInterpTo + 95% 도달 시 스냅)
	if (bIsCameraTransitioning && IsLocallyControlled() && FollowCamera && CameraBoom)
	{
		const float NewArm = FMath::FInterpTo(CameraBoom->TargetArmLength,
		                                      CameraTransitionTargetArmLength, DeltaSeconds, CameraInterpSpeed * 20);
		const float NewFOV = FMath::FInterpTo(FollowCamera->FieldOfView,
		                                      CameraTransitionTargetFOV, DeltaSeconds, CameraInterpSpeed * 20);

		CameraBoom->TargetArmLength = NewArm;
		FollowCamera->SetFieldOfView(NewFOV);

		const float ArmRange = CameraTransitionTargetArmLength - CameraTransitionStartArmLength;
		const float FOVRange = CameraTransitionTargetFOV - CameraTransitionStartFOV;
		const float ArmProgress = FMath::IsNearlyZero(ArmRange)
			                          ? 1.f
			                          : (NewArm - CameraTransitionStartArmLength) / ArmRange;
		const float FOVProgress = FMath::IsNearlyZero(FOVRange)
			                          ? 1.f
			                          : (NewFOV - CameraTransitionStartFOV) / FOVRange;
		float Progress = FMath::Min(ArmProgress, FOVProgress);

		// 컨트롤 회전 보간 (부착 시에만 활성 — release 시엔 자유 회전 유지)
		if (bIsControlRotationTransitioning)
		{
			if (AController* Ctrl = GetController())
			{
				const FRotator CurRot = Ctrl->GetControlRotation();
				const FRotator NewRot = FMath::RInterpTo(CurRot,
				                                         CameraTransitionTargetControlRotation, DeltaSeconds,
				                                         CameraInterpSpeed * 20);
				Ctrl->SetControlRotation(NewRot);

				const float StartAngle = CameraTransitionStartControlRotation.Quaternion()
				                                                             .AngularDistance(
					                                                             CameraTransitionTargetControlRotation.
					                                                             Quaternion());
				const float CurAngle = NewRot.Quaternion()
				                             .AngularDistance(CameraTransitionTargetControlRotation.Quaternion());
				const float RotProgress = (StartAngle > KINDA_SMALL_NUMBER)
					                          ? 1.f - (CurAngle / StartAngle)
					                          : 1.f;
				Progress = FMath::Min(Progress, RotProgress);
			}
		}

		if (Progress >= CameraSnapThreshold)
		{
			CameraBoom->TargetArmLength = CameraTransitionTargetArmLength;
			FollowCamera->SetFieldOfView(CameraTransitionTargetFOV);

			if (bIsControlRotationTransitioning)
			{
				if (AController* Ctrl = GetController())
				{
					Ctrl->SetControlRotation(CameraTransitionTargetControlRotation);
				}
				bIsControlRotationTransitioning = false;
			}

			bIsCameraTransitioning = false;
		}
	}

	// 매달림 전체 구간 카메라 lag 활성 (소유 클라 한정, 상태 전환 시에만 SpringArm 세팅)
	// Ascend → Flight 전환 시 lag가 꺼져 카메라가 스냅되는 문제 방지
	{
		const bool bShouldUseAscentLag = IsLocallyControlled()
			&& bIsHangingFromBird
			&& AttachedBird;

		if (bShouldUseAscentLag != bAscentCameraLagActive && CameraBoom)
		{
			CameraBoom->bEnableCameraLag = bShouldUseAscentLag;
			if (bShouldUseAscentLag)
			{
				CameraBoom->CameraLagSpeed = AscentCameraLagSpeed;
				CameraBoom->CameraLagMaxDistance = AscentCameraLagMaxDistance;
			}
			bAscentCameraLagActive = bShouldUseAscentLag;
		}
	}

	// 매달림 중: 매 frame 새의 HangPoint World로 위치 강제 + 새 진행 방향으로 회전 동기화 (모든 머신)
	// → attach 없이 deterministic 추적 → RepMovement/BasedMovement/NetSmoothing 경로 우회
	if (bIsHangingFromBird && AttachedBird)
	{
		if (USceneComponent* HP = AttachedBird->GetHangPoint())
		{
			FRotator BirdFacing = AttachedBird->GetActorForwardVector().Rotation();
			BirdFacing.Pitch = 0.f;
			BirdFacing.Roll = 0.f;

			SetActorLocationAndRotation(
				HP->GetComponentLocation(),
				BirdFacing,
				false, nullptr, ETeleportType::TeleportPhysics
			);
		}
		return;
	}

	const bool bIsSprinting =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Sprinting);

	const bool bIsAttacking =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking);

	// Sprint 중에는 LockOn 강제 회전을 끈다.
	// const bool bShouldRotateForLockOn = bIsLockOn && !bIsSprinting && !bIsAttacking;

	if (bIsLockOn)
	{
		if (LockOnComponent && LockOnComponent->IsLockOnActive())
		{
			DesiredActorRotation = LockOnComponent->GetDesiredRotationToTarget();
			DesiredLockOnYaw = DesiredActorRotation.Yaw;

			if (!HasAuthority() && IsLocallyControlled())
			{
				Server_UpdateLockOnYaw(DesiredLockOnYaw);
			}
		}
		else
		{
			ApplyLockOnState(false, GetActorRotation());
			return;
		}

		// 캐릭터 몸 회전은 질주, 공격 중에는 적용하지 않음
		if (!bIsSprinting)
		{
			if (HasAuthority() || IsLocallyControlled())
			{
				InterpActorRotation(DeltaSeconds);
			}
		}

		// 카메라 락온 타겟 바라보게 유지
		if (IsLocallyControlled() && bUseCameraYawOnLockOn)
		{
			const AActor* LockOnTarget = LockOnComponent->GetCurrentTarget();
			AController* Ctrl = GetController();

			if (IsValid(LockOnTarget) && Ctrl && FollowCamera)
			{
				const FVector TargetFocusLocation = LockOnTarget->GetActorLocation();
				const FVector CameraLocation = FollowCamera->GetComponentLocation();
				// 현재 카메라 위치에서 타겟 액터 중심까지의 방향의 회전값
				FRotator TargetCameraRotation = (TargetFocusLocation - CameraLocation).Rotation();
				// 가까운 거리에서는 Pitch 갱신 약하게
				const float Dist2D = FVector::Dist2D(CameraLocation, TargetFocusLocation);
				if (Dist2D < 300.0f)
				{
					TargetCameraRotation.Pitch = Ctrl->GetControlRotation().Pitch;
				}
				else
				{
					// 먼 타겟일수록 카메라가 수평이 되어 캐릭터-적이 겹치는 문제 보정:
					// 거리에 따라 상한 피치를 낮춰 카메라가 강제로 더 아래를 보게 함
					const float MaxPitch = FMath::GetMappedRangeValueClamped(
						FVector2D(LockOnPitchNearDistance, LockOnPitchFarDistance),
						FVector2D(LockOnPitchMaxNear, LockOnPitchMaxFar),
						Dist2D);
					TargetCameraRotation.Pitch = FMath::Clamp(TargetCameraRotation.Pitch, LockOnPitchMin, MaxPitch);
				}
				TargetCameraRotation.Roll = 0.0f;

				// 현재 컨트롤러 회전에서 타겟 액터 방향 회전으로 보간
				const FRotator NewControlRotation = FMath::RInterpTo(Ctrl->GetControlRotation(), TargetCameraRotation,
				                                                     DeltaSeconds, RotationInterpSpeed);

				// 보간된 회전을 컨트롤러에 적용
				Ctrl->SetControlRotation(NewControlRotation);
			}
		}

		return;
	}

	// 공격/콤보 회전: 로컬 + 서버
	if (bHasPendingActorRotation)
	{
		if (!HasAuthority() && !IsLocallyControlled())
		{
			return;
		}

		InterpActorRotation(DeltaSeconds);
	}
}

void AProjectERNCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UERNInputComponent* InputComp = Cast<UERNInputComponent>(PlayerInputComponent);
	if (!InputComp)
	{
		UE_LOG(LogProjectERN, Error, TEXT("%s failed to find ERNInputComponent."), *GetNameSafe(this));
		return;
	}

	if (!InputConfig)
	{
		UE_LOG(LogProjectERN, Warning, TEXT("%s has no InputConfig."), *GetNameSafe(this));
		return;
	}

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Move,
		ETriggerEvent::Triggered,
		this,
		&AProjectERNCharacter::Move);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Move,
		ETriggerEvent::Completed,
		this,
		&AProjectERNCharacter::MoveEnd);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Look,
		ETriggerEvent::Triggered,
		this,
		&AProjectERNCharacter::Look);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_MouseLook,
		ETriggerEvent::Triggered,
		this,
		&AProjectERNCharacter::Look);

	// Jump
	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Jump,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::DoJumpStart);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Jump,
		ETriggerEvent::Completed,
		this,
		&AProjectERNCharacter::DoJumpEnd);

	// Tap Shift -> Roll
	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Roll,
		ETriggerEvent::Triggered,
		this,
		&AProjectERNCharacter::Roll);

	// Attack
	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_LightAttack,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::LightAttack);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_HeavyAttack,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::HeavyAttack);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_LockOn,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::LockOn);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Sprint,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::ToggleSprint);

	// Flask
	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Flask,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::DrinkFlask);

	// Consumable
	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Consumable,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::UseConsumable);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_NormalSkill,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::NormalSkill);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_UltimateSkill,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::UltimateSkill);

	// Head Light 토글
	if (ToggleLightAction)
	{
		InputComp->BindAction(ToggleLightAction, ETriggerEvent::Started,
		                      this, &AProjectERNCharacter::ToggleLight);
	}
}

void AProjectERNCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// 움직일 수 없는 상태면
	if (LifeState == EERNPlayerLifeState::Collapsing ||
		LifeState == EERNPlayerLifeState::Reviving ||
		LifeState == EERNPlayerLifeState::Respawning)
	{
		CachedMoveInput = FVector2D::ZeroVector;
		return;
	}

	// 인트로 매달림 중: 좌우 입력만 새 조향에 전달
	if (bIsHangingFromBird)
	{
		if (AttachedBird)
		{
			AttachedBird->Server_SetSteeringInput(MovementVector.X);
		}
		return;
	}

	// 입력 값 갱신(Sprint에 사용)
	CachedMoveInput = MovementVector;

	// 해당 태그가 있으면 움직이지 못함
	const bool bIsAttacking =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking);

	const bool bIsLanding =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Landing);

	const bool bIsGettingHit =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Stagger);

	const bool bIsUsingSkill =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_UsingSkill);

	if ((bIsAttacking && !bCanMoveWhileAttacking) || bIsLanding || bIsGettingHit || bIsUsingSkill)
	{
		return;
	}

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AProjectERNCharacter::MoveEnd()
{
	// 인트로 매달림 중: 키 뗐을 때 새 입력을 0으로 리셋
	if (bIsHangingFromBird)
	{
		if (AttachedBird)
		{
			AttachedBird->Server_SetSteeringInput(0.f);
		}
		return;
	}

	CachedMoveInput = FVector2D::ZeroVector;
	StopSprint();
}

void AProjectERNCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the inputS
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AProjectERNCharacter::Roll()
{
	if (bIsHangingFromBird) return;

	if (!AbilitySystemComponent)
	{
		return;
	}

	// 방향 계산
	const FVector RollDirection = GetRollWorldDirection();

	if (HasAuthority())
	{
		// 구르기 방향 결정
		PendingRollDirection = RollDirection;

		// 구르기 태그를 가진 어빌리티 실행 시도
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Roll));
	}
	else
	{
		Server_RequestRoll(RollDirection);
	}
}

void AProjectERNCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AProjectERNCharacter::DoLook(float Yaw, float Pitch)
{
	if (bIsLockOn && LockOnComponent && LockOnComponent->IsLockOnActive())
	{
		return;
	}

	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AProjectERNCharacter::DoJumpStart()
{
	// 새에 매달려 있으면 점프 대신 새에서 해제 요청
	if (bIsHangingFromBird)
	{
		Server_ReleaseFromBird();
		return;
	}

	if (!AbilitySystemComponent)
	{
		return;
	}

	// 공중 상태라면 벽 점프 실행
	if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Falling))
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_WallJump));
		return;
	}

	// 점프 실행
	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Jump));
}

void AProjectERNCharacter::DoJumpEnd()
{
	StopJumping();
}

void AProjectERNCharacter::ExecuteJumpLaunch()
{
	Jump();
}

void AProjectERNCharacter::LockOn()
{
	if (bIsHangingFromBird || !LockOnComponent)
	{
		return;
	}

	const bool bNewLockOn = LockOnComponent->ToggleLockOn();

	const FRotator TargetRotation = bNewLockOn ? LockOnComponent->GetDesiredRotationToTarget() : GetActorRotation();

	ApplyLockOnState(bNewLockOn, TargetRotation);

	if (!HasAuthority())
	{
		Server_SetLockOn(bNewLockOn, TargetRotation);
	}
}

void AProjectERNCharacter::Server_SetLockOn_Implementation(bool bNewLockOn, FRotator TargetRotation)
{
	// 죽은 상태라면 LockOn 사용하지 않음
	if (!IsAlive())
	{
		if (LockOnComponent)
		{
			LockOnComponent->ClearLockOn();
		}
		
		ApplyLockOnState(false, GetActorRotation());
		return;
	}
	
	if (bNewLockOn)
	{
		const bool bServerLocked = LockOnComponent && LockOnComponent->TryLockOn();

		const FRotator ServerRotation = bServerLocked
			                                ? LockOnComponent->GetDesiredRotationToTarget()
			                                : GetActorRotation();

		ApplyLockOnState(bServerLocked, ServerRotation);
		return;
	}

	if (LockOnComponent)
	{
		LockOnComponent->ClearLockOn();
	}

	ApplyLockOnState(false, TargetRotation);
}

void AProjectERNCharacter::ApplyLockOnState(bool bNewLockOn, const FRotator& TargetRotation)
{
	bIsLockOn = bNewLockOn;

	if (bIsLockOn)
	{
		DesiredLockOnYaw = TargetRotation.Yaw;
		DesiredActorRotation = FRotator(0.f, TargetRotation.Yaw, 0.f);
	}

	UpdateRotationMode();
	UpdateMovementSpeed();
}

void AProjectERNCharacter::Server_UpdateLockOnYaw_Implementation(float NewYaw)
{
	if (!IsAlive())
	{
		return;
	}
	
	DesiredLockOnYaw = NewYaw;
}

void AProjectERNCharacter::OnRep_IsLockOn()
{
	UpdateRotationMode();
	UpdateMovementSpeed();
}

void AProjectERNCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AProjectERNCharacter, LifeState);
	DOREPLIFETIME(AProjectERNCharacter, bIsLockOn);
	DOREPLIFETIME(AProjectERNCharacter, bGodMode);
	DOREPLIFETIME(AProjectERNCharacter, bIsHangingFromBird);
	DOREPLIFETIME(AProjectERNCharacter, AttachedBird);
	DOREPLIFETIME(AProjectERNCharacter, bHeadLightOn);
}

void AProjectERNCharacter::ToggleLight()
{
	Server_ToggleLight();
}

void AProjectERNCharacter::Server_ToggleLight_Implementation()
{
	bHeadLightOn = !bHeadLightOn;
	ApplyHeadLightState();
}

void AProjectERNCharacter::OnRep_HeadLightOn()
{
	ApplyHeadLightState();
}

void AProjectERNCharacter::ApplyHeadLightState()
{
	if (HeadLight)
	{
		HeadLight->SetVisibility(bHeadLightOn);
	}

	if (HeadLightFX)
	{
		HeadLightFX->SetVisibility(bHeadLightOn);

		if (bHeadLightOn)
		{
			HeadLightFX->Activate(true);
		}
		else
		{
			HeadLightFX->DeactivateImmediate();
		}
	}
}

void AProjectERNCharacter::GodMode()
{
	Server_SetGodMode(!bGodMode);
}

void AProjectERNCharacter::Server_SetGodMode_Implementation(bool bEnable)
{
	bGodMode = bEnable;
	UE_LOG(LogTemp, Warning, TEXT("[GodMode] %s for %s"),
	       bEnable ? TEXT("ON") : TEXT("OFF"),
	       *GetName());
}

void AProjectERNCharacter::Bird()
{
	Server_SpawnRideBird();
}

void AProjectERNCharacter::SummonRideBird()
{
	Server_SpawnRideBird();
}

void AProjectERNCharacter::Server_SpawnRideBird_Implementation()
{
	// 플레이어 뒤 + 위 → 위에서 낚아채는 연출 (오프셋은 로컬 상수, 멤버 변수 X)
	const FVector SpawnLocation = GetActorLocation()
		- GetActorForwardVector() * 3000.f
		+ FVector(0.f, 0.f, 5000.f);
	const FTransform SpawnXform(GetActorRotation(), SpawnLocation);

	// 가드/스폰/Approach/재입력차단은 공용 헬퍼. bConsoleSummon=true → 빠른 비행.
	AERNIntroBird::RequestPickup(
		DebugBirdClass, this, SpawnXform, GetActorForwardVector(), /*bConsoleSummon=*/true);
}

// ===== 인트로: 새 매달림 =====

void AProjectERNCharacter::AttachToIntroBird(AERNIntroBird* Bird)
{
	if (!HasAuthority() || !Bird || !Bird->GetHangPoint())
	{
		return;
	}

	// attach 대신 새 참조만 Replicated로 동기화 → 모든 머신에서 Tick으로 HangPoint World 강제 추적
	AttachedBird = Bird;

	// 시작 위치를 즉시 HangPoint로 (서버에서 1회) — Tick 도착 전 한 프레임 어긋남 방지
	if (USceneComponent* HP = Bird->GetHangPoint())
	{
		SetActorLocation(HP->GetComponentLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	}

	// 중력/이동 차단
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_None);
	}

	// 새에 등록 + 소유권 설정 (Server RPC가 클라에서 서버로 전송되려면 Owner 필요)
	Bird->SetOwner(GetController());
	Bird->SetAttachedPlayer(this);

	// 상태 켜기 (리플리케이트 → OnRep에서 클라가 몽타주 보조 재생)
	bIsHangingFromBird = true;

	// 모든 머신에 매달림 몽타주 재생
	Multicast_StartHangingMontage();

	// 소유 클라에 비행 방향으로 시점 회전 + 매달림 FOV 적용
	const FVector Forward = Bird->GetActorForwardVector();
	FRotator FacingRot = Forward.Rotation();
	FacingRot.Pitch = 0.f;
	FacingRot.Roll = 0.f;
	Client_OnAttachedToBird(FacingRot);
}

void AProjectERNCharacter::Server_ReleaseFromBird_Implementation()
{
	if (!bIsHangingFromBird)
	{
		return;
	}

	// 새 참조 캐싱 후 해제 (Replicated → 모든 머신에서 Tick 추적 중단)
	AERNIntroBird* Bird = AttachedBird;
	AttachedBird = nullptr;

	// 하차 → BirdStatue 재입력 허용
	bBirdRideActive = false;

	// 중력 복구 → ABP가 Falling 자동 감지
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->SetMovementMode(MOVE_Falling);
	}

	// 상태 해제
	bIsHangingFromBird = false;

	// 모든 클라에 몽타주 정지
	Multicast_StopHangingMontage();

	// 소유 클라에 기본 FOV 복원
	Client_OnReleasedFromBird();

	// 새에 알림 → 위로 상승 후 destroy
	if (Bird)
	{
		Bird->OnPlayerReleased();
	}
}

void AProjectERNCharacter::Multicast_StartHangingMontage_Implementation()
{
	if (!HangingMontage || !GetMesh()) return;

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(HangingMontage);

		// 첫 섹션을 자기 자신으로 연결 → 무한 루프
		if (HangingMontage->CompositeSections.Num() > 0)
		{
			const FName FirstSection = HangingMontage->CompositeSections[0].SectionName;
			AnimInstance->Montage_SetNextSection(FirstSection, FirstSection, HangingMontage);
		}
	}
}

void AProjectERNCharacter::Multicast_StopHangingMontage_Implementation()
{
	if (!HangingMontage || !GetMesh()) return;

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(0.2f, HangingMontage);
	}
}

void AProjectERNCharacter::Client_PrewarmHangingCamera_Implementation()
{
	if (!FollowCamera || !CameraBoom)
	{
		return;
	}

	// 현재(기본) 값을 캐싱 후 매달림 값으로 보간 시작
	CachedDefaultFOV = FollowCamera->FieldOfView;
	CachedDefaultArmLength = CameraBoom->TargetArmLength;

	CameraTransitionStartFOV = CachedDefaultFOV;
	CameraTransitionStartArmLength = CachedDefaultArmLength;
	CameraTransitionTargetFOV = HangingFOV;
	CameraTransitionTargetArmLength = HangingArmLength;
	bIsCameraTransitioning = true;

	bHangingCameraPrewarmed = true;
}

void AProjectERNCharacter::Client_OnAttachedToBird_Implementation(FRotator FacingRotation)
{
	// 컨트롤 회전 보간 시작 (Tick에서 RInterpTo) — 즉시 스냅 대신 부드럽게
	if (AController* Ctrl = GetController())
	{
		CameraTransitionStartControlRotation = Ctrl->GetControlRotation();
		CameraTransitionTargetControlRotation = FacingRotation;
		bIsControlRotationTransitioning = true;
	}

	// FOV/암 길이는 Prewarm에서 이미 시작했으면 건너뜀 (캐싱된 기본값 보존)
	if (!bHangingCameraPrewarmed && FollowCamera && CameraBoom)
	{
		CachedDefaultFOV = FollowCamera->FieldOfView;
		CachedDefaultArmLength = CameraBoom->TargetArmLength;

		CameraTransitionStartFOV = CachedDefaultFOV;
		CameraTransitionStartArmLength = CachedDefaultArmLength;
		CameraTransitionTargetFOV = HangingFOV;
		CameraTransitionTargetArmLength = HangingArmLength;
		bIsCameraTransitioning = true;
	}
}

void AProjectERNCharacter::Client_OnReleasedFromBird_Implementation()
{
	bHangingCameraPrewarmed = false;

	// 캐싱된 기본 값으로 보간 복원
	if (FollowCamera && CameraBoom && CachedDefaultFOV > 0.f)
	{
		CameraTransitionStartFOV = FollowCamera->FieldOfView;
		CameraTransitionStartArmLength = CameraBoom->TargetArmLength;
		CameraTransitionTargetFOV = CachedDefaultFOV;
		CameraTransitionTargetArmLength = CachedDefaultArmLength;
		bIsCameraTransitioning = true;
	}
}

void AProjectERNCharacter::OnRep_IsHangingFromBird()
{
	// Multicast가 시점 문제로 누락된 경우의 안전망
	// 상태가 false로 바뀌었는데 몽타주가 아직 돌고 있다면 정지
	if (!bIsHangingFromBird && HangingMontage && GetMesh())
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			if (AnimInstance->Montage_IsPlaying(HangingMontage))
			{
				AnimInstance->Montage_Stop(0.2f, HangingMontage);
			}
		}
	}
}

void AProjectERNCharacter::OnRep_ReplicatedMovement()
{
	// 매달림 중에는 RepMovement.Location 적용 차단
	// → 서버 송신 시점 위치(과거)로 SetActorLocation되어 deterministic 새 위치와 충돌하는 흔들림 방지
	// → attach 자동 업데이트가 위치 책임 (ReplicatedBasedMovement는 별도 OnRep이라 attach 동기화는 정상)
	if (bIsHangingFromBird)
	{
		return;
	}

	Super::OnRep_ReplicatedMovement();
}

void AProjectERNCharacter::BindMovementSpeedTagEvents()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->RegisterGameplayTagEvent(TAG_State_Movement_Consumable, EGameplayTagEventType::NewOrRemoved)
	                      .AddUObject(this, &AProjectERNCharacter::HandleMovementSpeedTagChanged);

	AbilitySystemComponent->RegisterGameplayTagEvent(TAG_State_Movement_Flask, EGameplayTagEventType::NewOrRemoved)
	                      .AddUObject(this, &AProjectERNCharacter::HandleMovementSpeedTagChanged);
}

void AProjectERNCharacter::HandleMovementSpeedTagChanged(const FGameplayTag ChangedTag, int32 NewCount)
{
	UpdateMovementSpeed();
}

void AProjectERNCharacter::UpdateMovementSpeed()
{
	if (!GetCharacterMovement())
	{
		return;
	}

	// 기절 상태라면
	if (LifeState == EERNPlayerLifeState::Downed)
	{
		GetCharacterMovement()->MaxWalkSpeed = DownedMoveSpeed;
		return;
	}

	// 특정 상태 동안 움직임 방지
	if (LifeState == EERNPlayerLifeState::Collapsing ||	// 기절 
	LifeState == EERNPlayerLifeState::Reviving ||		// 부활
	LifeState == EERNPlayerLifeState::Respawning)		// 리스폰
	{
		GetCharacterMovement()->MaxWalkSpeed = 0.f;
		return;
	}
	
	// 태그 부여 여부 확인 (대시 스킬)
	const bool bIsDashSkill = AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_DashSkill);

	// 태그 부여 여부 확인 (전력질주)
	const bool bIsSprinting = AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Sprinting);

	// 태그 부여 여부 확인 (공격 중)
	const bool bIsAttacking = AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking);

	// 태그 부여 여부 확인 (아이템 사용 중)
	const bool bIsDrinking = AbilitySystemComponent && (
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Consumable) ||
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Flask));

	// 상황 별 속도 설정
	float NewSpeed = DefaultSpeed;
	if (bIsDashSkill) { NewSpeed = DashSkillSpeed; } // 대시 스킬
	else if (bIsSprinting) { NewSpeed = SprintSpeed; } // 전력질주
	else if (bIsLockOn) { NewSpeed = TargetingSpeed; } // 락온 상태

	// 공격 중 속도는 별도로 적용
	if (bIsAttacking) { NewSpeed = AttackingSpeed; }
	// 아이템 사용 중 속도 별도로 적용
	if (bIsDrinking) { NewSpeed = DrinkingSpeed; }

	// 속도 적용
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

void AProjectERNCharacter::UpdateRotationMode()
{
	if (!GetCharacterMovement())
	{
		return;
	}

	const bool bIsSprinting =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Sprinting);

	const bool bIsAttacking =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking);

	if (bIsAttacking)
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = false;
		return;
	}

	// LockOn상태와 관련 없이 회전이 가능하게 함
	const bool bShouldFaceMovement = !bIsLockOn || bIsSprinting;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = bShouldFaceMovement;
}

void AProjectERNCharacter::LightAttack()
{
	if (bIsHangingFromBird) return;

	if (!AbilitySystemComponent)
	{
		return;
	}

	const FRotator TargetRotation = GetAttackDesiredRotation();

	if (CacheActiveLightAttackComboInput(TargetRotation))
	{
		if (!HasAuthority())
		{
			Server_CacheLightAttackComboInput(TargetRotation);
		}

		return;
	}

	SetPendingAttackRotation(TargetRotation);

	if (!HasAuthority())
	{
		Server_SetPendingAttackRotation(TargetRotation);
	}

	// 활성 중인 LightAttack이 없으면 첫 공격을 시작한다.
	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Attack_Light));
}

void AProjectERNCharacter::HeavyAttack()
{
	if (bIsHangingFromBird) return;

	if (!AbilitySystemComponent)
	{
		return;
	}

	// True라면
	if (TryEndActiveChannelingWeaponSkill())
	{
		if (!HasAuthority())
		{
			Server_RequestEndActiveChannelingWeaponSkill();
		}

		return;
	}

	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Attack_Heavy));
}

void AProjectERNCharacter::ToggleSprint()
{
	if (bIsHangingFromBird) return;

	if (!AbilitySystemComponent)
	{
		return;
	}

	const bool bIsSprinting = AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Sprinting);

	if (bIsSprinting)
	{
		StopSprint();
		return;
	}

	if (!HasMoveInput())
	{
		return;
	}

	// 전력질주 실행
	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Sprint));
}

void AProjectERNCharacter::DrinkFlask()
{
	if (bIsHangingFromBird)
	{
		return;
	}

	if (AttributeSet->GetFlaskQuantity() < 1)
	{
		return;
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Flask));
	}
}

void AProjectERNCharacter::UseConsumable()
{
	if (bIsHangingFromBird)
	{
		return;
	}

	if (EquipmentComponent->GetCurrentConsumableQuantity() < 1)
	{
		return;
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Consumable));
	}
}

void AProjectERNCharacter::NormalSkill()
{
	if (bIsHangingFromBird)
	{
		return;
	}

	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Skill_Normal));
}

void AProjectERNCharacter::UltimateSkill()
{
	if (bIsHangingFromBird)
	{
		return;
	}

	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Skill_Ultimate));
}

void AProjectERNCharacter::StopSprint()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	FGameplayTagContainer SprintTags;
	SprintTags.AddTag(TAG_Ability_Movement_Sprint);

	AbilitySystemComponent->CancelAbilities(&SprintTags);
}

bool AProjectERNCharacter::HasMoveInput() const
{
	return !CachedMoveInput.IsNearlyZero();
}

FRotator AProjectERNCharacter::GetLockOnDesiredRotation() const
{
	return FRotator(0.f, DesiredLockOnYaw, 0.f);
}

FRotator AProjectERNCharacter::GetAttackDesiredRotation() const
{
	FRotator TargetRotation = GetActorRotation();

	// LockOn상태 일 때
	if (bIsLockOn && LockOnComponent && LockOnComponent->IsLockedOn())
	{
		return LockOnComponent->GetDesiredRotationToTarget();
	}

	// 입력이 없다면
	if (!CachedMoveInput.IsNearlyZero() && Controller)
	{
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		const FVector DesiredDirection =
			ForwardDirection * CachedMoveInput.Y +
			RightDirection * CachedMoveInput.X;

		if (!DesiredDirection.IsNearlyZero())
		{
			TargetRotation = DesiredDirection.Rotation();
		}
	}

	return FRotator(0.f, TargetRotation.Yaw, 0.f);
}

void AProjectERNCharacter::InterpActorRotation(float DeltaSeconds)
{
	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation(0.f, DesiredActorRotation.Yaw, 0.f);

	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaSeconds,
		RotationInterpSpeed);

	SetActorRotation(NewRotation);

	if (bHasPendingActorRotation && NewRotation.Equals(TargetRotation, 1.f))
	{
		bHasPendingActorRotation = false;
	}
}

void AProjectERNCharacter::SetPendingAttackRotation(const FRotator& TargetRotation)
{
	DesiredActorRotation = FRotator(0.f, TargetRotation.Yaw, 0.f);
	bHasPendingActorRotation = true;
}

void AProjectERNCharacter::Server_SetPendingAttackRotation_Implementation(FRotator TargetRotation)
{
	if (!IsAlive())
	{
		return;
	}
	SetPendingAttackRotation(TargetRotation);
}

void AProjectERNCharacter::Server_CacheLightAttackComboInput_Implementation(FRotator TargetRotation)
{
	if (!IsAlive())
	{
		return;
	}
	
	CacheActiveLightAttackComboInput(TargetRotation);
}

bool AProjectERNCharacter::CacheActiveLightAttackComboInput(const FRotator& TargetRotation)
{
	if (!AbilitySystemComponent)
	{
		return false;
	}

	const FGameplayTagContainer LightAttackTags(TAG_Ability_Attack_Light);

	for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!AbilitySpec.IsActive() || !AbilitySpec.Ability ||
			!AbilitySpec.Ability->GetAssetTags().HasAll(LightAttackTags))
		{
			continue;
		}

		for (UGameplayAbility* AbilityInstance : AbilitySpec.GetAbilityInstances())
		{
			if (UERNGA_LightAttack* LightAttackAbility = Cast<UERNGA_LightAttack>(AbilityInstance))
			{
				LightAttackAbility->CacheComboInput(TargetRotation);
				return true;
			}
		}
	}

	return false;
}

void AProjectERNCharacter::ApplyPlayerRegenEffects()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	FGameplayEffectContextHandle Context =
		AbilitySystemComponent->MakeEffectContext();

	Context.AddSourceObject(this);

	// 스태미나 재생
	if (StaminaRegenEffectClass)
	{
		FGameplayEffectSpecHandle SpecHandle =
			AbilitySystemComponent->MakeOutgoingSpec(
				StaminaRegenEffectClass,
				1.f,
				Context);

		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
				*SpecHandle.Data.Get());
		}
	}

	// 마나 재생
	if (ManaRegenEffectClass)
	{
		FGameplayEffectSpecHandle SpecHandle =
			AbilitySystemComponent->MakeOutgoingSpec(
				ManaRegenEffectClass,
				1.f,
				Context);

		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
				*SpecHandle.Data.Get());
		}
	}
}

FVector AProjectERNCharacter::GetRollWorldDirection() const
{
	if (!CachedMoveInput.IsNearlyZero() && Controller)
	{
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		return (Forward * CachedMoveInput.Y + Right * CachedMoveInput.X).GetSafeNormal();
	}

	return GetActorForwardVector();
}

bool AProjectERNCharacter::TryEndActiveChannelingWeaponSkill()
{
	if (!AbilitySystemComponent)
	{
		return false;
	}

	const FGameplayTagContainer WeaponSkillTags(TAG_Ability_Attack_Heavy);

	for (FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!AbilitySpec.IsActive() || !AbilitySpec.Ability)
		{
			continue;
		}

		if (!AbilitySpec.Ability->GetAssetTags().HasAll(WeaponSkillTags))
		{
			continue;
		}

		for (UGameplayAbility* AbilityInstance : AbilitySpec.GetAbilityInstances())
		{
			if (UERNGA_WeaponSkill_Channeling* ChannelingSkill = Cast<UERNGA_WeaponSkill_Channeling>(AbilityInstance))
			{
				ChannelingSkill->RequestEndChanneling();
				return true;
			}
		}
	}

	return false;
}

void AProjectERNCharacter::Server_RequestEndActiveChannelingWeaponSkill_Implementation()
{
	if (!IsAlive())
	{
		return;
	}
	
	TryEndActiveChannelingWeaponSkill();
}

void AProjectERNCharacter::Server_RequestRoll_Implementation(FVector_NetQuantizeNormal RollDirection)
{
	PendingRollDirection = RollDirection.GetSafeNormal();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Roll));
	}
}

float AProjectERNCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                                       AController* EventInstigator, AActor* DamageCauser)
{
	if (!IsAlive())
	{
		return 0.f;
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AProjectERNCharacter::Server_LevelUp_Implementation()
{
	if (!StatusCurveTable || !AttributeSet)
	{
		return;
	}

	const FName CurrentLevel(FString::FromInt(static_cast<int32>(AttributeSet->GetLevel())));
	const FName NextLevel(FString::FromInt(static_cast<int32>(AttributeSet->GetLevel()) + 1));;

	const FERNPlayerStatusTable* CurrentRow = StatusCurveTable->FindRow<FERNPlayerStatusTable>(
		CurrentLevel, TEXT("CurrentLevelRow"));
	const FERNPlayerStatusTable* NewRow = StatusCurveTable->FindRow<FERNPlayerStatusTable>(
		NextLevel, TEXT("TextLevelContext"));
	if (!NewRow)
	{
		return;
	}

	if (AttributeSet->GetGold() < CurrentRow->Cost)
	{
		return;
	}

	AttributeSet->SetGold(AttributeSet->GetGold() - NewRow->Cost);

	AttributeSet->SetLevel(static_cast<int32>(AttributeSet->GetLevel()) + 1);
	AttributeSet->SetMaxHealth(NewRow->MaxHealth);
	AttributeSet->SetHealth(NewRow->MaxHealth);
	AttributeSet->SetMaxMana(NewRow->MaxMana);
	AttributeSet->SetMana(NewRow->MaxMana);
	AttributeSet->SetManaRegenRate(NewRow->ManaRegenRate);
	AttributeSet->SetMaxStamina(NewRow->MaxStamina);
	AttributeSet->SetStamina(NewRow->MaxStamina);
	AttributeSet->SetStaminaRegenRate(NewRow->StaminaRegenRate);
	AttributeSet->SetAttackPower(NewRow->AttackPower);
	AttributeSet->SetDefense(NewRow->Defense);
	AttributeSet->SetStaggerResistance(NewRow->StaggerResistance);

	UE_LOG(LogTemp, Warning, TEXT("%s Level : %d"), *GetNameSafe(this), static_cast<int32>(AttributeSet->GetLevel()));
}

void AProjectERNCharacter::InteractionChurch_Implementation() const
{
	if (!AttributeSet)
	{
		return;
	}

	const int32 NewFlaskQuantity = static_cast<int32>(AttributeSet->GetMaxFlaskQuantity()) + 1;
	AttributeSet->SetMaxFlaskQuantity(NewFlaskQuantity);
	AttributeSet->SetFlaskQuantity(NewFlaskQuantity);

	UE_LOG(LogTemp, Warning, TEXT("%s, MaxFlaskQuantity: %d"), *GetNameSafe(this),
	       static_cast<int32>(AttributeSet->GetMaxFlaskQuantity()));
}

void AProjectERNCharacter::Multicast_PlayWeaponSkillInstantNiagaraEffects_Implementation(
	const TArray<FERNWeaponSkillInstantNiagaraSpawnData>& Effects)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	for (const FERNWeaponSkillInstantNiagaraSpawnData& EffectData : Effects)
	{
		// 이펙트가 없다면 Continue (발동하지 않음)
		if (!EffectData.NiagaraSystem)
		{
			continue;
		}

		// 시간차 적용되지 않았다면
		if (EffectData.StartDelay <= 0.f)
		{
			// 즉시 발동
			SpawnWeaponSkillInstantNiagaraEffect_Local(EffectData);
			continue;
		}

		// 시간차 적용되었다면 타이머로 딜레이적용
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(
			TimerHandle,
			FTimerDelegate::CreateUObject(
				this,
				&AProjectERNCharacter::SpawnWeaponSkillInstantNiagaraEffect_Local,
				EffectData),
			EffectData.StartDelay,
			false);
	}
}

void AProjectERNCharacter::SpawnWeaponSkillInstantNiagaraEffect_Local(FERNWeaponSkillInstantNiagaraSpawnData EffectData)
{
	// 나이아가라 적용되지 않았다면 return
	if (!EffectData.NiagaraSystem || !GetWorld())
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		EffectData.NiagaraSystem,
		EffectData.Location,
		EffectData.Rotation,
		EffectData.Scale,
		true,
		true,
		ENCPoolMethod::None,
		true);
}

// ===== 비전투 무한 스태미나 =====

void AProjectERNCharacter::NotifyDetectedBy(AERNEnemyCharacter* Enemy)
{
	if (!HasAuthority() || !Enemy)
	{
		return;
	}

	DetectingEnemies.Add(Enemy);
	ExitOutOfCombat();
}

void AProjectERNCharacter::NotifyLostBy(AERNEnemyCharacter* Enemy)
{
	if (!HasAuthority() || !Enemy)
	{
		return;
	}

	DetectingEnemies.Remove(Enemy);

	// 만료된 weak ptr 정리
	for (auto It = DetectingEnemies.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	if (DetectingEnemies.Num() == 0)
	{
		GetWorldTimerManager().SetTimer(
			OutOfCombatTimerHandle, this,
			&AProjectERNCharacter::EnterOutOfCombat,
			OutOfCombatGraceTime, false);
	}
}

void AProjectERNCharacter::EnterOutOfCombat()
{
	if (!HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (!ASC->HasMatchingGameplayTag(TAG_State_OutOfCombat))
	{
		ASC->AddLooseGameplayTag(TAG_State_OutOfCombat);
	}

	// 진입 시 스태미나 즉시 풀 채움 - 회복 GE 설정과 무관하게 무한 보장
	if (UERNAttributeSet* AS = const_cast<UERNAttributeSet*>(
		Cast<UERNAttributeSet>(ASC->GetAttributeSet(UERNAttributeSet::StaticClass()))))
	{
		AS->SetStamina(AS->GetMaxStamina());
	}
}

void AProjectERNCharacter::ExitOutOfCombat()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(OutOfCombatTimerHandle);

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(TAG_State_OutOfCombat))
		{
			ASC->RemoveLooseGameplayTag(TAG_State_OutOfCombat);
		}
	}
}

void AProjectERNCharacter::OnRep_LifeState()
{
	// 태그 갱신
	RefreshLifeStateTags();
	// 이동속도 갱신
	UpdateMovementSpeed();
}

void AProjectERNCharacter::SetLifeState(EERNPlayerLifeState NewState)
{
	// 서버에서만 실행 || 같은 상태면 return
	if (!HasAuthority() || LifeState == NewState)
	{
		return;
	}

	// Set
	LifeState = NewState;
	// 태그 갱신
	RefreshLifeStateTags();
	// Replication
	ForceNetUpdate();
}

void AProjectERNCharacter::RefreshLifeStateTags()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// 죽음
	AbilitySystemComponent->SetLooseGameplayTagCount(TAG_State_Life_Downed,
	                                                 LifeState == EERNPlayerLifeState::Downed ? 1 : 0);
}

void AProjectERNCharacter::OnDeath()
{
	// 서버에서만 처리
	if (!HasAuthority() || !IsAlive())
	{
		return;
	}

	// 죽음 상태 진입
	EnterCollapsingState();
}

void AProjectERNCharacter::EnterCollapsingState()
{
	// 쓰러지는 상태
	SetLifeState(EERNPlayerLifeState::Collapsing);

	// 입력 제거
	CachedMoveInput = FVector2D::ZeroVector;
	// 스프린트 종료
	StopSprint();

	// 모든 어빌리티 캔슬
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
	}

	// 락온 종료
	if (LockOnComponent)
	{
		LockOnComponent->ClearLockOn();
	}

	ApplyLockOnState(false, GetActorRotation());

	// 이동 즉시 비활성
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	// 기절 진입 몽타주 재생
	Multicast_PlayDeathMontage();

	// 몽타주와 관계 없이 최소 1초 적용
	const float CollapseDuration = DeathMontage ? DeathMontage->GetPlayLength() : CollapseFallbackDuration;

	// 죽음 상태 타이머
	GetWorldTimerManager().SetTimer(
		CollapseTimerHandle,
		this,
		&AProjectERNCharacter::FinishCollapsingState,
		FMath::Max(CollapseDuration, 0.01f), // 최소 시간 적용
		false);
}

void AProjectERNCharacter::FinishCollapsingState()
{
	// 서버에서만 적용 || Collapsing 상태일 때만 적용
	if (!HasAuthority() || LifeState != EERNPlayerLifeState::Collapsing)
	{
		return;
	}

	// 기절 상태 진입
	EnterDownedState();
}

void AProjectERNCharacter::EnterDownedState()
{
	// 기절 상태로 변경
	SetLifeState(EERNPlayerLifeState::Downed);

	if (DownedComponent)
	{
		// 패널티 스택 증가
		DownedComponent->EnterDownedState(DownedComponent->GetPenaltyStacks() + 1);
	}

	// 이동 모드를 지상 보행으로 변경
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}

	// 움직임 속도 제한 적용
	UpdateMovementSpeed();
}

bool AProjectERNCharacter::bApplyReviveHit(AController* Reviver)
{
	if (!CanApplyReviveHit(Reviver))
	{
		return false;
	}

	DownedComponent->ApplyReviveHit(Reviver);
	return true;
}

bool AProjectERNCharacter::TryApplyReviveHit(AActor* HitActor, AController* Reviver)
{
	AProjectERNCharacter* DownedPlayer = Cast<AProjectERNCharacter>(HitActor);
	if (!DownedPlayer || !DownedPlayer->IsDowned())
	{
		return false;
	}

	return DownedPlayer->bApplyReviveHit(Reviver);
}

bool AProjectERNCharacter::CanApplyReviveHit(AController* Reviver) const
{
	if (!HasAuthority() || !IsDowned() || !DownedComponent)
	{
		return false;
	}

	if (DownedComponent->GetDownedHealth() <= 0.f)
	{
		return false;
	}

	const APawn* ReviverPawn = Reviver ? Reviver->GetPawn() : nullptr;
	const AProjectERNCharacter* ReviverCharacter = Cast<AProjectERNCharacter>(ReviverPawn);

	return ReviverCharacter && ReviverCharacter != this && ReviverCharacter->IsAlive();
}

void AProjectERNCharacter::CompleteRevive()
{
	// 서버에서만 실행
	if (!HasAuthority() || !IsDowned())
	{
		return;
	}

	// 부활 시작
	EnterRevivingState();
}

void AProjectERNCharacter::EnterRevivingState()
{
	// 서버에서만
	if (!HasAuthority() || !IsDowned())
	{
		return;
	}

	// State 변경
	SetLifeState(EERNPlayerLifeState::Reviving);

	if (DownedComponent)
	{
		DownedComponent->ExitDownedState();
	}

	// 부활 체력 적용
	if (AttributeSet)
	{
		const float ReviveHealth = FMath::Max(1.f, AttributeSet->GetMaxHealth() * ReviveHealthRatio);
		AttributeSet->SetHealth(ReviveHealth);
	}

	// 캐싱된 입력 제거
	CachedMoveInput = FVector2D::ZeroVector;
	// 전력질주 Stop
	StopSprint();

	// 모든 어빌리티
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	// 몽타주 재생
	Multicast_PlayReviveMontage();
	
	// Fallback으로 최소 시간 설정
	const float ReviveDuration = ReviveMontage ? ReviveMontage->GetPlayLength() : ReviveFallbackDuration;

	GetWorldTimerManager().ClearTimer(ReviveTimerHandle);
	GetWorldTimerManager().SetTimer(
		ReviveTimerHandle,
		this,
		&AProjectERNCharacter::FinishRevivingState,
		FMath::Max(ReviveDuration, 0.01f),
		false);

	ForceNetUpdate();
}

void AProjectERNCharacter::FinishRevivingState()
{
	if (!HasAuthority() || LifeState != EERNPlayerLifeState::Reviving)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}

	SetLifeState(EERNPlayerLifeState::Alive);
	UpdateMovementSpeed();
	ForceNetUpdate();
}

void AProjectERNCharacter::Multicast_PlayReviveMontage_Implementation()
{
	if (!ReviveMontage || !GetMesh())
	{
		return;
	}
	
	// 멀티캐스트로 부활 몽타주 재생
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(ReviveMontage);
	}
}
