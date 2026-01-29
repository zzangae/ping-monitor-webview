[메인으로 돌아가기](../README.md)

## 문서

- [시스템 아키텍처](docs/ARCHITECTURE.md)
- [파일 구조 및 배포](docs/FILE_STRUCTURE.md)
- [설치 및 빌드 가이드](docs/INSTALLATION.md)
- [사용자 가이드](docs/USER_GUIDE.md)
- [설정 파일 상세](docs/CONFIGURATION.md)
- [문제 해결](docs/TROUBLESHOOTING.md)
- [버전 변경 이력](docs/CHANGELOG.md)

---

## 7. TROUBLESHOOTING.md

**포함 내용:**

- 일반적인 문제
- 에러 메시지별 해결
- 디버그 방법
- FAQ

### 404 에러 - 페이지를 찾을 수 없음

**원인:**

- 파일이 누락되었거나 잘못된 경로에 배치
- 한글 경로 문제

**해결:**

1. 필수 파일 확인
   ```
   ping_monitor.exe
   graph.html
   chart.umd.min.js
   css\ (6개 CSS 파일)
   ```
2. 영문 경로로 이동 (예: `C:\PingMonitor\`)
3. 프로그램 재시작

### Chart is not defined 에러

**원인:**

- Chart.js 파일 누락
- CDN 접속 차단 (회사 방화벽)

**해결:**

1. `chart.umd.min.js` 파일이 있는지 확인
2. 없으면 다운로드:
   - https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js
   - `ping_monitor.exe`와 같은 폴더에 저장
3. 브라우저 강력 새로고침 (Ctrl+F5)

### 포트 충돌 에러

**원인:**

- 8080 포트가 이미 사용 중

**해결:**

1. 트레이 아이콘 우클릭 → 포트 변경
2. 사용 가능한 포트 입력 (8000-9000)
3. 확인 → 서버 자동 재시작

### IP가 표시되지 않음

**원인:**

- 설정 파일에 IP가 없음
- 모니터링이 일시정지 상태

**해결:**

1. `ping_config.ini` 또는 `int_config.ini`에 IP 추가
   ```ini
   192.168.0.1, 테스트
   ```
2. 트레이 아이콘 우클릭 → 시작
3. 브라우저 새로고침

### 알림이 표시되지 않음

**원인:**

- 알림 비활성화 상태

**해결:**

1. 트레이 아이콘 우클릭 → 알림 활성화 체크
2. `ping_config.ini` 확인:
   ```ini
   [Settings]
   NotificationsEnabled=1
   ```

### 네트워크 접근 불가

**원인:**

- Windows 방화벽 차단

**해결:**

1. Windows 방화벽 설정 → ping_monitor.exe 허용
2. 또는 관리자 권한으로 실행
