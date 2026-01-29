# 🌐 Ping Monitor WebView2 v2.5

Windows용 실시간 네트워크 핑 모니터링 도구 with 웹 대시보드

![Version](https://img.shields.io/badge/version-2.5-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

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

## ✨ v2.5 주요 특징

### 🔐 이중 설정 파일 시스템
- **ping_config.ini**: 공개 IP (Git 추적)
- **int_config.ini**: 내부 IP (Git 무시)
- 보안 IP와 공개 IP 분리 관리

### 🔧 포트 변경 기능
- 트레이 아이콘에서 HTTP 포트 변경
- 8000-9000 범위에서 자동 탐지
- 포트 충돌 검사 및 안전 재시작

### 🌍 오프라인 환경 지원
- Chart.js 로컬 호스팅
- CDN 의존성 제거
- 폐쇄망/회사망에서 정상 작동

### 🚀 브라우저 통합
- 프로그램 시작 시 브라우저 자동 오픈
- 브라우저 닫기 = 프로그램 종료
- `/shutdown` 엔드포인트 구현

### 📦 배포 자동화
- build.bat로 원클릭 배포 패키지 생성
- 필수 파일 검증 기능
- 영문 경로 권장 안내

### 🛡️ 안정성 개선
- 포트 변경 다이얼로그 버튼 응답 수정
- 한글 경로 문제 문서화
- 파일 누락 시 명확한 에러 메시지

---

## 🎯 핵심 기능

✅ **실시간 ICMP Ping 모니터링**  
✅ **Chart.js 기반 인터랙티브 그래프**  
✅ **네트워크 타임아웃/복구 알림**  
✅ **IP 카드 드래그 앤 드롭**  
✅ **시스템 트레이 백그라운드 실행**  
✅ **필터링 (전체/내부/외부/오프라인)**

---

## 해결된 버그

| 버그 | 원인 | 해결 |
|------|------|------|
| ping_data.json 미생성 | g_hWebView NULL 체크 | 조건문 수정 |
| 시간 불규칙 | 파일 읽기/쓰기 충돌 | 원자적 쓰기 (tmp → json) |
| 그래프 춤추듯 움직임 | Chart.js 애니메이션 | animation: false |
| 버튼 먹통 | sendCommand 미구현 | toggleMonitoring 구현 |
| winsock2.h 경고 | include 순서 | winsock2.h를 windows.h 앞에 |
| 프로세스 미종료 | HTTP 스레드 블로킹 | shutdown + TerminateThread |

---

## 알려진 제한사항

1. **플랫폼** - Windows 전용 (ICMP API, Win32 API)
2. **권한** - 관리자 권한 불필요 (IcmpSendEcho 사용)
3. **네트워크** - HTTP 서버는 localhost만 접근 가능
4. **용량** - 최대 50개 IP 모니터링 가능

---

**🎉 Ping Monitor WebView2 v2.5를 사용해주셔서 감사합니다!**
