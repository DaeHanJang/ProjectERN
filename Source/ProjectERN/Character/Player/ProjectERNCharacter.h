// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/ERNCharacterBase.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_SkillBase.h"
#include "Logging/LogMacros.h"
#include "GAS/Abilities/WeaponSkill/ERNWeaponSkillTypes.h"
#include "ProjectERNCharacter.generated.h"

class UERNLockOnComponent;
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
class UERNSkillNiagaraComponent;
class UPostProcessComponent;
class AERNEnemyCharacter;
class UPointLightComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UERNDownedComponent;
class ANightRainZoneManager;
class UWidgetComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

// 플레이어의 생명 상태를 분류하는 열거형 클래스
UENUM(BlueprintType)
enum class EERNPlayerLifeState : uint8
{
	Alive UMETA(DisplayName = "Alive"),				// 살아 있음
	Collapsing UMETA(DisplayName = "Collapsing"),   // 쓰러지는 몽타주 재생 중
	Downed UMETA(DisplayName = "Downed"),			// 다운 상태
	Reviving UMETA(DisplayName = "Reviving"),		// 부활 중
	Respawning UMETA(DisplayName = "Respawning")	// 리스폰 중
};

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AProjectERNCharacter : public AERNCharacterBase
{
	GENERATED_BODY()

	friend class UERNGA_WeaponSkill_Instant;

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

	/** Skill Niagara Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	UERNSkillNiagaraComponent* SkillNiagaraComponent;

	/** LockOn Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UERNLockOnComponent> LockOnComponent;

	/** LockOn Detection Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> LockOnDetector;

	/** Head Light */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPointLightComponent> HeadLight;

	/** Head Light Niagara FX */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraComponent> HeadLightFX;
	
	/** Downed State Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UERNDownedComponent> DownedComponent;

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

	// 코요테 타임 처리 — 낙하 진입/착지 감지
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	// 코요테 타임 동안 공중에서도 일반 점프 허용
	virtual bool CanJumpInternal_Implementation() const override;

	// 점프 성공 시 코요테 타임 소비 (중복 점프 방지)
	virtual void OnJumped_Implementation() override;

	// 점프 버퍼 처리 — 착지 시 미리 누른 점프 실행
	virtual void Landed(const FHitResult& Hit) override;

	// 태그 기반 입력을 위한 InputConfig 부여
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Input")
	TObjectPtr<UERNInputConfig> InputConfig;

	// 코요테 타임 — 발판에서 걸어 떨어진 직후 이 시간(초) 동안은 일반 점프 허용. 0이면 비활성
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Movement")
	float CoyoteTime = 0.3f;

	// 점프 버퍼 — 착지 전에 미리 누른 점프를 이 시간(초) 안이면 착지 시 실행. 0이면 비활성
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Movement")
	float JumpBufferTime = 0.15f;

private:
	// 현재 코요테 점프가 가능한 상태인지
	bool bCoyoteJumpAvailable = false;

	// 코요테 타임 만료 타이머
	FTimerHandle CoyoteTimerHandle;

	// 코요테 타임 종료 (착지/소비/만료 시)
	void EndCoyoteTime();

	// 공중에서 점프를 누른 시각 (점프 버퍼 판정용)
	float JumpBufferedPressTime = -100.f;

protected:

public:
	/** Constructor */
	AProjectERNCharacter();

	FORCEINLINE UDataTable* GetStatusCurveTable() const { return StatusCurveTable; }

	UFUNCTION(BlueprintCallable, Category="Attribute")
	void InitStatus();

	// 특정 레벨의 StatusCurveTable 행으로 스탯 적용 (+SetLevel)
	void InitStatusForLevel(int32 Level);

	// BP possess 흐름 마지막에 호출 — PlayerState 스냅샷이 있으면 복원(없으면 기본값 유지)
	UFUNCTION(BlueprintCallable, Category="ERN|Run")
	void ApplyRunSnapshot();

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

	/** Called for NormalSkill on input */
	void NormalSkill();

	/** Called for UltimateSkill on input */
	void UltimateSkill();

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
	bool IsLockOn() const { return bIsLockOn; }

	UFUNCTION(Server, Reliable)
	void Server_SetLockOn(bool bNewLockOn, FRotator TargetRotation);

	void ApplyLockOnState(bool bNewLockOn, const FRotator& TargetRotation);

	// 락온 마커(흰 점) 화면 위치 갱신 — 로컬 전용, Tick에서 매 프레임 호출
	void UpdateLockOnMarker();

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

	// 락온 카메라 피치 보정 — 먼 적일수록 카메라가 수평이 되어 캐릭터-적이 겹치는 문제 방지
	// 이 거리(2D) 이하면 기존처럼 자유로운 상한 피치 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LockOn|Pitch")
	float LockOnPitchNearDistance = 300.f;

	// 이 거리(2D) 이상이면 상한 피치를 LockOnPitchMaxFar까지 낮춰 강제로 내려다봄
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LockOn|Pitch")
	float LockOnPitchFarDistance = 300.f;

	// 가까울 때 상한 피치(위로 허용하는 각도)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LockOn|Pitch")
	float LockOnPitchMaxNear = 15.f;

	// 멀 때 상한 피치(음수 = 강제로 아래를 보게 함)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LockOn|Pitch")
	float LockOnPitchMaxFar = -20.f;

	// 하한 피치(너무 가파르게 내려다보지 않도록 제한)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LockOn|Pitch")
	float LockOnPitchMin = -35.f;

	// 서버의 값을 클라이언트로 복제할 멤버 변수 목록을 등록하는 함수
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// ************** 임시 락온 기능 구현 **************
public:
	// 공격 중 움직일 수 있게 하기 위함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Combat")
	bool bCanMoveWhileAttacking = false;

	// 라이프스틸 — 적에게 준 데미지 중 회복할 비율 (0.15 = 15%). 0이면 비활성 (Warrior BP에만 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Combat")
	float LifestealFraction = 0.f;

	// 가한 데미지 기반 자가 회복 (서버 권위). 적 TakeDamage에서 공격자로 호출됨
	void ApplyLifesteal(float DamageDealt);

	// 피격 시 카메라 흔들림 (데미지/MaxHealth 비율로 강도 분기)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|CameraShake")
	TSubclassOf<UCameraShakeBase> TakeDamageShakeClass_Small;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|CameraShake")
	TSubclassOf<UCameraShakeBase> TakeDamageShakeClass_Medium;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|CameraShake")
	TSubclassOf<UCameraShakeBase> TakeDamageShakeClass_Big;

	// 데미지/MaxHealth 비율 임계값 — 미만이면 없음, 10 이하면 small, 20 이하면 medium, 30 이상이면 Big
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|CameraShake",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageShakeThresholdSmall = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|CameraShake",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageShakeThresholdMedium = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|CameraShake",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageShakeThresholdBig = 30.f;

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	                         AActor* DamageCauser) override;

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

	// ===== 디버그: 새 호출 (콘솔 명령 bird) =====

	// 콘솔 명령: ~ 키 → bird 입력. 플레이어 뒤+위에서 새가 swoop으로 내려와 태우고 빠르게 전진.
	UFUNCTION(Exec)
	void Bird();

	// ===== 치트: 골드 지급 (콘솔 명령 gold) =====
	// 콘솔 명령: gold → 100만 골드 지급 (인자 주면 그 액수)
	UFUNCTION(Exec)
	void Gold(int32 Amount = 1000000);

	UFUNCTION(Server, Reliable)
	void Server_GiveGold(int32 Amount);

	// 시퀀서 이벤트 → 이 노드 호출. 내부에서 서버로 라우팅되어 자기 새 1마리 스폰.
	// (각 클라가 Get Player Character(0) 로컬 캐릭터에 대해 호출 → 플레이어당 1마리)
	UFUNCTION(BlueprintCallable, Category = "Intro|Bird")
	void SummonRideBird();

	// exec는 소유 클라 실행 → 서버 라우팅 (스폰/부착은 서버 권한)
	UFUNCTION(Server, Reliable)
	void Server_SpawnRideBird();

	// 스폰할 새 클래스 (동상이 쓰는 BP_IntroBird 계열 할당)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug|Bird")
	TSubclassOf<class AERNIntroBird> DebugBirdClass;

	// ===== 인트로: 새에 매달림 =====

	// 매달림 상태 (Replicated)
	UPROPERTY(ReplicatedUsing = OnRep_IsHangingFromBird, BlueprintReadOnly, Category = "Intro")
	bool bIsHangingFromBird = false;

	// 매달림 시 따라갈 새 (Replicated — Tick에서 HangPoint World로 자기 위치 강제 동기화)
	UPROPERTY(Replicated)
	TObjectPtr<class AERNIntroBird> AttachedBird;

	// 서버 전용: 새 호출~하차 구간 중복 입력 차단 플래그
	bool bBirdRideActive = false;

	// 매달림 루프 몽타주 (BP에서 AM_Shared_Hanging 할당)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	TObjectPtr<UAnimMontage> HangingMontage;

	// 매달림 중 카메라 FOV (기본보다 넓게)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	float HangingFOV = 110.f;

	// 매달림 중 카메라 암 길이 (기본보다 길게 — 확대 연출)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	float HangingArmLength = 800.f;

	// FOV/ArmLength FInterpTo 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	float CameraInterpSpeed = 1.f;

	// 보간 진행도 이 값 이상이면 목표값으로 스냅 (FInterpTo 점근 꼬리 절단)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	float CameraSnapThreshold = 0.98f;

	// 매달림 시작 시점에 캐싱한 기본 FOV (해제 시 복원)
	UPROPERTY(Transient)
	float CachedDefaultFOV = 90.f;

	// 매달림 시작 시점에 캐싱한 기본 카메라 암 길이 (해제 시 복원)
	UPROPERTY(Transient)
	float CachedDefaultArmLength = 400.f;

	// 카메라 보간 진행 중 여부 (소유 클라 Tick에서 보간)
	bool bIsCameraTransitioning = false;

	// Approach 중 카메라 미리 widen 했는지 — Client_OnAttachedToBird에서 중복 캐싱 방지
	bool bHangingCameraPrewarmed = false;

	// 보간 시작/목표 값 (진행도 계산용)
	float CameraTransitionStartArmLength = 0.f;
	float CameraTransitionStartFOV = 0.f;
	float CameraTransitionTargetArmLength = 0.f;
	float CameraTransitionTargetFOV = 0.f;

	// 부착 시 컨트롤 회전도 함께 보간 (release 시에는 사용 안 함)
	bool bIsControlRotationTransitioning = false;
	FRotator CameraTransitionStartControlRotation = FRotator::ZeroRotator;
	FRotator CameraTransitionTargetControlRotation = FRotator::ZeroRotator;

	// Ascend 동안 카메라 lag 파라미터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	float AscentCameraLagSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	float AscentCameraLagMaxDistance = 300.f;

	// 현재 lag 활성 상태 추적 (전환 순간에만 SpringArm 세팅)
	bool bAscentCameraLagActive = false;

	// 서버: 새에 부착 (인트로 매니저가 호출)
	void AttachToIntroBird(class AERNIntroBird* Bird);

	// 부착된 새 반환 (PC가 조향 RPC 호출 시 사용)
	class AERNIntroBird* GetAttachedBird() const { return AttachedBird; }

	// 서버: BirdStatue로 새를 호출한 순간부터 새에서 내릴 때까지 true.
	// Approach 중에는 AttachedBird가 아직 null이라, 이 플래그로 중복 호출(새 여러 마리)을 차단.
	bool IsBirdRideActive() const { return bBirdRideActive; }
	void SetBirdRideActive(bool bActive) { bBirdRideActive = bActive; }

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

	// 새가 attach 직전(LeadTime 초 전)에 카메라 FOV/암 길이를 미리 widen 시작
	UFUNCTION(Client, Reliable)
	void Client_PrewarmHangingCamera();

	UFUNCTION()
	void OnRep_IsHangingFromBird();

	// 매달림 중에는 RepMovement.Location 적용 차단 (attach 자동 업데이트가 위치 책임)
	virtual void OnRep_ReplicatedMovement() override;

protected:
	// 상태 별 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float DefaultSpeed = 600; // 기본 속도

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float DrinkingSpeed = 300; // 아이템 사용 중 속도

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float TargetingSpeed = 300; // 타겟팅 중 속도

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float TargetingRunSpeed = 500; // 타겟팅 중 달리기 속도

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float AttackingSpeed = 200; // 공격 중 속도

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float SprintSpeed = 1000; // 전력질주 속도

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float DashSkillSpeed = 1500; // 대시 스킬용 속도 (전사 일반 스킬)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Movement")
	float DownedMoveSpeed = 100.f;	// 기절 상태 속도

	// 회전 보간 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Movement")
	float RotationInterpSpeed = 10.f;

	// 현재 회전 보간 목표
	FRotator DesiredActorRotation = FRotator::ZeroRotator;

	// 콤보/공격처럼 한 번만 회전시키고 끝낼 때 사용
	bool bHasPendingActorRotation = false;

	// 이동 속도에 영향을 주는 GAS 태그가 추가되거나 제거될 때 호출
	void BindMovementSpeedTagEvents();

	void HandleMovementSpeedTagChanged(const FGameplayTag ChangedTag, int32 NewCount);

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NightRain|PostProcess",
		meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPostProcessComponent> NightRainPostProcessComponent;
#pragma endregion

public:
	// 캐릭터 HeavyAttack 입력을 토글로 변경
	bool TryEndActiveChannelingWeaponSkill();

	UFUNCTION(Server, Reliable)
	void Server_RequestEndActiveChannelingWeaponSkill();

private:
	// 무기 스킬 이펙트 Multicast (Unreliable로 가볍게)
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayWeaponSkillInstantNiagaraEffects(const TArray<FERNWeaponSkillInstantNiagaraSpawnData>& Effects);

	void SpawnWeaponSkillInstantNiagaraEffect_Local(FERNWeaponSkillInstantNiagaraSpawnData EffectData);

	// ===== 비전투 무한 스태미나 =====
public:
	// 비전투 진입 그레이스 (감지 해제 후 이 시간 뒤 OutOfCombat 부여)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ERN|Combat")
	float OutOfCombatGraceTime = 5.0f;

	// 적/AIController가 호출 - 시야 감지 시작 (서버)
	void NotifyDetectedBy(AERNEnemyCharacter* Enemy);

	// 적/AIController가 호출 - 시야 감지 상실 또는 적 사망/소멸 (서버)
	void NotifyLostBy(AERNEnemyCharacter* Enemy);

protected:
	// 나를 시야로 감지 중인 적 목록 (서버 전용)
	TSet<TWeakObjectPtr<AERNEnemyCharacter>> DetectingEnemies;

	FTimerHandle OutOfCombatTimerHandle;

	// 전투 해제 거리 — 감지 중인 모든 적이 이 거리보다 멀면 전투 해제 (피격으로만 감지된 원거리 케이스 대응)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Combat", meta = (ClampMin = "0.0"))
	float CombatReleaseDistance = 4000.f;

	FTimerHandle CombatLeashTimerHandle;

	// 주기적으로 호출 - 너무 멀어진(또는 무효) 적을 DetectingEnemies에서 제거 (서버)
	void CheckCombatLeash();

	// 그레이스 타이머 만료 시 호출 - OutOfCombat 태그 부여
	void EnterOutOfCombat();

	// OutOfCombat 태그 즉시 제거 + 타이머 취소
	void ExitOutOfCombat();

	// ===== 머리 조명 토글 =====
public:
	// L키로 바인딩할 InputAction
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Input")
	TObjectPtr<UInputAction> ToggleLightAction;

	// 켤 때 재생할 Niagara
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|HeadLight")
	TObjectPtr<UNiagaraSystem> HeadLightFXSystem;

protected:
	// 조명 ON/OFF 상태 (모든 클라 동기화)
	UPROPERTY(ReplicatedUsing=OnRep_HeadLightOn, BlueprintReadOnly, Category="ERN|HeadLight")
	bool bHeadLightOn = false;

	UFUNCTION()
	void OnRep_HeadLightOn();

	// L 입력 → 서버 요청
	void ToggleLight();

	UFUNCTION(Server, Reliable)
	void Server_ToggleLight();

	// 라이트/Niagara 가시성 적용 (서버·클라 공통)
	void ApplyHeadLightState();

#pragma region PlayerLifeState
	// ===== 플레이어 생명 상태 관련 구현 =====
public:
	UFUNCTION(BlueprintPure, Category = "ERN|LifeState")
	EERNPlayerLifeState GetLifeState() const { return LifeState; }

	UFUNCTION(BlueprintPure, Category = "ERN|LifeState")
	bool IsDowned() const { return LifeState == EERNPlayerLifeState::Downed; }
	
	UFUNCTION(BlueprintPure, Category = "ERN|LifeState")
	bool IsAlive() const { return LifeState == EERNPlayerLifeState::Alive; }

	// DownedComponent Getter
	FORCEINLINE UERNDownedComponent* GetDownedComponent() const
	{
		return DownedComponent;
	}
	
	// 플레이어 자살 명령어
	UFUNCTION(Exec)
	void KillSelf();

	UFUNCTION(Server, Reliable)
	void Server_DebugKillSelf();
	
protected:
	UPROPERTY(ReplicatedUsing = OnRep_LifeState, BlueprintReadOnly, Category = "ERN|LifeState")
	EERNPlayerLifeState LifeState = EERNPlayerLifeState::Alive;

	// 생명상태 Replicate
	UFUNCTION()
	void OnRep_LifeState();

	// 생명상태 설정
	void SetLifeState(EERNPlayerLifeState NewState);
	// 생명상태 태그 갱신
	void RefreshLifeStateTags();
	
	// 죽었을 때 처리함수 (플레이어 전용)
	virtual void OnDeath() override;

	// 쓰러지는 상태
	void EnterCollapsingState();
	// 쓰러지기가 끝난 상태
	void FinishCollapsingState();
	// 기절 상태 진입
	void EnterDownedState();

	FTimerHandle CollapseTimerHandle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LifeState")
	float CollapseFallbackDuration = 1.f;

	// 최종 자기장 수렴 이후 GameOver 판정을 GameState에 요청
	void RequestGameOverCheck() const;
	
#pragma endregion PlayerLifeState
	
#pragma region PlayerRevive
	
	// ===== 부활 처리 ======
public:
	// 기절 상태 공격 처리 적용 여부
	UFUNCTION(BlueprintCallable, Category="ERN|Downed")
	bool bApplyReviveHit(AController* Reviver);

	static bool TryApplyReviveHit(AActor* HitActor, AController* Reviver);
	
protected:
	// 부활 공격 조건 검사
	bool CanApplyReviveHit(AController* Reviver) const;
	
	// 부활
	void CompleteRevive();

	// 부활 몽타주 재생 상태 진입
	void EnterRevivingState();

	// 부활 몽타주 종료 후 Alive 복귀
	void FinishRevivingState();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayReviveMontage();

	// UI 초기화
	void InitializeDownedStatusWidget();
	
	// 부활 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LifeState|Revive")
	TObjectPtr<UAnimMontage> ReviveMontage;

	// 부활 fallback 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LifeState|Revive")
	float ReviveFallbackDuration = 1.f;

	// fallback 적용을 위한 부활 타이머
	FTimerHandle ReviveTimerHandle;
	
	// 부활 시 적용할 체력 비율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LifeState", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ReviveHealthRatio = 0.5f;
	
	// 기절 UI
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ERN|UI", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UWidgetComponent> DownedStatusWidgetComponent;
	
#pragma endregion PlayerRevive
	
#pragma region PlayerRespawn
	
public:
	// 리스폰 시작 진입점
	UFUNCTION(BlueprintCallable, Category="ERN|Respawn")
	void CompleteDownedCountdown();

	// 남은 리스폰 시간을 초 단위로 반환하는 함수
	UFUNCTION(BlueprintPure, Category="ERN|Respawn")
	float GetDownedRespawnRemainingTime() const;

	// UI용 비율
	UFUNCTION(BlueprintPure, Category="ERN|Respawn")
	float GetDownedRespawnRemainingPercent() const;
	
	// RespawnCountDown을 보여줄지 여부 확인 UI에서 활용
	UFUNCTION(BlueprintPure, Category = "ERN|Respawn")
	bool ShouldShowDownedRespawnCountdown() const;
	
protected:
	// 죽음 몽타주 끝난 후 실행
	void FinishRespawnDeathMontage();

	// 멀티캐스트 몽타주 재생
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayRespawnDeathMontage();
	
	// 캐릭터 LifeState를 리스폰 상태로 바꿔줌
	void EnterRespawningState();
	
	// 리스폰 프리로드 시작 (충돌값 조절 필요?)
	void BeginRespawnPreload(const FTransform& RespawnTransform);
	
	// 프리로드 준비상태 확인 (타이머 돌면서 지속 확인)
	void CheckRespawnPreloadReady();
	
	// 실제 리스폰 적용
	void FinishRespawningState();
	
	// 리스폰 프리로드 정리
	void CleanupRespawnPreload();
	
	// 리스폰 위치 계산
	FTransform ResolveRespawnTransform() const;
	
	// FindNightRainZoneManager에서 리스폰 위치를 받아오기 때문에 먼저 찾아야 함
	ANightRainZoneManager* FindNightRainZoneManager() const;

	// 리스폰 전 죽는 모션 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LifeState|Respawn")
	TObjectPtr<UAnimMontage> RespawnDeathMontage;

	// 죽음 몽타주 fallback시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LifeState|Respawn", meta=(ClampMin="0.01"))
	float RespawnDeathFallbackDuration = 1.f;
	
	// 자동 리스폰까지 대기 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LifeState|Respawn", meta=(ClampMin="0.0"))
	float DownedRespawnCountdownDuration = 30.f;

	// 리스폰 위치 주변을 미리 로딩할 반경
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LifeState|Respawn", meta=(ClampMin="1000.0"))
	float RespawnPreloadRadius = 20000.f;

	// 월드 파티션 로딩 완료 여부 확인 주기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LifeState|Respawn", meta=(ClampMin="0.01"))
	float RespawnPreloadCheckInterval = 0.1f;

	// 프리로드 최대 대기 시간: 이 시간이 지나면 프리로드가 적용되지 않아도 리스폰 진행
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LifeState|Respawn", meta=(ClampMin="0.1"))
	float RespawnPreloadTimeout = 5.f;

	// 캐릭터 텔레포트 후 프리로드 스트리밍 소스 유지 시간 (텔레포트 후 스트리밍 소스가 사라져야 안정적이기 때문)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LifeState|Respawn", meta=(ClampMin="0.0"))
	float RespawnPostTeleportPreloadKeepTime = 1.f;

	// 리스폰 후 적용할 체력 비율 (1.f == 최대 체력)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LifeState|Respawn", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RespawnHealthRatio = 1.f;

	// 리스폰 가능 여부 확인 함수
	UFUNCTION(BlueprintPure, Category = "ERN|Respawn")
	bool CanAutoRespawnFromDowned() const;
	
	// 죽음 몽타주 적용 타이머
	FTimerHandle RespawnDeathMontageTimerHandle;
	// Downed 상태에서 리스폰 카운트다운 타이머
	FTimerHandle DownedRespawnTimerHandle;
	// 리스폰 위치 로딩 완료 체크용 (CheckRespawnPreloadReady 함수 반복 실행)
	FTimerHandle RespawnPreloadCheckTimerHandle;
	// 텔레포트 후 일정 시간 뒤 프리로드 정리까지 대기용 타이머
	FTimerHandle RespawnPreloadCleanupTimerHandle;

	// 리스폰 목적지 저장 변수
	FTransform PendingRespawnTransform;
	// 프리로드 대기 시간이 얼마나 지났는지 기록하는 값
	float RespawnPreloadElapsedTime = 0.f;
	
	// 리스폰이 실행될 서버 기준 종료 시간
	UPROPERTY(Replicated, BlueprintReadOnly, Category="ERN|LifeState|Respawn")
	float DownedRespawnEndServerTime = 0.f;

	// 클라이언트에서도 서버 기준 현재 시간을 얻기 위한 helper
	float GetSyncedServerWorldTimeSeconds() const;
	
#pragma endregion PlayerRespawn
};
