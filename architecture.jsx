import React, { useState, useEffect } from 'react';

const PingMonitorArchitecture = () => {
  const [activeTab, setActiveTab] = useState('overview');
  const [hoveredNode, setHoveredNode] = useState(null);
  const [pulsePhase, setPulsePhase] = useState(0);
  const [selectedFile, setSelectedFile] = useState(null);

  useEffect(() => {
    const interval = setInterval(() => {
      setPulsePhase(p => (p + 1) % 100);
    }, 50);
    return () => clearInterval(interval);
  }, []);

  // ESC 키로 모달 닫기
  useEffect(() => {
    const handleKeyDown = (e) => {
      if (e.key === 'Escape') setSelectedFile(null);
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, []);

  // 소스코드 데이터
  const sourceCode = {
    c: {
      name: 'ping_monitor_webview.c',
      color: '#00ff88',
      language: 'C',
      code: `/**
 * PING MONITOR with WebView2 Graph
 * WebView2를 사용한 실시간 네트워크 모니터링 + 그래프 시각화
 */

#define UNICODE
#define _UNICODE
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// 상수 정의
// ============================================================================
#define MAX_IP_COUNT 50
#define MAX_IP_LEN 64
#define MAX_NAME_LEN 64
#define PING_TIMEOUT 1000
#define PING_INTERVAL 1000  // 1초

// ============================================================================
// 데이터 구조체
// ============================================================================
typedef struct {
    WCHAR ip[MAX_IP_LEN];
    WCHAR name[MAX_NAME_LEN];
    int latency;        // -1 = timeout
    int isOnline;       // 1 = online, 0 = timeout, -1 = ready
    int history[60];    // 최근 60개
    int historyIndex;
    int historyCount;
    int totalPings;
    int successPings;
    int minLatency;
    int maxLatency;
    double avgLatency;
} IPTarget;

// ============================================================================
// 전역 변수
// ============================================================================
HWND g_hMainWnd = NULL;
IPTarget g_targets[MAX_IP_COUNT];
int g_targetCount = 0;
BOOL g_isRunning = FALSE;
HANDLE g_hIcmp = NULL;

// ============================================================================
// 핑 실행
// ============================================================================
BOOL DoPing(const WCHAR* ipAddress, int* latency) {
    *latency = -1;
    
    if (!g_hIcmp) {
        g_hIcmp = IcmpCreateFile();
        if (g_hIcmp == INVALID_HANDLE_VALUE) return FALSE;
    }
    
    // IP 주소 변환
    char ipAnsi[MAX_IP_LEN];
    WideCharToMultiByte(CP_ACP, 0, ipAddress, -1, ipAnsi, MAX_IP_LEN, NULL, NULL);
    ULONG ipAddr = inet_addr(ipAnsi);
    
    // ICMP 요청
    char sendData[32] = "PingMonitor";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8;
    BYTE* replyBuffer = (BYTE*)malloc(replySize);
    
    DWORD ret = IcmpSendEcho(g_hIcmp, ipAddr, sendData, sizeof(sendData),
                             NULL, replyBuffer, replySize, PING_TIMEOUT);
    
    if (ret > 0) {
        PICMP_ECHO_REPLY reply = (PICMP_ECHO_REPLY)replyBuffer;
        if (reply->Status == IP_SUCCESS) {
            *latency = (int)reply->RoundTripTime;
            free(replyBuffer);
            return TRUE;
        }
    }
    
    free(replyBuffer);
    return FALSE;
}

// ============================================================================
// JSON 데이터 생성
// ============================================================================
WCHAR* BuildJsonData(void) {
    static WCHAR json[32768];
    int pos = swprintf(json, 32768, 
        L"{\\"running\\":%s,\\"elapsed\\":%d,\\"targets\\":[",
        g_isRunning ? L"true" : L"false", g_elapsedSeconds);
    
    for (int i = 0; i < g_targetCount; i++) {
        IPTarget* t = &g_targets[i];
        // ... JSON 생성 로직
    }
    
    pos += swprintf(json + pos, 32768 - pos, L"]}");
    return json;
}

// ============================================================================
// 윈도우 프로시저
// ============================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TIMER:
            if (wParam == IDT_PING_TIMER && g_isRunning) {
                for (int i = 0; i < g_targetCount; i++) {
                    UpdateTarget(i);
                }
                SendDataToWebView();
            }
            return 0;
            
        case WM_KEYDOWN:
            if (wParam == VK_F5) {
                g_isRunning ? StopMonitoring() : StartMonitoring();
            }
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ============================================================================
// 메인 함수
// ============================================================================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                    LPWSTR lpCmdLine, int nCmdShow) {
    // Winsock 초기화
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    // 설정 로드 & 윈도우 생성
    LoadConfig();
    // ... 윈도우 생성 및 메시지 루프
    
    WSACleanup();
    return 0;
}`
    },
    html: {
      name: 'graph.html',
      color: '#00d4ff',
      language: 'HTML/JS',
      code: `<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <title>Ping Monitor - Network Graph</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    
    <style>
        :root {
            --bg-primary: #0a0e17;
            --bg-card: #1a2234;
            --text-primary: #f8fafc;
            --accent-green: #10b981;
            --accent-red: #ef4444;
        }
        
        body {
            font-family: 'Outfit', sans-serif;
            background: var(--bg-primary);
            color: var(--text-primary);
        }
        
        .summary-card {
            background: var(--bg-card);
            border-radius: 16px;
            padding: 20px;
            text-align: center;
        }
        
        .ip-card {
            background: var(--bg-card);
            border-radius: 12px;
            padding: 16px;
        }
        
        .ip-card.online { border-left: 3px solid var(--accent-green); }
        .ip-card.offline { border-left: 3px solid var(--accent-red); }
        
        .packet-bar {
            width: 8px;
            height: 20px;
            border-radius: 2px;
        }
        .packet-bar.success { background: var(--accent-green); }
        .packet-bar.fail { background: var(--accent-red); }
    </style>
</head>
<body>
    <div class="container">
        <header class="header">
            <h1>🌐 Ping Monitor</h1>
            <div class="current-time" id="current-time">--:--:--</div>
        </header>
        
        <!-- 요약 카드 -->
        <div class="summary-grid">
            <div class="summary-card online">
                <div class="summary-value" id="online-count">0</div>
                <div class="summary-label">온라인</div>
            </div>
            <div class="summary-card offline">
                <div class="summary-value" id="offline-count">0</div>
                <div class="summary-label">오프라인</div>
            </div>
        </div>
        
        <!-- IP 그리드 -->
        <div class="ip-grid" id="ip-grid"></div>
        
        <!-- 비교 차트 -->
        <div class="chart-container">
            <canvas id="comparison-chart"></canvas>
        </div>
    </div>
    
    <script>
        let comparisonChart = null;
        let ipCharts = {};
        const chartColors = ['#10b981', '#3b82f6', '#f59e0b', '#ef4444', '#8b5cf6'];
        
        // IP 카드 생성
        function createIPCard(target, index) {
            return \`
                <div class="ip-card \${target.online === 1 ? 'online' : 'offline'}" id="ip-card-\${index}">
                    <div class="ip-header">
                        <span class="ip-address">\${target.ip}</span>
                        <span class="ip-name">\${target.name}</span>
                    </div>
                    <div class="ip-latency">\${target.latency}ms</div>
                    <div class="packet-container" id="packets-\${index}">
                        \${generatePacketBars(target.history)}
                    </div>
                    <canvas id="chart-\${index}" height="60"></canvas>
                </div>
            \`;
        }
        
        // 패킷 바 생성
        function generatePacketBars(history) {
            return history.slice(-60).map(h => 
                \`<div class="packet-bar \${h === -1 ? 'fail' : 'success'}"></div>\`
            ).join('');
        }
        
        // 데이터 로드
        async function loadData() {
            try {
                const response = await fetch(\`ping_data.json?t=\${Date.now()}\`);
                const data = await response.json();
                updateUI(data);
            } catch (error) {
                console.log('데이터 로드 실패:', error);
            }
        }
        
        // UI 업데이트
        function updateUI(data) {
            // 요약 통계 업데이트
            let onlineCount = data.targets.filter(t => t.online === 1).length;
            document.getElementById('online-count').textContent = onlineCount;
            document.getElementById('offline-count').textContent = 
                data.targets.length - onlineCount;
            
            // IP 카드 업데이트
            // ... 차트 업데이트 로직
        }
        
        // 초기화
        document.addEventListener('DOMContentLoaded', () => {
            setInterval(loadData, 1000);
            loadData();
        });
    </script>
</body>
</html>`
    },
    ini: {
      name: 'ping_config.ini',
      color: '#c084fc',
      language: 'INI',
      code: `# Ping Monitor 설정 파일
# 형식: IP주소,설명
# 주석은 # 또는 ; 로 시작

# Google DNS
8.8.8.8,Google DNS
8.8.4.4,Google DNS 2

# Cloudflare DNS
1.1.1.1,Cloudflare
1.0.0.1,Cloudflare 2

# 한국 DNS
168.126.63.1,KT DNS
168.126.63.2,KT DNS 2
219.250.36.130,SK DNS

# 로컬 네트워크
192.168.1.1,Gateway
192.168.1.100,NAS Server

# 기타
208.67.222.222,OpenDNS`
    },
    bat: {
      name: 'build.bat',
      color: '#ffaa00',
      language: 'Batch',
      code: `@echo off
chcp 65001 > nul
echo ========================================
echo   Ping Monitor Build Script
echo   MinGW-w64 Required
echo ========================================
echo.

REM 컴파일러 확인
where gcc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] gcc를 찾을 수 없습니다.
    echo MinGW-w64를 설치하고 PATH에 추가해주세요.
    pause
    exit /b 1
)

echo [INFO] 컴파일 시작...
echo.

REM 컴파일 실행
gcc -o ping_monitor.exe ping_monitor_webview.c ^
    -lws2_32 ^
    -liphlpapi ^
    -lshlwapi ^
    -lole32 ^
    -loleaut32 ^
    -mwindows ^
    -municode ^
    -O2

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] 컴파일 완료!
    echo 출력 파일: ping_monitor.exe
    echo.
    echo 실행 방법:
    echo   1. ping_monitor.exe 실행
    echo   2. graph.html을 브라우저에서 열기
) else (
    echo.
    echo [ERROR] 컴파일 실패
)

echo.
pause`
    },
    md: {
      name: 'README.md',
      color: '#94a3b8',
      language: 'Markdown',
      code: `# 🌐 Ping Monitor - WebView2 Graph Edition

실시간 네트워크 핑 모니터링 + 아름다운 그래프 시각화

## 📋 특징

- **실시간 그래프** - Chart.js 기반 아름다운 시각화
- **패킷 히트맵** - 최근 60초 패킷 상태를 색상으로 표시
- **IP 비교 타임라인** - 모든 IP를 한 그래프에서 비교
- **통계 대시보드** - 평균/최대/최소/손실률 실시간 계산
- **다크 테마** - 눈이 편안한 모던 UI

## 🚀 시작하기

### 요구사항
- Windows 10 이상
- MinGW-w64 (컴파일용)
- 웹 브라우저 (Chrome, Edge, Firefox 등)

### 컴파일
\`\`\`batch
build.bat
\`\`\`

### 실행
1. \`ping_monitor.exe\` 실행
2. \`graph.html\`을 웹 브라우저에서 열기
3. 실시간 그래프 확인!

## ⚙️ 설정

### ping_config.ini
\`\`\`ini
# IP주소,설명 형식
8.8.8.8,Google DNS
1.1.1.1,Cloudflare
192.168.1.1,Gateway
\`\`\`

## ⌨️ 단축키

| 키 | 기능 |
|----|------|
| F5 | 모니터링 시작/중지 |
| ESC | 모니터링 중지 |

## 📄 라이선스

MIT License`
    }
  };

  const cCodeBlocks = [
    { name: '상수 정의', lines: '70-78', desc: 'MAX_IP_COUNT, PING_TIMEOUT, PING_INTERVAL', color: '#00ff88' },
    { name: 'IPTarget', lines: '83-100', desc: 'IP, 이름, 지연시간, 히스토리[60], 통계', color: '#00d4ff' },
    { name: 'TimeSettings', lines: '102-107', desc: '시/분/초, 루프모드', color: '#00d4ff' },
    { name: 'LoadConfig()', lines: '147-212', desc: 'ping_config.ini UTF-8 파싱', color: '#ffaa00' },
    { name: 'DoPing()', lines: '217-265', desc: 'IcmpSendEcho(), DNS 해석', color: '#ff6b6b' },
    { name: 'UpdateTarget()', lines: '269-301', desc: '통계 계산, 히스토리 갱신', color: '#ff6b6b' },
    { name: 'BuildJsonData()', lines: '306-348', desc: 'JSON 문자열 동적 생성', color: '#c084fc' },
    { name: 'SendDataToWebView()', lines: '353-370', desc: 'ping_data.json 파일 저장', color: '#c084fc' },
    { name: 'Start/Stop()', lines: '375-412', desc: 'SetTimer(), KillTimer()', color: '#00ff88' },
    { name: 'WndProc()', lines: '417-482', desc: 'WM_TIMER, WM_KEYDOWN 처리', color: '#ffaa00' },
    { name: 'wWinMain()', lines: '487-567', desc: '초기화, 메시지 루프', color: '#00ff88' },
  ];

  const jsBlocks = [
    { name: 'CSS Variables', lines: '17-35', desc: '다크 테마 색상 팔레트', color: '#c084fc' },
    { name: 'Chart Config', lines: '500-600', desc: 'Chart.js 기본 설정', color: '#00d4ff' },
    { name: 'createIPCard()', lines: '600-680', desc: 'IP 카드 HTML 템플릿', color: '#00ff88' },
    { name: 'initIPChart()', lines: '700-780', desc: '개별 라인 차트 생성', color: '#00d4ff' },
    { name: 'initComparisonChart()', lines: '800-880', desc: '통합 비교 차트', color: '#00d4ff' },
    { name: 'updateUI()', lines: '895-982', desc: '전체 DOM 업데이트', color: '#ff6b6b' },
    { name: 'loadData()', lines: '1009-1036', desc: 'fetch() + JSON 파싱', color: '#ffaa00' },
    { name: 'loadDemoData()', lines: '1039-1094', desc: '테스트용 목 데이터', color: '#c084fc' },
  ];

  const DataFlowLine = ({ active }) => (
    <div className="relative h-1 w-full overflow-hidden rounded-full bg-slate-800">
      <div 
        className="absolute h-full w-8 rounded-full transition-all duration-75"
        style={{
          background: 'linear-gradient(90deg, transparent, #00ff88, transparent)',
          left: `${(pulsePhase * 1.5) % 120 - 20}%`,
          opacity: active ? 1 : 0.3,
        }}
      />
    </div>
  );

  return (
    <div className="min-h-screen bg-[#050a08] text-white overflow-hidden" style={{ fontFamily: "'Rajdhani', 'Segoe UI', sans-serif" }}>
      {/* 배경 그리드 효과 */}
      <div className="fixed inset-0 opacity-5" style={{
        backgroundImage: `
          linear-gradient(#00ff88 1px, transparent 1px),
          linear-gradient(90deg, #00ff88 1px, transparent 1px)
        `,
        backgroundSize: '50px 50px',
      }} />
      
      {/* 글로우 효과 */}
      <div className="fixed top-0 left-1/4 w-96 h-96 bg-emerald-500/10 rounded-full blur-3xl" />
      <div className="fixed bottom-0 right-1/4 w-96 h-96 bg-cyan-500/10 rounded-full blur-3xl" />

      <div className="relative z-10 max-w-7xl mx-auto p-6">
        {/* 헤더 */}
        <header className="mb-8">
          <div className="flex items-center gap-4 mb-2">
            <div className="w-3 h-3 rounded-full bg-[#00ff88] animate-pulse shadow-lg shadow-emerald-500/50" />
            <h1 className="text-4xl font-bold tracking-wider" style={{ 
              background: 'linear-gradient(135deg, #00ff88 0%, #00d4ff 50%, #c084fc 100%)',
              WebkitBackgroundClip: 'text',
              WebkitTextFillColor: 'transparent',
            }}>
              PING MONITOR
            </h1>
            <span className="text-xs tracking-widest text-slate-500 border border-slate-700 px-2 py-1 rounded">
              ARCHITECTURE v1.0
            </span>
          </div>
          <p className="text-slate-500 text-sm tracking-wide ml-7">
            WebView2 Graph Edition • 실시간 네트워크 모니터링 시스템
          </p>
        </header>

        {/* 탭 네비게이션 */}
        <nav className="flex gap-1 mb-6 p-1 bg-slate-900/50 rounded-lg border border-slate-800 w-fit backdrop-blur">
          {[
            { id: 'overview', label: 'SYSTEM', icon: '◈' },
            { id: 'files', label: 'FILES', icon: '📁' },
            { id: 'c-code', label: 'C ENGINE', icon: '⚙' },
            { id: 'js-code', label: 'FRONTEND', icon: '◉' },
            { id: 'dataflow', label: 'DATA FLOW', icon: '↯' },
            { id: 'json', label: 'SCHEMA', icon: '{ }' },
          ].map(tab => (
            <button
              key={tab.id}
              onClick={() => setActiveTab(tab.id)}
              className={`px-4 py-2 rounded-md text-sm font-semibold tracking-wider transition-all duration-300 ${
                activeTab === tab.id
                  ? 'bg-[#00ff88]/20 text-[#00ff88] border border-[#00ff88]/50 shadow-lg shadow-emerald-500/20'
                  : 'text-slate-500 hover:text-white hover:bg-slate-800'
              }`}
            >
              <span className="mr-2">{tab.icon}</span>
              {tab.label}
            </button>
          ))}
        </nav>

        {/* 메인 콘텐츠 */}
        <main className="space-y-6">
          {/* === OVERVIEW 탭 === */}
          {activeTab === 'overview' && (
            <div className="space-y-6">
              {/* 아키텍처 다이어그램 */}
              <div className="relative bg-slate-900/30 rounded-2xl border border-slate-800 p-8 backdrop-blur overflow-hidden">
                <div className="absolute top-4 right-4 text-xs text-slate-600 tracking-widest">SYSTEM OVERVIEW</div>
                
                <div className="flex flex-col items-center gap-6">
                  {/* 레이어 1: 사용자 인터페이스 */}
                  <div className="w-full max-w-4xl">
                    <div className="text-xs text-cyan-400 tracking-widest mb-3 flex items-center gap-2">
                      <span className="w-8 h-px bg-cyan-400/50" />
                      LAYER 01 — USER INTERFACE
                    </div>
                    <div className="grid grid-cols-2 gap-4">
                      <div 
                        className="group relative bg-gradient-to-br from-slate-800/50 to-slate-900 rounded-xl p-5 border border-slate-700 hover:border-[#00ff88]/50 transition-all duration-300 cursor-pointer"
                        onMouseEnter={() => setHoveredNode('exe')}
                        onMouseLeave={() => setHoveredNode(null)}
                      >
                        <div className="absolute -top-px -left-px -right-px h-px bg-gradient-to-r from-transparent via-[#00ff88]/50 to-transparent opacity-0 group-hover:opacity-100 transition-opacity" />
                        <div className="flex items-center gap-4">
                          <div className="w-12 h-12 rounded-lg bg-[#00ff88]/10 border border-[#00ff88]/30 flex items-center justify-center text-[#00ff88] font-mono text-lg">
                            C
                          </div>
                          <div>
                            <div className="font-bold tracking-wide">ping_monitor.exe</div>
                            <div className="text-xs text-slate-500">Win32 + ICMP Engine</div>
                          </div>
                        </div>
                        <div className="mt-4 text-xs text-slate-400 space-y-1">
                          <div>• ICMP Echo Request/Reply</div>
                          <div>• 타이머 기반 폴링 (1초)</div>
                          <div>• JSON 데이터 생성</div>
                        </div>
                      </div>

                      <div 
                        className="group relative bg-gradient-to-br from-slate-800/50 to-slate-900 rounded-xl p-5 border border-slate-700 hover:border-cyan-400/50 transition-all duration-300 cursor-pointer"
                        onMouseEnter={() => setHoveredNode('html')}
                        onMouseLeave={() => setHoveredNode(null)}
                      >
                        <div className="absolute -top-px -left-px -right-px h-px bg-gradient-to-r from-transparent via-cyan-400/50 to-transparent opacity-0 group-hover:opacity-100 transition-opacity" />
                        <div className="flex items-center gap-4">
                          <div className="w-12 h-12 rounded-lg bg-cyan-400/10 border border-cyan-400/30 flex items-center justify-center text-cyan-400 font-mono text-lg">
                            JS
                          </div>
                          <div>
                            <div className="font-bold tracking-wide">graph.html</div>
                            <div className="text-xs text-slate-500">Chart.js Dashboard</div>
                          </div>
                        </div>
                        <div className="mt-4 text-xs text-slate-400 space-y-1">
                          <div>• 실시간 그래프 렌더링</div>
                          <div>• 패킷 히트맵 시각화</div>
                          <div>• 통계 대시보드</div>
                        </div>
                      </div>
                    </div>
                  </div>

                  {/* 연결선 */}
                  <div className="w-full max-w-md px-8">
                    <DataFlowLine active={hoveredNode === 'exe' || hoveredNode === 'html'} />
                    <div className="text-center text-xs text-slate-600 mt-2 tracking-wider">FILE I/O</div>
                  </div>

                  {/* 레이어 2: 데이터 */}
                  <div className="w-full max-w-4xl">
                    <div className="text-xs text-purple-400 tracking-widest mb-3 flex items-center gap-2">
                      <span className="w-8 h-px bg-purple-400/50" />
                      LAYER 02 — DATA STORAGE
                    </div>
                    <div className="flex justify-center gap-4">
                      <div className="bg-slate-800/50 rounded-lg px-6 py-4 border border-slate-700 text-center">
                        <div className="text-2xl mb-2">📄</div>
                        <div className="text-sm font-mono text-slate-300">ping_config.ini</div>
                        <div className="text-xs text-slate-500 mt-1">IP 목록 설정</div>
                      </div>
                      <div className="bg-gradient-to-br from-amber-500/10 to-orange-500/10 rounded-lg px-6 py-4 border border-amber-500/30 text-center">
                        <div className="text-2xl mb-2">📊</div>
                        <div className="text-sm font-mono text-amber-300">ping_data.json</div>
                        <div className="text-xs text-amber-400/70 mt-1">실시간 데이터</div>
                      </div>
                    </div>
                  </div>

                  {/* 연결선 */}
                  <div className="w-full max-w-md px-8">
                    <DataFlowLine active={true} />
                    <div className="text-center text-xs text-slate-600 mt-2 tracking-wider">NETWORK</div>
                  </div>

                  {/* 레이어 3: 네트워크 */}
                  <div className="w-full max-w-4xl">
                    <div className="text-xs text-red-400 tracking-widest mb-3 flex items-center gap-2">
                      <span className="w-8 h-px bg-red-400/50" />
                      LAYER 03 — NETWORK TARGETS
                    </div>
                    <div className="flex justify-center gap-2 flex-wrap">
                      {[
                        { ip: '8.8.8.8', name: 'Google DNS' },
                        { ip: '1.1.1.1', name: 'Cloudflare' },
                        { ip: '168.126.63.1', name: 'KT DNS' },
                        { ip: '192.168.1.1', name: 'Gateway' },
                      ].map((target, i) => (
                        <div key={i} className="bg-slate-800/30 rounded-lg px-4 py-2 border border-slate-700 text-center hover:border-red-400/30 transition-colors">
                          <div className="font-mono text-sm text-[#00ff88]">{target.ip}</div>
                          <div className="text-xs text-slate-500">{target.name}</div>
                        </div>
                      ))}
                    </div>
                  </div>
                </div>
              </div>

              {/* 기술 스택 카드 */}
              <div className="grid grid-cols-3 gap-4">
                {[
                  { 
                    title: 'C ENGINE', 
                    color: '#00ff88', 
                    icon: '⚙', 
                    items: ['Win32 API', 'ICMP API', 'Winsock2', 'Unicode'] 
                  },
                  { 
                    title: 'FRONTEND', 
                    color: '#00d4ff', 
                    icon: '◉', 
                    items: ['Chart.js', 'Vanilla JS', 'CSS3', 'Fetch API'] 
                  },
                  { 
                    title: 'FEATURES', 
                    color: '#c084fc', 
                    icon: '✦', 
                    items: ['실시간 그래프', '패킷 히트맵', 'IP 비교', '통계 계산'] 
                  },
                ].map((stack, i) => (
                  <div key={i} className="bg-slate-900/50 rounded-xl p-5 border border-slate-800 hover:border-opacity-50 transition-all" style={{ borderColor: stack.color + '30' }}>
                    <div className="flex items-center gap-3 mb-4">
                      <span className="text-2xl" style={{ color: stack.color }}>{stack.icon}</span>
                      <span className="font-bold tracking-wider text-sm" style={{ color: stack.color }}>{stack.title}</span>
                    </div>
                    <div className="space-y-2">
                      {stack.items.map((item, j) => (
                        <div key={j} className="text-sm text-slate-400 flex items-center gap-2">
                          <span className="w-1 h-1 rounded-full" style={{ background: stack.color }} />
                          {item}
                        </div>
                      ))}
                    </div>
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* === FILES 탭 === */}
          {activeTab === 'files' && (
            <div className="space-y-6">
              {/* 파일 구조 다이어그램 */}
              <div className="bg-slate-900/30 rounded-2xl border border-slate-800 p-6 backdrop-blur">
                <div className="flex items-center justify-between mb-6">
                  <div>
                    <h2 className="text-xl font-bold tracking-wide text-amber-400">📁 PROJECT STRUCTURE</h2>
                    <p className="text-sm text-slate-500">ping_monitor_webview/ 디렉토리 구성 • 파일 클릭시 소스코드 보기</p>
                  </div>
                  <div className="text-xs text-slate-600 font-mono bg-slate-800 px-3 py-1 rounded">6 files • 66KB</div>
                </div>

                {/* 파일 트리 */}
                <div className="bg-[#0a0f0d] rounded-xl p-5 border border-slate-800 font-mono text-sm mb-6">
                  <div className="text-amber-400 mb-3">ping_monitor_webview/</div>
                  <div className="space-y-2 ml-4">
                    {[
                      { name: 'ping_monitor_webview.c', color: '#00ff88', size: '19KB', key: 'c' },
                      { name: 'graph.html', color: '#00d4ff', size: '39KB', key: 'html' },
                      { name: 'ping_config.ini', color: '#c084fc', size: '325B', key: 'ini' },
                      { name: 'build.bat', color: '#ffaa00', size: '1.3KB', key: 'bat' },
                      { name: 'README.md', color: '#94a3b8', size: '6.8KB', key: 'md' },
                    ].map((f, i, arr) => (
                      <div 
                        key={i}
                        className="flex items-center gap-3 cursor-pointer hover:bg-slate-800/50 rounded px-2 py-1 -mx-2 transition-colors"
                        onClick={() => setSelectedFile(f.key)}
                      >
                        <span className="text-slate-600">{i === arr.length - 1 ? '└──' : '├──'}</span>
                        <span style={{ color: f.color }}>{f.name}</span>
                        <span className="text-slate-600 text-xs">{f.size}</span>
                        <span className="text-slate-700 text-xs ml-auto">클릭하여 보기 →</span>
                      </div>
                    ))}
                    <div className="flex items-center gap-3 text-slate-500">
                      <span className="text-slate-600">└──</span>
                      <span className="text-orange-400/70">ping_data.json</span>
                      <span className="text-slate-600 text-xs">(자동 생성)</span>
                    </div>
                  </div>
                </div>

                {/* 파일 상세 카드 */}
                <div className="grid grid-cols-2 gap-4">
                  {[
                    {
                      name: 'ping_monitor_webview.c',
                      color: '#00ff88',
                      icon: 'C',
                      type: '핵심 핑 엔진',
                      desc: 'Win32 API + ICMP를 사용한 네트워크 모니터링 백엔드',
                      details: ['ICMP Echo Request/Reply', 'UTF-8 설정 파일 파싱', 'JSON 데이터 생성', '타이머 기반 폴링'],
                      lines: '567줄',
                      key: 'c',
                    },
                    {
                      name: 'graph.html',
                      color: '#00d4ff',
                      icon: 'JS',
                      type: 'Chart.js 대시보드',
                      desc: 'HTML + CSS + JavaScript 단일 파일 프론트엔드',
                      details: ['실시간 그래프 렌더링', '패킷 히트맵 시각화', '통계 대시보드', '다크 테마 UI'],
                      lines: '1,119줄',
                      key: 'html',
                    },
                    {
                      name: 'ping_config.ini',
                      color: '#c084fc',
                      icon: '⚙',
                      type: 'IP 설정 파일',
                      desc: '모니터링할 IP 주소와 별칭을 정의하는 설정 파일',
                      details: ['IP,이름 형식', 'UTF-8 인코딩', '주석 지원 (#, ;)', '최대 50개 IP'],
                      lines: '설정',
                      key: 'ini',
                    },
                    {
                      name: 'build.bat',
                      color: '#ffaa00',
                      icon: '▶',
                      type: '빌드 스크립트',
                      desc: 'MinGW-w64로 C 소스를 컴파일하는 배치 스크립트',
                      details: ['gcc 컴파일 명령', '라이브러리 링크', '-mwindows 옵션', 'Unicode 지원'],
                      lines: '배치',
                      key: 'bat',
                    },
                  ].map((file, i) => (
                    <div 
                      key={i}
                      className="group relative bg-gradient-to-br from-slate-800/50 to-slate-900 rounded-xl p-5 border border-slate-700 hover:border-opacity-100 transition-all duration-300 cursor-pointer"
                      style={{ borderColor: file.color + '30' }}
                      onClick={() => setSelectedFile(file.key)}
                    >
                      <div className="absolute -top-px -left-px -right-px h-px bg-gradient-to-r from-transparent via-current to-transparent opacity-0 group-hover:opacity-50 transition-opacity" style={{ color: file.color }} />
                      
                      <div className="flex items-start gap-4 mb-4">
                        <div 
                          className="w-12 h-12 rounded-lg flex items-center justify-center font-mono text-lg border"
                          style={{ background: file.color + '10', borderColor: file.color + '30', color: file.color }}
                        >
                          {file.icon}
                        </div>
                        <div className="flex-1">
                          <div className="font-bold tracking-wide" style={{ color: file.color }}>{file.name}</div>
                          <div className="text-xs text-slate-500">{file.type}</div>
                        </div>
                        <div className="text-xs text-slate-600 bg-slate-900 px-2 py-1 rounded">{file.lines}</div>
                      </div>
                      
                      <p className="text-sm text-slate-400 mb-4">{file.desc}</p>
                      
                      <div className="space-y-1">
                        {file.details.map((detail, j) => (
                          <div key={j} className="text-xs text-slate-500 flex items-center gap-2">
                            <span className="w-1 h-1 rounded-full" style={{ background: file.color }} />
                            {detail}
                          </div>
                        ))}
                      </div>
                      
                      <div className="mt-4 pt-3 border-t border-slate-700/50 text-xs text-center group-hover:text-white transition-colors" style={{ color: file.color + '80' }}>
                        클릭하여 소스코드 보기 →
                      </div>
                    </div>
                  ))}
                </div>
              </div>

              {/* 파일 관계도 */}
              <div className="bg-slate-900/30 rounded-xl p-5 border border-slate-800">
                <h3 className="font-bold text-slate-300 mb-4 tracking-wide">◈ FILE RELATIONSHIPS</h3>
                <div className="flex items-center justify-center gap-4 flex-wrap">
                  <div className="bg-purple-500/10 border border-purple-500/30 rounded-lg px-4 py-2 text-center">
                    <div className="text-xs text-purple-400">INPUT</div>
                    <div className="font-mono text-sm text-purple-300">ping_config.ini</div>
                  </div>
                  <div className="text-slate-600">→</div>
                  <div className="bg-[#00ff88]/10 border border-[#00ff88]/30 rounded-lg px-4 py-2 text-center">
                    <div className="text-xs text-[#00ff88]">COMPILE</div>
                    <div className="font-mono text-sm text-[#00ff88]">.c → .exe</div>
                  </div>
                  <div className="text-slate-600">→</div>
                  <div className="bg-amber-500/10 border border-amber-500/30 rounded-lg px-4 py-2 text-center">
                    <div className="text-xs text-amber-400">OUTPUT</div>
                    <div className="font-mono text-sm text-amber-300">ping_data.json</div>
                  </div>
                  <div className="text-slate-600">→</div>
                  <div className="bg-cyan-500/10 border border-cyan-500/30 rounded-lg px-4 py-2 text-center">
                    <div className="text-xs text-cyan-400">RENDER</div>
                    <div className="font-mono text-sm text-cyan-300">graph.html</div>
                  </div>
                </div>
              </div>
            </div>
          )}

          {/* === C CODE 탭 === */}
          {activeTab === 'c-code' && (
            <div className="space-y-6">
              <div className="bg-slate-900/30 rounded-2xl border border-slate-800 p-6 backdrop-blur">
                <div className="flex items-center justify-between mb-6">
                  <div>
                    <h2 className="text-xl font-bold tracking-wide text-[#00ff88]">ping_monitor_webview.c</h2>
                    <p className="text-sm text-slate-500">567 lines • ICMP Ping Engine + Win32 Application</p>
                  </div>
                  <div className="text-xs text-slate-600 font-mono bg-slate-800 px-3 py-1 rounded">MinGW-w64</div>
                </div>

                <div className="grid grid-cols-2 gap-3">
                  {cCodeBlocks.map((block, i) => (
                    <div 
                      key={i} 
                      className="group relative bg-slate-800/30 rounded-lg p-4 border border-slate-700/50 hover:border-opacity-100 transition-all cursor-pointer overflow-hidden"
                      style={{ borderColor: block.color + '30' }}
                    >
                      <div className="absolute inset-0 opacity-0 group-hover:opacity-100 transition-opacity" style={{ background: `linear-gradient(135deg, ${block.color}05, transparent)` }} />
                      <div className="relative flex justify-between items-start">
                        <div className="font-mono text-sm font-semibold" style={{ color: block.color }}>{block.name}</div>
                        <div className="text-xs text-slate-600 bg-slate-900 px-2 py-0.5 rounded font-mono">L{block.lines}</div>
                      </div>
                      <div className="relative text-xs text-slate-500 mt-2">{block.desc}</div>
                    </div>
                  ))}
                </div>
              </div>

              {/* 핵심 로직 */}
              <div className="grid grid-cols-2 gap-4">
                <div className="bg-gradient-to-br from-[#00ff88]/5 to-transparent rounded-xl p-5 border border-[#00ff88]/20">
                  <h3 className="font-bold text-[#00ff88] mb-4 tracking-wide">◈ ICMP PING FLOW</h3>
                  <div className="font-mono text-xs text-slate-400 space-y-2">
                    <div className="flex items-center gap-2"><span className="text-[#00ff88]">1</span> IcmpCreateFile()</div>
                    <div className="flex items-center gap-2"><span className="text-[#00ff88]">2</span> inet_addr() / getaddrinfo()</div>
                    <div className="flex items-center gap-2"><span className="text-[#00ff88]">3</span> IcmpSendEcho(timeout: 1000ms)</div>
                    <div className="flex items-center gap-2"><span className="text-[#00ff88]">4</span> ICMP_ECHO_REPLY → latency</div>
                  </div>
                </div>
                <div className="bg-gradient-to-br from-amber-500/5 to-transparent rounded-xl p-5 border border-amber-500/20">
                  <h3 className="font-bold text-amber-400 mb-4 tracking-wide">◈ TIMER CYCLE</h3>
                  <div className="font-mono text-xs text-slate-400 space-y-2">
                    <div className="flex items-center gap-2"><span className="text-amber-400">1</span> SetTimer(1000ms)</div>
                    <div className="flex items-center gap-2"><span className="text-amber-400">2</span> WM_TIMER → UpdateTarget() × n</div>
                    <div className="flex items-center gap-2"><span className="text-amber-400">3</span> BuildJsonData()</div>
                    <div className="flex items-center gap-2"><span className="text-amber-400">4</span> fwprintf → ping_data.json</div>
                  </div>
                </div>
              </div>
            </div>
          )}

          {/* === JS CODE 탭 === */}
          {activeTab === 'js-code' && (
            <div className="space-y-6">
              <div className="bg-slate-900/30 rounded-2xl border border-slate-800 p-6 backdrop-blur">
                <div className="flex items-center justify-between mb-6">
                  <div>
                    <h2 className="text-xl font-bold tracking-wide text-cyan-400">graph.html</h2>
                    <p className="text-sm text-slate-500">1,119 lines • Single-file Dashboard (HTML + CSS + JS)</p>
                  </div>
                  <div className="text-xs text-slate-600 font-mono bg-slate-800 px-3 py-1 rounded">Chart.js CDN</div>
                </div>

                <div className="grid grid-cols-2 gap-3">
                  {jsBlocks.map((block, i) => (
                    <div 
                      key={i} 
                      className="group relative bg-slate-800/30 rounded-lg p-4 border border-slate-700/50 hover:border-opacity-100 transition-all cursor-pointer overflow-hidden"
                      style={{ borderColor: block.color + '30' }}
                    >
                      <div className="absolute inset-0 opacity-0 group-hover:opacity-100 transition-opacity" style={{ background: `linear-gradient(135deg, ${block.color}05, transparent)` }} />
                      <div className="relative flex justify-between items-start">
                        <div className="font-mono text-sm font-semibold" style={{ color: block.color }}>{block.name}</div>
                        <div className="text-xs text-slate-600 bg-slate-900 px-2 py-0.5 rounded font-mono">L{block.lines}</div>
                      </div>
                      <div className="relative text-xs text-slate-500 mt-2">{block.desc}</div>
                    </div>
                  ))}
                </div>
              </div>

              {/* UI 컴포넌트 */}
              <div className="bg-slate-900/30 rounded-xl p-5 border border-slate-800">
                <h3 className="font-bold text-cyan-400 mb-4 tracking-wide">◉ UI COMPONENTS</h3>
                <div className="grid grid-cols-4 gap-3">
                  {[
                    { icon: '📊', name: '요약 카드', desc: '온라인/오프라인/평균/손실률' },
                    { icon: '📈', name: '비교 타임라인', desc: '전체 IP 통합 그래프' },
                    { icon: '🎯', name: 'IP 상태 카드', desc: '개별 상세 + 미니 차트' },
                    { icon: '🟩', name: '패킷 히트맵', desc: '60초 성공/실패 블록' },
                  ].map((comp, i) => (
                    <div key={i} className="bg-slate-800/30 rounded-lg p-4 text-center border border-slate-700/30 hover:border-cyan-400/30 transition-colors">
                      <div className="text-3xl mb-2">{comp.icon}</div>
                      <div className="text-sm font-semibold text-slate-300">{comp.name}</div>
                      <div className="text-xs text-slate-500 mt-1">{comp.desc}</div>
                    </div>
                  ))}
                </div>
              </div>
            </div>
          )}

          {/* === DATA FLOW 탭 === */}
          {activeTab === 'dataflow' && (
            <div className="bg-slate-900/30 rounded-2xl border border-slate-800 p-6 backdrop-blur">
              <h2 className="text-xl font-bold tracking-wide text-[#00ff88] mb-6">↯ DATA FLOW SEQUENCE</h2>
              
              <div className="space-y-4">
                {[
                  { step: 1, from: 'ping_config.ini', to: 'C Program', action: 'LoadConfig()', color: '#00ff88', desc: 'IP 목록 및 이름 파싱' },
                  { step: 2, from: 'Windows Timer', to: 'WndProc()', action: 'WM_TIMER', color: '#00d4ff', desc: '1초마다 트리거' },
                  { step: 3, from: 'C Program', to: 'ICMP API', action: 'IcmpSendEcho()', color: '#ffaa00', desc: '각 IP에 Echo Request' },
                  { step: 4, from: 'Network', to: 'ICMP API', action: 'ECHO_REPLY', color: '#ff6b6b', desc: '응답 또는 1초 타임아웃' },
                  { step: 5, from: 'ICMP API', to: 'IPTarget[]', action: 'UpdateTarget()', color: '#c084fc', desc: '통계 계산, 히스토리 추가' },
                  { step: 6, from: 'IPTarget[]', to: 'JSON String', action: 'BuildJsonData()', color: '#00ff88', desc: '전체 상태 직렬화' },
                  { step: 7, from: 'JSON String', to: 'ping_data.json', action: 'fwprintf()', color: '#00d4ff', desc: 'UTF-8 파일 저장' },
                  { step: 8, from: 'graph.html', to: 'ping_data.json', action: 'fetch()', color: '#ffaa00', desc: '1초마다 HTTP 요청' },
                  { step: 9, from: 'JSON Data', to: 'Chart.js', action: 'updateUI()', color: '#ff6b6b', desc: '그래프 + DOM 업데이트' },
                ].map((flow, i) => (
                  <div key={i} className="flex items-center gap-4 group">
                    <div 
                      className="w-10 h-10 rounded-full flex items-center justify-center font-bold text-sm border-2 transition-all group-hover:scale-110"
                      style={{ borderColor: flow.color, color: flow.color, background: flow.color + '10' }}
                    >
                      {flow.step}
                    </div>
                    <div className="flex-1 bg-slate-800/30 rounded-lg p-3 border border-slate-700/50 group-hover:border-opacity-100 transition-all" style={{ borderColor: flow.color + '30' }}>
                      <div className="flex items-center gap-2 text-sm">
                        <span className="text-slate-400">{flow.from}</span>
                        <span className="text-slate-600">→</span>
                        <span className="font-mono text-xs px-2 py-0.5 rounded" style={{ background: flow.color + '20', color: flow.color }}>{flow.action}</span>
                        <span className="text-slate-600">→</span>
                        <span className="text-slate-300">{flow.to}</span>
                      </div>
                      <div className="text-xs text-slate-500 mt-1">{flow.desc}</div>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* === JSON SCHEMA 탭 === */}
          {activeTab === 'json' && (
            <div className="space-y-6">
              <div className="bg-slate-900/30 rounded-2xl border border-slate-800 p-6 backdrop-blur">
                <h2 className="text-xl font-bold tracking-wide text-purple-400 mb-2">{ } JSON SCHEMA</h2>
                <p className="text-sm text-slate-500 mb-6">ping_data.json 실시간 데이터 구조</p>
                
                <div className="bg-[#0a0f0d] rounded-xl p-5 border border-slate-800 font-mono text-sm overflow-x-auto">
                  <pre className="text-slate-300">
{`{
  `}<span className="text-purple-400">"running"</span>{`: `}<span className="text-[#00ff88]">true</span>{`,              `}<span className="text-slate-600">// 모니터링 상태</span>{`
  `}<span className="text-purple-400">"elapsed"</span>{`: `}<span className="text-cyan-400">125</span>{`,              `}<span className="text-slate-600">// 경과 시간 (초)</span>{`
  `}<span className="text-purple-400">"total"</span>{`: `}<span className="text-cyan-400">600</span>{`,                `}<span className="text-slate-600">// 총 시간 (초)</span>{`
  `}<span className="text-purple-400">"loop"</span>{`: `}<span className="text-[#00ff88]">false</span>{`,              `}<span className="text-slate-600">// 무한 반복 모드</span>{`
  `}<span className="text-purple-400">"targets"</span>{`: [
    {
      `}<span className="text-amber-400">"ip"</span>{`: `}<span className="text-[#00ff88]">"8.8.8.8"</span>{`,        `}<span className="text-slate-600">// IP 주소</span>{`
      `}<span className="text-amber-400">"name"</span>{`: `}<span className="text-[#00ff88]">"Google DNS"</span>{`,   `}<span className="text-slate-600">// 별칭</span>{`
      `}<span className="text-amber-400">"latency"</span>{`: `}<span className="text-cyan-400">15</span>{`,           `}<span className="text-slate-600">// 현재 ms (-1=타임아웃)</span>{`
      `}<span className="text-amber-400">"online"</span>{`: `}<span className="text-cyan-400">1</span>{`,             `}<span className="text-slate-600">// 1=OK, 0=Fail, -1=Ready</span>{`
      `}<span className="text-amber-400">"total"</span>{`: `}<span className="text-cyan-400">125</span>{`,            `}<span className="text-slate-600">// 총 핑 횟수</span>{`
      `}<span className="text-amber-400">"success"</span>{`: `}<span className="text-cyan-400">122</span>{`,          `}<span className="text-slate-600">// 성공 횟수</span>{`
      `}<span className="text-amber-400">"min"</span>{`: `}<span className="text-cyan-400">12</span>{`,               `}<span className="text-slate-600">// 최소 지연</span>{`
      `}<span className="text-amber-400">"max"</span>{`: `}<span className="text-cyan-400">45</span>{`,               `}<span className="text-slate-600">// 최대 지연</span>{`
      `}<span className="text-amber-400">"avg"</span>{`: `}<span className="text-cyan-400">18.5</span>{`,             `}<span className="text-slate-600">// 평균 지연</span>{`
      `}<span className="text-amber-400">"history"</span>{`: [`}<span className="text-cyan-400">15</span>{`, `}<span className="text-cyan-400">14</span>{`, `}<span className="text-red-400">-1</span>{`, `}<span className="text-cyan-400">16</span>{`, ...]  `}<span className="text-slate-600">// 최근 60개</span>{`
    }
  ]
}`}
                  </pre>
                </div>
              </div>

              {/* 필드 설명 */}
              <div className="grid grid-cols-2 gap-4">
                <div className="bg-slate-900/30 rounded-xl p-5 border border-purple-500/20">
                  <h3 className="font-bold text-purple-400 mb-4">ROOT FIELDS</h3>
                  <div className="space-y-2 text-sm">
                    {[
                      { key: 'running', type: 'bool', desc: '모니터링 실행 중' },
                      { key: 'elapsed', type: 'int', desc: '경과된 초' },
                      { key: 'total', type: 'int', desc: '설정된 총 시간' },
                      { key: 'loop', type: 'bool', desc: '무한 반복 여부' },
                    ].map((f, i) => (
                      <div key={i} className="flex items-center gap-3">
                        <span className="font-mono text-purple-300">{f.key}</span>
                        <span className="text-xs text-slate-600 bg-slate-800 px-2 py-0.5 rounded">{f.type}</span>
                        <span className="text-slate-500">{f.desc}</span>
                      </div>
                    ))}
                  </div>
                </div>
                <div className="bg-slate-900/30 rounded-xl p-5 border border-amber-500/20">
                  <h3 className="font-bold text-amber-400 mb-4">TARGET FIELDS</h3>
                  <div className="space-y-2 text-sm">
                    {[
                      { key: 'history', type: 'int[]', desc: '최근 60개 지연시간' },
                      { key: 'online', type: 'int', desc: '1=OK, 0=Fail, -1=Ready' },
                      { key: 'latency', type: 'int', desc: '-1이면 타임아웃' },
                      { key: 'avg', type: 'float', desc: '소수점 1자리' },
                    ].map((f, i) => (
                      <div key={i} className="flex items-center gap-3">
                        <span className="font-mono text-amber-300">{f.key}</span>
                        <span className="text-xs text-slate-600 bg-slate-800 px-2 py-0.5 rounded">{f.type}</span>
                        <span className="text-slate-500">{f.desc}</span>
                      </div>
                    ))}
                  </div>
                </div>
              </div>
            </div>
          )}
        </main>

        {/* 푸터 */}
        <footer className="mt-12 pt-6 border-t border-slate-800 text-center">
          <div className="text-xs text-slate-600 tracking-widest">
            PING MONITOR WEBVIEW2 EDITION • MIT LICENSE
          </div>
        </footer>
      </div>

      {/* 소스코드 모달 */}
      {selectedFile && sourceCode[selectedFile] && (
        <div 
          className="fixed inset-0 bg-black/80 backdrop-blur-sm z-50 flex items-center justify-center p-4"
          onClick={() => setSelectedFile(null)}
        >
          <div 
            className="bg-[#0a0f0d] rounded-2xl border border-slate-700 w-full max-w-5xl max-h-[85vh] flex flex-col overflow-hidden"
            onClick={e => e.stopPropagation()}
          >
            {/* 모달 헤더 */}
            <div className="flex items-center justify-between p-4 border-b border-slate-800">
              <div className="flex items-center gap-3">
                <div 
                  className="w-10 h-10 rounded-lg flex items-center justify-center font-mono text-sm border"
                  style={{ 
                    background: sourceCode[selectedFile].color + '10', 
                    borderColor: sourceCode[selectedFile].color + '30', 
                    color: sourceCode[selectedFile].color 
                  }}
                >
                  {sourceCode[selectedFile].language}
                </div>
                <div>
                  <div className="font-bold" style={{ color: sourceCode[selectedFile].color }}>
                    {sourceCode[selectedFile].name}
                  </div>
                  <div className="text-xs text-slate-500">소스코드 보기</div>
                </div>
              </div>
              <button 
                className="w-10 h-10 rounded-lg bg-slate-800 hover:bg-slate-700 flex items-center justify-center text-slate-400 hover:text-white transition-colors"
                onClick={() => setSelectedFile(null)}
              >
                ✕
              </button>
            </div>
            
            {/* 코드 영역 */}
            <div className="flex-1 overflow-auto p-4">
              <pre className="font-mono text-sm text-slate-300 leading-relaxed whitespace-pre-wrap">
                <code>{sourceCode[selectedFile].code}</code>
              </pre>
            </div>
            
            {/* 모달 푸터 */}
            <div className="flex items-center justify-between p-4 border-t border-slate-800 bg-slate-900/50">
              <div className="text-xs text-slate-500">
                ESC 또는 바깥 클릭으로 닫기
              </div>
              <div className="flex gap-2">
                <button 
                  className="px-4 py-2 rounded-lg bg-slate-800 hover:bg-slate-700 text-sm text-slate-300 transition-colors"
                  onClick={() => {
                    navigator.clipboard.writeText(sourceCode[selectedFile].code);
                  }}
                >
                  📋 복사
                </button>
                <button 
                  className="px-4 py-2 rounded-lg text-sm transition-colors"
                  style={{ 
                    background: sourceCode[selectedFile].color + '20', 
                    color: sourceCode[selectedFile].color 
                  }}
                  onClick={() => setSelectedFile(null)}
                >
                  닫기
                </button>
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};

export default PingMonitorArchitecture;