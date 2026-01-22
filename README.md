# Ping Monitor - WebView2 Integrated Version

## 🎯 특징

- **단일 EXE 실행** - HTML 파일 없이 실행 파일 하나로 동작
- **직접 메시지 통신** - 파일 I/O 없이 PostWebMessageAsJson으로 실시간 통신
- **내장 브라우저** - Edge WebView2로 네이티브 성능 + 웹 UI의 유연성

```
┌──────────────────────────────────────────────────┐
│  ping_monitor.exe (단일 실행 파일)               │
│  ┌─────────────┐   PostMessage    ┌──────────┐  │
│  │  C 핑 엔진  │ ◀═════════════▶  │ WebView2 │  │
│  │  - ICMP     │   (직접 통신)    │ (내장)   │  │
│  │  - 타이머   │                  │ Chart.js │  │
│  └─────────────┘                  └──────────┘  │
└──────────────────────────────────────────────────┘
```

## 📦 파일 구조

```
ping_monitor_webview/
├── ping_monitor_webview.c      # 메인 소스 코드
├── ping_config.ini             # IP 설정 파일
├── graph.html                  # localhost
├── build.bat                   # 빌드 스크립트
├── ping_monitor.exe            # 배포 파일
└── ping_data.json
```

## 🔧 빌드 방법

### 1. 필수 요구사항

- **MinGW-w64** (GCC for Windows)
  - 다운로드: https://winlibs.com/
  - 또는 MSYS2: `pacman -S mingw-w64-x86_64-gcc`

### 2. 빌드 실행
build.bat

### 3. 실행
ping_monitor.exe

### 4. 서버 실행 (같은 폴더에서)
python -m http.server 8080

### 5. 브라우저
http://localhost:8080/graph.html

### 3. 수동 컴파일 (선택사항)
```batch
gcc -o ping_monitor.exe ping_monitor_integrated.c -lws2_32 -liphlpapi -lshlwapi -lole32 -loleaut32 -mwindows -municode -O2
```

### 실행 요구사항
- Windows 10 버전 1803 이상
- Microsoft Edge WebView2 Runtime (대부분 기본 설치됨)

## ⚙️ 설정
### ping_config.ini
```ini
# 형식: IP주소,설명
8.8.8.8,Google DNS
1.1.1.1,Cloudflare
168.126.63.1,KT DNS
192.168.1.1,Gateway
```

## 🎨 UI 기능

| 기능                | 설명                       |
| ------------------- | -------------------------- |
| 📈 실시간 그래프    | Chart.js 기반 Area Chart   |
| 🎯 IP 비교 타임라인 | 모든 IP의 지연 시간 비교   |
| 🗺️ 패킷 히트맵      | 최근 60초 패킷 상태 시각화 |
| 📊 통계             | 평균/최대/최소/손실률      |
| 🌙 다크 테마        | 눈에 편한 다크 모드        |

## 🔄 기존 버전과 비교

| 항목      | 기존 (파일 통신) | 통합 버전     |
| --------- | ---------------- | ------------- |
| 실행 파일 | EXE + HTML       | EXE만         |
| 통신 지연 | ~1초 (파일 I/O)  | 즉시 (메모리) |
| 배포      | 여러 파일        | 2개 파일      |
| 브라우저  | 별도 실행        | 내장          |

## 🛠️ 개발 노트

### WebView2 COM 인터페이스
MinGW에서 WebView2를 사용하기 위해 COM 인터페이스를 수동으로 정의:

- ICoreWebView2
- ICoreWebView2Controller
- ICoreWebView2Environment
- 콜백 핸들러들

## 📋 TODO

