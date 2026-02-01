# 7. TROUBLESHOOTING.md

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

- 컴파일 오류 해결
- 실행 오류 해결
- 웹 대시보드 오류
- 알림 오류
- 성능 문제
- FAQ

---

## 🔨 컴파일 오류

### "gcc: command not found"

**원인:** MinGW 미설치 또는 PATH 미설정

**해결:**

```cmd
# 1. gcc 위치 확인
where gcc

# 2. 출력 없으면 PATH 확인
echo %PATH%

# 3. MinGW bin 경로 추가
set PATH=%PATH%;C:\mingw64\bin

# 4. 새 CMD 창 열기
exit

# 5. 다시 확인
gcc --version
```

**영구 설정:**
```
시스템 속성 → 고급 → 환경 변수
→ Path → 편집 → 새로 만들기
→ C:\mingw64\bin 추가
```

### "undefined reference to `CreateFontW'"

**원인:** `-lgdi32` 라이브러리 누락

**해결:**

```cmd
# build.bat 확인
notepad build.bat

# 링킹 명령에 -lgdi32 포함되어 있는지 확인
gcc ... -lgdi32
```

**수동 컴파일:**
```cmd
gcc -o ping_monitor.exe ... -lgdi32
```

### "undefined reference to `IcmpCreateFile'"

**원인:** `-liphlpapi` 라이브러리 누락

**해결:**
```cmd
gcc -o ping_monitor.exe ... -liphlpapi
```

### "multiple definition of ..."

**원인:** 중복 파일 또는 백업 파일

**해결:**

```cmd
cd main

# 백업 파일 삭제
del *_backup.c
del *.bak

# 오브젝트 파일 삭제
del *.o

# 재컴파일
cd ..
build.bat
```

### 컴파일은 되는데 실행 안 됨

**원인:** 필수 DLL 누락

**해결:**

```cmd
# 1. 의존성 확인 (Dependency Walker)
# 2. MinGW bin 폴더에서 DLL 복사
copy C:\mingw64\bin\*.dll .

# 또는 정적 링킹
gcc ... -static -static-libgcc
```

---

## ⚙️ 실행 오류

### "필수 파일 누락"

**원인:** web 폴더 또는 CSS 파일 없음

**해결:**

```cmd
# 1. 필수 파일 확인
dir web\graph.html
dir web\css\*.css

# 2. 출력 확인
# graph.html        ~80KB
# variables.css     ~0.5KB
# base.css          ~0.8KB
# components.css    ~4KB
# dashboard.css     ~12KB
# notifications.css ~1.5KB
# outages.css       ~3KB
# settings.css      ~6KB
# responsive.css    ~5KB

# 3. 없으면 GitHub에서 다운로드
git clone https://github.com/zzangae/pings.git
```

### "HTTP 서버 시작 실패"

**원인:** 포트 8080-8099 모두 사용 중

**해결:**

```cmd
# 1. 사용 중인 포트 확인
netstat -ano | findstr :8080

# 2. 프로세스 종료
taskkill /PID <PID> /F

# 3. 또는 다른 포트 사용
# 트레이 아이콘 우클릭 → "서버 포트 변경"
```

### "설정 파일에 IP가 없습니다"

**원인:** ping_config.ini에 [Targets] 섹션 없음

**해결:**

```cmd
notepad config\ping_config.ini
```

```ini
[Targets]
8.8.8.8,Google DNS
1.1.1.1,Cloudflare DNS
```

**최소 1개 IP 필요!**

### 한글 경로 문제

**원인:** 프로그램이 한글 경로에서 실행

**증상:**
- 파일 로드 실패
- 404 에러
- 차트 표시 안 됨

**해결:**

```cmd
# ❌ 잘못된 경로
C:\사용자\문서\핑_모니터\

# ✅ 올바른 경로
C:\PingMonitor\
D:\Tools\PingMonitor\

# 프로그램을 영문 경로로 이동
```

---

## 🌐 웹 대시보드 오류

### 404 에러 - 페이지를 찾을 수 없음

**원인:** 파일 경로 오류

**해결:**

```cmd
# 1. 파일 위치 확인
dir web\graph.html

# 2. 브라우저 URL 확인
http://localhost:8080/web/graph.html
                      ^^^^
                      web/ 경로 포함되어야 함

# 3. 캐시 삭제 후 새로고침
Ctrl+Shift+Delete → 캐시 삭제
Ctrl+F5 → 강력 새로고침
```

### "Chart is not defined" 에러

**원인:** Chart.js 파일 누락

**해결:**

```cmd
# 1. Chart.js 파일 확인
dir web\chart.umd.min.js

# 2. 파일 크기 확인 (약 205KB)
# 없으면 다운로드
DOWNLOAD_CHARTJS.bat

# 3. 브라우저 강력 새로고침
Ctrl+F5
```

**수동 다운로드:**
```
https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js
→ web/chart.umd.min.js로 저장
```

### 차트가 표시되지 않음

**원인:** Chart.js 로드 실패 또는 데이터 없음

**해결:**

```javascript
// 1. 브라우저 개발자 도구 (F12)
// 2. Console 탭에서 에러 확인

// Chart.js 로드 확인
typeof Chart
// 'function'이면 정상, 'undefined'면 로드 실패

// 데이터 확인
fetch('/data/ping_data.json')
  .then(r => r.json())
  .then(d => console.log(d))
```

### CSS가 깨짐

**원인:** CSS 파일 경로 오류

**해결:**

```cmd
# 1. CSS 파일 확인
dir web\css\*.css

# 2. 8개 파일 모두 있는지 확인
variables.css
base.css
components.css
dashboard.css
notifications.css
outages.css
settings.css
responsive.css

# 3. 브라우저 개발자 도구 (F12)
# → Network 탭
# → 404 오류 파일 확인
```

### 데이터가 표시되지 않음

**원인:** data 폴더 없음 또는 권한 문제

**해결:**

```cmd
# 1. data 폴더 생성
mkdir data

# 2. 관리자 권한으로 실행
우클릭 → "관리자 권한으로 실행"

# 3. 데이터 파일 확인
dir data\ping_data.json
# 파일이 생성되는지 확인 (1초마다 업데이트)
```

---

## 🔔 알림 오류

### 알림창이 나타나지 않음

**원인:** 알림 비활성화 또는 임계값 미도달

**해결:**

```
1. 트레이 아이콘 우클릭
2. "알림 활성화" 체크 확인 ☑

3. ping_config.ini 확인
[Settings]
NotificationsEnabled=1
ConsecutiveFailures=3  ← 낮추면 더 빨리 알림
```

**테스트:**
```ini
# 테스트용 설정
ConsecutiveFailures=1  # 1회 실패만으로 알림
NotificationCooldown=10  # 10초 쿨다운
```

### 알림창이 무한 로딩

**원인:** v2.6 이전 버전 사용

**해결:**

```
1. v2.6 이상으로 업그레이드
2. -lgdi32 링킹 확인
3. 재컴파일

# v2.6에서 수정됨:
# - 폰트 전역 캐싱
# - PostMessage 스레드 안전
```

### 알림창이 트레이 아이콘 가림

**원인:** v2.6 이전 버전 사용

**해결:**

```
v2.6 이상으로 업그레이드

# v2.6 특징:
# - 화면 우하단 배치
# - 트레이 아이콘 가리지 않음
# - 10px 간격 자동 정렬
```

### 알림이 너무 많음

**원인:** 쿨다운 시간 짧음

**해결:**

```ini
# ping_config.ini
[Settings]
NotificationCooldown=600  # 10분 쿨다운
NotifyOnRecovery=0        # 복구 알림 off
ConsecutiveFailures=5     # 연속 5회 실패
```

---

## 📊 성능 문제

### CPU 사용률 높음

**원인:** IP 개수 과다 또는 업데이트 간격 짧음

**해결:**

```
# IP 개수 줄이기
ping_config.ini에서 불필요한 IP 제거

# 권장: 50개 이하

# 업데이트 간격 확인
현재: 1초마다 핑 (기본값)
변경 불가 (소스 코드 수정 필요)
```

### 메모리 누수

**원인:** 차트 인스턴스 미정리 (브라우저)

**해결:**

```
# 1. 브라우저 새로고침
F5

# 2. 브라우저 재시작

# 3. 프로그램 재시작
트레이 아이콘 우클릭 → 종료
ping_monitor.exe 실행
```

### 디스크 사용량 증가

**원인:** 로그 파일 누적

**해결:**

```cmd
# 1. 로그 파일 크기 확인
dir data\*.json

# 2. 오래된 로그 삭제
del data\notification_log.json
del data\outage_log.json

# 3. 자동 정리 (v2.7 예정)
# - 30일 이상 로그 자동 삭제
# - 로그 파일 크기 제한
```

---

## 🔍 디버그 방법

### 디버그 모드 실행

```cmd
build.bat
→ 3 선택 (디버그 모드)
```

**특징:**
- 콘솔 창에 모든 메시지 출력
- `ping_monitor_debug.log` 파일 생성
- 오류 진단 용이

**로그 예시:**
```
HTTP 서버 시작 시도 (포트: 8080)...
HTTP 서버 시작 성공: http://localhost:8080
설정 로드 완료: 총 5개 타겟
Browser launched successfully
알림: 활성화
커스텀 알림 시스템 초기화 완료
```

### 브라우저 개발자 도구

```
F12 → Console 탭

# 에러 확인
모든 빨간색 메시지 확인

# Chart.js 로드 확인
typeof Chart

# 데이터 로드 확인
fetch('/data/ping_data.json').then(r=>r.json()).then(d=>console.log(d))
```

### 네트워크 탭 확인

```
F12 → Network 탭

# 404 에러 확인
빨간색 항목 확인
경로 확인

# 데이터 폴링 확인
ping_data.json이 1초마다 업데이트되는지 확인
```

---

## ❓ FAQ

### Q: 브라우저를 닫으면 프로그램도 종료되나요?

**A:** 아니요, 프로그램은 백그라운드에서 계속 실행됩니다.
- 트레이 아이콘 확인
- "웹 대시보드 열기"로 재접속 가능
- 종료: 트레이 아이콘 우클릭 → "종료"

### Q: 여러 PC에서 동시에 모니터링 가능한가요?

**A:** 같은 네트워크에서는 IP 주소로 접속 가능합니다.
```
http://192.168.0.100:8080/web/graph.html
```
단, 방화벽 설정 필요

### Q: 모니터링 데이터를 백업하려면?

**A:** data 폴더 전체 복사
```cmd
xcopy /E /I data data_backup_%DATE%
```

### Q: ping_config.ini를 수정했는데 반영 안 돼요

**A:** "설정 다시 불러오기" 필요
```
1. 트레이 아이콘 우클릭
2. "설정 다시 불러오기"
3. 브라우저 새로고침 (F5)
```

### Q: Chart.js를 로컬에 설치해야 하나요?

**A:** CDN 자동 로드되지만, 오프라인 사용 시 로컬 설치 권장
```cmd
DOWNLOAD_CHARTJS.bat
```

### Q: int_config.ini를 Git에 올려도 되나요?

**A:** 절대 안 됩니다!
- 내부 IP 노출 위험
- .gitignore에 추가
```gitignore
int_config.ini
```

### Q: 알림을 완전히 끄려면?

**A:** 두 가지 방법
```
1. 트레이 메뉴: "알림 활성화" 체크 해제

2. ping_config.ini:
[Settings]
NotificationsEnabled=0
```

### Q: 장애 로그를 Excel로 분석하려면?

**A:** outage_log.json을 CSV로 변환
```python
import json
import csv

with open('data/outage_log.json') as f:
    data = json.load(f)

with open('outages.csv', 'w', newline='', encoding='utf-8') as f:
    writer = csv.DictWriter(f, fieldnames=data[0].keys())
    writer.writeheader()
    writer.writerows(data)
```

---

## 🚨 긴급 복구

### 프로그램이 시작되지 않음

```cmd
# 1. 설정 파일 확인
notepad config\ping_config.ini

# 2. 최소 설정으로 테스트
[Targets]
8.8.8.8,Google DNS

# 3. 디버그 모드 실행
build.bat → 3

# 4. 로그 확인
notepad ping_monitor_debug.log
```

### 데이터 파일 손상

```cmd
# 1. 손상된 파일 삭제
del data\*.json

# 2. 프로그램 재시작
ping_monitor.exe

# 3. 자동 재생성 확인
dir data
```

### 설정 파일 복구

```cmd
# 백업에서 복구
copy config\ping_config.ini.bak config\ping_config.ini

# 또는 초기화
notepad config\ping_config.ini
```

---

**작성일:** 2025-02-01  
**버전:** Ping Monitor v2.6
