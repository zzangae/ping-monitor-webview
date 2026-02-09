/**
 * 최소화된 IP 섹션 모듈 (v2.7)
 * 경로: web/minwin-ip/minwin-ip.js
 * 
 * 기능:
 * - IP 카드 최소화/복원
 * - 최소화된 IP 영역 관리
 * - 상태 저장/복원 (localStorage)
 */

// ============================================
// 최소화된 IP 영역 이벤트 초기화
// ============================================

/**
 * 최소화된 IP 영역 이벤트 리스너 초기화
 * DOMContentLoaded 후 호출 필요
 */
function initMinimizedAreaEvents() {
    const minimizedAreaMinimizeBtn = document.getElementById('minimizedAreaMinimizeBtn');
    const minimizedAreaContent = document.getElementById('minimizedAreaContent');
    
    if (minimizedAreaMinimizeBtn && minimizedAreaContent) {
        minimizedAreaMinimizeBtn.addEventListener('click', () => {
            const isCollapsed = minimizedAreaContent.classList.toggle('collapsed');
            minimizedAreaMinimizeBtn.classList.toggle('minimized', isCollapsed);
            
            // 상태 저장
            localStorage.setItem('minimizedAreaCollapsed', isCollapsed ? '1' : '0');
        });
        
        // 저장된 상태 복원
        if (localStorage.getItem('minimizedAreaCollapsed') === '1') {
            minimizedAreaContent.classList.add('collapsed');
            minimizedAreaMinimizeBtn.classList.add('minimized');
        }
    }
}

// ============================================
// 최소화 상태 저장/로드
// ============================================

/**
 * 최소화 상태 저장
 * localStorage에 최소화된 카드 인덱스 저장
 */
function saveMinimizedState() {
    const minimizedCards = document.querySelectorAll('.ip-card.minimized');
    const minimizedIndices = Array.from(minimizedCards).map(card => card.dataset.index);
    localStorage.setItem('minimizedCards', JSON.stringify(minimizedIndices));
    console.log('최소화 상태 저장:', minimizedIndices);
}

/**
 * 최소화 상태 로드
 * localStorage에서 최소화된 카드 인덱스 복원
 */
function loadMinimizedState() {
    const saved = localStorage.getItem('minimizedCards');
    if (saved) {
        try {
            const minimizedIndices = JSON.parse(saved);
            console.log('최소화 상태 로드:', minimizedIndices);
            
            setTimeout(() => {
                minimizedIndices.forEach(index => {
                    const card = document.getElementById(`ip-card-${index}`);
                    if (card && !card.classList.contains('minimized')) {
                        toggleMinimize(parseInt(index));
                    }
                });
            }, 100);
        } catch (e) {
            console.error('최소화 상태 로드 실패:', e);
        }
    }
}

// ============================================
// 최소화 토글 기능
// ============================================

/**
 * IP 카드 최소화/복원 토글
 * @param {number} index - IP 카드 인덱스
 */
function toggleMinimize(index) {
    const card = document.getElementById(`ip-card-${index}`);
    
    if (!card) return;
    
    const isMinimized = card.classList.contains('minimized');
    const ipCardsContainer = document.getElementById('ipCardsContainer');
    const minimizedCardsContainer = document.getElementById('minimizedCardsContainer');
    
    if (isMinimized) {
        // 복원: 최소화 영역 → IP 카드 그리드
        card.classList.remove('minimized');
        card.draggable = true;
        ipCardsContainer.appendChild(card);
    } else {
        // 최소화: IP 카드 그리드 → 최소화 영역
        card.classList.add('minimized');
        card.draggable = false;
        minimizedCardsContainer.appendChild(card);
    }
    
    updateMinimizedArea();
    saveMinimizedState();
}

// ============================================
// 최소화 영역 업데이트
// ============================================

/**
 * 최소화된 IP 영역 표시/숨김 및 카운트 업데이트
 */
function updateMinimizedArea() {
    const minimizedCardsContainer = document.getElementById('minimizedCardsContainer');
    const minimizedCardsArea = document.getElementById('minimizedCardsArea');
    const minimizedCount = document.getElementById('minimizedCount');
    
    if (!minimizedCardsContainer || !minimizedCardsArea || !minimizedCount) return;
    
    const count = minimizedCardsContainer.children.length;
    
    if (count > 0) {
        minimizedCardsArea.style.display = 'block';
    } else {
        minimizedCardsArea.style.display = 'none';
    }
    
    minimizedCount.textContent = count;
}

// ============================================
// 초기화 및 리셋
// ============================================

/**
 * 최소화된 IP 영역 초기화
 * 새로고침 시 호출
 */
function resetMinimizedArea() {
    const minimizedCardsContainer = document.getElementById('minimizedCardsContainer');
    if (minimizedCardsContainer) {
        minimizedCardsContainer.innerHTML = '';
    }
    
    const minimizedCardsArea = document.getElementById('minimizedCardsArea');
    if (minimizedCardsArea) {
        minimizedCardsArea.style.display = 'none';
    }
}

/**
 * 모든 카드 복원
 */
function restoreAllCards() {
    const minimizedCards = document.querySelectorAll('.ip-card.minimized');
    minimizedCards.forEach(card => {
        const index = parseInt(card.dataset.index);
        if (!isNaN(index)) {
            toggleMinimize(index);
        }
    });
}

// 전역 함수로 내보내기 (window 객체에 등록)
window.initMinimizedAreaEvents = initMinimizedAreaEvents;
window.saveMinimizedState = saveMinimizedState;
window.loadMinimizedState = loadMinimizedState;
window.toggleMinimize = toggleMinimize;
window.updateMinimizedArea = updateMinimizedArea;
window.resetMinimizedArea = resetMinimizedArea;
window.restoreAllCards = restoreAllCards;