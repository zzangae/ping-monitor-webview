/**
 * IP 비교 타임라인 섹션 JavaScript
 * Ping Monitor v2.7
 */

// ========================================================================
// 전역 변수 (Timeline)
// ========================================================================
let comparisonChart = null;
let extendedHistory = {};  // IP별 확장 히스토리 저장 {ip: [latency, ...]}
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
            plugins: {
                legend: {
                    display: true,
                    position: 'top',
                    labels: { color: '#e4e6eb' }
                }
            },
            scales: {
                y: {
                    beginAtZero: true,
                    grid: { color: '#3a3f5c' },
                    ticks: { color: '#b0b3b8' }
                },
                x: {
                    grid: { color: '#3a3f5c' },
                    ticks: { color: '#b0b3b8' }
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

/**
 * 비교 차트 업데이트 (v2.7 - Extended Timeline Support)
 * @param {Array} targets - 핑 대상 배열
 */
function updateComparisonChart(targets) {
    if (!comparisonChart || !targets) return;
    
    // 확장 히스토리 업데이트
    targets.forEach(target => {
        const key = target.ip;
        if (!extendedHistory[key]) {
            extendedHistory[key] = [];
        }
        
        // 최신 latency 값 추가 (초당 1개)
        const currentLatency = target.online ? target.latency : 0;
        extendedHistory[key].push(currentLatency);
        
        // 최대 포인트 수 제한
        if (extendedHistory[key].length > MAX_HISTORY_POINTS) {
            extendedHistory[key].shift();
        }
    });
    
    // 시간 범위에 따른 레이블 생성
    const labels = generateTimeLabels(selectedTimeRange);
    comparisonChart.data.labels = labels;
    
    const filteredTargets = targets.filter((target, index) => visibleIPs.has(index));
    
    comparisonChart.data.datasets = filteredTargets.map((target) => {
        const originalIndex = targets.findIndex(t => t.ip === target.ip && t.name === target.name);
        const key = target.ip;
        
        // 확장 히스토리에서 필요한 범위만 가져오기
        const history = extendedHistory[key] || [];
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
    
    comparisonChart.update();
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
    const interval = Math.floor(timeRangeSeconds / totalPoints);
    
    // 필요한 데이터 포인트 수
    const neededPoints = timeRangeSeconds;
    
    // 히스토리가 충분하지 않으면 있는 만큼만 사용
    const availableHistory = history.slice(-neededPoints);
    
    if (availableHistory.length === 0) {
        return Array(totalPoints).fill(null);
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
    localStorage.setItem('selectedTimeRange', seconds);
    
    // 레이블 업데이트
    updateTimelineRangeLabel();
    
    console.log('Timeline range changed to:', seconds, 'seconds');
}

/**
 * 타임라인 범위 레이블 업데이트
 */
function updateTimelineRangeLabel() {
    const label = document.getElementById('timelineRangeLabel');
    if (!label) return;
    
    const rangeText = formatTimeRange(selectedTimeRange);
    label.textContent = `(${rangeText})`;
}

/**
 * 시간 범위 포맷팅
 * @param {number} seconds - 초
 * @returns {string} 포맷된 문자열
 */
function formatTimeRange(seconds) {
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
 */
function loadTimelineRange() {
    const saved = localStorage.getItem('selectedTimeRange');
    if (saved) {
        selectedTimeRange = parseInt(saved);
        updateTimelineRangeLabel();
    }
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
            visibleIPs = new Set(visibleArray);
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
    extendedHistory = {};
}

// ========================================================================
// 전역 노출 (window 객체)
// ========================================================================
window.comparisonChart = comparisonChart;
window.extendedHistory = extendedHistory;
window.selectedTimeRange = selectedTimeRange;
window.visibleIPs = visibleIPs;
window.brightColors = brightColors;

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