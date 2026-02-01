# 8. CHANGELOG.md

[메인으로 돌아가기](../README.md)

## 문서

- [시스템 아키텍처](ARCHITECTURE.md)
- [파일 구조 및 배포](FILE_STRUCTURE.md)
- [설치 및 빌드 가이드](INSTALLATION.md)
- [사용자 가이드](USER_GUIDE.md)
- [설정 파일 상세](CONFIGURATION.md)
- [문제 해결](TROUBLESHOOTING.md)
- [버전 변경 이력](CHANGELOG.md)

---

## 📋 포함 내용

- 버전별 변경사항
- 신규 기능
- 버그 수정
- 개선사항
- 알려진 이슈

---

# v2.6 (2025-02-01)

## 🆕 주요 신규 기능

### 커스텀 알림 시스템

**Outlook 스타일 커스텀 알림 창**

- Windows 기본 balloon 알림 대체
- 화면 우하단 배치 (트레이 아이콘 가리지 않음)
- 다크 테마 (RGB 45, 45, 48)
- 부드러운 페이드 인/아웃 애니메이션 (0.4초)
- 5초 자동 닫힘 또는 클릭 시 즉시 닫기
- 최대 5개 동시 표시, 자동 정렬
- PostMessage 기반 스레드 안전 처리

**알림 유형:**
- ⚠️ 타임아웃: 연속 3회 실패 (주황색)
- ✅ 복구: 오프라인 → 온라인 (초록색)

**구현:**
```c
// notification.c/h 모듈
- InitNotificationSystem()
- ShowCustomNotification()
- ProcessShowNotification()
- NotificationWindowProc()
- CleanupNotificationSystem()
```

### 최소화 기능 개선

**개별 최소화 버튼 추가**

- 최상단 헤더 최소화 버튼 제거
- IP 비교 타임라인 개별 최소화 (차트만 숨김)
- 최소화된 IP 영역 개별 최소화 (아이콘만 숨김)
- 서서히 나타나고 숨기는 애니메이션 (0.4초 ease-in-out)
- 버튼 회전 효과 (180도 회전)
- localStorage 상태 저장 (페이지 새로고침 시 유지)

**CSS 애니메이션:**
```css
.collapsible-content {
    max-height: 5000px;
    transition: max-height 0.4s ease-in-out, 
                opacity 0.4s ease-in-out;
    opacity: 1;
}

.collapsible-content.collapsed {
    max-height: 0;
    opacity: 0;
}
```

### 배포 시스템

**build.bat 배포 기능 추가 (옵션 4)**

- `PingMonitor_v2.6_Release/` 폴더 자동 생성
- 필수 파일 자동 복사
- data 폴더 자동 생성 (.gitkeep 포함)
- Chart.js 포함
- PowerShell Compress-Archive로 ZIP 파일 자동 생성

**배포 패키지 구조:**
```
PingMonitor_v2.6_Release/
├── ping_monitor.exe
├── config/
│   ├── ping_config.ini
│   └── int_config.ini
├── data/
│   └── .gitkeep
└── web/
    ├── graph.html
    ├── chart.umd.min.js
    └── css/ (8개 파일)
```

### 장애 관리 시스템
- 5분 연속 다운 시 자동 장애 판정
- `outage_log.json`에 장애 이력 자동 기록
- 그룹별 임계값 설정 가능 (서버/네트워크/방화벽/DB/웹서버/스토리지/기타)
- 장애 시작/종료 시간, 지속 시간 추적

### 통합 설정 UI
- 대시보드 차트 설정 버튼(⚙️)에서 접근
- **Chart Settings 탭**: IP별 표시/숨김
- **Group Thresholds 탭**: 그룹별 장애 임계값 설정
- **IP Group Management 탭**: IP별 그룹 분류 및 우선순위 설정
- Empty State: 데이터 없을 때 안내 메시지 표시

### IP 그룹 및 우선순위
- 7개 그룹 지원: 서버, 네트워크, 방화벽, 데이터베이스, 웹서버, 스토리지, 기타
- 우선순위 5단계 (P1~P5)
- `ping_config.ini`의 `[IPGroups]` 섹션에 저장

### 설정 불러오기
- 트레이 메뉴에 "설정 불러오기" 추가
- 실행 중 `ping_config.ini` / `int_config.ini` 수정 후 즉시 반영
- 프로그램 재시작 불필요
- IP 추가/삭제/변경 실시간 적용

### 장애 타임라인 탭
- 대시보드에 "Outage Timeline" 탭 추가
- 장애 이력 시각적 표시
- 장애 시작/종료/지속 시간 표시
- 그룹 및 우선순위별 필터링

### 알림 최적화
- 불필요한 balloon 알림 제거
  - "설정 불러오기 완료" 제거
  - "알림 활성화" 제거
- 네트워크 타임아웃/복구 알림만 유지
- 트레이 메뉴에서 알림 On/Off 가능

---

## 🔧 버그 수정

### 컴파일 오류

**CreateFontW 링킹 에러**
- **문제:** `-lgdi32` 라이브러리 누락
- **해결:** build.bat 링킹 명령에 `-lgdi32` 추가
- **영향:** 커스텀 알림 창 폰트 생성 실패

**중복 링킹 문제**
- **문제:** `module_network.o` 중복 링킹
- **해결:** build.bat에서 중복 제거
- **영향:** "multiple definition" 컴파일 에러

### 알림 시스템

**알림창 무한 로딩**
- **문제:** 매번 CreateFontW 호출로 폰트 생성
- **해결:** 전역 폰트 캐싱 (`g_titleFont`, `g_messageFont`)
- **영향:** 알림창이 표시되지 않고 무한 로딩

**알림창 배경 흰색 표시**
- **문제:** WM_ERASEBKGND 처리 없음
- **해결:** WM_ERASEBKGND에서 배경색 직접 그리기
- **영향:** 알림창 배경이 하얗게 깜빡임

**알림 비활성화 시 창 안 닫힘**
- **문제:** CleanupNotificationSystem() 호출 누락
- **해결:** 알림 비활성화 시 CleanupNotificationSystem() 호출
- **영향:** 알림 끈 후에도 기존 알림창 남아있음

**스레드 안전성 문제**
- **문제:** 별도 스레드에서 CreateWindowExW 호출
- **해결:** PostMessage 방식으로 메인 스레드에서 창 생성
- **영향:** 간헐적 크래시, 무한 로딩

### 차트 로딩

**배포 파일에서 Chart.js 미포함**
- **문제:** build.bat 배포 시 chart.umd.min.js 복사 누락
- **해결:** 배포 스크립트에 Chart.js 복사 추가
- **영향:** 배포 패키지에서 차트 표시 안 됨

**data 폴더 미생성**
- **문제:** 배포 패키지에 data 폴더 없음
- **해결:** 빈 폴더 생성 + .gitkeep 파일 추가
- **영향:** 실행 시 파일 쓰기 실패

### 경로 문제

**exe 위치와 data 경로 불일치**
- **문제:** 복잡한 경로 감지 로직
- **해결:** PathRemoveFileSpecW 단순화
- **영향:** data 파일 생성 경로 오류

**main 폴더 감지 로직 복잡**
- **문제:** main 폴더 존재 여부 체크 로직 불필요
- **해결:** main 폴더 체크 로직 제거
- **영향:** 불필요한 복잡도, 유지보수 어려움

### Chart.js 로딩 오류
- 로컬 파일 누락 시 차트 표시 안 되는 문제 해결
- CDN fallback 로직 추가
- 오류 메시지 개선

### 빌드 오류 수정
- `STILL_ACTIVE` 중복 정의 제거
- `browser_monitor.h` include 누락 수정
- 컴파일 경고 제거 (`-lgdi32` 라이브러리 추가)

### 파일 체크 로직
- 필수 파일 누락 시 명확한 에러 메시지
- 선택 파일(chart.umd.min.js)과 필수 파일 구분
- 빌드 전 상세 파일 체크

### 변수 정의 오류
- `g_browserStartTime` 정의 누락 수정
- `BROWSER_STARTUP_GRACE_PERIOD` 정의 추가
- `ID_TIMER_BROWSER_CHECK` 중복 정의 제거

---

## ⚡ 아키텍처 개선

### 모듈화

**notification.c/h 분리**
- 커스텀 알림 전용 모듈
- `NotificationWindow` 구조체 도입
- `NotificationRequest` 구조체 (PostMessage 전달용)
- `outage.h/c` - 장애 관리 로직 분리
- `config_api.h/c` - 설정 파일 API 분리
- `browser_monitor.h/c` - 브라우저 모니터링 분리
- 코드 가독성 및 유지보수성 향상

**구조체 정의:**
```c
typedef struct {
    HWND hwnd;
    wchar_t title[128];
    wchar_t message[256];
    DWORD type;
    DWORD startTime;
} NotificationWindow;

typedef struct {
    wchar_t title[128];
    wchar_t message[256];
    DWORD type;
} NotificationRequest;
```

### 메모리 관리

**폰트 전역 캐싱**
```c
static HFONT g_titleFont = NULL;    // 16pt Bold
static HFONT g_messageFont = NULL;  // 14pt Normal

void InitNotificationSystem(void) {
    g_titleFont = CreateFontW(16, ..., FW_BOLD, ...);
    g_messageFont = CreateFontW(14, ..., FW_NORMAL, ...);
}
```

**NotificationRequest 동적 할당/해제**
```c
NotificationRequest* req = (NotificationRequest*)malloc(sizeof(NotificationRequest));
PostMessage(g_mainHwnd, WM_SHOW_NOTIFICATION, 0, (LPARAM)req);
// 메인 스레드에서 free(req)
```

### 스레드 안전성

**WM_SHOW_NOTIFICATION 메시지 추가**
```c
#define WM_SHOW_NOTIFICATION (WM_USER + 100)

case WM_SHOW_NOTIFICATION:
{
    NotificationRequest* req = (NotificationRequest*)lParam;
    if (req)
    {
        ProcessShowNotification(req);
        free(req);
    }
    break;
}
```

**메인 스레드 처리**
```
[Ping 스레드]
    ↓
ShowCustomNotification()
    ↓
PostMessage(WM_SHOW_NOTIFICATION)
    ↓
[메인 스레드]
    ↓
ProcessShowNotification()
    ↓
CreateWindowExW()
```

---

## 🎨 UI/UX 개선

### 최소화 버튼 위치

**IP 비교 타임라인:**
```html
<div class="card-header">
  <div class="card-title">IP 비교 타임라인</div>
  <div style="display: flex; gap: 8px;">
    <button id="chartSettingsBtn">⚙️ 설정</button>
    <button id="timelineMinimizeBtn">▲</button>
  </div>
</div>
```

**최소화된 IP 영역:**
```html
<div class="minimized-area-header">
  <span>📦 최소화된 IP</span>
  <div style="display: flex; gap: 8px;">
    <span id="minimizedCount">0</span>
    <button id="minimizedAreaMinimizeBtn">▲</button>
  </div>
</div>
```

### 애니메이션

**최소화 애니메이션:**
```css
transition: max-height 0.4s ease-in-out, 
            opacity 0.4s ease-in-out;
```

**버튼 회전:**
```css
.minimize-btn svg {
    transition: transform 0.3s ease;
}

.minimize-btn.minimized svg {
    transform: rotate(180deg);
}
```

### 버튼 스타일

```css
.minimize-btn {
    background: rgba(255, 255, 255, 0.1);
    border: 1px solid rgba(255, 255, 255, 0.2);
    color: #fff;
    padding: 6px 12px;
    border-radius: 4px;
    cursor: pointer;
    transition: all 0.2s;
}

.minimize-btn:hover {
    background: rgba(255, 255, 255, 0.2);
}
```

### CSS 구조 개선
- `css/outages.css` - 장애 UI 스타일
- `css/settings.css` - 설정 UI 스타일
- 총 8개 CSS 파일로 모듈화

### 빌드 스크립트 강화
- 프로세스 자동 체크 및 종료 (빌드 전)
- 디버그 모드 추가 (선택지 3번)
  - 콘솔 창에 모든 메시지 출력
  - `-mwindows` 플래그 제거
  - 오류 진단 용이
- 파일 체크 개선 (✓/✗/⚠ 아이콘)
- Chart.js 파일 체크 및 CDN fallback 안내

### Chart.js 로딩 개선
- CDN 직접 로드로 변경
- `chart.umd.min.js` 로컬 파일 설치 불필요
- 인터넷 연결 시 자동 로드
- 오프라인 사용 가이드 제공 (`CHART_INSTALL.md`)

---

## 📚 문서화

### 추가된 문서

**완전 문서:**
- `DOCUMENTATION_v2.6.md` - 7개 섹션 통합 문서

**개별 문서:**
- `02_ARCHITECTURE.md` - 시스템 아키텍처
- `03_FILE_STRUCTURE.md` - 파일 구조 및 배포
- `04_INSTALLATION.md` - 설치 및 빌드 가이드
- `05_USER_GUIDE.md` - 사용자 가이드
- `06_CONFIGURATION.md` - 설정 파일 상세
- `07_TROUBLESHOOTING.md` - 문제 해결
- `08_CHANGELOG.md` - 버전 변경 이력

**구조 요약:**
- `GRAPH_HTML_SUMMARY.md` - graph.html 구조 요약

---

## ⚠️ 알려진 제한사항

### Windows 전용
- Linux/macOS 미지원
- POSIX 호환 버전 없음

### IPv4만 지원
- IPv6 주소 미지원
- IPv4/IPv6 혼합 환경에서 IPv4만 모니터링

### 최대 IP 개수
- 이론상 100개
- 실제 권장: 50개 이하 (성능 고려)

### 브라우저 자동 닫기 비활성화
- 브라우저 닫아도 프로그램 계속 실행
- 수동 종료 필요 (트레이 메뉴)

### 설정 UI 저장 기능 미구현
- **현재**: 설정 UI는 읽기 전용
- **해결 방법**: `ping_config.ini` 파일 직접 수정 후 "설정 불러오기"
- **향후 계획**: v2.7에서 `/api/save_config` 엔드포인트 구현 예정

### 브라우저 자동 종료 비활성화
- **원인**: 브라우저가 새 탭으로 열릴 경우 즉시 종료되는 안정성 문제
- **현재 상태**: 타이머 비활성화
- **해결 방법**: 트레이 아이콘 우클릭 → "종료"로 수동 종료
- **향후 계획**: 안정적인 감지 로직 재구현 예정

### Chart.js CDN 의존성
- **제한**: 인터넷 연결 필요
- **해결 방법**: 로컬 파일 다운로드 (CHART_INSTALL.md 참조)
- **영향**: 오프라인 환경에서 차트 표시 안 됨

### 브라우저 멀티 윈도우
- **제한**: 브라우저가 새 탭으로 열리면 프로세스 감지 불가
- **영향**: 브라우저 자동 종료 기능 작동 안 함
- **해결 방법**: HTTP 타임아웃 방식으로 변경 검토 중

---

# v2.5 (이전 버전)

## 주요 변경 사항
- 이중 설정 파일 시스템 (ping_config.ini + int_config.ini)
- 포트 변경 기능 (트레이 메뉴)
- 브라우저 종료 감지 (/shutdown 엔드포인트)
- Chart.js 로컬 호스팅 (CDN 의존성 제거)
- 배포 패키지 자동 생성 (build.bat)
- 필수 파일 검증 기능
- 한글 경로 문제 해결 가이드

## 버그 수정
- 포트 변경 다이얼로그 버튼 응답 문제
- 한글 경로에서 파일 로드 실패 문서화
- CDN 접속 차단 시 Chart.js 로드 실패 해결

---

# v2.3
- 알림 기능 추가 (타임아웃/복구)
- 알림 로그 저장 (notification_log.json)
- 알림 설정 [Settings] 섹션

---

# v2.0
- 웹 기반 대시보드 도입
- Chart.js 그래프 시각화
- 드래그 앤 드롭 기능
- IP 필터링

---

# v1.0
- 기본 ICMP ping 모니터링
- 시스템 트레이 통합
- HTTP 서버 내장

---

**최종 업데이트:** 2025-02-01  
**현재 버전:** Ping Monitor v2.6
