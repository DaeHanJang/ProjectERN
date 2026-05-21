# 📋 Daily Commit — 2026-05-21 (수)

## 🎯 주요 작업 요약

강화(Upgrade) 시스템 UI 완성도 개선 및 상점(Shop) 중복 아이템 구매 버그 수정

---

## ✅ 변경 내역

### 1. 상점 — 중복 아이템 구매 버그 수정

| 파일 | 변경 내용 |
|------|-----------|
| `ERNShopTypes.h` | `FERNShopItemData`, `FERNShopTransaction`에 `FGuid UniqueID` 필드 추가 |
| `ERNDataTableShopProvider.cpp` | 상품 캐싱 시 `FGuid::NewGuid()`로 고유 ID 발급, 구매 시 `UniqueID`로 상품 식별 |
| `ERNShopComponent.h/.cpp` | `TryPurchaseItem`, `Server_RequestPurchase` 서명을 `FGuid UniqueID` 기반으로 변경 |
| `ERNConfirmPurchaseWidget.cpp` | `TryPurchaseItem` 호출 시 `UniqueID` 전달로 변경 |

> **원인**: 동일한 `ItemID`를 가진 상품이 테이블에 중복 등록되었을 때, 항상 첫 번째 항목만 차감되는 문제  
> **해결**: 각 상품에 고유 `FGuid`를 부여하여 정확한 슬롯을 식별

---

### 2. 상점 — 구매 확인 팝업 텍스트 동기화

| 파일 | 변경 내용 |
|------|-----------|
| `ERNConfirmPurchaseWidget.h` | `ItemNameText`, `PriceText` (`UTextBlock`) BindWidget 바인딩 추가 |
| `ERNConfirmPurchaseWidget.cpp` | `SetupPopup`에서 아이템 이름(등급 색상 적용) + 가격 텍스트 자동 세팅 로직 구현 |

> 블루프린트에서 **`ItemNameText`**, **`PriceText`** 이름으로 텍스트 위젯을 배치하면 C++이 자동으로 데이터를 채워넣음

---

### 3. 강화 — 슬롯 위젯 아이콘 & 텍스트 표시 수정

| 파일 | 변경 내용 |
|------|-----------|
| `ERNUpgradeSlotWidget.cpp` | `DataAsset.IsValid()` → `!DataAsset.IsNull()`로 변경 (TSoftObjectPtr 미로드 상태에서도 경로 유효성 판단) |
| `ERNUpgradeSlotWidget.cpp` | 아이콘 로드 성공 시 `SetVisibility(SelfHitTestInvisible)` 강제 적용 |
| `ERNUpgradeSlotWidget.cpp` | `ItemNameText`에 `SetVisibility(SelfHitTestInvisible)` 추가로 텍스트 강제 표시 |
| `ERNUpgradeSlotWidget.cpp` | 아이템 이름 텍스트에 등급별 색상(`SetColorAndOpacity`) 자동 적용 |

---

### 4. 강화 — 프리뷰 위젯 데미지 표기 변경

| 파일 | 변경 내용 |
|------|-----------|
| `ERNUpgradePreviewWidget.cpp` | `"약공격: {0}"` → 숫자만 표기 (`FText::AsNumber`)로 변경 (레이아웃 내 "공격력:" 라벨과 중복 방지) |
| `ERNUpgradePreviewWidget.cpp` | `HeavyAttackText`를 `Collapsed`로 숨김 처리 (강공격 정보 미표시) |

---

### 5. 강화 — "강화 전 / 강화 후" 타이틀 텍스트 가시성 제어

| 파일 | 변경 내용 |
|------|-----------|
| `ERNUpgradeMainWidget.h` | `BeforeTitleText`, `AfterTitleText` (`UWidget`) BindWidgetOptional 추가 |
| `ERNUpgradeMainWidget.cpp` | `ClearSelection`에서 Hidden, `OnSlotSelected`에서 SelfHitTestInvisible 토글 |

> 블루프린트에서 **`BeforeTitleText`**, **`AfterTitleText`** 이름으로 위젯을 배치하면 자동 연동

---

### 6. 강화 — 강화 실행 버튼 추가

| 파일 | 변경 내용 |
|------|-----------|
| `ERNUpgradeMainWidget.h` | `UpgradeButton` (`UButton`) BindWidgetOptional 추가, `UButton` forward declare |
| `ERNUpgradeMainWidget.cpp` | `NativeConstruct`에서 `OnClicked` → `OnUpgradeConfirmed` 자동 바인딩 |
| `ERNUpgradeMainWidget.cpp` | 아이템 미선택 시 Hidden, 선택 시 Visible 가시성 토글 |

> 블루프린트에서 **`UpgradeButton`** 이름으로 버튼을 배치하면 C++이 클릭 이벤트를 강화 로직에 자동 연결

---

## 📝 블루프린트 세팅 메모 (에디터 작업 필요)

| 위젯 | 필요한 이름 | 타입 |
|------|------------|------|
| 구매 확인 팝업 — 아이템 이름 | `ItemNameText` | TextBlock |
| 구매 확인 팝업 — 가격 | `PriceText` | TextBlock |
| 강화 메인 — 강화 전 타이틀 | `BeforeTitleText` | TextBlock (또는 Widget) |
| 강화 메인 — 강화 후 타이틀 | `AfterTitleText` | TextBlock (또는 Widget) |
| 강화 메인 — 강화 실행 버튼 | `UpgradeButton` | Button |
| 모루 액터 — InteractionPromptWidget | Widget Class 할당 필요 | WidgetComponent |

---

## 🐛 알려진 이슈 / TODO

- [ ] 강화 모루 액터 `BP_UpgradeActor`에 `InteractionPromptWidget`의 Widget Class 에디터 할당 필요
- [ ] 강화 UI `ERNUpgradeActor`의 `UIManager` 게이트 연동 (현재 주석 처리 상태)
- [ ] 강화 성공/실패 연출 (`BP_OnUpgradeSuccess`) 블루프린트 이벤트 구현
- [ ] 강화 슬롯 위젯 레이아웃 미세 조정 (ZOrder, 폰트 크기 등)
