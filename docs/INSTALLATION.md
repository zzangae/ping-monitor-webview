# 4. INSTALLATION.md

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

**포함 내용:**

- 시스템 요구사항
- 개발 환경 설정
- 컴파일 방법
- 배포 패키지 생성
- 다른 PC에 설치

### 사전 요구사항

- **OS**: Windows 10/11 (64-bit)
- **컴파일러**: MinGW-w64 GCC (빌드 시에만 필요)
- **브라우저**: Chrome, Edge, Firefox 등

### 개발 환경 설정

1. **MinGW-w64 설치**

   ```bash
   # Windows에서 MSYS2 사용
   pacman -S mingw-w64-x86_64-gcc
   ```

2. **저장소 클론**

   ```bash
   git clone https://github.com/zzangae/ping-monitor-webview.git
   cd ping-monitor-webview
   ```

3. **Chart.js 다운로드**
   - 다운로드: https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js
   - 저장 위치: 프로젝트 루트에 `chart.umd.min.js`로 저장

4. **컴파일**

   ```bash
   build.bat
   ```

5. **실행**
   - 옵션 1 선택: 프로그램 즉시 실행
   - 옵션 2 선택: 배포 패키지 생성

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
gcc -o ping_monitor.exe ping_monitor_webview.c http_server.c -lws2_32 -liphlpapi -lshlwapi -lole32 -loleaut32 -lshell32 -mwindows -municode -O2
```

**옵션 선택:**

1. **Run program** - 즉시 실행
2. **Create deployment package** - 배포 패키지 생성
3. **Exit** - 종료

---

## 배포 패키지 생성

`PingMonitor` 폴더에 다음 파일들이 자동 복사됩니다:

```
PingMonitor/
├─ ping_monitor.exe
├─ graph.html
├─ chart.umd.min.js
├─ ping_config.ini (샘플 생성 또는 복사)
├─ int_config.ini (있는 경우 복사)
├─ css/ (전체 복사)
└─ README.txt
```

### 다른 PC에 배포

1. `PingMonitor` 폴더 전체를 압축
2. 대상 PC에 복사 및 압축 해제
3. **중요**: 영문 경로에 배치
   - ✅ `C:\PingMonitor\`
   - ✅ `D:\Tools\PingMonitor\`
   - ❌ `C:\Downloads\_클라우드 내부 외부 모니터링\` (한글 경로 문제)
4. `ping_monitor.exe` 실행

---

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

- **자동 병합**: 두 파일의 IP를 합쳐서 모니터링
- **선택적**: int_config.ini가 없어도 정상 작동
- **.gitignore**: int_config.ini는 Git에서 자동 제외
- **예시 파일**: int_config.ini.example 제공

### 사용 방법

```bash
# 1. 예시 파일 복사
copy int_config.ini.example int_config.ini

# 2. int_config.ini 편집하여 내부 IP 추가
notepad int_config.ini

# 3. 빌드 및 실행
build.bat
```

---

## 🚀 업그레이드 방법

### v2.5에서 v2.6으로

1. **기존 데이터 백업**
   ```batch
   copy ping_config.ini ping_config.ini.bak
   copy notification_log.json notification_log.json.bak
   ```

2. **새 파일 배치**
   - v2.6 파일 압축 해제
   - 기존 `ping_config.ini` 복사

3. **설정 파일 수정**
   ```ini
   # ping_config.ini에 추가
   [OutageDetection]
   OutageThreshold=300
   
   [IPGroups]
   # IP별 그룹과 우선순위 추가
   ```

4. **프로그램 실행**
   ```batch
   ping_monitor.exe
   ```

5. **브라우저에서 확인**
   - 장애 타임라인 탭 확인
   - 설정 UI (⚙️) 확인

---

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
