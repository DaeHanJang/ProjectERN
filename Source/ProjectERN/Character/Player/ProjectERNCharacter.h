// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/ERNCharacterBase.h"
#include "Logging/LogMacros.h"
#include "ProjectERNCharacter.generated.h"

class UERNLevelUpWidget;
class USphereComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
enum class ECharacterType : uint8;
class UERNInventoryComponent;
class UERNEquipmentComponent;
class UERNShopComponent;
class UERNUpgradeComponent;
class UERNInputConfig;
class UGameplayEffect;
class UCameraShakeBase;
class UPostProcessComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AProjectERNCharacter : public AERNCharacterBase
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Inventory Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UERNInventoryComponent* InventoryComponent;

	/** Equipment Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UERNEquipmentComponent* EquipmentComponent;

	/** Shop Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UERNShopComponent* ShopComponent;

	/** Upgrade Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UERNUpgradeComponent* UpgradeComponent;
	
	/** Interaction Detection Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* InteractionDetector;
	
	// InteractionDetector TimerHandle
	FTimerHandle DetectionTimerHandle;

protected:
	// InteractionDetector Update
	void UpdateInteractionDetector();
	
	// Status Curve Table
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Character")
	TObjectPtr<UDataTable> StatusCurveTable;
	
	/** Character Type - 블루프린트에서 설정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Character")
	ECharacterType CharacterType;

	/** Called when character is possessed by a controller */
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	
	// 회전 보간을 위해 사용
	virtual void Tick(float DeltaSeconds) override;
	
	// 태그 기반 입력을 위한 InputConfig 부여
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Input")
	TObjectPtr<UERNInputConfig> InputConfig;

public:
	/** Constructor */
	AProjectERNCharacter();
	
	FORCEINLINE UDataTable* GetStatusCurveTable() const { return StatusCurveTable; }
	
	// Level Up
	UFUNCTION(Server, Reliable)
	void Server_LevelUp();
	
	// 교회 상호작용
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Attributes")
	void InteractionChurch() const;

protected:

	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	/** Called for movement input */
	void Move(const FInputActionValue& Value);
	
	/** Called for movement input end*/
	void MoveEnd();

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for roll input */
	void Roll();
	
	/** Called for light attack input */
	void LightAttack();

	/** Called for heavy attack input */
	void HeavyAttack();
	
	/** Called for lock on input */
	void LockOn();
	
	/** Called for sprint on input */
	void ToggleSprint();
	
	/** Called for flask on input */
	void DrinkFlask();
	
	/** Called for Consumable on input */
	void UseConsumable();

public:
	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="ERN|Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="ERN|Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="ERN|Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="ERN|Input")
	virtual void DoJumpEnd();
	
	UFUNCTION(BlueprintCallable, Category="ERN|Action")
	void ExecuteJumpLaunch();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Returns InventoryComponent **/
	FORCEINLINE class UERNInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** Returns EquipmentComponent **/
	FORCEINLINE class UERNEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	/** Returns ShopComponent **/
	FORCEINLINE class UERNShopComponent* GetShopComponent() const { return ShopComponent; }

	/** Returns UpgradeComponent **/
	FORCEINLINE class UERNUpgradeComponent* GetUpgradeComponent() const { return UpgradeComponent; }
	
	// ************** 임시 락온 기능 구현 **************
public:
	UFUNCTION(BlueprintCallable, Category="ERN|LockOn")
	void ToggleTemporaryLockOn();

	bool IsLockOn() const { return bIsLockOn; }
	
	UFUNCTION(Server, Reliable)
	void Server_SetLockOn(bool bNewLockOn, FRotator TargetRotation);
	
	void ApplyLockOnState(bool bNewLockOn, const FRotator& TargetRotation);
	
protected:
	float DesiredLockOnYaw = 0.f;
	
	// Server에서 Yaw방향 갱신
	// Unreliable을 사용하는 이유는 LockOn yaw는 매번 최신값만 중요하고, 오래된 패킷이 늦게 도착해도 의미가 적기 때문이에요.
	UFUNCTION(Server, Unreliable)
	void Server_UpdateLockOnYaw(float NewYaw);
	
	UPROPERTY(ReplicatedUsing=OnRep_IsLockOn, BlueprintReadOnly, Category="ERN|LockOn")
	bool bIsLockOn = false;
	
	UFUNCTION()
	void OnRep_IsLockOn();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LockOn")
	bool bUseCameraYawOnLockOn = true;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	// ************** 임시 락온 기능 구현 **************
	
public:
	// 공격 중 움직일 수 있게 하기 위함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Combat")
	bool bCanMoveWhileAttacking = false;

	// 피격 시 카메라 흔들림 (데미지/MaxHealth 비율로 강도 분기)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|CameraShake")
	TSubclassOf<UCameraShakeBase> TakeDamageShakeClass_Small;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|CameraShake")
	TSubclassOf<UCameraShakeBase> TakeDamageShakeClass_Medium;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|CameraShake")
	TSubclassOf<UCameraShakeBase> TakeDamageShakeClass_Big;

	// 데미지/MaxHealth 비율 임계값 — 미만이면 없음, 10 이하면 small, 20 이하면 medium, 30 이상이면 Big
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|CameraShake", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageShakeThresholdSmall = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|CameraShake", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageShakeThresholdMedium = 20.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|CameraShake", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageShakeThresholdBig = 30.f;

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// StaggerPower에 따른 카메라 흔들림을 위한 경직 재정의
	virtual void TryApplyStagger(float IncomingStaggerPower, const FVector& HitOrigin = FVector::ZeroVector) override;
	
	// 디버그 무적 (HP가 1 아래로 안 떨어짐) — 콘솔 명령 GodMode로 토글
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Debug")
	bool bGodMode = false;

	// 콘솔 명령: ~ 키 → GodMode 입력으로 토글 (대소문자 무관)
	UFUNCTION(Exec)
	void GodMode();

	UFUNCTION(Server, Reliable)
	void Server_SetGodMode(bool bEnable);

	// ===== 인트로: 새에 매달림 =====

	// 매달림 상태 (Replicated)
	UPROPERTY(ReplicatedUsing = OnRep_IsHangingFromBird, BlueprintReadOnly, Category = "Intro")
	bool bIsHangingFromBird = false;

	// 매달림 시 따라갈 새 (Replicated — Tick에서 HangPoint World로 자기 위치 강제 동기화)
	UPROPERTY(Replicated)
	TObjectPtr<class AERNIntroBird> AttachedBird;

	// 매달림 루프 몽타주 (BP에서 AM_Shared_Hanging 할당)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	TObjectPtr<UAnimMontage> HangingMontage;

	// 매달림 중 카메라 FOV (기본보다 넓게)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	float HangingFOV = 110.f;

	// 매달림 시작 시점에 캐싱한 기본 FOV (해제 시 복원)
	UPROPERTY(Transient)
	float CachedDefaultFOV = 90.f;

	// 서버: 새에 부착 (인트로 매니저가 호출)
	void AttachToIntroBird(class AERNIntroBird* Bird);

	// 부착된 새 반환 (PC가 조향 RPC 호출 시 사용)
	class AERNIntroBird* GetAttachedBird() const { return AttachedBird; }

	// 서버: 새에서 해제 (Jump 입력 시 호출)
	UFUNCTION(Server, Reliable)
	void Server_ReleaseFromBird();

	// 모든 클라: 매달림 몽타주 재생
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartHangingMontage();

	// 모든 클라: 매달림 몽타주 정지
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopHangingMontage();

	// 해당 클라(소유 PC)에서 비행 방향으로 시점 회전 + 매달림 FOV 적용
	UFUNCTION(Client, Reliable)
	void Client_OnAttachedToBird(FRotator FacingRotation);

	// 해당 클라(소유 PC)에서 기본 FOV 복원
	UFUNCTION(Client, Reliable)
	void Client_OnReleasedFromBird();

	UFUNCTION()
	void OnRep_IsHangingFromBird();

	// 매달림 중에는 RepMovement.Location 적용 차단 (attach 자동 업데이트가 위치 책임)
	virtual void OnRep_ReplicatedMovement() override;

protected:
	// 상태 별 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float DefaultSpeed = 600;	// 기본 속도
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float TargetingSpeed = 300;	// 타겟팅 중 속도
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float TargetingRunSpeed = 500;	// 타겟팅 중 달리기 속도
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float AttackingSpeed = 200;	// 공격 중 속도
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float SprintSpeed = 1000;	// 전력질주 속도
	
	// 회전 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Movement")
	float RotationInterpSpeed = 10.f;
	
	// 현재 회전 보간 목표
	FRotator DesiredActorRotation = FRotator::ZeroRotator;

	// 콤보/공격처럼 한 번만 회전시키고 끝낼 때 사용
	bool bHasPendingActorRotation = false;
	
public:
	// 움직임 속도 변화 함수
	UFUNCTION(BlueprintCallable, Category="ERN|Movement")
	void UpdateMovementSpeed();
	
	// 캐릭터 회전 변화 함수
	UFUNCTION(BlueprintCallable, Category="ERN|Movement")
	void UpdateRotationMode();
	
	// 공격의 1회용 회전 적용
	void SetPendingAttackRotation(const FRotator& TargetRotation);
	
protected:
	// Sprint 관련 변수/함수
	FVector2D CachedMoveInput = FVector2D::ZeroVector;
	
	// Sprint 멈추기 함수
	void StopSprint();
	
	// 움직임 입력 여부 확인(Sprint 비활성 확인용)
	bool HasMoveInput() const;
	
	// LockOn 목표 회전 계산
	FRotator GetLockOnDesiredRotation() const;

	// 공격 목표 회전 계산
	FRotator GetAttackDesiredRotation() const;

	// 실제 회전 보간 실행
	void InterpActorRotation(float DeltaSeconds);
	
	// 서버에서의 공격 회전을 적용
	UFUNCTION(Server, Reliable)
	void Server_SetPendingAttackRotation(FRotator TargetRotation);
	
	// 클라이언트가 누른 추가 공격 입력을 서버의 Ability 인스턴스에도 전달하기 위함
	UFUNCTION(Server, Reliable)
	void Server_CacheLightAttackComboInput(FRotator TargetRotation);

	bool CacheActiveLightAttackComboInput(const FRotator& TargetRotation);
	
protected:
	// 스태미나 재생 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|GE")
	TSubclassOf<UGameplayEffect> StaminaRegenEffectClass;
	
	// 마나 재생 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|GE")
	TSubclassOf<UGameplayEffect> ManaRegenEffectClass;
	
	// 재생효과 적용
	void ApplyPlayerRegenEffects();
	
	// *** 멀티 환경에서 방향별 구르기 적용시키기 ***
private:
	// 구르기 입력 방향 저장 변수
	FVector PendingRollDirection = FVector::ForwardVector;
	
	// 회피 입력 순간 방향 계산
	FVector GetRollWorldDirection() const;

public:
	FVector GetPendingRollDirection() const { return PendingRollDirection; }

private:
	// 서버에서 실행
	UFUNCTION(Server, Reliable)
	void Server_RequestRoll(FVector_NetQuantizeNormal RollDirection);
	
	// *** 멀티 환경에서 방향별 구르기 적용시키기 ***
	
	// NightRainZone 자기장 밤의비
#pragma region NightRainZone
public:
	UPostProcessComponent* GetNightRainPostProcessComponent() const { return NightRainPostProcessComponent; }
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NightRain|PostProcess", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPostProcessComponent> NightRainPostProcessComponent;
#pragma endregion
public:
	// 캐릭터 HeavyAttack 입력을 토글로 변경
	bool TryEndActiveChannelingWeaponSkill();
	
	UFUNCTION(Server, Reliable)
	void Server_RequestEndActiveChannelingWeaponSkill();
};
