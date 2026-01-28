# 🌐 Ping Monitor WebView2 v2.5

Windows용 실시간 네트워크 핑 모니터링 도구 with 웹 대시보드

![Version](https://img.shields.io/badge/version-2.5-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

**GitHub**: https://github.com/zzangae/pings

---

## 📋 목차

- [프로젝트 개요](#-프로젝트-개요)
- [새로운 기능 (v2.5)](#-새로운-기능-v25)
- [완료된 기능](#-완료된-기능)
- [파일 구조](#-파일-구조)
- [설정 파일](#-설정-파일)
- [빌드 방법](#-빌드-방법)
- [사용 방법](#-사용-방법)
- [보안 가이드](#-보안-가이드)

---

## 🎯 프로젝트 개요

**Ping Monitor WebView2**는 C 백엔드와 HTML/JavaScript 프론트엔드로 구성된 Windows용 네트워크 모니터링 도구입니다.

### 특징
- 🚀 **실시간 모니터링**: 1초 간격 ICMP 핑
- 📊 **웹 대시보드**: Chart.js 기반 실시간 그래프
- 🔔 **알림 시스템**: 타임아웃/복구 시 풍선 알림
- 🎨 **모던 UI**: 다크 테마, 반응형 디자인
- 🔒 **이중 설정 파일**: 공개 IP + 내부 IP 분리 관리 **(v2.5 NEW)**

### 시스템 요구사항
- Windows 10 이상
- MinGW-w64 (gcc)
- 관리자 권한 불필요

---

## 🆕 새로운 기능 (v2.5)

### ⭐ 이중 설정 파일 지원

**배경**: 공개 저장소에 내부 IP 주소가 노출되는 보안 문제를 해결하기 위해 설정 파일을 분리했습니다.

#### 1. ping_config.ini (공개 IP)
```ini
[Settings]
NotificationsEnabled=1

# 공개 가능한 IP들
8.8.8.8,Google DNS
1.1.1.1,Cloudflare DNS
168.126.63.1,KT DNS
```

#### 2. int_config.ini (내부 IP, 선택)
```ini
# 내부/비공개 IP들
192.168.1.100,내부 웹서버
10.0.0.50,데이터베이스
172.16.0.10,파일서버
```

### 특징
- ✅ **자동 병합**: 두 파일의 IP를 합쳐서 모니터링
- ✅ **선택적**: int_config.ini가 없어도 정상 작동
- ✅ **.gitignore**: int_config.ini는 Git에서 자동 제외
- ✅ **예시 파일**: int_config.ini.example 제공

### 사용 방법
```bash
# 1. 예시 파일 복사
copy int_config.ini.example int_config.ini

# 2. int_config.ini 편집하여 내부 IP 추가
notepad int_config.ini

# 3. 빌드 및 실행
build.bat
```

### 로딩 과정
```
==========================================
설정 파일 로딩 시작
==========================================
설정 파일 읽는 중: ping_config.ini
  ping_config.ini에서 타겟 로드 완료
------------------------------------------
설정 파일 읽는 중: int_config.ini
  int_config.ini에서 타겟 로드 완료
==========================================
설정 로드 완료: 총 10개 타겟
알림 설정: 활성화 (쿨다운: 60초, 연속실패: 3회)
==========================================
```

---

## ✅ 완료된 기능

### 1. 핵심 기능

| 기능 | 설명 |
|------|------|
| ICMP 핑 | Windows ICMP API로 1초 간격 핑 실행 |
| JSON 데이터 생성 | ping_data.json 원자적 쓰기 |
| 무한 루프 모드 | 수동 중지 전까지 계속 실행 |

### 2. HTTP 서버 (내장)

| 기능 | 설명 |
|------|------|
| 포트 | localhost:8080 |
| 정적 파일 서빙 | html, js, css, json, 이미지 |
| CORS 지원 | Access-Control-Allow-Origin: * |
| 별도 스레드 | 비동기 실행 |

### 3. 시스템 트레이 아이콘

| 기능 | 설명 |
|------|------|
| 우클릭 메뉴 | 시작/중지, 브라우저 열기, 알림 토글, 종료 |
| 더블클릭 | 브라우저 자동 열기 |
| 툴팁 | 상태 표시 |

### 4. 프론트엔드 (graph.html)

| 기능 | 설명 |
|------|------|
| 실시간 그래프 | Chart.js, 60초 히스토리 |
| IP 비교 타임라인 | 모든 IP 한 그래프에 표시 |
| 패킷 히트맵 | 60초 성공/실패 시각화 |
| 통계 카드 | 온라인/오프라인/평균/손실률 |
| 일시정지/재개 | 모니터링 제어 (시간 멈춤) |
| 새로고침 | 시간 및 그래프 완전 초기화 |

### 5. 고급 기능 (v2.3+)

| 기능 | 설명 |
|------|------|
| 알림 시스템 | 타임아웃/복구 시 풍선 알림 |
| 알림 로그 | notification_log.json에 기록 |
| 알림 설정 | 쿨다운, 연속 실패 임계값, 켜기/끄기 |
| IP 카드 최소화 | 카드를 100x100px 아이콘으로 축소 (v2.4) |
| 상세 모달 | IP별 상세 정보 팝업 (v2.4) |
| 드래그 앤 드롭 | IP 카드 순서 변경 |
| CSS 모듈화 | 6개 파일로 분리 (유지보수성) |

---

## 📁 파일 구조

```
프로젝트/
├── ping_monitor_webview.c      # 메인 프로그램 (C)
├── http_server.c               # HTTP 서버 구현
├── http_server.h               # HTTP 서버 헤더
├── build.bat                   # 빌드 스크립트
├── ping_config.ini             # 공개 IP 설정 ← Git 포함 ✅
├── int_config.ini.example      # 내부 IP 예시 ← Git 포함 ✅
├── int_config.ini              # 내부 IP 실제 ← Git 제외 ⛔
├── .gitignore                  # Git 제외 목록
├── README.md                   # 이 문서
├── graph.html                  # 웹 대시보드
└── css/
    ├── variables.css           # CSS 변수 (색상)
    ├── base.css                # 기본 스타일
    ├── components.css          # 컴포넌트
    ├── dashboard.css           # 대시보드
    ├── notifications.css       # 알림
    └── responsive.css          # 반응형
```

---

## ⚙️ 설정 파일

### 1. ping_config.ini (필수)

공개 가능한 IP 주소들을 등록합니다.

```ini
[Settings]
NotificationsEnabled=1          # 알림 활성화 (1=ON, 0=OFF)
NotificationCooldown=60         # 알림 쿨다운 (초)
NotifyOnTimeout=1               # 타임아웃 시 알림
NotifyOnRecovery=1              # 복구 시 알림
ConsecutiveFailures=3           # 연속 실패 임계값

# 국내 DNS
168.126.63.1,KT DNS
164.124.101.2,SK DNS
210.220.163.82,LG DNS

# 해외 DNS
8.8.8.8,Google DNS
1.1.1.1,Cloudflare DNS
```

### 2. int_config.ini (선택, 비공개)

**⚠️ 중요: 이 파일은 Git에 커밋하지 마세요!**

```ini
# 형식: ip,name
# 한 줄에 하나씩

192.168.1.100,내부 웹서버
192.168.1.200,데이터베이스 서버
10.0.0.50,파일 서버
172.16.0.10,개발 서버
```

#### int_config.ini 생성 방법

```bash
# Windows 명령 프롬프트
copy int_config.ini.example int_config.ini

# 편집기로 열어서 내부 IP 추가
notepad int_config.ini
```

### 3. 설정 파일 형식

#### 공통 규칙
```
✅ [Settings]          # 설정 섹션 (선택)
✅ key=value          # 설정 값
✅ ip,name            # IP 설정 (쉼표로 구분)
✅ # 주석             # 주석
✅                    # 빈 줄 (무시됨)
```

#### 로딩 순서
```
1. ping_config.ini 로드 (필수)
   └─ [Settings] 섹션 적용
   └─ IP 목록 추가
   
2. int_config.ini 로드 (선택)
   └─ [Settings] 무시 (ping_config.ini 우선)
   └─ IP 목록 추가
   
3. 결과: 두 파일의 IP가 합쳐짐
```

---

## 🔨 빌드 방법

### 요구사항
- Windows 10 이상
- MinGW-w64 (gcc)

### MinGW-w64 설치
```bash
# https://www.mingw-w64.org/ 에서 다운로드
# PATH 환경변수에 추가

# 설치 확인
gcc --version
```

### 빌드 실행

```bash
build.bat
```

### 수동 빌드 (선택)
```bash
gcc -o ping_monitor.exe ^
    ping_monitor_webview.c ^
    http_server.c ^
    -lws2_32 ^
    -liphlpapi ^
    -lshlwapi ^
    -lole32 ^
    -loleaut32 ^
    -lshell32 ^
    -mwindows ^
    -municode ^
    -O2
```

---

## 🚀 사용 방법

### 기본 사용

```bash
# 실행
ping_monitor.exe

# 자동으로:
# 1. HTTP 서버 시작 (localhost:8080)
# 2. 브라우저 열림 (graph.html)
# 3. 시스템 트레이 아이콘 표시
```

### 시스템 트레이 메뉴

```
우클릭 메뉴:
├─ 모니터링 시작
├─ 모니터링 중지
├─ 브라우저 열기
├─ 알림 켜기/끄기 ← 실시간 토글
└─ 종료

더블클릭:
└─ 브라우저 열기
```

### 웹 대시보드

```
주소: http://localhost:8080/graph.html

기능:
├─ 현재 시간/날짜 표시
├─ 모니터링 경과 시간
├─ 일시정지/시작 버튼 (시간 멈춤)
├─ 새로고침 버튼 (완전 초기화)
├─ IP 카드 드래그 앤 드롭
├─ IP 카드 최소화/복원 (− / + 버튼)
├─ 상세 모달 (⊕ 버튼)
└─ 정렬 (이름순, 지연순, 상태순)
```

---

## 🛡️ 보안 가이드

### ⚠️ 중요: int_config.ini 관리

#### .gitignore 설정

```.gitignore
# Internal configuration file (contains private IP addresses)
int_config.ini

# Build artifacts
*.exe
*.obj
*.o

# Data files
ping_data.json
notification_log.json

# Logs
*.log
```

#### Git 사용 시 체크리스트

```bash
# ✅ 커밋해야 할 파일
git add ping_config.ini          # 공개 IP
git add int_config.ini.example   # 내부 IP 예시
git add .gitignore
git add *.c *.h *.bat
git add graph.html
git add css/

# ⛔ 절대 커밋하지 말 것!
# git add int_config.ini  ← 실제 내부 IP 파일
```

#### 확인 방법

```bash
# Git 상태 확인
git status
# int_config.ini가 표시되지 않아야 함 ✅

# .gitignore 확인
cat .gitignore
# int_config.ini가 포함되어 있어야 함
```

#### 이미 커밋한 경우

```bash
# Git 캐시에서 제거
git rm --cached int_config.ini

# .gitignore에 추가
echo "int_config.ini" >> .gitignore

# 커밋
git add .gitignore
git commit -m "Remove int_config.ini from repository"
git push
```

---

## 📊 데이터 형식

### ping_data.json (자동 생성)

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
      "history": [35, 34, 36, 37, ...]
    }
  ]
}
```

### notification_log.json (자동 생성)

```json
[
  {
    "type": "timeout",
    "name": "Google DNS",
    "ip": "8.8.8.8",
    "date": "2026-01-28",
    "time": "14:32:15"
  },
  {
    "type": "recovery",
    "name": "Google DNS",
    "ip": "8.8.8.8",
    "date": "2026-01-28",
    "time": "14:33:20"
  }
]
```

---

## 🐛 해결된 버그

| 버그 | 원인 | 해결 |
|------|------|------|
| ping_data.json 미생성 | g_hWebView NULL 체크 | 조건문 수정 |
| 시간 불규칙 | 파일 읽기/쓰기 충돌 | 원자적 쓰기 (tmp → json) |
| 그래프 춤추듯 움직임 | Chart.js 애니메이션 | animation: false |
| 버튼 먹통 | sendCommand 미구현 | toggleMonitoring 구현 |
| winsock2.h 경고 | include 순서 | winsock2.h를 windows.h 앞에 |
| 프로세스 미종료 | HTTP 스레드 블로킹 | shutdown + TerminateThread |

---

## 🔧 문제 해결

### int_config.ini가 없을 때

```
설정 파일을 열 수 없습니다: int_config.ini (무시)
→ 정상입니다. 선택적 파일이므로 무시하고 계속 진행합니다.
```

### ping_config.ini가 없을 때

```
설정 파일을 열 수 없습니다: ping_config.ini (무시)
설정 로드 완료: 총 0개 타겟
→ ping_config.ini는 필수입니다! 파일을 생성하세요.
```

### 포트 8080이 사용 중일 때

```
HTTP 서버 시작 실패
→ 이전 인스턴스가 실행 중입니다.
→ 트레이 아이콘에서 "종료" 후 재실행
```

---

## 📈 버전 히스토리

### v2.5 (2026-01-28) - 현재
```
✨ 새 기능
- 이중 설정 파일 지원 (ping_config.ini + int_config.ini)
- .gitignore 추가 (보안 강화)
- int_config.ini.example 샘플 제공
- LoadConfigFromFile() 함수 분리

🔧 개선
- 설정 로딩 메시지 개선
- 빌드 스크립트 업데이트
- README.md 대폭 개선
```

### v2.4 (2026-01-22)
```
✨ 새 기능
- IP 카드 최소화/복원
- 상세 모달 팝업
- 시간 추적 (일시정지 시 멈춤)
- 새로고침 버튼 (완전 초기화)

🎨 UI 개선
- 100% 뷰포트 레이아웃
- 고정 크기 IP 카드 (350px)
- 최소화 전용 영역 (100px 그리드)
```

### v2.3 (2026-01-22)
```
✨ 새 기능
- 알림 시스템 (풍선 알림)
- 알림 로그 (notification_log.json)
- 트레이 메뉴에서 알림 토글
- CSS 모듈화 (6개 파일)

🐛 버그 수정
- Settings 섹션 파싱 오류 수정
- Winsock 초기화 순서 수정
```

### v2.0-2.2
```
✨ 새 기능
- HTTP 서버 내장
- 웹 대시보드
- 시스템 트레이 아이콘
- 드래그 앤 드롭
- 정렬 기능
```

---

## ⚠️ 알려진 제한사항

1. **플랫폼** - Windows 전용 (ICMP API, Win32 API)
2. **권한** - 관리자 권한 불필요 (IcmpSendEcho 사용)
3. **네트워크** - HTTP 서버는 localhost만 접근 가능
4. **용량** - 최대 50개 IP 모니터링 가능

---

## 📄 라이선스

MIT License - 자유롭게 사용, 수정, 배포 가능

---

## 🤝 기여하기

1. Fork the repository
2. Create your feature branch
3. **⚠️ int_config.ini는 절대 커밋하지 마세요!**
4. Commit your changes
5. Push to the branch
6. Create a Pull Request

---

## 📞 문의

- **GitHub**: https://github.com/zzangae/pings
- **Issues**: 버그 제보 및 기능 요청

---

**🎉 Ping Monitor WebView2 v2.5를 사용해주셔서 감사합니다!**
