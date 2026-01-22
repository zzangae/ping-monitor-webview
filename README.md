# 🌐 Ping Monitor WebView2 - 프로젝트 현황

## 프로젝트 개요

Windows용 실시간 네트워크 핑 모니터링 도구. C 백엔드 + HTML/JS 프론트엔드 구조.

---

## 📁 파일 구조

```
ping_monitor_webview/
├── ping_monitor_webview.c   # 메인 프로그램 (약 700줄)
├── http_server.h            # HTTP 서버 헤더
├── http_server.c            # HTTP 서버 구현 (약 280줄)
├── graph.html               # Chart.js 대시보드 (약 1,150줄)
├── ping_config.ini          # IP 설정 파일
├── build.bat                # 빌드 스크립트
└── docs/
    └── TRAY_ICON.md         # 트레이 아이콘 구조 문서
```

---

## ✅ 완료된 기능

### 1. 핵심 기능

| 기능             | 설명                                |
| ---------------- | ----------------------------------- |
| ICMP 핑          | Windows ICMP API로 1초 간격 핑 실행 |
| JSON 데이터 생성 | ping_data.json 원자적 쓰기          |
| 무한 루프 모드   | 수동 중지 전까지 계속 실행          |

### 2. HTTP 서버 (내장)

| 기능           | 설명                            |
| -------------- | ------------------------------- |
| 포트           | localhost:8080                  |
| 정적 파일 서빙 | html, js, css, json, 이미지     |
| CORS 지원      | Access-Control-Allow-Origin: \* |
| 별도 스레드    | 비동기 실행                     |

### 3. 시스템 트레이 아이콘

| 기능        | 설명                           |
| ----------- | ------------------------------ |
| 우클릭 메뉴 | 시작/중지, 브라우저 열기, 종료 |
| 더블클릭    | 브라우저 자동 열기             |
| 툴팁        | 상태 표시                      |

### 4. 프론트엔드 (graph.html)

| 기능             | 설명                          |
| ---------------- | ----------------------------- |
| 실시간 그래프    | Chart.js, 애니메이션 비활성화 |
| IP 비교 타임라인 | 모든 IP 한 그래프에 표시      |
| 패킷 히트맵      | 60초 성공/실패 시각화         |
| 통계 카드        | 온라인/오프라인/평균/손실률   |
| 시작/중지 버튼   | 그래프 업데이트 일시정지/재개 |

### 5. 편의 기능

| 기능               | 설명                                        |
| ------------------ | ------------------------------------------- |
| 자동 브라우저 열기 | 실행 시 localhost:8080/graph.html 자동 오픈 |
| 이전 인스턴스 종료 | 재실행 시 기존 프로세스 자동 종료           |
| 창 숨김            | 백그라운드 실행 (트레이만 표시)             |

---

## 🐛 해결된 버그

| 버그                  | 원인                 | 해결                        |
| --------------------- | -------------------- | --------------------------- |
| ping_data.json 미생성 | g_hWebView NULL 체크 | 조건문 수정                 |
| 시간 불규칙           | 파일 읽기/쓰기 충돌  | 원자적 쓰기 (tmp → json)    |
| 그래프 춤추듯 움직임  | Chart.js 애니메이션  | animation: false            |
| 버튼 먹통             | sendCommand 미구현   | toggleMonitoring 구현       |
| winsock2.h 경고       | include 순서         | winsock2.h를 windows.h 앞에 |
| 프로세스 미종료       | HTTP 스레드 블로킹   | shutdown + TerminateThread  |

---

## 🔧 빌드 방법

### 요구사항

- Windows 10 이상
- MinGW-w64 (gcc)

### 컴파일

```batch
build.bat
```

또는 직접:

```batch
gcc -o ping_monitor.exe ping_monitor_webview.c http_server.c -lws2_32 -liphlpapi -lshlwapi -lole32 -loleaut32 -lshell32 -mwindows -municode -O2
```

### 실행

```batch
ping_monitor.exe
```

- 자동으로 브라우저 열림
- 트레이 아이콘으로 제어

---

## 🚀 다음 단계 (미구현)

| #   | 기능               | 설명                               | 난이도 |
| --- | ------------------ | ---------------------------------- | ------ |
| 2   | 자동 브라우저 열기 | ✅ 완료됨                          | -      |
| 3   | 원클릭 실행        | ✅ 완료됨 (서버 내장)              | -      |
| 4   | 🔴 알림 기능       | 타임아웃 발생 시 Windows 알림      | 중     |
| 5   | 💾 로그 저장       | CSV/JSON 로그 파일                 | 쉬움   |
| 6   | ⚙️ 설정 UI         | ping_config.ini에서 시간/옵션 설정 | 중     |
| 7   | 🎨 UI 개선         | 대시보드 디자인 업그레이드         | 중     |
| 8   | 🖼️ 커스텀 아이콘   | 트레이 아이콘 .ico 파일            | 쉬움   |
| 9   | 📊 리포트 생성     | 일별/주별 통계 리포트              | 어려움 |

---

## 📝 주요 코드 구조

### ping_monitor_webview.c

```
[상수 정의]
├── MAX_IP_COUNT, PING_TIMEOUT 등
├── 트레이 아이콘 ID (WM_TRAYICON, ID_TRAY_*)
└── HTTP 포트 (HTTP_PORT 8080)

[구조체]
├── IPTarget - IP 정보, 지연시간, 히스토리, 통계
└── TimeSettings - 시간 설정

[전역 변수]
├── g_targets[] - IP 타겟 배열
├── g_isRunning - 모니터링 상태
├── g_nid - 트레이 아이콘 데이터
└── g_exePath - 실행 파일 경로

[함수]
├── LoadConfig() - ping_config.ini 로드
├── DoPing() - ICMP 핑 실행
├── UpdateTarget() - 통계 업데이트
├── BuildJsonData() - JSON 문자열 생성
├── SendDataToWebView() - 파일 쓰기 (원자적)
├── StartMonitoring() / StopMonitoring()
├── InitTrayIcon() / RemoveTrayIcon() / ShowTrayMenu()
├── OpenBrowser() - localhost URL 열기
├── KillPreviousInstance() - 이전 프로세스 종료
└── WndProc() - 메시지 처리
```

### http_server.c

```
[함수]
├── StartHttpServer() - 서버 시작 (스레드 생성)
├── StopHttpServer() - 서버 중지
├── HttpServerThread() - accept 루프
├── HandleClient() - 요청 파싱 및 응답
├── GetMimeType() - 확장자별 MIME 타입
├── SendFile() - 파일 읽어서 전송
└── Send404() / Send500() - 에러 응답
```

### graph.html

```
[CSS]
├── CSS 변수 (다크 테마 색상)
├── 레이아웃 (헤더, 카드, 그리드)
└── 애니메이션 (로딩 스피너)

[JavaScript]
├── comparisonChart - 전체 비교 차트
├── ipCharts{} - 개별 IP 차트
├── loadData() - fetch로 JSON 로드 (1초 폴링)
├── updateUI() - DOM 업데이트
├── updateComparisonChart() - 차트 데이터 갱신
├── createIPCard() - IP 카드 HTML 생성
├── toggleMonitoring() - 시작/중지 토글
└── generatePacketBars() - 히트맵 생성
```

---

## 📋 ping_config.ini 형식

```ini
# 주석
8.8.8.8,Google DNS
1.1.1.1,Cloudflare
168.126.63.1,KT DNS
192.168.1.1,Gateway
```

---

## 📊 ping_data.json 형식

```json
{
  "running": true,
  "elapsed": 120,
  "total": 0,
  "loop": true,
  "targets": [
    {
      "ip": "8.8.8.8",
      "name": "Google DNS",
      "latency": 35,
      "online": 1,
      "total": 120,
      "success": 118,
      "min": 32,
      "max": 45,
      "avg": 35.2,
      "history": [35, 34, 36, ...]
    }
  ]
}
```

---

## 🔗 관련 문서

- `docs/TRAY_ICON.md` - 시스템 트레이 아이콘 구조 상세

---

## ⚠️ 알려진 제한사항

1. Windows 전용 (ICMP API, Win32 API 사용)
2. 관리자 권한 불필요 (IcmpSendEcho 사용)
3. HTTP 서버는 localhost만 접근 가능
4. 최대 50개 IP 모니터링 가능

---

## 📅 최종 업데이트

- 날짜: 2026-01-22
- 버전: 1.0
- 상태: 핵심 기능 완료, 추가 기능 개발 가능
