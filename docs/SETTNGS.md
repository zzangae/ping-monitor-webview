# Ping Monitor v2.7 - 환경설정 가이드

## 개요

환경설정은 IP 비교 타임라인 카드의 ⚙️ 버튼을 클릭하여 열 수 있습니다.
모든 설정은 브라우저의 localStorage에 저장되어 페이지 새로고침 후에도 유지됩니다.

---

## 파일 구조

```
web/
└── config/
    ├── settings.css    (7.7KB)  - 환경설정 모달 스타일
    └── settings.js     (17KB)   - 환경설정 모달 로직
```

---

## 탭 구성

### 1. 📊 차트 설정

IP 비교 타임라인 차트에 표시할 IP를 선택합니다.

| 기능        | 설명                   |
| ----------- | ---------------------- |
| IP 체크박스 | 개별 IP 표시/숨김 토글 |
| 전체 선택   | 모든 IP 차트에 표시    |
| 전체 해제   | 모든 IP 차트에서 숨김  |

**localStorage 키:** `visibleIPs`

```json
[0, 1, 3, 5] // 표시할 IP 인덱스 배열
```

---

### 2. ⏱️ 시간 범위

IP 비교 타임라인 차트의 X축 시간 범위를 설정합니다.

| 옵션   | 값(초) | 설명   |
| ------ | ------ | ------ |
| 1분    | 60     | 기본값 |
| 5분    | 300    |        |
| 10분   | 600    |        |
| 30분   | 1800   |        |
| 1시간  | 3600   |        |
| 3시간  | 10800  |        |
| 5시간  | 18000  |        |
| 12시간 | 43200  |        |
| 24시간 | 86400  |        |

**localStorage 키:** `timelineRange`

```json
"3600" // 선택된 시간(초)
```

**UI 반영:**

- 타임라인 제목에 `(1분)`, `(1시간)` 등으로 표시

---

### 3. 🎯 그룹별 임계값

장애 판정 기준 시간을 그룹별로 설정합니다.
IP가 설정된 시간 동안 연속으로 다운되면 장애로 기록됩니다.

| 항목            | 기본값      | 범위      |
| --------------- | ----------- | --------- |
| 기본 임계값     | 300초 (5분) | 30~3600초 |
| 🖥️ 서버         | 180초 (3분) | 30~3600초 |
| 🌐 네트워크     | 300초 (5분) | 30~3600초 |
| 🛡️ 방화벽       | 120초 (2분) | 30~3600초 |
| 💾 데이터베이스 | 180초 (3분) | 30~3600초 |
| 🌍 웹서버       | 240초 (4분) | 30~3600초 |
| 📦 스토리지     | 300초 (5분) | 30~3600초 |
| 🔧 기타         | 300초 (5분) | 30~3600초 |

**localStorage 키:** `thresholdSettings`

```json
{
  "default": 300,
  "서버": 180,
  "네트워크": 300,
  "방화벽": 120,
  "데이터베이스": 180,
  "웹서버": 240,
  "스토리지": 300,
  "기타": 300
}
```

---

### 4. 📋 IP 그룹 관리

각 IP의 그룹과 우선순위를 설정합니다.

**그룹 종류:**

- 🖥️ 서버
- 🌐 네트워크
- 🛡️ 방화벽
- 💾 데이터베이스
- 🌍 웹서버
- 📦 스토리지
- 🔧 기타

**우선순위:** 1~5 (1이 가장 높음)

**localStorage 키:** `ipGroupSettings`

```json
{
  "8.8.8.8": {
    "group": "네트워크",
    "priority": 1
  },
  "192.168.1.1": {
    "group": "서버",
    "priority": 2
  }
}
```

---

## 저장 버튼

모달 하단의 **"💾 저장"** 버튼 클릭 시:

1. 차트 설정 (visibleIPs) 저장
2. 시간 범위 (timelineRange) 저장
3. 그룹별 임계값 (thresholdSettings) 저장
4. IP 그룹 설정 (ipGroupSettings) 저장
5. 저장 완료 메시지 표시
6. 모달 자동 닫힘

---

## JavaScript API

### 주요 함수

```javascript
// 모달 열기/닫기
openChartSettings();
closeChartSettingsModal();

// 저장 함수
saveAllSettings(); // 모든 설정 저장
saveVisibleIPsState(); // 차트 설정 저장
saveThresholdSettings(); // 임계값 저장
saveIPGroupSettings(); // IP 그룹 저장

// 로드 함수
loadVisibleIPsState(); // 차트 설정 로드
loadThresholdSettings(); // 임계값 로드 (localStorage 우선)
loadIPConfigTable(); // IP 그룹 로드 (localStorage 우선)

// 시간 범위
initTimeRangeRadios(); // 라디오 버튼 초기화
updateTimelineRangeLabel(); // 제목 라벨 업데이트
formatTimeRangeLabel(); // 초→분/시간 변환

// IP 선택
populateIPCheckboxList(); // IP 체크박스 목록 생성
selectAllIPsForChart(); // 전체 선택
deselectAllIPsForChart(); // 전체 해제

// 탭 전환
initSettingsTabEvents(); // 탭 버튼 이벤트 초기화
```

### 전역 변수 의존성

settings.js는 다음 전역 변수에 의존합니다:

```javascript
// graph.html에서 정의
let currentData = null;        // 핑 데이터
let visibleIPs = new Set();    // 표시 IP 목록
let selectedTimeRange = 60;    // 선택된 시간 범위(초)
const brightColors = [...];    // 차트 색상 배열

// 함수 의존성
updateComparisonChart()        // 차트 업데이트 함수
```

---

## CSS 클래스

### 모달 구조

```css
.settings-modal-content     /* 모달 컨테이너 (max-width: 700px) */
.settings-tabs-row          /* 탭 버튼 행 */
.settings-tab-btn           /* 탭 버튼 */
.settings-tab-btn.active    /* 활성 탭 */
.settings-tab-container     /* 탭 내용 컨테이너 */
.settings-tab-content       /* 개별 탭 내용 */
.settings-scroll-box        /* 스크롤 박스 (max-height: 350px) */
.settings-footer            /* 하단 저장 버튼 영역 */
```

### 폼 요소

```css
.settings-section           /* 설정 섹션 */
.settings-section-title     /* 섹션 제목 */
.settings-form-group        /* 폼 그룹 */
.settings-label             /* 라벨 */
.settings-input             /* 입력 필드 */
.settings-help              /* 도움말 텍스트 */
.settings-success           /* 저장 완료 메시지 */
```

### 시간 범위 라디오

```css
.time-range-radio-group     /* 3x3 그리드 */
.time-range-radio           /* 개별 라디오 항목 */
.radio-label                /* 라디오 라벨 */
```

### 테이블

```css
.threshold-table            /* 임계값 테이블 */
.ip-config-table            /* IP 설정 테이블 */
.ip-checkbox-item           /* IP 체크박스 항목 */
.ip-checkbox-label          /* 체크박스 라벨 */
.ip-checkbox-color          /* 색상 표시 박스 */
```

---

## localStorage 전체 키 목록

| 키                       | 타입   | 설명                  |
| ------------------------ | ------ | --------------------- |
| `visibleIPs`             | Array  | 표시할 IP 인덱스 배열 |
| `timelineRange`          | String | 시간 범위 (초)        |
| `thresholdSettings`      | Object | 그룹별 임계값         |
| `ipGroupSettings`        | Object | IP별 그룹/우선순위    |
| `timelineCollapsed`      | String | 타임라인 최소화 상태  |
| `minimizedAreaCollapsed` | String | 최소화 영역 상태      |
| `minimizedCards`         | Array  | 최소화된 카드 목록    |

---

## 버전 이력

### v2.7 (2025-02-02)

- 환경설정 코드 `web/config/` 폴더로 분리
- 시간 범위 선택 기능 추가 (1분~24시간)
- 타임라인 제목에 시간 범위 표시
- 공통 저장 버튼으로 통합
- localStorage 우선 로드 방식 적용

### v2.6

- 환경설정 모달 기본 구조
- 차트 설정, 그룹별 임계값, IP 그룹 관리 탭

---

**문서 버전:** v2.7  
**최종 수정일:** 2025-02-02
