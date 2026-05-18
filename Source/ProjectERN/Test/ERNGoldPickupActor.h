// 테스트용 골드 지급 액터 - 오버랩 시 1000 골드 지급

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERNGoldPickupActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * AERNGoldPickupActor
 * 
 * 테스트 전용 액터입니다.
 * 플레이어가 오버랩하면 1000 골드를 지급합니다.
 * 레벨에 배치한 뒤 플레이하면 바로 사용 가능합니다.
 */
UCLASS()
class PROJECTERN_API AERNGoldPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AERNGoldPickupActor();

protected:
	virtual void BeginPlay() override;

	// 오버랩 감지 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* OverlapSphere;

	// 시각적 표시용 메쉬 (에디터에서 보이도록)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	// 지급할 골드 양 (에디터에서 변경 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gold")
	int32 GoldAmount = 1000;

	// 반복 지급 가능 여부 (true면 떠날 후 다시 오버랩 시 재지급)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gold")
	bool bReusable = true;

	// 오버랩 콜백
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

private:
	// 이미 지급했는지 여부 (bReusable이 false일 때 사용)
	bool bAlreadyUsed = false;
};
