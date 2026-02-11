/**
 * IP 비교 타임라인 섹션 JavaScript
 * Ping Monitor v2.7 - TypedArray 최적화 버전
 */

// ========================================================================
// CircularBuffer 클래스 (TypedArray 기반 - 메모리 최적화)
// ========================================================================

/**
 * 고정 크기 순환 버퍼 (Float32Array 사용)
 * - 배열 재할당 없음 (slice/shift 제거)
 * - 메모리 사용량 고정 (86,400 * 4 bytes = 약 337KB per IP)
 * - O(1) 삽입 성능
 */
class CircularBuffer {
    constructor(size) {
        this.size = size;
        this.buffer = new Float32Array(size);  // 4 bytes per element
        this.head = 0;      // 다음 쓰기 위치
        this.count = 0;     // 현재 데이터 개수
    }
    
    /**
     * 데이터 추가 (O(1))
     */
    push(value) {
        this.buffer[this.head] = value;
        this.head = (this.head + 1) % this.size;
        if (this.count < this.size) {
            this.count++;
        }
    }
    
    /**
     * 최근 n개 데이터 가져오기
     * @param {number} n - 가져올 개수
     * @returns {Array} 일반 배열 (Chart.js 호환)
     */
    getRecent(n) {
        const count = Math.min(n, this.count);
        const result = new Array(count);
        
        // 가장 오래된 데이터부터 순서대로
        let readPos = (this.head - this.count + this.size) % this.size;
        const startPos = Math.max(0, this.count - n);
        readPos = (readPos + startPos) % this.size;
        
        for (let i = 0; i < count; i++) {
            result[i] = this.buffer[readPos];
            readPos = (readPos + 1) % this.size;
        }
        
        return result;
    }
    
    /**
     * 현재 저장된 데이터 개수
     */
    get length() {
        return this.count;
    }
    
    /**
     * 버퍼 초기화
     */
    clear() {
        this.buffer.fill(0);
        this.head = 0;
        this.count = 0;
    }
}

// ========================================================================
// 전역 변수 (Timeline)
// ========================================================================
let comparisonChart = null;
let extendedHistory = new Map();  // IP별 CircularBuffer 저장 {ip: CircularBuffer}
let selectedTimeRange = 60;  // 기본 1분 (60초)
const MAX_HISTORY_POINTS = 86400;  // 최대 24시간 (초 단위)

// Chart Visibility (차트에 표시할 IP)
let visibleIPs = new Set();

// 차트 색상 팔레트
const brightColors = [
    '#FF6B9D', '#C44569', '#FFA07A', '#FF8C42',
    '#FFD93D', '#6BCF7F', '#4ECDC4', '#45B7D1',
    '#5F27CD', '#A55EEA', '#FF6348', '#FF9FF3',
    '#54A0FF', '#48DBFB', '#1DD1A1', '#00D2D3',
    '#FD79A8', '#FDCB6E', '#6C5CE7', '#A29BFE',
    '#74B9FF', '#81ECEC', '#55EFC4', '#FFEAA7'
];

// ========================================================================
// 초기화 함수
// ========================================================================

/**
 * 비교 차트 초기화
 */
function initComparisonChart() {
    const ctx = document.getElementById('comparisonChart');
    if (!ctx) {
        console.error('comparisonChart canvas not found');
        return;
    }
    
    comparisonChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: []
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            normalized: true,        // 데이터 정규화 (성능 향상)
            spanGaps: true,          // null 값 건너뛰기
            plugins: {
                legend: {
                    display: true,
                    position: 'top',
                    labels: { color: '#e4e6eb' }
                },
                decimation: {         // 데이터 간소화 (성능 향상)
                    enabled: true,
                    algorithm: 'lttb'
                }
            },
            scales: {
                y: {
                    beginAtZero: true,
                    grid: { color: '#3a3f5c' },
                    ticks: { color: '#b0b3b8' },
                    title: {
                        display: true,
                        text: 'ms',
                        color: '#b0b3b8'
                    }
                },
                x: {
                    grid: { color: '#3a3f5c' },
                    ticks: { color: '#b0b3b8' }
                }
            },
            elements: {
                line: {
                    borderJoinStyle: 'round'
                },
                point: {
                    radius: 0,           // 기본 점 비활성화
                    hoverRadius: 4       // 호버 시 점 표시
                }
            }
        }
    });
}

/**
 * 타임라인 이벤트 초기화
 */
function initTimelineEvents() {
    // 최소화 버튼
    const timelineMinimizeBtn = document.getElementById('timelineMinimizeBtn');
    const timelineContent = document.getElementById('timelineContent');
    
    if (timelineMinimizeBtn && timelineContent) {
        timelineMinimizeBtn.addEventListener('click', () => {
            const isCollapsed = timelineContent.classList.toggle('collapsed');
            timelineMinimizeBtn.classList.toggle('minimized', isCollapsed);
            
            // 상태 저장
            localStorage.setItem('timelineCollapsed', isCollapsed ? '1' : '0');
        });
        
        // 저장된 상태 복원
        if (localStorage.getItem('timelineCollapsed') === '1') {
            timelineContent.classList.add('collapsed');
            timelineMinimizeBtn.classList.add('minimized');
        }
    }
}

// ========================================================================
// 업데이트 함수
// ========================================================================

// 마지막 차트 업데이트 시간 (전역으로 관리)
window.lastChartUpdate = 0;
window.chartErrorCount = 0;  // 에러 카운트
window.lastChartReset = Date.now();  // 마지막 차트 리셋 시간

/**
 * 비교 차트 업데이트 (v2.7 - Extended Timeline Support)
 * @param {Array} targets - 핑 대상 배열
 */
function updateComparisonChart(targets) {
    try {
        if (!comparisonChart || !targets) return;
        
        const now = Date.now();
        
        // 12시간마다 차트 인스턴스 재생성 (메모리 누수 방지)
        const hoursSinceReset = (now - window.lastChartReset) / (1000 * 60 * 60);
        if (hoursSinceReset >= 12) {
            console.log('12시간 경과 - 차트 인스턴스 재생성');
            resetChartInstance();
            window.lastChartReset = now;
            return;
        }
        
        // 확장 히스토리 업데이트 (CircularBuffer 사용)
        targets.forEach(target => {
            const key = target.ip;
            if (!extendedHistory.has(key)) {
                extendedHistory.set(key, new CircularBuffer(MAX_HISTORY_POINTS));
            }
            
            // 최신 latency 값 추가 (O(1) 연산)
            const currentLatency = target.online ? target.latency : 0;
            extendedHistory.get(key).push(currentLatency);
        });
        
        // 사용하지 않는 IP 히스토리 정리
        const activeIPs = new Set(targets.map(t => t.ip));
        for (const key of extendedHistory.keys()) {
            if (!activeIPs.has(key)) {
                extendedHistory.delete(key);
            }
        }
        
        // 차트 업데이트 빈도 조절 (시간 범위에 따라)
        let updateInterval;
        if (selectedTimeRange <= 60) {
            updateInterval = 1000;       // 1분 이하: 매초
        } else if (selectedTimeRange <= 300) {
            updateInterval = 2000;       // 5분 이하: 2초마다
        } else if (selectedTimeRange <= 1800) {
            updateInterval = 5000;       // 30분 이하: 5초마다
        } else if (selectedTimeRange <= 3600) {
            updateInterval = 10000;      // 1시간 이하: 10초마다
        } else {
            updateInterval = 30000;      // 1시간 초과: 30초마다
        }
        
        // 업데이트 간격 확인
        if (now - window.lastChartUpdate < updateInterval) {
            return;  // 아직 업데이트 시간이 안 됨
        }
        window.lastChartUpdate = now;
        
        // 시간 범위에 따른 레이블 생성
        const labels = generateTimeLabels(selectedTimeRange);
        comparisonChart.data.labels = labels;
        
        const filteredTargets = targets.filter((target, index) => visibleIPs.has(index));
        
        comparisonChart.data.datasets = filteredTargets.map((target) => {
            const originalIndex = targets.findIndex(t => t.ip === target.ip && t.name === target.name);
            const key = target.ip;
            
            // CircularBuffer에서 필요한 범위만 가져오기
            const buffer = extendedHistory.get(key);
            const history = buffer ? buffer.getRecent(selectedTimeRange) : [];
            const displayData = getDisplayData(history, selectedTimeRange);
            
            return {
                label: target.name,
                data: displayData,
                borderColor: brightColors[originalIndex % brightColors.length],
                backgroundColor: brightColors[originalIndex % brightColors.length] + '20',
                tension: 0.4,
                borderWidth: 2,
                fill: false,
                pointRadius: selectedTimeRange <= 300 ? 2 : 0,  // 5분 이하면 점 표시
                pointHoverRadius: 4
            };
        });
        
        // requestAnimationFrame으로 렌더링 최적화
        requestAnimationFrame(() => {
            comparisonChart.update('none');  // 애니메이션 없이 업데이트
        });
        
        // 성공 시 에러 카운트 리셋
        window.chartErrorCount = 0;
        
    } catch (error) {
        console.error('차트 업데이트 오류:', error);
        window.chartErrorCount++;
        
        // 연속 5회 에러 시 차트 재생성
        if (window.chartErrorCount >= 5) {
            console.log('연속 에러 발생 - 차트 인스턴스 재생성 시도');
            resetChartInstance();
            window.chartErrorCount = 0;
        }
    }
}

/**
 * 차트 인스턴스 재생성 (메모리 정리)
 */
function resetChartInstance() {
    try {
        if (comparisonChart) {
            comparisonChart.destroy();
            comparisonChart = null;
        }
        
        // 잠시 후 재생성
        setTimeout(() => {
            initComparisonChart();
            console.log('차트 인스턴스 재생성 완료');
        }, 100);
    } catch (error) {
        console.error('차트 재생성 오류:', error);
    }
}

/**
 * 시간 범위에 따른 레이블 생성 (v2.7)
 * @param {number} timeRangeSeconds - 시간 범위 (초)
 * @returns {Array} 레이블 배열
 */
function generateTimeLabels(timeRangeSeconds) {
    const totalPoints = Math.min(60, timeRangeSeconds);  // 최대 60개 포인트
    const interval = timeRangeSeconds / totalPoints;
    const labels = [];
    
    for (let i = totalPoints; i >= 1; i--) {
        const secondsAgo = Math.round(i * interval);
        
        if (timeRangeSeconds <= 60) {
            // 1분: 초 단위
            labels.push(secondsAgo + '초');
        } else if (timeRangeSeconds <= 3600) {
            // 1시간 이하: 분:초
            const mins = Math.floor(secondsAgo / 60);
            const secs = secondsAgo % 60;
            if (secs === 0) {
                labels.push(mins + '분');
            } else {
                labels.push(mins + ':' + String(secs).padStart(2, '0'));
            }
        } else {
            // 1시간 초과: 시:분
            const hours = Math.floor(secondsAgo / 3600);
            const mins = Math.floor((secondsAgo % 3600) / 60);
            if (hours > 0) {
                labels.push(hours + '시간 ' + mins + '분');
            } else {
                labels.push(mins + '분');
            }
        }
    }
    
    return labels;
}

/**
 * 확장 히스토리에서 표시할 데이터 추출 (v2.7)
 * @param {Array} history - 히스토리 데이터
 * @param {number} timeRangeSeconds - 시간 범위 (초)
 * @returns {Array} 표시할 데이터
 */
function getDisplayData(history, timeRangeSeconds) {
    const totalPoints = Math.min(60, timeRangeSeconds);  // 최대 60개 포인트
    
    // 히스토리가 없으면 즉시 빈 배열 반환
    if (!history || history.length === 0) {
        return Array(totalPoints).fill(null);
    }
    
    const interval = Math.floor(timeRangeSeconds / totalPoints);
    
    // 실제 사용할 데이터 범위 (최대 timeRangeSeconds개, 하지만 있는 만큼만)
    const availableHistory = history.length > timeRangeSeconds 
        ? history.slice(-timeRangeSeconds) 
        : history;
    
    // 데이터가 너무 적으면 있는 만큼만 표시
    if (availableHistory.length < totalPoints) {
        // 데이터가 적을 때는 단순하게 처리
        const result = Array(totalPoints).fill(null);
        const startPos = totalPoints - availableHistory.length;
        for (let i = 0; i < availableHistory.length; i++) {
            result[startPos + i] = availableHistory[i] > 0 ? availableHistory[i] : null;
        }
        return result;
    }
    
    // 샘플링하여 60개 이하의 포인트로 줄이기
    const result = [];
    for (let i = 0; i < totalPoints; i++) {
        const startIdx = Math.floor(i * interval);
        const endIdx = Math.min(startIdx + interval, availableHistory.length);
        
        if (startIdx < availableHistory.length) {
            // 해당 구간의 평균값 계산
            const slice = availableHistory.slice(startIdx, endIdx);
            const validValues = slice.filter(v => v !== null && v !== undefined && v > 0);
            
            if (validValues.length > 0) {
                const avg = validValues.reduce((a, b) => a + b, 0) / validValues.length;
                result.push(Math.round(avg));
            } else {
                result.push(null);
            }
        } else {
            result.push(null);
        }
    }
    
    return result;
}

// ========================================================================
// 유틸리티 함수
// ========================================================================

/**
 * 시간 범위 변경
 * @param {number} seconds - 새 시간 범위 (초)
 */
function setTimelineRange(seconds) {
    selectedTimeRange = seconds;
    sessionStorage.setItem('selectedTimeRange', seconds);
    
    // 레이블 업데이트
    updateTimelineRangeLabel();
    
    // 즉시 차트 업데이트를 위해 타이머 리셋
    window.lastChartUpdate = 0;
    
    console.log('Timeline range changed to:', seconds, 'seconds');
}

/**
 * 타임라인 범위 레이블 업데이트
 */
function updateTimelineRangeLabel() {
    const label = document.getElementById('timelineRangeLabel');
    if (!label) return;
    
    const rangeText = formatTimeRange(selectedTimeRange);
    // 빈 값이면 공백으로 처리
    label.textContent = rangeText ? `(${rangeText})` : '';
}

/**
 * 시간 범위 포맷팅
 * @param {number} seconds - 초
 * @returns {string} 포맷된 문자열
 */
function formatTimeRange(seconds) {
    // NaN 또는 유효하지 않은 값 처리
    if (!seconds || isNaN(seconds) || seconds <= 0) {
        return '';
    }
    
    if (seconds < 60) {
        return seconds + '초';
    } else if (seconds < 3600) {
        return (seconds / 60) + '분';
    } else {
        const hours = seconds / 3600;
        return hours + '시간';
    }
}

/**
 * 저장된 시간 범위 로드
 * sessionStorage 사용: 브라우저 탭/창이 열려있는 동안만 유지
 * 프로그램 재시작 시에는 1분(60초)으로 시작
 */
function loadTimelineRange() {
    const saved = sessionStorage.getItem('selectedTimeRange');
    if (saved) {
        selectedTimeRange = parseInt(saved);
    } else {
        selectedTimeRange = 60;  // 기본 1분
    }
    updateTimelineRangeLabel();
}

/**
 * 표시할 IP 상태 저장
 */
function saveVisibleIPsState() {
    const visibleArray = Array.from(visibleIPs);
    localStorage.setItem('visibleIPs', JSON.stringify(visibleArray));
    console.log('필터 설정 저장:', visibleArray);
}

/**
 * 표시할 IP 상태 로드
 * @returns {boolean} 로드 성공 여부
 */
function loadVisibleIPsState() {
    const saved = localStorage.getItem('visibleIPs');
    if (saved) {
        try {
            const visibleArray = JSON.parse(saved);
            // 기존 Set을 비우고 새 값 추가 (참조 유지)
            visibleIPs.clear();
            visibleArray.forEach(index => visibleIPs.add(index));
            console.log('필터 설정 로드:', visibleArray);
            return true;
        } catch (e) {
            console.error('필터 설정 로드 실패:', e);
        }
    }
    return false;
}

/**
 * 차트 데이터 리셋
 */
function resetComparisonChart() {
    if (comparisonChart) {
        comparisonChart.data.labels = [];
        comparisonChart.data.datasets = [];
        comparisonChart.update();
    }
    // Map 초기화 (기존 CircularBuffer 메모리 해제)
    extendedHistory.clear();
}

// ========================================================================
// 전역 노출 (window 객체)
// ========================================================================
window.CircularBuffer = CircularBuffer;  // 클래스 노출
window.comparisonChart = comparisonChart;
window.extendedHistory = extendedHistory;
window.selectedTimeRange = selectedTimeRange;
window.visibleIPs = visibleIPs;
window.brightColors = brightColors;
// window.lastChartUpdate는 이미 선언됨

window.initComparisonChart = initComparisonChart;
window.initTimelineEvents = initTimelineEvents;
window.updateComparisonChart = updateComparisonChart;
window.generateTimeLabels = generateTimeLabels;
window.getDisplayData = getDisplayData;
window.setTimelineRange = setTimelineRange;
window.updateTimelineRangeLabel = updateTimelineRangeLabel;
window.formatTimeRange = formatTimeRange;
window.loadTimelineRange = loadTimelineRange;
window.saveVisibleIPsState = saveVisibleIPsState;
window.loadVisibleIPsState = loadVisibleIPsState;
window.resetComparisonChart = resetComparisonChart;
window.resetChartInstance = resetChartInstance;