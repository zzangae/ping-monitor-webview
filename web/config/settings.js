/**
 * Ping Monitor v2.7 - Settings Module
 * 환경설정 모달 관련 JavaScript
 * 
 * 의존성:
 * - currentData (전역 변수)
 * - visibleIPs (전역 변수)
 * - selectedTimeRange (전역 변수)
 * - brightColors (전역 변수)
 * - updateComparisonChart (함수)
 */

// ============================================
// 환경설정 모달 열기/닫기
// ============================================

/**
 * 환경설정 모달 열기
 */
function openChartSettings() {
    if (!currentData || !currentData.targets) return;
    
    populateIPCheckboxList(currentData.targets);
    document.getElementById('chartSettingsModal').classList.add('active');
    document.body.style.overflow = 'hidden';  // 배경 스크롤 방지
    
    // 마우스 휠 이벤트 초기화 (그룹별 임계값 탭용)
    setTimeout(() => {
        initThresholdWheelEvents();
    }, 100);
}

/**
 * 환경설정 모달 닫기
 */
function closeChartSettingsModal() {
    document.getElementById('chartSettingsModal').classList.remove('active');
    document.body.style.overflow = '';  // 배경 스크롤 복원
}

// ============================================
// 차트 설정 탭 기능
// ============================================

/**
 * IP 체크박스 목록 생성
 * @param {Array} targets - IP 타겟 배열
 */
function populateIPCheckboxList(targets) {
    const listContainer = document.getElementById('ipCheckboxList');
    if (!listContainer) return;
    
    listContainer.innerHTML = '';

    targets.forEach((target, index) => {
        const item = document.createElement('div');
        item.className = 'ip-checkbox-item';
        
        const checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.id = `ip-check-${index}`;
        checkbox.checked = visibleIPs.has(index);
        checkbox.addEventListener('change', (e) => {
            if (e.target.checked) {
                visibleIPs.add(index);
            } else {
                visibleIPs.delete(index);
            }
            updateComparisonChart(targets);
        });

        const label = document.createElement('label');
        label.className = 'ip-checkbox-label';
        label.htmlFor = `ip-check-${index}`;
        
        const colorBox = document.createElement('div');
        colorBox.className = 'ip-checkbox-color';
        colorBox.style.backgroundColor = brightColors[index % brightColors.length];
        
        const nameSpan = document.createElement('span');
        nameSpan.className = 'ip-checkbox-name';
        nameSpan.textContent = target.name;
        
        const ipSpan = document.createElement('span');
        ipSpan.className = 'ip-checkbox-ip';
        ipSpan.textContent = target.ip;
        
        label.appendChild(colorBox);
        label.appendChild(nameSpan);
        label.appendChild(ipSpan);
        
        item.appendChild(checkbox);
        item.appendChild(label);
        listContainer.appendChild(item);
    });
}

/**
 * 모든 IP 선택
 */
function selectAllIPsForChart() {
    if (!currentData || !currentData.targets) return;
    
    currentData.targets.forEach((_, index) => {
        visibleIPs.add(index);
        const checkbox = document.getElementById(`ip-check-${index}`);
        if (checkbox) checkbox.checked = true;
    });
    
    updateComparisonChart(currentData.targets);
}

/**
 * 모든 IP 선택 해제
 */
function deselectAllIPsForChart() {
    if (!currentData || !currentData.targets) return;
    
    visibleIPs.clear();
    currentData.targets.forEach((_, index) => {
        const checkbox = document.getElementById(`ip-check-${index}`);
        if (checkbox) checkbox.checked = false;
    });
    
    updateComparisonChart(currentData.targets);
}

// ============================================
// 시간 범위 탭 기능
// ============================================

/**
 * 초 단위를 읽기 쉬운 형식으로 변환
 * @param {number} seconds - 초 단위 값
 * @returns {string} 포맷된 문자열 (예: "1분", "1시간")
 */
function formatTimeRangeLabel(seconds) {
    // NaN 또는 유효하지 않은 값 처리
    if (!seconds || isNaN(seconds) || seconds <= 0) {
        return '';
    }
    
    if (seconds < 60) {
        return seconds + '초';
    } else if (seconds < 3600) {
        const minutes = Math.floor(seconds / 60);
        return minutes + '분';
    } else {
        const hours = Math.floor(seconds / 3600);
        return hours + '시간';
    }
}

/**
 * 타임라인 제목의 시간 범위 라벨 업데이트
 * @param {number} seconds - 선택된 시간 범위 (초)
 */
function updateTimelineRangeLabel(seconds) {
    const labelElement = document.getElementById('timelineRangeLabel');
    if (labelElement) {
        const rangeText = formatTimeRangeLabel(seconds);
        // 빈 값이면 공백으로 처리
        labelElement.textContent = rangeText ? '(' + rangeText + ')' : '';
    }
}

/**
 * 시간 범위 라디오 버튼 초기화
 */
function initTimeRangeRadios() {
    const timeRangeRadios = document.querySelectorAll('input[name="timeRange"]');
    if (timeRangeRadios.length === 0) return;
    
    // 저장된 시간 범위 복원 (sessionStorage - 재시작 시 리셋됨)
    const savedRange = sessionStorage.getItem('timelineRange');
    if (savedRange) {
        selectedTimeRange = parseInt(savedRange);
        const savedRadio = document.querySelector(`input[name="timeRange"][value="${savedRange}"]`);
        if (savedRadio) {
            savedRadio.checked = true;
        }
        // 라벨 업데이트
        updateTimelineRangeLabel(selectedTimeRange);
    } else {
        // 기본값 1분 선택
        selectedTimeRange = 60;
        const defaultRadio = document.querySelector('input[name="timeRange"][value="60"]');
        if (defaultRadio) {
            defaultRadio.checked = true;
        }
        updateTimelineRangeLabel(60);
    }
    
    // 라디오 버튼 변경 이벤트
    timeRangeRadios.forEach(radio => {
        radio.addEventListener('change', (e) => {
            selectedTimeRange = parseInt(e.target.value);
            sessionStorage.setItem('timelineRange', selectedTimeRange.toString());
            console.log('타임라인 범위 변경:', selectedTimeRange, '초');
            
            // 타임라인 제목 라벨 업데이트
            updateTimelineRangeLabel(selectedTimeRange);
            
            // 즉시 차트 업데이트를 위해 타이머 리셋
            window.lastChartUpdate = 0;
            
            // 차트 즉시 업데이트
            if (currentData && currentData.targets) {
                updateComparisonChart(currentData.targets);
            }
        });
    });
}

// ============================================
// 그룹별 임계값 탭 기능
// ============================================

/**
 * 슬라이더 값을 숫자 입력 필드에 동기화
 * @param {string} baseId - 기본 ID (예: 'thresh_서버')
 */
function syncThresholdInput(baseId) {
    const slider = document.getElementById(baseId + '_slider');
    const input = document.getElementById(baseId);
    if (slider && input) {
        input.value = slider.value;
    }
}

/**
 * 숫자 입력 필드 값을 슬라이더에 동기화
 * @param {string} baseId - 기본 ID (예: 'thresh_서버')
 */
function syncThresholdSlider(baseId) {
    const input = document.getElementById(baseId);
    const slider = document.getElementById(baseId + '_slider');
    if (input && slider) {
        // 슬라이더 범위 내로 제한 (30~600)
        let value = parseInt(input.value) || 300;
        value = Math.max(30, Math.min(600, value));
        slider.value = value;
    }
}

/**
 * 모든 슬라이더를 입력 필드 값과 동기화
 */
function syncAllThresholdSliders() {
    const ids = ['defaultThreshold', 'thresh_서버', 'thresh_네트워크', 'thresh_방화벽', 
                 'thresh_데이터베이스', 'thresh_웹서버', 'thresh_스토리지', 'thresh_기타'];
    ids.forEach(id => syncThresholdSlider(id));
}

/**
 * 숫자 입력 필드에 마우스 휠 이벤트 추가
 * 휠 위로: 값 증가, 휠 아래로: 값 감소
 */
function initThresholdWheelEvents() {
    const inputs = document.querySelectorAll('#settings-thresholds input[type="number"]');
    
    inputs.forEach(input => {
        // 이미 이벤트가 바인딩되어 있으면 스킵
        if (input.dataset.wheelBound) return;
        input.dataset.wheelBound = 'true';
        
        input.addEventListener('wheel', function(e) {
            e.preventDefault();
            e.stopPropagation();
            
            const step = parseInt(this.step) || 10;
            const min = parseInt(this.min) || 30;
            const max = parseInt(this.max) || 3600;
            let value = parseInt(this.value) || 300;
            
            if (e.deltaY < 0) {
                // 휠 위로 - 값 증가
                value = Math.min(value + step, max);
            } else {
                // 휠 아래로 - 값 감소
                value = Math.max(value - step, min);
            }
            
            this.value = value;
            
            // 슬라이더 동기화
            const baseId = this.id;
            syncThresholdSlider(baseId);
            
            // 시각적 피드백
            this.style.backgroundColor = 'rgba(99, 102, 241, 0.2)';
            setTimeout(() => {
                this.style.backgroundColor = '';
            }, 150);
        }, { passive: false });
    });
    
    console.log('임계값 마우스 휠 이벤트 초기화:', inputs.length, '개 입력 필드');
}

/**
 * 임계값 설정 로드 (localStorage 우선)
 */
async function loadThresholdSettings() {
    // 마우스 휠 이벤트 초기화
    initThresholdWheelEvents();
    
    // 먼저 localStorage에서 로드 시도
    const savedThresholds = localStorage.getItem('thresholdSettings');
    if (savedThresholds) {
        try {
            const thresholds = JSON.parse(savedThresholds);
            
            // 기본 임계값
            if (thresholds.default) {
                const input = document.getElementById('defaultThreshold');
                if (input) input.value = thresholds.default;
            }
            
            // 그룹별 임계값
            const groups = ['서버', '네트워크', '방화벽', '데이터베이스', '웹서버', '스토리지', '기타'];
            groups.forEach(group => {
                if (thresholds[group]) {
                    const input = document.getElementById('thresh_' + group);
                    if (input) input.value = thresholds[group];
                }
            });
            
            // 슬라이더 동기화
            syncAllThresholdSliders();
            
            console.log('임계값 설정 로드 (localStorage):', thresholds);
            return;
        } catch (e) {
            console.error('localStorage 임계값 파싱 실패:', e);
        }
    }
    
    // localStorage에 없으면 서버에서 로드
    try {
        const response = await fetch('/config/ping_config.ini?t=' + Date.now());
        const text = await response.text();
        
        // Parse [OutageDetection] section
        const outageRegex = /\[OutageDetection\]([\s\S]*?)(?:\[|$)/;
        const match = text.match(outageRegex);
        
        if (match) {
            const section = match[1];
            
            const parseValue = (key) => {
                const regex = new RegExp(key + '\\s*=\\s*(\\d+)');
                const m = section.match(regex);
                return m ? parseInt(m[1]) : null;
            };
            
            // Set default threshold
            const defaultVal = parseValue('OutageThreshold');
            if (defaultVal) {
                const input = document.getElementById('defaultThreshold');
                if (input) input.value = defaultVal;
            }
            
            // Set group thresholds
            const groups = {
                '서버': 'ServerOutageThreshold',
                '네트워크': 'NetworkOutageThreshold',
                '방화벽': 'FirewallOutageThreshold',
                '데이터베이스': 'DatabaseOutageThreshold',
                '웹서버': 'WebServerOutageThreshold',
                '스토리지': 'StorageOutageThreshold',
                '기타': 'OtherOutageThreshold'
            };
            
            for (const [groupName, key] of Object.entries(groups)) {
                const value = parseValue(key);
                if (value) {
                    const input = document.getElementById('thresh_' + groupName);
                    if (input) input.value = value;
                }
            }
            
            // 슬라이더 동기화
            syncAllThresholdSliders();
            
            console.log('임계값 설정 로드 (서버)');
        }
    } catch (error) {
        console.error('Failed to load threshold settings:', error);
    }
    
    // 기본값에서도 슬라이더 동기화
    syncAllThresholdSliders();
}

// ============================================
// IP 그룹 관리 탭 기능
// ============================================

/**
 * IP 설정 테이블 로드 (localStorage 우선)
 */
function loadIPConfigTable() {
    const emptyState = document.getElementById('ipgroupEmptyState');
    const contentDiv = document.getElementById('ipgroupContent');
    const tbody = document.getElementById('ipConfigTableBody');
    
    if (!tbody) {
        console.error('ipConfigTableBody not found');
        return;
    }
    
    if (!currentData || !currentData.targets || currentData.targets.length === 0) {
        console.warn('No ping data available');
        // Show empty state
        if (emptyState) emptyState.style.display = 'block';
        if (contentDiv) contentDiv.style.display = 'none';
        return;
    }
    
    // localStorage에서 저장된 설정 로드
    let savedIPSettings = {};
    const savedData = localStorage.getItem('ipGroupSettings');
    if (savedData) {
        try {
            savedIPSettings = JSON.parse(savedData);
            console.log('IP 그룹 설정 로드 (localStorage):', savedIPSettings);
        } catch (e) {
            console.error('IP 그룹 설정 파싱 실패:', e);
        }
    }
    
    // Hide empty state, show content
    if (emptyState) emptyState.style.display = 'none';
    if (contentDiv) contentDiv.style.display = 'block';
    
    tbody.innerHTML = currentData.targets.map(target => {
        // localStorage 값 우선, 없으면 서버 값 사용
        const saved = savedIPSettings[target.ip] || {};
        const group = saved.group || target.group || '기타';
        const priority = saved.priority || target.priority || 5;
        
        return `
        <tr>
            <td style="font-family: 'Courier New', monospace;">${target.ip}</td>
            <td>${target.name}</td>
            <td>
                <select data-ip="${target.ip}" class="ip-group-select">
                    <option value="서버" ${group === '서버' || group === 'Server' ? 'selected' : ''}>🖥️ 서버</option>
                    <option value="네트워크" ${group === '네트워크' || group === 'Network' ? 'selected' : ''}>🌐 네트워크</option>
                    <option value="방화벽" ${group === '방화벽' || group === 'Firewall' ? 'selected' : ''}>🛡️ 방화벽</option>
                    <option value="데이터베이스" ${group === '데이터베이스' ? 'selected' : ''}>💾 데이터베이스</option>
                    <option value="웹서버" ${group === '웹서버' ? 'selected' : ''}>🌍 웹서버</option>
                    <option value="스토리지" ${group === '스토리지' ? 'selected' : ''}>📦 스토리지</option>
                    <option value="기타" ${group === '기타' || group === 'Other' || !group ? 'selected' : ''}>🔧 기타</option>
                </select>
            </td>
            <td>
                <input type="number" min="1" max="5" value="${priority}" 
                       data-ip="${target.ip}" class="ip-priority-input">
            </td>
        </tr>
    `}).join('');
    
    console.log('IP config table loaded with', currentData.targets.length, 'entries');
}

// ============================================
// 설정 탭 전환 이벤트
// ============================================

/**
 * 설정 탭 전환 이벤트 초기화
 */
function initSettingsTabEvents() {
    const tabButtons = document.querySelectorAll('.settings-tab-btn');
    if (tabButtons.length === 0) return;
    
    console.log('Initializing settings tabs:', tabButtons.length);
    
    tabButtons.forEach(btn => {
        btn.addEventListener('click', function(e) {
            e.preventDefault();
            e.stopPropagation();
            
            const tabId = this.getAttribute('data-settings-tab');
            console.log('Tab clicked:', tabId);
            
            // Remove active class from all buttons
            tabButtons.forEach(b => b.classList.remove('active'));
            
            // Add active to clicked button
            this.classList.add('active');
            
            // Hide all tab contents
            document.querySelectorAll('.settings-tab-content').forEach(content => {
                content.classList.remove('active');
            });
            
            // Show target tab content
            const targetTab = document.getElementById('settings-' + tabId);
            if (targetTab) {
                targetTab.classList.add('active');
                console.log('Activated tab:', 'settings-' + tabId);
            } else {
                console.error('Tab not found:', 'settings-' + tabId);
            }
            
            // Load data for specific tabs
            if (tabId === 'thresholds') {
                loadThresholdSettings();
            } else if (tabId === 'ipgroups') {
                loadIPConfigTable();
            }
        });
    });
}

// ============================================
// 모달 이벤트 초기화
// ============================================

/**
 * 환경설정 모달 이벤트 초기화
 */
function initSettingsModalEvents() {
    const chartSettingsBtn = document.getElementById('chartSettingsBtn');
    const chartSettingsModal = document.getElementById('chartSettingsModal');
    const closeChartSettings = document.getElementById('closeChartSettings');
    const selectAllIPs = document.getElementById('selectAllIPs');
    const deselectAllIPs = document.getElementById('deselectAllIPs');
    const saveAllSettingsBtn = document.getElementById('saveAllSettings');

    if (chartSettingsBtn) {
        chartSettingsBtn.addEventListener('click', () => {
            openChartSettings();
        });
    }

    if (closeChartSettings) {
        closeChartSettings.addEventListener('click', () => {
            closeChartSettingsModal();
        });
    }

    if (chartSettingsModal) {
        chartSettingsModal.addEventListener('click', (e) => {
            if (e.target.id === 'chartSettingsModal') {
                closeChartSettingsModal();
            }
        });
    }

    if (selectAllIPs) {
        selectAllIPs.addEventListener('click', () => {
            selectAllIPsForChart();
        });
    }

    if (deselectAllIPs) {
        deselectAllIPs.addEventListener('click', () => {
            deselectAllIPsForChart();
        });
    }

    // 공통 저장 버튼
    if (saveAllSettingsBtn) {
        saveAllSettingsBtn.addEventListener('click', () => {
            saveAllSettings();
        });
    }
}

// ============================================
// 설정 저장
// ============================================

/**
 * 모든 설정 저장 (공통 저장 버튼)
 */
function saveAllSettings() {
    console.log('모든 설정 저장 시작...');
    
    // 1. 차트 설정 (표시 IP) 저장
    saveVisibleIPsState();
    
    // 2. 시간 범위 저장 (라디오 버튼 변경 시 이미 저장됨)
    const selectedRadio = document.querySelector('input[name="timeRange"]:checked');
    if (selectedRadio) {
        localStorage.setItem('timelineRange', selectedRadio.value);
        console.log('시간 범위 저장:', selectedRadio.value);
    }
    
    // 3. 그룹별 임계값 저장
    saveThresholdSettings();
    
    // 4. IP 그룹 설정 저장
    saveIPGroupSettings();
    
    console.log('모든 설정 저장 완료');
    
    // 저장 완료 메시지 표시
    showSaveSuccess();
    
    // 모달 닫기
    closeChartSettingsModal();
}

/**
 * 저장 완료 메시지 표시
 */
function showSaveSuccess() {
    const successMsg = document.getElementById('settingsSuccess');
    if (successMsg) {
        successMsg.classList.add('show');
        setTimeout(() => {
            successMsg.classList.remove('show');
        }, 2000);
    }
}

/**
 * 그룹별 임계값 저장
 */
function saveThresholdSettings() {
    const thresholds = {};
    
    // 기본 임계값
    const defaultThreshold = document.getElementById('defaultThreshold');
    if (defaultThreshold) {
        thresholds.default = parseInt(defaultThreshold.value) || 300;
    }
    
    // 그룹별 임계값
    const groups = ['서버', '네트워크', '방화벽', '데이터베이스', '웹서버', '스토리지', '기타'];
    groups.forEach(group => {
        const input = document.getElementById('thresh_' + group);
        if (input) {
            thresholds[group] = parseInt(input.value) || 300;
        }
    });
    
    localStorage.setItem('thresholdSettings', JSON.stringify(thresholds));
    console.log('임계값 설정 저장:', thresholds);
}

/**
 * IP 그룹 설정 저장
 */
function saveIPGroupSettings() {
    const ipSettings = {};
    
    // 그룹 설정
    const groupSelects = document.querySelectorAll('.ip-group-select');
    groupSelects.forEach(select => {
        const ip = select.dataset.ip;
        if (ip) {
            if (!ipSettings[ip]) ipSettings[ip] = {};
            ipSettings[ip].group = select.value;
        }
    });
    
    // 우선순위 설정
    const priorityInputs = document.querySelectorAll('.ip-priority-input');
    priorityInputs.forEach(input => {
        const ip = input.dataset.ip;
        if (ip) {
            if (!ipSettings[ip]) ipSettings[ip] = {};
            ipSettings[ip].priority = parseInt(input.value) || 5;
        }
    });
    
    localStorage.setItem('ipGroupSettings', JSON.stringify(ipSettings));
    console.log('IP 그룹 설정 저장:', ipSettings);
}

/**
 * 표시 IP 상태 저장
 */
function saveVisibleIPsState() {
    const visibleArray = Array.from(visibleIPs);
    localStorage.setItem('visibleIPs', JSON.stringify(visibleArray));
    console.log('필터 설정 저장:', visibleArray);
}

/**
 * 표시 IP 상태 로드
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

// ============================================
// 초기화 함수
// ============================================

/**
 * 환경설정 모듈 초기화
 * DOM 로드 후 호출해야 함
 */
function initSettingsModule() {
    console.log('Settings module initializing...');
    
    // 이벤트 초기화
    initSettingsModalEvents();
    initTimeRangeRadios();
    initSettingsTabEvents();
    
    console.log('Settings module initialized');
}

// DOM 로드 시 자동 초기화
document.addEventListener('DOMContentLoaded', initSettingsModule);