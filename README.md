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

<br>

## 📖 프로젝트 소개

**ProjectERN(Ashen Oath)**은 **Elden Ring: Nightreign**을 레퍼런스로 제작한 3인 협동 소울라이크 액션 RPG입니다.

플레이어는 제한된 시간 동안 월드를 탐험하며 장비와 아이템을 획득하고, 성장한 뒤 보스를 공략하는 게임 루프를 기반으로 합니다. 또한 **검은비(자기장) 시스템**과 **인스턴스 던전**을 추가하여 협동 플레이 중심의 새로운 게임 경험을 구현했습니다.

저는 프로젝트에서 **Gameplay Programmer**를 담당하여 플레이어와 관련된 Gameplay 시스템 및 아이템 시스템을 개발했습니다.

<br>

## 👨‍👨‍👧‍👦 담당 역할

| 이름 | 담당 기능 |
|------|-----------|
| **유지원 (팀장)** | AI 시스템, 네트워크 시스템 |
| **백원석** | 플레이어 시스템, 전투 시스템 |
| **장대한** | 록온 시스템, 상호작용 시스템, 인벤토리/장비 시스템, 아이템 시스템 |
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

### 🟠 Gameplay System

Gameplay Framework를 기반으로 플레이어 시스템을 컴포넌트 단위로 분리하여 유지보수성과 확장성을 고려한 구조를 설계했습니다.

#### 주요 기능

- Lock-On System
- Interaction System
- Inventory System
- Equipment System
- ItemManagerSubsystem
- Consumable Item
- PSO Prewarm

---

### 🟠 Lock-On & Interaction

플레이어의 조작 경험을 향상시키기 위한 Lock-On 및 Interaction 시스템을 구현했습니다.

#### Lock-On

- Detection Sphere 기반 대상 탐색
- Dot Product를 이용한 시야각 판별
- Line Trace 기반 장애물 검사
- Lock-On State 관리
- Lost Grace Time 적용

#### Interaction

- Interaction Sphere 기반 상호작용 감지
- Interface 기반 상호작용 시스템 설계
- Local / Server 실행 분리
- 확장 가능한 구조 설계

---

### 🟠 Inventory & Equipment

멀티플레이 환경을 고려한 서버 권한 기반 인벤토리 및 장비 시스템을 구현했습니다.

#### 주요 기능

- FastArraySerializer 기반 Delta Replication
- Delegate 기반 UI 갱신
- 서버 권한 장비 관리
- Sync / Async Asset Loading

---

### 🟠 Item System

Data Driven 구조를 기반으로 아이템 관리 시스템을 구현했습니다.

#### 주요 기능

- ItemManagerSubsystem
- DataTable + DataAsset 기반 관리
- Soft Reference 기반 리소스 관리
- Consumable Item 구현

---

### 🟠 Optimization

게임 플레이 중 발생하는 렌더링 지연을 줄이기 위해 PSO Prewarm을 적용했습니다.

#### 주요 기능

- PSO Prewarm
- Shader Warm-up
- 최초 렌더링 Hitch 감소

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

- 🎥 [Play Video](플레이 영상 링크)
- 🎥 [Technical Video](기술 영상 링크)
- 📄 [Portfolio](포트폴리오 링크)
- 📚 [Notion](https://tree-limpet-ff9.notion.site/ProjectERN-d235690eaca682e89f8901a9293d08be)
