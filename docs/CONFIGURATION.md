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

**ping_config.ini - 새 섹션 추가: v2.6**

```ini
[OutageDetection]
OutageThreshold=300
ServerOutageThreshold=180
NetworkOutageThreshold=300
FirewallOutageThreshold=120
DatabaseOutageThreshold=180
WebServerOutageThreshold=240
StorageOutageThreshold=300
OtherOutageThreshold=300

[IPGroups]
192.168.0.1=네트워크,1
10.0.0.5=서버,1
8.8.8.8=네트워크,2
```

**형식:**
- `IPGroups`: `IP주소=그룹명,우선순위`
- 우선순위: 1(최고) ~ 5(최저)

---

**포함 내용:**

- ping_config.ini 형식
- int_config.ini 형식
- 설정 옵션 상세
- 예제 설정
- 보안 고려사항

### ping_config.ini (필수)

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

### ping_config.ini (공개 IP 설정)

```ini
[Settings]
NotificationsEnabled=1           # 알림 활성화 (0=비활성화, 1=활성화)
NotificationCooldown=60          # 알림 쿨다운 (초)
NotifyOnTimeout=1                # 타임아웃 알림
NotifyOnRecovery=1               # 복구 알림
ConsecutiveFailures=3            # 연속 실패 임계값

# IP 목록 (형식: IP, 이름)
8.8.8.8, Google DNS
1.1.1.1, Cloudflare DNS
208.67.222.222, OpenDNS
```

### int_config.ini (선택, 비공개)

** 중요: 이 파일은 Git에 커밋하지 마세요!**

```ini
# 형식: ip,name
# 한 줄에 하나씩

# Internal IP Configuration (Example)
# Format: ip,name

# Settings (optional - will use ping_config.ini settings)
[Settings]
# NotificationsEnabled=1
# NotificationCooldown=60

# 모니터링대상
```

#### int_config.ini 생성 방법

```bash
# Windows 명령 프롬프트
copy int_config.ini.example int_config.ini

# 편집기로 열어서 내부 IP 추가
notepad int_config.ini
```

### 설정 파일 형식

#### 공통 규칙

```
[Settings]         # 설정 섹션 (선택)
key=value          # 설정 값
ip,name            # IP 설정 (쉼표로 구분)
# 주석             # 주석
                   # 빈 줄 (무시됨)
```

### int_config.ini (내부 IP 설정)

```ini
# [Settings] 섹션 없이 IP만 나열
192.168.0.1, 공유기
10.20.33.116, 내부 방화벽#1
10.20.33.117, 내부 방화벽#2
```

**주의사항:**

- `ping_config.ini`: Git에 커밋 (공개 IP)
- `int_config.ini`: Git에서 무시 (.gitignore) (보안 IP)
- 최소 1개 이상의 IP가 설정되어야 프로그램 실행 가능
- IP와 이름은 쉼표(,)로 구분

### 로딩 순서

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

## 보안 가이드

### 중요: int_config.ini 관리

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
