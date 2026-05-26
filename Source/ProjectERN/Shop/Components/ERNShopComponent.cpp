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

void UERNShopComponent::OpenShopRandom(FName RequestShopID, EShopType ShopType, const TArray<FERNShopSlotConfig>& SlotConfigs, AActor* TargetNPC)
{
    CurrentShopID = RequestShopID;
    CurrentShopType = ShopType;
    CurrentTargetNPC = TargetNPC;
    bIsShopOpen = true;

    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent] 마스터 테이블 기반 상점 열기: %s (Authority: %s)"),
        *RequestShopID.ToString(), GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));

    Server_OpenShopRandom(RequestShopID, ShopType, SlotConfigs, TargetNPC);
}

void UERNShopComponent::OpenShop(EShopType ShopType, AActor* TargetNPC)
{
    CurrentShopType = ShopType;
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

void UERNShopComponent::TryPurchaseItem(FGuid UniqueID, int32 Quantity)
{
    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent] 구매 시도: %s x%d"), *UniqueID.ToString(), Quantity);

    // 보안 및 검증을 위해 필수 데이터와 타겟 NPC만 담아 서버로 위임 (Proxy Pattern)
    // 호스트 클라이언트도 RPC 함수를 호출하여 동일한 서버 검증 파이프라인을 타게 함
    Server_RequestPurchase(UniqueID, Quantity, CurrentTargetNPC);
}

TArray<FERNShopItemData> UERNShopComponent::GetCurrentShopItems() const
{
    return CurrentShopData.Items;
}

// ===== Server RPC =====

void UERNShopComponent::Server_OpenShopRandom_Implementation(FName RequestShopID, EShopType ShopType, const TArray<FERNShopSlotConfig>& SlotConfigs, AActor* TargetNPC)
{
    CurrentShopID = RequestShopID;
    CurrentShopType = ShopType;
    CurrentTargetNPC = TargetNPC;
    bIsShopOpen = true;

    if (DataProvider)
    {
        if (UERNDataTableShopProvider* DTProvider = Cast<UERNDataTableShopProvider>(Cast<UObject>(DataProvider)))
        {
            FERNShopInventory InventoryData;

            if (DTProvider->CachedShopData.Contains(RequestShopID))
            {
                InventoryData = DTProvider->CachedShopData[RequestShopID];
            }
            else
            {
                InventoryData = DTProvider->GenerateRandomInventory(RequestShopID, ShopType, SlotConfigs);
                DTProvider->CachedShopData.Add(RequestShopID, InventoryData);
            }

            Client_ReceiveShopData(InventoryData);
        }
    }
}

void UERNShopComponent::Server_RequestShopData_Implementation(EShopType ShopType, AActor* TargetNPC)
{
    // 서버 측 컴포넌트 상태 동기화
    CurrentShopType = ShopType;
    CurrentTargetNPC = TargetNPC;
    bIsShopOpen = true;

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

void UERNShopComponent::Server_RequestPurchase_Implementation(FGuid UniqueID, int32 Quantity, AActor* TargetNPC)
{
    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent:Server] 구매 요청 수신: %s x%d"),
        *UniqueID.ToString(), Quantity);

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
        ServerTx.ShopID = CurrentShopID;
        ServerTx.ShopType = CurrentShopType; // 위임 패턴의 핵심: 현재 열린 상점 명시
        ServerTx.UniqueID = UniqueID;
        // ItemID는 식별만으로는 필요 없으나, 추가 처리를 위해 나중에 채울 수 있음
        ServerTx.Quantity = Quantity;
        ServerTx.Timestamp = GetWorld()->GetTimeSeconds();
        ServerTx.Buyer = PlayerChar; // Buyer 세팅

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

    // 멀티플레이어 환경에서 서버 Provider의 재고 변경 브로드캐스트를 수신합니다.
    if (GetOwner()->HasAuthority())
    {
        // 현재 상점을 열고 있고, 해당 상점 타입이 갱신된 타입과 같다면
        if (bIsShopOpen && CurrentShopType == ShopData.ShopType)
        {
            // 클라이언트에게 갱신된 상점 데이터를 전송합니다.
            Client_ReceiveShopData(ShopData);
        }
    }
}

void UERNShopComponent::OnPurchaseComplete(const FERNShopTransaction& Transaction)
{
    UE_LOG(LogShopProvider, Log, TEXT("[ShopComponent] OnPurchaseComplete: %s (결과: %d)"),
        *Transaction.ItemID.ToString(), static_cast<int32>(Transaction.Result));

    // 이 콜백은 서버 측에서 Provider가 검증을 끝낸 후 발생합니다.
    if (GetOwner()->HasAuthority())
    {
        // 내 트랜잭션인지 확인
        if (Transaction.Buyer != GetOwner())
        {
            return;
        }

        // 인벤토리 아이템 추가는 Provider(RequestPurchase_Implementation)에서 이미 처리됨
        // → FInventoryList::AddItem() 직접 호출 + OnInventorySlotChanged 브로드캐스트

        // 결과를 클라이언트에게 전송 (Client RPC)
        // 호스트 플레이어(Listen Server)인 경우 Client RPC가 즉시 로컬에서 실행되어 UI를 갱신합니다.
        Client_PurchaseResult(Transaction);
    }
}
