// Copyright Epic Games, Inc. All Rights Reserved.

#include "Shop/Components/ERNShopComponent.h"
#include "Shop/Provider/ERNShopDataProvider.h"
#include "Shop/Provider/ERNDummyShopProvider.h"
#include "Shop/Provider/ERNNetworkShopProvider.h"
#include "Shop/Provider/ERNDataTableShopProvider.h" // [Fix] 추가
#include "Core/ERNGameInstance.h"
#include "Inventory/Components/ERNInventoryComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UERNShopComponent::UERNShopComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UERNShopComponent::BeginPlay()
{
    Super::BeginPlay();
    AcquireProvider();
}

void UERNShopComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // 필요 시 리플리케이션 프로퍼티 추가
}

// ===== Provider 획득 =====

void UERNShopComponent::AcquireProvider()
{
    if (UERNGameInstance* GI = Cast<UERNGameInstance>(GetWorld()->GetGameInstance()))
    {
        DataProvider = GI->GetShopDataProvider().GetInterface();

        if (DataProvider)
        {
            // Provider 이벤트 바인딩
            // 각 Provider 클래스로 캐스팅하여 델리게이트 바인딩 진행
            if (UERNDummyShopProvider* DummyProvider = Cast<UERNDummyShopProvider>(
                    GI->GetShopDataProviderObject()))
            {
                DummyProvider->OnShopDataReceived.AddDynamic(this, &UERNShopComponent::OnDataReceived);
                DummyProvider->OnPurchaseComplete.AddDynamic(this, &UERNShopComponent::OnPurchaseComplete);
            }
            else if (UERNNetworkShopProvider* NetworkProvider = Cast<UERNNetworkShopProvider>(
                    GI->GetShopDataProviderObject()))
            {
                NetworkProvider->OnShopDataReceived.AddDynamic(this, &UERNShopComponent::OnDataReceived);
                NetworkProvider->OnPurchaseComplete.AddDynamic(this, &UERNShopComponent::OnPurchaseComplete);
            }
            // [Fix] DataTable Provider 바인딩 추가 (Phase 4 대비)
            else if (UERNDataTableShopProvider* DataTableProvider = Cast<UERNDataTableShopProvider>(
                    GI->GetShopDataProviderObject()))
            {
                DataTableProvider->OnShopDataReceived.AddDynamic(this, &UERNShopComponent::OnDataReceived);
                DataTableProvider->OnPurchaseComplete.AddDynamic(this, &UERNShopComponent::OnPurchaseComplete);
            }

            UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent] Provider 획득 성공"));
        }
        else
        {
            UE_LOG(LogShopProvider, Warning, TEXT("[ShopComponent] Provider를 찾을 수 없음"));
        }
    }
}

// ===== 공개 API =====

void UERNShopComponent::OpenShop(EShopType ShopType, AActor* TargetNPC)
{
    CurrentTargetNPC = TargetNPC;
    bIsShopOpen = true;

    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent] 상점 열기: %d (Authority: %s)"),
        (int32)ShopType, GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));

    // 서버 RPC 호출 (호스트일 경우 로컬에서 즉시 실행됨)
    Server_RequestShopData(ShopType, TargetNPC);
}

void UERNShopComponent::CloseShop()
{
    bIsShopOpen = false;
    CurrentShopType = EShopType::None;
    CurrentTargetNPC = nullptr;
    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent] 상점 닫기"));
}

void UERNShopComponent::TryPurchaseItem(FName ItemID, int32 Quantity)
{
    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent] 구매 시도: %s x%d"), *ItemID.ToString(), Quantity);

    // 보안 및 검증을 위해 필수 데이터와 타겟 NPC만 담아 서버로 위임 (Proxy Pattern)
    // 호스트 클라이언트도 RPC 함수를 호출하여 동일한 서버 검증 파이프라인을 타게 함
    Server_RequestPurchase(ItemID, Quantity, CurrentTargetNPC);
}

TArray<FERNShopItemData> UERNShopComponent::GetCurrentShopItems() const
{
    return CurrentShopData.Items;
}

// ===== Server RPC =====

void UERNShopComponent::Server_RequestShopData_Implementation(EShopType ShopType, AActor* TargetNPC)
{
    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent:Server] 클라이언트로부터 데이터 요청 수신: %d"),
        (int32)ShopType);

    // 서버 측에서 Provider 호출
    if (DataProvider)
    {
        FERNShopInventory Data = IERNShopDataProvider::Execute_GetCachedShopData(
            Cast<UObject>(DataProvider), ShopType);

        if (Data.Items.Num() == 0)
        {
            IERNShopDataProvider::Execute_RequestShopData(
                Cast<UObject>(DataProvider), ShopType);

            Data = IERNShopDataProvider::Execute_GetCachedShopData(
                Cast<UObject>(DataProvider), ShopType);
        }

        // 디버그: 전달되는 ItemID 확인
        for (const FERNShopItemData& Item : Data.Items)
        {
            UE_LOG(LogShopProvider, Warning, TEXT("[ShopComponent:Server] ★ 전달 ItemID: %s"), *Item.ItemID.ToString());
        }

        // 가져온 데이터를 클라이언트에게 전달
        Client_ReceiveShopData(Data);
    }
}

void UERNShopComponent::Server_RequestPurchase_Implementation(FName ItemID, int32 Quantity, AActor* TargetNPC)
{
    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent:Server] 구매 요청 수신: %s x%d"),
        *ItemID.ToString(), Quantity);

    // 1. 보안 검증: Target NPC 유효성 및 플레이어와의 거리 체크 (핵 방지)
    ACharacter* PlayerChar = Cast<ACharacter>(GetOwner());
    if (!PlayerChar || !TargetNPC)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[ShopComponent:Server] 구매 불가 - 유효하지 않은 구매자 또는 TargetNPC"));
        return;
    }

    float Distance = FVector::Dist(PlayerChar->GetActorLocation(), TargetNPC->GetActorLocation());
    if (Distance > 500.f) // 허용 오차 거리 (추후 매크로나 설정값으로 분리 권장)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[ShopComponent:Server] 구매 불가 - NPC와 너무 멉니다! (거리: %f)"), Distance);
        return;
    }

    // 2. 서버 측에서 안전하게 트랜잭션 객체 생성
    if (DataProvider)
    {
        FERNShopTransaction ServerTx;
        ServerTx.ShopType = CurrentShopType; // 위임 패턴의 핵심: 현재 열린 상점 명시
        ServerTx.ItemID = ItemID;
        ServerTx.Quantity = Quantity;
        ServerTx.Timestamp = GetWorld()->GetTimeSeconds();

        // 3. Provider로 검증 및 결제/차감 위임
        IERNShopDataProvider::Execute_RequestPurchase(
            Cast<UObject>(DataProvider), ServerTx);
        // 결과는 OnPurchaseComplete 콜백으로 전달됨
    }
}

// ===== Client RPC =====

void UERNShopComponent::Client_ReceiveShopData_Implementation(FERNShopInventory ShopData)
{
    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent:Client] 상점 데이터 수신. 아이템 수: %d"),
        ShopData.Items.Num());

    CurrentShopData = ShopData;

    // Provider 캐시에도 저장 (네트워크 Provider용, 현재 Dummy에서는 무시됨)
    if (DataProvider)
    {
        IERNShopDataProvider::Execute_HandleReceivedData(
            Cast<UObject>(DataProvider), ShopData);
    }

    OnShopUIUpdate.Broadcast();
}

void UERNShopComponent::Client_PurchaseResult_Implementation(const FERNShopTransaction& Transaction)
{
    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent:Client] 구매 결과 수신: %s (결과: %d)"),
        *Transaction.ItemID.ToString(), static_cast<int32>(Transaction.Result));

    OnPurchaseResult.Broadcast(Transaction);
    OnShopUIUpdate.Broadcast();
}

// ===== 내부 콜백 =====

void UERNShopComponent::OnDataReceived(const FERNShopInventory& ShopData)
{
    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent] OnDataReceived. 아이템 수: %d"),
        ShopData.Items.Num());

    // 서버 사이드에서 이 이벤트가 발생했다면 특정 클라이언트에게 보내주어야 하지만
    // DummyProvider를 단일 구조로 쓰기 때문에 Server_RequestShopData_Implementation에서
    // 이미 Client_ReceiveShopData를 호출하여 데이터를 내려보냅니다.
    // 이 콜백은 현재 호스트의 로컬 이벤트를 위해 유지합니다.
}

void UERNShopComponent::OnPurchaseComplete(const FERNShopTransaction& Transaction)
{
    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent] OnPurchaseComplete: %s (결과: %d)"),
        *Transaction.ItemID.ToString(), static_cast<int32>(Transaction.Result));

    // 이 콜백은 서버 측에서 Provider가 검증을 끝낸 후 발생합니다.
    if (GetOwner()->HasAuthority())
    {
        // 1. 서버 측에서 구매 성공 시 인벤토리에 아이템 추가 (안전한 처리)
        if (Transaction.Result == EERNTransactionResult::Success)
        {
            if (UERNInventoryComponent* InvComp = GetOwner()->FindComponentByClass<UERNInventoryComponent>())
            {
                //InvComp->Server_AddItem(Transaction.ItemID, Transaction.Quantity);
            }
        }

        // 2. 결과를 클라이언트에게 전송 (Client RPC)
        // 호스트 플레이어(Listen Server)인 경우 Client RPC가 즉시 로컬에서 실행되어 UI를 갱신합니다.
        Client_PurchaseResult(Transaction);
    }
}
