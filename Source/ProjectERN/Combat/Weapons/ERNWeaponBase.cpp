// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Weapons/ERNWeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

AERNWeaponBase::AERNWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// SceneRoot 생성
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	
	// Static Mesh 무기 생성
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(SceneRoot);

	// Skeletal Mesh 무기 생성
	SkeletalWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalWeaponMesh"));
	SkeletalWeaponMesh->SetupAttachment(SceneRoot);
	
	// 리플리케이션 활성화
	bReplicates = true;
}

void AERNWeaponBase::BeginPlay()
{
	Super::BeginPlay();
}
