# 6. CONFIGURATION.md

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

- ping_config.ini 형식 및 섹션
- int_config.ini 형식
- 설정 옵션 상세 설명
- 예제 설정
- 보안 가이드

---

## 📄 ping_config.ini (필수)

### 기본 구조

```ini
[Settings]
NotificationsEnabled=1
NotificationCooldown=300
NotifyOnTimeout=1
NotifyOnRecovery=1
ConsecutiveFailures=3
OutageThreshold=300

[Targets]
8.8.8.8,Google DNS,1,DNS
1.1.1.1,Cloudflare DNS,1,DNS
208.67.222.222,OpenDNS,2,DNS
```

---

## ⚙️ [Settings] 섹션

### NotificationsEnabled

**설명:** 알림 활성화 여부

| 값 | 설명 |
|----|------|
| 0  | 비활성화 (알림 표시 안 됨) |
| 1  | 활성화 (커스텀 알림 창 표시) |

**기본값:** `1`

**예제:**
```ini
NotificationsEnabled=1  # 알림 on
```

### NotificationCooldown

**설명:** 같은 IP에 대한 알림 쿨다운 (초)

**범위:** 60 ~ 3600 (1분 ~ 1시간)

**기본값:** `300` (5분)

**예제:**
```ini
NotificationCooldown=300  # 5분 내 재알림 방지
```

**동작:**
- Google DNS에서 타임아웃 알림 발생
- 5분 이내에 다시 타임아웃 → 알림 표시 안 됨
- 5분 후 타임아웃 → 알림 표시

### NotifyOnTimeout

**설명:** 타임아웃 시 알림 표시 여부

| 값 | 설명 |
|----|------|
| 0  | 타임아웃 알림 off |
| 1  | 타임아웃 알림 on |

**기본값:** `1`

**예제:**
```ini
NotifyOnTimeout=1  # 네트워크 타임아웃 시 알림
```

### NotifyOnRecovery

**설명:** 복구 시 알림 표시 여부

| 값 | 설명 |
|----|------|
| 0  | 복구 알림 off |
| 1  | 복구 알림 on |

**기본값:** `1`

**예제:**
```ini
NotifyOnRecovery=1  # 네트워크 복구 시 알림
```

### ConsecutiveFailures

**설명:** 알림 발생 연속 실패 임계값

**범위:** 1 ~ 10

**기본값:** `3`

**예제:**
```ini
ConsecutiveFailures=3  # 연속 3회 실패 시 알림
```

**동작:**
```
실패 1회 → 알림 X
실패 2회 → 알림 X
실패 3회 → ⚠️ 알림 발생!
```

### OutageThreshold

**설명:** 장애 판정 임계값 (분)

**범위:** 1 ~ 60

**기본값:** `5` (5분)

**예제:**
```ini
OutageThreshold=5  # 5분 이상 다운 시 장애로 기록
```

**동작:**
```
오프라인 3분 → 장애 X
오프라인 5분 → 🔥 장애 기록 (outage_log.json)
```

---

## 🎯 [Targets] 섹션

### 형식

```
IP주소,이름,우선순위,그룹
```

### 필드 설명

| 필드       | 필수 | 설명                  | 예제           |
| ---------- | ---- | --------------------- | -------------- |
| IP주소     | ✅   | IPv4 주소             | 8.8.8.8        |
| 이름       | ✅   | 표시 이름 (공백 가능) | Google DNS     |
| 우선순위   | ⭕   | 1~5 (기본값: 2)       | 1              |
| 그룹       | ⭕   | 그룹 이름             | DNS            |

### 예제

```ini
[Targets]
# 형식: IP,이름,우선순위,그룹

# DNS 서버 (우선순위 높음)
8.8.8.8,Google DNS,1,DNS
1.1.1.1,Cloudflare DNS,1,DNS

# 게이트웨이 (우선순위 높음)
192.168.0.1,Gateway,1,Network

# 내부 서버 (우선순위 보통)
192.168.1.100,Web Server,2,Server
192.168.1.101,Database Server,1,Database

# 외부 서비스 (우선순위 낮음)
example.com,Example Site,3,External
```

### 우선순위

| 값 | 레벨 | 설명                    | 사용 예시                |
| :---: | :---: | :---: | :---: |
| 1  | 최고 | 매우 중요한 시스템      | 게이트웨이, 핵심 서버    |
| 2  | 높음 | 중요한 서비스           | DNS, 일반 서버           |
| 3  | 보통 | 일반 서비스             | 외부 사이트              |
| 4  | 낮음 | 덜 중요한 서비스        | 테스트 서버              |
| 5  | 최저 | 모니터링만 하는 서비스  | 백업 서버                |

### 그룹

| 그룹       | 설명                 | 임계값 설정 |
| :---: | :---: | :---: |
| DNS        | DNS 서버             | ✅          |
| Network    | 네트워크 장비        | ✅          |
| Server     | 일반 서버            | ✅          |
| Database   | 데이터베이스 서버    | ✅          |
| Web        | 웹 서버              | ✅          |
| Storage    | 스토리지             | ✅          |
| Other      | 기타                 | ✅          |

---

## 📄 int_config.ini (선택)

### 용도

- 내부 네트워크 IP 분리 관리
- 보안상 공개 설정과 분리
- Git에서 제외 (.gitignore)

### 형식

```ini
# [Settings] 섹션 없음 (ping_config.ini 공유)

# 형식: IP,이름,우선순위,그룹
192.168.0.1,Router,1,Network
192.168.1.10,NAS,2,Storage
10.0.0.50,Database Server,1,Database
```

**주의:**
- Settings 섹션 무시 (ping_config.ini 우선)
- Targets 섹션만 사용
- ping_config.ini와 병합됨

### 예제

```ini
# Internal IP Configuration
# Git에 커밋하지 마세요!

# 네트워크 장비
192.168.0.1,Gateway,1,Network
192.168.0.2,Switch,2,Network
192.168.0.3,Firewall,1,Firewall

# 서버
10.0.0.10,Web Server 1,1,Server
10.0.0.11,Web Server 2,1,Server
10.0.0.20,DB Master,1,Database
10.0.0.21,DB Slave,2,Database

# 스토리지
10.0.1.100,NAS 1,2,Storage
10.0.1.101,NAS 2,2,Storage
```

---

## 🔧 고급 설정

### 그룹별 임계값 (미래 기능)

**v2.7 예정:**

```ini
[GroupThresholds]
DNS=100,200          # 경고: 100ms, 위험: 200ms
Server=150,300
Network=100,250
Database=200,400
```

### IP 그룹 매핑 (미래 기능)

**v2.7 예정:**

```ini
[IPGroups]
8.8.8.8=DNS,1
192.168.0.1=Network,1
10.0.0.10=Server,2
```

---

## 🔐 보안 가이드

### int_config.ini 관리

#### .gitignore 설정

```gitignore
# Internal configuration file (contains private IP addresses)
int_config.ini

# Backup files
*.bak
*_backup.*
```

#### Git 사용 시 체크리스트

```bash
# ✅ 커밋해야 할 파일
git add ping_config.ini          # 공개 IP
git add .gitignore

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

### 백업

```cmd
# 설정 파일 백업
copy config\ping_config.ini config\ping_config.ini.bak
copy config\int_config.ini config\int_config.ini.bak

# 타임스탬프 백업
for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set DATE=%%a%%b%%c)
copy config\ping_config.ini config\ping_config.ini.%DATE%
```

---

## 📝 설정 예제

### 예제 1: 기본 설정

```ini
[Settings]
NotificationsEnabled=1
NotificationCooldown=300
NotifyOnTimeout=1
NotifyOnRecovery=1
ConsecutiveFailures=3
OutageThreshold=5

[Targets]
8.8.8.8,Google DNS,1,DNS
1.1.1.1,Cloudflare DNS,1,DNS
```

### 예제 2: 엔터프라이즈 설정

```ini
[Settings]
NotificationsEnabled=1
NotificationCooldown=600        # 10분 쿨다운
NotifyOnTimeout=1
NotifyOnRecovery=0              # 복구 알림 off
ConsecutiveFailures=5           # 연속 5회 실패
OutageThreshold=10              # 10분 장애 임계값

[Targets]
# 핵심 인프라 (P1)
192.168.0.1,Core Router,1,Network
192.168.0.2,Core Switch,1,Network

# 주요 서버 (P1)
10.0.0.10,Primary DB,1,Database
10.0.0.20,Web Server 1,1,Server

# 보조 서버 (P2)
10.0.0.11,Secondary DB,2,Database
10.0.0.21,Web Server 2,2,Server

# 모니터링용 (P3)
8.8.8.8,Google DNS,3,External
```

### 예제 3: 홈랩 설정

```ini
[Settings]
NotificationsEnabled=1
NotificationCooldown=60          # 1분 쿨다운 (테스트용)
NotifyOnTimeout=1
NotifyOnRecovery=1
ConsecutiveFailures=2            # 연속 2회 실패
OutageThreshold=3                # 3분 장애 임계값

[Targets]
# 공유기
192.168.0.1,Router,1,Network

# 홈서버
192.168.0.100,Home Server,1,Server
192.168.0.101,Plex Media Server,2,Server
192.168.0.102,NAS,2,Storage

# 외부
8.8.8.8,Google DNS,3,External
```

---

## 🔄 설정 적용

### 실시간 적용

```
1. ping_config.ini 또는 int_config.ini 편집
2. 저장
3. 트레이 아이콘 우클릭 → "설정 다시 불러오기"
4. 브라우저 새로고침 (F5)
```

**재시작 불필요!**

### 로딩 순서

```
1. ping_config.ini 로드 (필수)
   └─ [Settings] 섹션 적용
   └─ [Targets] 섹션 IP 추가

2. int_config.ini 로드 (선택)
   └─ [Settings] 무시
   └─ [Targets] 섹션 IP 추가

3. 결과: 두 파일의 IP가 합쳐짐
```

### 로딩 메시지 (디버그 모드)

```
==========================================
설정 파일 로딩 시작
==========================================
설정 파일 읽는 중: config/ping_config.ini
  ping_config.ini에서 타겟 로드 완료
------------------------------------------
설정 파일 읽는 중: config/int_config.ini
  int_config.ini에서 타겟 로드 완료
==========================================
설정 로드 완료: 총 10개 타겟
알림 설정: 활성화 (쿨다운: 300초, 연속실패: 3회)
==========================================
```

---

## ⚠️ 주의사항

### 필수 조건

- **최소 1개 IP 필요**
  - ping_config.ini 또는 int_config.ini에 최소 1개 IP 등록
  - IP 없으면 프로그램 시작 불가

### 형식 규칙

- **IP 주소**: IPv4 형식 (점으로 구분된 4개 숫자)
- **이름**: 공백 가능, 특수 문자 주의
- **쉼표**: 필드 구분자로 사용

### 오류 처리

**잘못된 형식:**
```ini
# ❌ 쉼표 누락
8.8.8.8 Google DNS

# ❌ IP 형식 오류
8.8.8,Google DNS

# ✅ 올바른 형식
8.8.8.8,Google DNS
```

**로딩 실패 시:**
- 오류 메시지 표시
- 기본 설정으로 실행
- 로그 파일 확인 (`ping_monitor_debug.log`)

---

**작성일:** 2025-02-01  
**버전:** Ping Monitor v2.6
