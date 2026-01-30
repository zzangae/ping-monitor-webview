# 🌐 Ping Monitor WebView2 v2.6

Windows용 실시간 네트워크 핑 모니터링 도구 with 웹 대시보드

![Version](https://img.shields.io/badge/version-2.6-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](https://www.apache.org/licenses/LICENSE-2.0)

**GitHub**: https://github.com/zzangae/pings

---

## 문서
- [시스템 아키텍처](docs/ARCHITECTURE.md)
- [파일 구조 및 배포](docs/FILE_STRUCTURE.md)
- [설치 및 빌드 가이드](docs/INSTALLATION.md)
- [사용자 가이드](docs/USER_GUIDE.md)
- [설정 파일 상세](docs/CONFIGURATION.md)
- [문제 해결](docs/TROUBLESHOOTING.md)
- [버전 변경 이력](docs/CHANGELOG.md)

---

## 프로젝트 개요

**Ping Monitor WebView2**는 C 백엔드와 HTML/JavaScript 프론트엔드로 구성된 Windows용 네트워크 모니터링 도구입니다.

---

# Ping Monitor v2.6 - 새로운 기능

## 📊 주요 업데이트

### 1. 장애 관리 시스템 (Outage Management)
**IP가 다운된 시간을 자동으로 추적하고 기록합니다.**

- **자동 장애 감지**: 5분(300초) 연속 다운 시 장애로 판정
- **그룹별 임계값**: 7개 그룹마다 다른 임계값 설정 가능
  - 서버, 네트워크, 방화벽, 데이터베이스, 웹서버, 스토리지, 기타
- **장애 로그**: `outage_log.json`에 자동 기록
  - 시작 시간, 종료 시간, 지속 시간
  - IP, 이름, 그룹, 우선순위
- **장애 타임라인 탭**: 대시보드에서 장애 이력 확인

**설정 예시:**
```ini
[OutageDetection]
OutageThreshold=300              # 기본 5분
ServerOutageThreshold=180        # 서버: 3분
DatabaseOutageThreshold=180      # DB: 3분
FirewallOutageThreshold=120      # 방화벽: 2분
```

---

### 2. 통합 설정 UI
**대시보드에서 모든 설정을 관리할 수 있습니다.**

**접근:** IP 비교 타임라인 그래프 우측 상단 ⚙️ 버튼 클릭

#### 📊 Chart Settings 탭
- IP별 표시/숨김 체크박스

#### ⏱️ Group Thresholds 탭
- 기본 임계값 설정
- 7개 그룹별 장애 판정 시간 설정

#### 📋 IP Group Management 탭
- IP별 그룹 분류 (드롭다운)
- 우선순위 설정 (P1~P5)

**Empty State**: 데이터 없을 때 안내 메시지 표시

---

### 3. IP 그룹 및 우선순위
**IP를 그룹으로 분류하고 중요도를 설정합니다.**

**7개 그룹:**
- 🖥️ 서버
- 🌐 네트워크
- 🛡️ 방화벽
- 🗄️ 데이터베이스
- 🌐 웹서버
- 💾 스토리지
- 📦 기타

**우선순위 레벨:**
- P1 (최고) ~ P5 (최저)

**설정 파일:**
```ini
[IPGroups]
192.168.0.1=네트워크,1
10.0.0.5=서버,1
```

---

### 4. 트레이 메뉴 개선

**새로운 메뉴:**
```
├─ 시작/일시정지
├─ 브라우저 열기
├─ ─────────────
├─ ☑ 알림 활성화
├─ ─────────────
├─ 설정 불러오기  ⭐ NEW
├─ 포트 변경...
└─ 종료
```

#### 설정 불러오기
- 실행 중 `ping_config.ini` / `int_config.ini` 수정 후
- 프로그램 재시작 없이 설정 다시 로드
- IP 추가/삭제/변경 즉시 반영

---

### 5. 빌드 스크립트 개선

**build.bat 새 기능:**
```
[0/5] 프로세스 체크 및 자동 종료
[1/5] 컴파일
[2/5] 파일 체크 (상세 표시)
[3/5] 빌드 완료
[4/5] 선택:
  1. Run program
  2. Create deployment package
  3. Run with console (debug mode)  ⭐ NEW
  4. Exit
```

**디버그 모드:**
- 콘솔 창에 모든 메시지 출력
- 오류 진단 용이
- `-mwindows` 플래그 제거

---

### 6. 브라우저 모니터링 (실험적)

**별도 모듈로 분리:**
- `browser_monitor.h/c` - 브라우저 프로세스 추적
- `BROWSER_CLOSE_DETECTION.md` - 완전 가이드

**현재 상태:** 안정성 문제로 비활성화
- 추후 재활성화 가능

---

### 7. Chart.js 로딩 개선

**CDN 직접 로드:**
```html
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
```

**장점:**
- 설치 불필요
- 항상 최신 버전

**오프라인 사용:** `CHART_INSTALL.md` 참조

---

### 8. 알림 개선

**불필요한 알림 제거:**
- ❌ "설정 불러오기 완료" 제거
- ❌ "알림 활성화" 제거

**유지되는 알림:**
- ⚠️ 네트워크 타임아웃
- ✅ 네트워크 복구

**트레이 메뉴에서 On/Off 가능**

---

## 💡 사용 팁

### 장애 관리
- 중요한 서버는 임계값을 짧게 (2-3분)
- 일반 장비는 기본값 (5분) 사용
- `outage_log.json`을 정기적으로 분석

### 설정 UI
- 대시보드에서 직접 수정 가능
- 변경 후 "Save" 버튼 (향후 구현 예정)
- 현재는 `ping_config.ini` 직접 수정

### 설정 불러오기
- IP 추가/삭제 후 바로 적용
- 프로그램 재시작 불필요
- 브라우저 새로고침 (F5) 필요

---

## 알려진 제한사항

1. **설정 UI 저장 기능**
   - 현재 읽기 전용
   - 저장 API 구현 예정

2. **브라우저 자동 종료**
   - 안정성 문제로 비활성화
   - 수동 종료 필요 (트레이 메뉴)

3. **Chart.js**
   - CDN 사용 (인터넷 필요)
   - 오프라인: 로컬 파일 다운로드 필요

4. **용량** - 최대 50개 IP 모니터링 가능

---

## 📊 성능 개선

- 모듈화된 코드 구조
- 파일 I/O 최적화 (원자적 쓰기)
- 메모리 관리 개선

---

## 🔮 향후 계획 (v2.7)

- [ ] 설정 UI 저장 기능 구현
- [ ] 브라우저 자동 종료 안정화
- [ ] 장애 통계 및 리포트
- [ ] 이메일 알림 (SMTP)
- [ ] Telegram 봇 연동
- [ ] 다중 사용자 지원

---

**버전:** v2.6  
**릴리스 날짜:** 2026-01-30  
