# graph.html 구조 요약

## 1. HTML 구조

### 헤더 (Header)

- **제목**: "🌐 Ping Monitor v2.6 Dashboard"
- **부제**: "Real-time Network Monitoring with Outage Management"
- ~~최소화 버튼 제거됨~~

### 탭 네비게이션 (Tab Navigation)

```
📊 대시보드 | 🔔 알림 기록 | 🔥 장애 현황
```

---

## 2. 대시보드 탭 (Dashboard Tab)

### 2.1 통합 컨트롤 바 (Unified Control Bar)

**시간 정보:**

- 현재 시간 (HH:MM:SS)
- 현재 날짜 (YYYY-MM-DD)
- 모니터링 시간 (uptime)

**컨트롤 버튼:**

- 일시정지/시작 버튼
- 새로고침 버튼

**통계:**

- 전체 IP 개수
- 온라인 개수 (초록색)
- 오프라인 개수 (빨간색)
- 평균 지연시간

### 2.2 IP 비교 타임라인 (Comparison Chart)

```html
<div class="card" id="comparisonChartCard">
  <div class="card-header">
    <div class="card-title">IP 비교 타임라인</div>
    <button>⚙️ 설정</button>
    <button>▲ 최소화</button>
  </div>
  <div class="collapsible-content">
    <canvas id="comparisonChart"></canvas>
  </div>
</div>
```

**기능:**

- Chart.js 라인 차트로 여러 IP의 지연시간을 실시간 비교
- 설정 버튼: IP 선택, 그룹별 임계값, IP 그룹 관리
- 최소화 버튼: 차트만 숨기고 제목 유지 (애니메이션)

**차트 설정 모달:**

1. **📊 차트 설정 탭**
   - IP 체크박스 리스트
   - 전체 선택/해제

2. **⏱️ 그룹별 임계값 탭**
   - 그룹별 경고/위험 임계값 설정
   - 기본값: 경고 100ms, 위험 200ms

3. **📋 IP 그룹 관리 탭**
   - IP를 그룹으로 분류
   - 그룹 추가/삭제/수정

### 2.3 최소화된 IP 영역 (Minimized Cards Area)

```html
<div id="minimizedCardsArea">
  <div class="minimized-area-header">
    <span>📦 최소화된 IP</span>
    <span id="minimizedCount">0</span>
    <button>▲ 최소화</button>
  </div>
  <div class="collapsible-content">
    <div id="minimizedCardsContainer"></div>
  </div>
</div>
```

**기능:**

- 최소화된 IP 카드를 작은 아이콘으로 표시
- 클릭하면 원래 크기로 복원
- 최소화 버튼: 아이콘들만 숨기고 제목 유지

### 2.4 IP 카드 그리드 (IP Cards Container)

```html
<div id="ipCardsContainer" class="ip-grid"></div>
```

**각 IP 카드 구조 (동적 생성):**

```
┌─────────────────────────────────┐
│ 🟢/🔴 [IP 이름]          ⚙️ ▼  │
│ IP: xxx.xxx.xxx.xxx             │
│ 지연: XXX ms                    │
│ 성공률: XX%                     │
│ 연속 실패: X회                  │
│ ━━━━━━━━━━━━━━━━━━━━━━━       │ (미니 차트)
└─────────────────────────────────┘
```

**카드 기능:**

- 온라인/오프라인 상태 표시
- 실시간 지연시간 차트 (Chart.js)
- 설정 버튼: 우선순위/그룹 변경
- 최소화 버튼: 카드를 작은 아이콘으로 축소

---

## 3. 알림 기록 탭 (Notifications Tab)

### 3.1 통계 (Stats Grid)

- 전체 알림 개수
- 타임아웃 알림 (빨간색)
- 복구 알림 (초록색)

### 3.2 필터 바 (Filter Bar)

- 타입 필터: 전체/타임아웃/복구
- 날짜 필터: 오늘/어제/최근 7일/최근 30일/전체

### 3.3 알림 테이블

```
시간 | 타입 | IP 이름 | IP 주소 | 날짜
```

---

## 4. 장애 현황 탭 (Outages Tab)

### 4.1 통계

- 전체 장애 개수
- 진행 중 장애 (빨간색)
- 복구 완료 장애 (초록색)
- 평균 장애 시간

### 4.2 필터 바

- 상태: 전체/진행중/복구완료
- 날짜: 오늘/어제/최근 7일/최근 30일/전체

### 4.3 장애 테이블

```
시작 시간 | IP 이름 | IP 주소 | 상태 | 지속 시간 | 종료 시간
```

---

## 5. JavaScript 주요 기능

### 5.1 전역 변수

```javascript
let comparisonChart = null; // 비교 차트 인스턴스
let individualCharts = {}; // 개별 IP 차트들
let visibleIPs = new Set(); // 표시 중인 IP 목록
let ipGroups = {}; // IP 그룹 매핑
let groupThresholds = {}; // 그룹별 임계값
let isRunning = true; // 일시정지/시작 상태
let updateInterval = null; // 데이터 업데이트 인터벌
let timeInterval = null; // 시간 업데이트 인터벌
```

### 5.2 데이터 로딩

```javascript
async function loadData() {
  // /data/ping_data.json 불러오기
  // IP 카드 렌더링
  // 차트 업데이트
}
```

### 5.3 차트 초기화

```javascript
function initComparisonChart() {
  // Chart.js 라인 차트 생성
  // 실시간 데이터 업데이트
}

function createIndividualChart(canvasId, targetData) {
  // 각 IP 카드의 미니 차트 생성
}
```

### 5.4 최소화 기능

```javascript
// IP 비교 타임라인 최소화
timelineMinimizeBtn.addEventListener("click", () => {
  timelineContent.classList.toggle("collapsed");
  // localStorage에 상태 저장
});

// 최소화된 IP 영역 최소화
minimizedAreaMinimizeBtn.addEventListener("click", () => {
  minimizedAreaContent.classList.toggle("collapsed");
  // localStorage에 상태 저장
});
```

### 5.5 알림 로딩

```javascript
async function loadNotifications() {
  // /data/notification_log.json
}
```

### 5.6 장애 로딩

```javascript
async function loadOutages() {
  // /data/outage_log.json
}
```

### 5.7 설정 저장/로드

```javascript
function saveVisibleIPsState()      // 표시 IP 저장
function loadVisibleIPsState()      // 표시 IP 복원
function saveIPGroups()             // IP 그룹 저장
function loadIPGroups()             // IP 그룹 복원
function saveGroupThresholds()      // 임계값 저장
function loadGroupThresholds()      // 임계값 복원
```

---

## 6. CSS 애니메이션

### 6.1 최소화 애니메이션

```css
.collapsible-content {
  max-height: 5000px;
  transition:
    max-height 0.4s ease-in-out,
    opacity 0.4s ease-in-out;
  opacity: 1;
}

.collapsible-content.collapsed {
  max-height: 0;
  opacity: 0;
}
```

### 6.2 최소화 버튼

```css
.minimize-btn svg {
  transition: transform 0.3s ease;
}

.minimize-btn.minimized svg {
  transform: rotate(180deg);
}
```

---

## 7. 데이터 구조

### 7.1 ping_data.json

```json
[
  {
    "ip": "8.8.8.8",
    "name": "Google DNS",
    "online": 1,
    "latency": 25,
    "sent": 100,
    "received": 98,
    "lost": 2,
    "success_rate": 98.0,
    "consecutive_failures": 0,
    "avg_latency": 26,
    "min_latency": 20,
    "max_latency": 35,
    "priority": 1,
    "group": "DNS"
  }
]
```

### 7.2 notification_log.json

```json
[
  {
    "type": "timeout",
    "name": "Google DNS",
    "ip": "8.8.8.8",
    "time": "14:30:25",
    "date": "2024-02-01"
  }
]
```

### 7.3 outage_log.json

```json
[
  {
    "ip": "8.8.8.8",
    "name": "Google DNS",
    "start_time": "2024-02-01 14:30:25",
    "end_time": "2024-02-01 14:35:10",
    "duration_seconds": 285,
    "status": "복구완료"
  }
]
```

---

## 8. LocalStorage 키

```javascript
"timelineCollapsed"; // IP 비교 타임라인 최소화 상태
"minimizedAreaCollapsed"; // 최소화된 IP 영역 상태
"visibleIPs"; // 표시 중인 IP 목록
"ipGroups"; // IP 그룹 설정
"groupThresholds"; // 그룹별 임계값
"minimizedCards"; // 최소화된 카드 목록
```

---

## 9. API 엔드포인트

```
GET  /data/ping_data.json          - 실시간 핑 데이터
GET  /data/notification_log.json   - 알림 로그
GET  /data/outage_log.json         - 장애 로그
GET  /config/ping_config.ini       - 설정 파일
POST /shutdown                      - 서버 종료 (브라우저 닫을 때)
```

---

## 10. 주요 특징

1. **반응형 디자인**: 모바일/태블릿/데스크톱 대응
2. **실시간 업데이트**: 1초마다 데이터 갱신
3. **상태 유지**: localStorage로 설정 저장
4. **부드러운 애니메이션**: CSS transition 활용
5. **모듈화된 구조**: 탭별로 콘텐츠 분리
6. **차트 시각화**: Chart.js 활용
7. **필터링 기능**: 알림/장애 데이터 필터
8. **그룹 관리**: IP를 그룹으로 분류
9. **임계값 설정**: 그룹별 경고/위험 레벨
10. **최소화 기능**: 타임라인/IP 영역 개별 최소화
