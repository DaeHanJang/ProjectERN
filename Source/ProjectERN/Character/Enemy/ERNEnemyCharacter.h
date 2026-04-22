// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/ERNCharacterBase.h"
#include "ERNEnemyCharacter.generated.h"

class UWidgetComponent;
class AERNProjectileBase;
class UBoxComponent;
class UMotionWarpingComponent;

USTRUCT(BlueprintType)
struct FEnemyHitboxConfig
{
	GENERATED_BODY()

	// BoxComponent의 ComponentTag와 일치시킬 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Tag;

	// 이 히트박스가 줄 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 10.f;

	// 이 히트박스가 줄 경직력
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StaggerPower = 10.f;
};

USTRUCT(BlueprintType)
struct FEnemyProjectileConfig
{
	GENERATED_BODY()

	// AnimNotify에서 참조할 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Tag;

	// 스폰할 투사체 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AERNProjectileBase> ProjectileClass;

	// 발사 소켓 이름 (스켈레탈 메시 소켓)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SpawnSocket = TEXT("hand_r");
};


 // 드랍 아이템 정보 
USTRUCT(BlueprintType)
struct FDropItemInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DropChance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxCount = 1;
};

UCLASS(Abstract)
class PROJECTERN_API AERNEnemyCharacter : public AERNCharacterBase
{
	GENERATED_BODY()

public:
	AERNEnemyCharacter();

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// 체력바 표시 (피격/락온 시 호출)
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "UI")
	void Multicast_ShowHealthBar();

	// 체력바 숨김
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "UI")
	void Multicast_HideHealthBar();

	// 공격 몽타주 재생 (모든 클라이언트에 동기화)
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Combat")
	void Multicast_PlayAttackMontage(UAnimMontage* Montage);

protected:
	virtual void BeginPlay() override;

	// 사망 처리 오버라이드
	virtual void OnDeath() override;

	// 머리 위 체력바 위젯 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HealthBarWidget;

	// 모션 워핑 컴포넌트 (공격 시 타겟 방향으로 이동)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UMotionWarpingComponent* MotionWarpingComponent;

	// 피격 후 체력바 자동 숨김까지 대기 시간 (블루프린트에서 수정 가능)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	float HealthBarHideDelay = 15.0f;

private:
	FTimerHandle HealthBarHideTimerHandle;

public:
	// 드롭 테이블
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
	TArray<FDropItemInfo> DropTable;

	// 골드 보상
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
	int32 MinGold = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
	int32 MaxGold = 50;

	// 경험치 보상
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
	int32 ExpReward = 100;

	// 초기 스탯 (BP마다 다르게 설정, BeginPlay에서 AttributeSet에 적용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float InitialMaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float InitialMaxMana = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float InitialMaxStamina = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float InitialAttackPower = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float InitialDefense = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float InitialStaggerResistance = 10.f;

	// 근접 히트박스 설정 (태그로 구분, 태그별 데미지 값 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<FEnemyHitboxConfig> HitboxConfigs;

	// 원거리 투사체 설정 (태그로 구분)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<FEnemyProjectileConfig> ProjectileConfigs;

	// 탐지 범위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DetectionRange = 1000.0f;

	// 공격 범위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange = 200.0f;

	// 순찰 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float PatrolSpeed = 200.0f;

	// 추적 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ChaseSpeed = 450.0f;

	// 순찰 포인트 배열 (TargetPoint 액터들)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TArray<AActor*> PatrolPoints;

	// 한 번의 공격에서 중복 히트 방지
	TArray<AActor*> HitActors;

	// NotifyState End 시 호출
	void ClearHitActors() { HitActors.Empty(); }

protected:
	// 드랍 처리
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	virtual void SpawnDrops();

	// 골드 드롭
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	virtual void SpawnGold();

private:
	UFUNCTION()
	void OnHitboxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	void BindHitboxOverlaps();
};
