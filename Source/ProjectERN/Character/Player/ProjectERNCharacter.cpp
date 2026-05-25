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
#include "Character/Player/ERNPlayerState.h"
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
#include "Components/PostProcessComponent.h"
#include "Inventory/Item/ERNItemActor.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

void AProjectERNCharacter::TryApplyStagger(float IncomingStaggerPower, const FVector& HitOrigin)
{
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
	
	// NightRainZone 자기장 밤의비
	NightRainPostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("NightRainPostProcessComponent"));
	NightRainPostProcessComponent->SetupAttachment(FollowCamera);
	NightRainPostProcessComponent->bEnabled = true;
	NightRainPostProcessComponent->bUnbound = true;
	NightRainPostProcessComponent->BlendWeight = 0.f;
	NightRainPostProcessComponent->Priority = 100.f;
	
	// GAS 컴포넌트는 부모 클래스(ERNCharacterBase)에서 생성됨

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// 기본값 설정
	CharacterType = ECharacterType::Warrior;
}

void AProjectERNCharacter::BeginPlay()
{
	Super::BeginPlay();
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
		GetWorldTimerManager().SetTimer(DetectionTimerHandle, this, &AProjectERNCharacter::UpdateInteractionDetector, 0.2f, true, 0.0f);
	}
}

void AProjectERNCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	
	// InteractionDetector 감지 시작
	if (!GetWorldTimerManager().IsTimerActive(DetectionTimerHandle))
	{
		GetWorldTimerManager().SetTimer(DetectionTimerHandle, this, &AProjectERNCharacter::UpdateInteractionDetector, 0.2f, true, 0.0f);
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

	if (bIsLockOn && !bIsSprinting && !bIsAttacking)
	{
		if (IsLocallyControlled() && Controller)
		{
			const float ControlYaw = Controller->GetControlRotation().Yaw;
			DesiredLockOnYaw = ControlYaw;

			if (!HasAuthority())
			{
				Server_UpdateLockOnYaw(ControlYaw);
			}
		}

		DesiredActorRotation = GetLockOnDesiredRotation();

		if (HasAuthority() || IsLocallyControlled())
		{
			InterpActorRotation(DeltaSeconds);
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
}

void AProjectERNCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

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
	
	const bool bIsGetHit =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Stagger);

	if ((bIsAttacking && !bCanMoveWhileAttacking) || bIsLanding || bIsGetHit)
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

void AProjectERNCharacter::ToggleTemporaryLockOn()
{
	const bool bNewLockOn = !bIsLockOn;
	const FRotator ControlRotation = Controller ? Controller->GetControlRotation() : GetActorRotation();

	ApplyLockOnState(bNewLockOn, ControlRotation);

	if (!HasAuthority())
	{
		Server_SetLockOn(bNewLockOn, ControlRotation);
	}
}

void AProjectERNCharacter::Server_SetLockOn_Implementation(bool bNewLockOn, FRotator TargetRotation)
{
	ApplyLockOnState(bNewLockOn, TargetRotation);
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

	DOREPLIFETIME(AProjectERNCharacter, bIsLockOn);
	DOREPLIFETIME(AProjectERNCharacter, bGodMode);
	DOREPLIFETIME(AProjectERNCharacter, bIsHangingFromBird);
	DOREPLIFETIME(AProjectERNCharacter, AttachedBird);
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

	// 새에 등록
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

void AProjectERNCharacter::Client_OnAttachedToBird_Implementation(FRotator FacingRotation)
{
	// 시점을 새의 비행 방향으로 회전
	if (AController* Ctrl = GetController())
	{
		Ctrl->SetControlRotation(FacingRotation);
	}

	// 기본 FOV 캐싱 후 매달림 FOV 적용
	if (FollowCamera)
	{
		CachedDefaultFOV = FollowCamera->FieldOfView;
		FollowCamera->SetFieldOfView(HangingFOV);
	}
}

void AProjectERNCharacter::Client_OnReleasedFromBird_Implementation()
{
	// 기본 FOV 복원
	if (FollowCamera && CachedDefaultFOV > 0.f)
	{
		FollowCamera->SetFieldOfView(CachedDefaultFOV);
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

void AProjectERNCharacter::UpdateMovementSpeed()
{
	if (!GetCharacterMovement())
	{
		return;
	}

	const bool bIsSprinting = AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(
		TAG_State_Movement_Sprinting);

	float NewSpeed = DefaultSpeed;

	if (bIsSprinting)
	{
		NewSpeed = SprintSpeed;
	}
	else if (bIsLockOn)
	{
		NewSpeed = TargetingSpeed;
	}

	if (AbilitySystemComponent && (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking)))
	{
		NewSpeed = AttackingSpeed;
	}

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

void AProjectERNCharacter::LockOn()
{
	if (bIsHangingFromBird) return;

	ToggleTemporaryLockOn();
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
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Flask));
	}
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
	if (bIsLockOn && Controller)
	{
		// 카메라 방향과 일치하는 방향으로 회전
		const FRotator ControlRotation = Controller->GetControlRotation();
		return FRotator(0.f, ControlRotation.Yaw, 0.f);
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
	SetPendingAttackRotation(TargetRotation);
}

void AProjectERNCharacter::Server_CacheLightAttackComboInput_Implementation(FRotator TargetRotation)
{
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

float AProjectERNCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	return ActualDamage;
}

void AProjectERNCharacter::Server_LevelUp_Implementation()
{
	if (!StatusCurveTable || !AttributeSet)
	{
		return;
	}
	
	const FName CurrentLevel(FString::FromInt(static_cast<int32>(AttributeSet->GetLevel())));
	const FName NextLevel(FString::FromInt(static_cast<int32>(AttributeSet->GetLevel()) + 1));;
	
	const FERNPlayerStatusTable* CurrentRow = StatusCurveTable->FindRow<FERNPlayerStatusTable>(CurrentLevel, TEXT("CurrentLevelRow"));
	const FERNPlayerStatusTable* NewRow = StatusCurveTable->FindRow<FERNPlayerStatusTable>(NextLevel, TEXT("TextLevelContext"));
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
	
	UE_LOG(LogTemp, Warning, TEXT("%s, MaxFlaskQuantity: %d"), *GetNameSafe(this), static_cast<int32>(AttributeSet->GetMaxFlaskQuantity()));
}
