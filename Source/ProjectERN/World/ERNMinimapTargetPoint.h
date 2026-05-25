// Fill out your copyright notice in the Description page of Project Settings.
// 월드에 배치한 액터가 미니맵에 아이콘으로 표시할 대상이면
// 해당 위치에 ERNMinimapTargetPoint를 반드시 같이 배치해야 합니다

#pragma once

#include "CoreMinimal.h"
#include "Data/ERNMinimapIconTypes.h"
#include "Engine/TargetPoint.h"
#include "ERNMinimapTargetPoint.generated.h"

UCLASS()
class PROJECTERN_API AERNMinimapTargetPoint : public ATargetPoint
{
	GENERATED_BODY()

public:
	AERNMinimapTargetPoint();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual FVector GetMinimapWorldLocation() const { return GetActorLocation(); }
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_MinimapMarkerData, Category="Minimap")
	EERNMinimapIconType IconType = EERNMinimapIconType::Building;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_MinimapMarkerData, Category="Minimap")
	bool bVisibleOnMinimap = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_MinimapMarkerData, Category="Minimap", meta=(ClampMin="0.1"))
	float IconScale = 1.f;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UFUNCTION()
	virtual void OnRep_MinimapMarkerData();
	
	void RegisterToMinimapSubsystem();
	void UnregisterFromMinimapSubsystem();
	void NotifyMinimapChanged() const;

};
