// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/Chest.h"

#include "Character/Player/ERNPlayerController.h"
#include "Components/SphereComponent.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"

// Sets default values
AChest::AChest()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);

	// Collision
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(150.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	// Mesh
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(GetRootComponent());
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned
void AChest::BeginPlay()
{
	Super::BeginPlay();
	
	// Collision Overlap Binding
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AChest::OnSphereBeginOverlap);
	Collision->OnComponentEndOverlap.AddDynamic(this, &AChest::OnSphereEndOverlap);
}

void AChest::Interact_Implementation(APlayerController* PlayerController)
{
	if (!HasAuthority())
	{
		return;
	}
	if (!PlayerController)
	{
		return;
	}
	
	// TODO: 상호작용 시 DropTable에 따라 ItemActor 생성 후 제거
	
	if (UItemManagerSubsystem* ItemManager = GetItemManager())
	{
		ItemManager->SpawnItem(FName("TEST_STAFF"), 1, GetActorLocation() + FVector(-150.0f, 0.0f, 0.0f), GetActorRotation());
	}
}

bool AChest::CanInteract_Implementation() const
{
	// TODO: 검증 코드 작성
	return true;
}

FText AChest::GetInteractionText_Implementation() const
{
	return FText::FromString(TEXT("E키를 눌러 상자 열기"));
}

EInteractionExecutionPolicy AChest::GetInteractionExecutionPolicy_Implementation() const
{
	return EInteractionExecutionPolicy::ServerAuthority;
}

UItemManagerSubsystem* AChest::GetItemManager() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<UItemManagerSubsystem>();
		}
	}
	
	return nullptr;
}

void AChest::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (const APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(Pawn->GetController()))
		{
			PC->SetCurrentInteractable(this);
		}
	}
}

void AChest::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (const APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(Pawn->GetController()))
		{
			PC->ClearCurrentInteractable();
		}
	}
}
