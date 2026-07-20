# ⚔️ ProjectERN (Ashen Oath)

> **프로젝트명**: Ashen Oath
>
> **장르**: 3인 협동 소울라이크 ARPG
>
> **개발 인원**: 5인
>
> **개발 기간**: 2026.04 ~ 2026.06 (8주)
>
> **개발 환경**: C++, Unreal Engine 5.7
>
> **레퍼런스**: Elden Ring: Nightreign
<img width="491" height="313" alt="Image" src="https://github.com/user-attachments/assets/67fc31d6-55ab-48d9-b69f-abda25e0c3d8"/>

<br>

## 📖 프로젝트 소개

<strong>ProjectERN (Ashen Oath)</strong>은 엘든 링 밤의 통치자을 레퍼런스로 제작한 3인 협동 소울라이크 액션 RPG입니다.

플레이어는 제한된 시간 동안 월드를 탐험하며 장비와 아이템을 획득하고, 성장한 뒤 보스를 공략하는 게임 루프를 기반으로 합니다. 또한 검은비(자기장) 시스템과 인스턴스 던전을 추가하여 협동 플레이 중심의 새로운 게임 경험을 구현했습니다.

<br>

## 👨‍👨‍👧‍👦 담당 역할

| 이름 | 담당 기능 |
|------|-----------|
| **유지원 (팀장)** | AI 시스템, 네트워크 시스템 |
| **백원석** | 플레이어 시스템, 전투 시스템 |
| **장대한 (나)** | 록온 시스템, 상호작용 시스템, 인벤토리/장비 시스템, 아이템 시스템 |
| **강리한** | 상점 시스템, 강화 시스템 |
| **김현진** | 월드 제작, 레벨 디자인 |

<br>

## 🛠 기술 스택

<p>
<img src="https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white"/>
<img src="https://img.shields.io/badge/Unreal_Engine_5.7-313131?style=for-the-badge&logo=unrealengine&logoColor=white"/>
</p>

<br>

## ⭐ 담당 구현

### 🟠 [Lock-On System](https://github.com/DaeHanJang/ProjectERN/blob/main/Source/ProjectERN/Components/ERNLockOnComponent.cpp)

Detection Sphere와 Dot Product를 이용하여 록온 대상을 탐색하고,
Line Trace를 통해 최종 대상을 선택하는 컴포넌트 기반 록온 시스템을 구현했습니다.

#### 주요 기능

- Detection Sphere 기반 대상 탐색
- Dot Product 기반 시야각 판별
- Line Trace 기반 장애물 검사
- Lock-On State 관리
- Lost Grace Time 적용

### 🟠 [Interaction System](https://github.com/DaeHanJang/ProjectERN/blob/c84772a745c75e05be7c8770e73bdff62a42e4c9/Source/ProjectERN/Character/Player/ProjectERNCharacter.cpp#L618)

플레이어 중심의 Interaction Sphere와 IInteractable 인터페이스를 활용하여
확장 가능한 상호작용 시스템을 구현했습니다.

#### 주요 기능

- Interaction Sphere 기반 상호작용 감지
- IInteractable Interface 설계
- Execution Policy(Local / Server) 기반 실행 분기
- 감지와 실행을 분리한 구조 설계

### 🟠 [PSO Prewarm](https://github.com/DaeHanJang/ProjectERN/blob/7f519568d3f542e50b1d640f7f7ff7954683d056/Source/ProjectERN/Subsystem/ERNCutsceneSubsystem.cpp#L216)

필드 진입 시 아이템을 사전 렌더링하여 최초 생성 시 발생하는
Shader Compilation Hitch를 완화했습니다.

#### 주요 기능

- PostLoadMap 이후 Prewarm 실행
- 아이템 사전 렌더링
- 최초 렌더링 Hitch 감소

### 🟠 [Inventory](https://github.com/DaeHanJang/ProjectERN/blob/main/Source/ProjectERN/Inventory/Data/ERNInventoryList.h) & [Equipment](https://github.com/DaeHanJang/ProjectERN/blob/main/Source/ProjectERN/Inventory/Components/ERNEquipmentComponent.cpp)

멀티플레이 환경을 고려하여 서버 권한 기반 인벤토리 및 장착 시스템을 구현했습니다.

#### Inventory

- FastArraySerializer 기반 Delta Replication
- Delegate 기반 UI 갱신
- 서버 권한 인벤토리 관리

#### Equipment

- 서버 권한 장착 처리
- Sync / Async Loading 전략 적용
- 장착 상태 관리

### 🟠 [Item System](https://github.com/DaeHanJang/ProjectERN/blob/main/Source/ProjectERN/Inventory/Item/Data/ERNItemTable.h)

Data Driven 구조를 기반으로 아이템 관리 시스템을 구현했습니다.

#### 주요 기능

- ItemManagerSubsystem
- DataTable + DataAsset 기반 관리
- Soft Reference 기반 리소스 관리
- Consumable Item 구현

<br>

## 📌 프로젝트 목표

- Gameplay Framework 기반 플레이어 시스템 설계
- 컴포넌트 기반 Gameplay 아키텍처 구축
- 서버 권한 기반 멀티플레이 시스템 구현
- Data Driven 아이템 시스템 설계
- 유지보수와 확장이 용이한 구조 설계
- Unreal Engine Gameplay Programming 역량 강화

<br>

## 🔗 Links

- 🎥 [Play Video](https://www.youtube.com/watch?v=9clhEwnciS4)
- 🎥 [Technical Video](https://youtu.be/7vt_ZvKAM10)
- 📄 [Portfolio](https://drive.google.com/file/d/1KmhRwLhF0O5aue9g8mAH8zCzrUTTcX9o/view?usp=sharing)
- 📚 [Notion](https://tree-limpet-ff9.notion.site/ProjectERN-d235690eaca682e89f8901a9293d08be)
