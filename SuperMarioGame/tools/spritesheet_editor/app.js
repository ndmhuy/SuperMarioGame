/**
 * Sprite & Animation Studio - Core Logic
 */

document.addEventListener('DOMContentLoaded', () => {
    // UI Elements
    const dropzone = document.getElementById('image-dropzone');
    const imageInput = document.getElementById('image-input');
    const filenameDisplay = document.getElementById('filename-display');
    
    const inputGridW = document.getElementById('grid-w');
    const inputGridH = document.getElementById('grid-h');
    const inputGridOx = document.getElementById('grid-ox');
    const inputGridOy = document.getElementById('grid-oy');
    const inputGridGx = document.getElementById('grid-gx');
    const inputGridGy = document.getElementById('grid-gy');
    const checkShowGrid = document.getElementById('show-grid');
    const checkSnapGrid = document.getElementById('snap-grid');
    const btnAutoSlice = document.getElementById('btn-auto-slice');
    
    const btnAddSprite = document.getElementById('btn-add-sprite');
    const spriteSearch = document.getElementById('sprite-search');
    const spriteListContainer = document.getElementById('sprite-list');
    
    const coordsDisplay = document.getElementById('coords-display');
    const zoomDisplay = document.getElementById('canvas-zoom-display');
    const btnZoomIn = document.getElementById('btn-zoom-in');
    const btnZoomOut = document.getElementById('btn-zoom-out');
    const btnZoomReset = document.getElementById('btn-zoom-reset');
    const canvasViewport = document.getElementById('canvas-viewport');
    const canvasZoomWrapper = document.getElementById('canvas-zoom-wrapper');
    const mainCanvas = document.getElementById('spritesheet-canvas');
    const mainCtx = mainCanvas.getContext('2d');
    
    const btnNewAnim = document.getElementById('btn-new-anim');
    const animSelector = document.getElementById('anim-selector');
    const btnDeleteAnim = document.getElementById('btn-delete-anim');
    const checkAnimLoop = document.getElementById('anim-loop');
    const frameCountDisplay = document.getElementById('frame-count-display');
    const timelineFrames = document.getElementById('timeline-frames');
    
    const btnPrevPlay = document.getElementById('btn-prev-play');
    const previewCanvas = document.getElementById('preview-canvas');
    const previewCtx = previewCanvas.getContext('2d');
    const previewScaleInput = document.getElementById('preview-scale');
    const previewScaleLabel = document.getElementById('preview-scale-label');
    const bgPickerContainer = document.querySelector('.preview-bg-picker');
    const previewDisplayContainer = document.getElementById('preview-display-container');
    
    const jsonTextarea = document.getElementById('json-textarea');
    const btnCopyJson = document.getElementById('btn-copy-json');
    const btnDownloadJson = document.getElementById('btn-download-json');
    const btnImportJson = document.getElementById('btn-import-json');

    // App State
    const state = {
        texturePath: "assets/textures/sample_spritesheet.png",
        image: null,
        imageLoaded: false,
        sprites: {}, // name -> { x, y, w, h }
        animations: {}, // name -> { loop: true, frames: [ { sprite: "name", duration: 0.1 } ] }
        activeSpriteId: null, // key of selected sprite
        activeAnimId: null, // key of selected animation
        
        // Grid configurations
        grid: {
            w: 32,
            h: 32,
            ox: 0,
            oy: 0,
            gx: 0,
            gy: 0,
            show: true,
            snap: true
        },
        
        // Viewport Zoom & Pan
        zoom: 1.0,
        panX: 0,
        panY: 0,
        isPanning: false,
        panStartX: 0,
        panStartY: 0,
        
        // Drawing and interaction state
        dragMode: null, // 'create', 'move', or integer 0-7 representing handles
        dragStartMouse: { x: 0, y: 0 },
        dragStartRect: { x: 0, y: 0, w: 0, h: 0 },
        activeHandle: -1,
        spacePressed: false,
        
        // Preview State
        isPlaying: false,
        previewFrameIdx: 0,
        previewTimer: 0,
        previewScale: 4,
        previewBg: 'checkerboard',
        lastTime: 0
    };

    // Handle Sizes
    const HANDLE_SIZE = 6;
    const MIN_BOX_SIZE = 1;

    // Load defaults
    function init() {
        // Load image first
        state.image = new Image();
        state.image.onload = () => {
            state.imageLoaded = true;
            mainCanvas.width = state.image.width;
            mainCanvas.height = state.image.height;
            
            // Initial view fits image to screen
            fitZoomToViewport();
            
            // Create default sprite lists
            if (Object.keys(state.sprites).length === 0) {
                // Pre-populate some sprites from our generated plumber spritesheet (rows of 32x32 sprites)
                prepopulateDefaultMarioSprites();
            }
            
            updateUI();
            drawEditor();
        };
        state.image.onerror = () => {
            alert("Error loading sample_spritesheet.png. You can drag and drop a custom PNG.");
        };
        state.image.src = 'sample_spritesheet.png';
        
        // Set up default callbacks & listeners
        setupEventListeners();
        loadFromLocalStorage();
        
        // Start animation playback loop
        requestAnimationFrame(animationLoop);
    }

    function prepopulateDefaultMarioSprites() {
        // Row 0: Idle & Run
        state.sprites["mario_idle_1"] = { x: 0, y: 0, w: 32, h: 32 };
        state.sprites["mario_idle_2"] = { x: 32, y: 0, w: 32, h: 32 };
        state.sprites["mario_run_1"] = { x: 64, y: 0, w: 32, h: 32 };
        state.sprites["mario_run_2"] = { x: 96, y: 0, w: 32, h: 32 };
        state.sprites["mario_run_3"] = { x: 128, y: 0, w: 32, h: 32 };
        state.sprites["mario_run_4"] = { x: 160, y: 0, w: 32, h: 32 };
        
        // Row 1: Jump & Wall slide
        state.sprites["mario_jump_up"] = { x: 0, y: 32, w: 32, h: 32 };
        state.sprites["mario_jump_peak"] = { x: 32, y: 32, w: 32, h: 32 };
        state.sprites["mario_fall"] = { x: 64, y: 32, w: 32, h: 32 };
        state.sprites["mario_wall_slide"] = { x: 96, y: 32, w: 32, h: 32 };
        
        // Row 2: Slide & Crouch
        state.sprites["mario_slide_1"] = { x: 0, y: 64, w: 32, h: 32 };
        state.sprites["mario_slide_2"] = { x: 32, y: 64, w: 32, h: 32 };
        state.sprites["mario_slide_3"] = { x: 64, y: 64, w: 32, h: 32 };
        state.sprites["mario_crouch"] = { x: 96, y: 64, w: 32, h: 32 };

        // Row 3: Attack
        state.sprites["mario_attack_1"] = { x: 0, y: 96, w: 32, h: 32 };
        state.sprites["mario_attack_2"] = { x: 32, y: 96, w: 32, h: 32 };
        state.sprites["mario_hurt"] = { x: 64, y: 96, w: 32, h: 32 };
        
        // Set up animations
        state.animations["run"] = {
            loop: true,
            frames: [
                { sprite: "mario_run_1", duration: 0.12 },
                { sprite: "mario_run_2", duration: 0.12 },
                { sprite: "mario_run_3", duration: 0.12 },
                { sprite: "mario_run_4", duration: 0.12 }
            ]
        };
        state.animations["idle"] = {
            loop: true,
            frames: [
                { sprite: "mario_idle_1", duration: 0.2 },
                { sprite: "mario_idle_2", duration: 0.2 }
            ]
        };
        state.animations["jump"] = {
            loop: false,
            frames: [
                { sprite: "mario_jump_up", duration: 0.15 },
                { sprite: "mario_jump_peak", duration: 0.15 },
                { sprite: "mario_fall", duration: 0.15 }
            ]
        };
        
        state.activeAnimId = "run";
    }

    // Fit image to viewport
    function fitZoomToViewport() {
        if (!state.imageLoaded) return;
        const viewportW = canvasViewport.clientWidth;
        const viewportH = canvasViewport.clientHeight;
        const scaleX = (viewportW - 40) / state.image.width;
        const scaleY = (viewportH - 40) / state.image.height;
        state.zoom = Math.min(Math.min(scaleX, scaleY), 4.0); // Cap fit-zoom at 4x
        state.panX = (viewportW - state.image.width * state.zoom) / 2;
        state.panY = (viewportH - state.image.height * state.zoom) / 2;
        updateZoomDisplay();
    }

    function updateZoomDisplay() {
        zoomDisplay.textContent = `${Math.round(state.zoom * 100)}%`;
        canvasZoomWrapper.style.transform = `translate(${state.panX}px, ${state.panY}px) scale(${state.zoom})`;
    }

    // LOCAL STORAGE PERSISTENCE
    function saveToLocalStorage() {
        const config = {
            texturePath: state.texturePath,
            sprites: state.sprites,
            animations: state.animations,
            grid: state.grid
        };
        localStorage.setItem('mario_sprite_studio_state', JSON.stringify(config));
    }

    function loadFromLocalStorage() {
        try {
            const data = localStorage.getItem('mario_sprite_studio_state');
            if (data) {
                const parsed = JSON.parse(data);
                if (parsed.texturePath) state.texturePath = parsed.texturePath;
                if (parsed.sprites) state.sprites = parsed.sprites;
                if (parsed.animations) state.animations = parsed.animations;
                if (parsed.grid) {
                    state.grid = parsed.grid;
                    // Sync values back to inputs
                    inputGridW.value = state.grid.w;
                    inputGridH.value = state.grid.h;
                    inputGridOx.value = state.grid.ox;
                    inputGridOy.value = state.grid.oy;
                    inputGridGx.value = state.grid.gx;
                    inputGridGy.value = state.grid.gy;
                    checkShowGrid.checked = state.grid.show;
                    checkSnapGrid.checked = state.grid.snap;
                }
                
                // Set first anim as active if present
                const anims = Object.keys(state.animations);
                if (anims.length > 0) {
                    state.activeAnimId = anims[0];
                }
            }
        } catch (e) {
            console.error("Failed to load state from localStorage:", e);
        }
    }

    // DRAW EDITOR CANVAS
    function drawEditor() {
        if (!state.imageLoaded) return;
        
        // Clear canvas context
        mainCtx.clearRect(0, 0, mainCanvas.width, mainCanvas.height);
        
        // Draw primary image
        mainCtx.drawImage(state.image, 0, 0);
        
        // Draw grid overlay
        if (state.grid.show) {
            drawGrid();
        }
        
        // Draw bounding boxes for all sprites
        drawSpriteBoxes();
        
        // Draw resize handles for active sprite
        if (state.activeSpriteId && state.sprites[state.activeSpriteId]) {
            drawResizeHandles(state.sprites[state.activeSpriteId]);
        }
    }

    function drawGrid() {
        const gw = state.grid.w;
        const gh = state.grid.h;
        const ox = state.grid.ox;
        const oy = state.grid.oy;
        const gx = state.grid.gx;
        const gy = state.grid.gy;
        
        mainCtx.save();
        mainCtx.strokeStyle = 'rgba(255, 255, 255, 0.18)';
        mainCtx.lineWidth = 1;
        
        // If gaps are zero, we can draw lines across the sheet
        if (gx === 0 && gy === 0) {
            // Vertical grid lines
            for (let x = ox; x < mainCanvas.width; x += gw) {
                mainCtx.beginPath();
                mainCtx.moveTo(x, 0);
                mainCtx.lineTo(x, mainCanvas.height);
                mainCtx.stroke();
            }
            // Horizontal grid lines
            for (let y = oy; y < mainCanvas.height; y += gh) {
                mainCtx.beginPath();
                mainCtx.moveTo(0, y);
                mainCtx.lineTo(mainCanvas.width, y);
                mainCtx.stroke();
            }
        } else {
            // Draw cells individually if there are gaps
            mainCtx.strokeStyle = 'rgba(255, 255, 255, 0.15)';
            for (let y = oy; y + gh <= mainCanvas.height; y += gh + gy) {
                for (let x = ox; x + gw <= mainCanvas.width; x += gw + gx) {
                    mainCtx.strokeRect(x, y, gw, gh);
                }
            }
        }
        mainCtx.restore();
    }

    function drawSpriteBoxes() {
        mainCtx.save();
        
        for (const [id, rect] of Object.entries(state.sprites)) {
            const isActive = (id === state.activeSpriteId);
            
            if (isActive) {
                // Active Box
                mainCtx.strokeStyle = 'rgba(0, 240, 255, 0.85)';
                mainCtx.fillStyle = 'rgba(0, 240, 255, 0.08)';
                mainCtx.lineWidth = 2;
                mainCtx.fillRect(rect.x, rect.y, rect.w, rect.h);
                mainCtx.strokeRect(rect.x, rect.y, rect.w, rect.h);
                
                // Label box
                mainCtx.font = 'bold 9px var(--font-sans)';
                mainCtx.fillStyle = '#00f0ff';
                mainCtx.fillText(id, rect.x + 3, rect.y - 4);
            } else {
                // Inactive Box
                mainCtx.strokeStyle = 'rgba(189, 0, 255, 0.45)';
                mainCtx.fillStyle = 'rgba(189, 0, 255, 0.02)';
                mainCtx.lineWidth = 1;
                mainCtx.fillRect(rect.x, rect.y, rect.w, rect.h);
                mainCtx.strokeRect(rect.x, rect.y, rect.w, rect.h);
            }
        }
        
        // Draw temporary drag-selection box
        if (state.dragMode === 'create' && state.isDragging) {
            mainCtx.strokeStyle = '#00f0ff';
            mainCtx.setLineDash([4, 4]);
            mainCtx.lineWidth = 1.5;
            mainCtx.strokeRect(
                state.dragStartRect.x,
                state.dragStartRect.y,
                state.dragStartRect.w,
                state.dragStartRect.h
            );
        }
        
        mainCtx.restore();
    }

    function drawResizeHandles(rect) {
        mainCtx.save();
        mainCtx.fillStyle = '#ffffff';
        mainCtx.strokeStyle = '#00f0ff';
        mainCtx.lineWidth = 1.5;
        
        const handles = getHandleCoordinates(rect);
        
        handles.forEach((pt) => {
            mainCtx.beginPath();
            mainCtx.arc(pt.x, pt.y, HANDLE_SIZE / 2, 0, Math.PI * 2);
            mainCtx.fill();
            mainCtx.stroke();
        });
        
        mainCtx.restore();
    }

    // Helper to extract handle point positions
    function getHandleCoordinates(rect) {
        const { x, y, w, h } = rect;
        return [
            { x: x, y: y },         // 0: Top-Left
            { x: x + w/2, y: y },   // 1: Top-Center
            { x: x + w, y: y },     // 2: Top-Right
            { x: x + w, y: y + h/2 }, // 3: Middle-Right
            { x: x + w, y: y + h },   // 4: Bottom-Right
            { x: x + w/2, y: y + h }, // 5: Bottom-Center
            { x: x, y: y + h },     // 6: Bottom-Left
            { x: x, y: y + h/2 }    // 7: Middle-Left
        ];
    }

    // Collision detection: Check if mouse is on a handle
    function getHandleAtCoord(rect, mouseX, mouseY) {
        const handles = getHandleCoordinates(rect);
        for (let i = 0; i < handles.length; i++) {
            const dx = mouseX - handles[i].x;
            const dy = mouseY - handles[i].y;
            // Radius of collision
            if (dx*dx + dy*dy <= (HANDLE_SIZE + 4) * (HANDLE_SIZE + 4)) {
                return i;
            }
        }
        return -1;
    }

    // Check if point is inside rectangle
    function isPointInRect(px, py, rect) {
        return px >= rect.x && px <= rect.x + rect.w &&
               py >= rect.y && py <= rect.y + rect.h;
    }

    // Snapping Logic
    function getSnappedVal(coord, isX = true) {
        if (!state.grid.snap) return coord;
        
        const cell = isX ? state.grid.w : state.grid.h;
        const offset = isX ? state.grid.ox : state.grid.oy;
        const gap = isX ? state.grid.gx : state.grid.gy;
        
        // Find closest cell coordinate index
        const index = Math.round((coord - offset) / (cell + gap));
        const snapped = offset + index * (cell + gap);
        
        return snapped;
    }

    // Get mouse coords relative to canvas pixels, accounting for zoom/pan
    function getCanvasCoords(e) {
        const rect = mainCanvas.getBoundingClientRect();
        
        // Translate client position to relative to zoom viewport
        const clientX = e.clientX - rect.left;
        const clientY = e.clientY - rect.top;
        
        // Account for CSS scale transformation
        const canvasX = Math.round((clientX / rect.width) * mainCanvas.width);
        const canvasY = Math.round((clientY / rect.height) * mainCanvas.height);
        
        return {
            x: Math.max(0, Math.min(mainCanvas.width, canvasX)),
            y: Math.max(0, Math.min(mainCanvas.height, canvasY))
        };
    }

    // EVENT LISTENERS REGISTER
    function setupEventListeners() {
        // Drag and drop image
        dropzone.addEventListener('dragover', (e) => {
            e.preventDefault();
            dropzone.classList.add('dragover');
        });
        dropzone.addEventListener('dragleave', () => {
            dropzone.classList.remove('dragover');
        });
        dropzone.addEventListener('drop', (e) => {
            e.preventDefault();
            dropzone.classList.remove('dragover');
            
            const file = e.dataTransfer.files[0];
            if (file && file.type === "image/png") {
                loadCustomImage(file);
            }
        });
        imageInput.addEventListener('change', (e) => {
            const file = e.target.files[0];
            if (file && file.type === "image/png") {
                loadCustomImage(file);
            }
        });
        
        // Grid configuration input updates
        inputGridW.addEventListener('input', (e) => {
            state.grid.w = parseInt(e.target.value) || 32;
            updateUI();
        });
        inputGridH.addEventListener('input', (e) => {
            state.grid.h = parseInt(e.target.value) || 32;
            updateUI();
        });
        inputGridOx.addEventListener('input', (e) => {
            state.grid.ox = parseInt(e.target.value) || 0;
            updateUI();
        });
        inputGridOy.addEventListener('input', (e) => {
            state.grid.oy = parseInt(e.target.value) || 0;
            updateUI();
        });
        inputGridGx.addEventListener('input', (e) => {
            state.grid.gx = parseInt(e.target.value) || 0;
            updateUI();
        });
        inputGridGy.addEventListener('input', (e) => {
            state.grid.gy = parseInt(e.target.value) || 0;
            updateUI();
        });
        
        checkShowGrid.addEventListener('change', (e) => {
            state.grid.show = e.target.checked;
            drawEditor();
            saveToLocalStorage();
        });
        
        checkSnapGrid.addEventListener('change', (e) => {
            state.grid.snap = e.target.checked;
            saveToLocalStorage();
        });
        
        btnAutoSlice.addEventListener('click', () => {
            if (confirm("This will auto-generate sprites for all cells in the sheet. Current sprites will be preserved. Proceed?")) {
                autoSliceGrid();
            }
        });
        
        // Sprite list panel additions
        btnAddSprite.addEventListener('click', () => {
            const name = getUniqueSpriteName();
            const w = state.grid.w;
            const h = state.grid.h;
            // Spawn box in center or offsets
            state.sprites[name] = { x: state.grid.ox, y: state.grid.oy, w: w, h: h };
            state.activeSpriteId = name;
            
            updateUI();
            drawEditor();
        });
        
        spriteSearch.addEventListener('input', () => {
            renderSpriteList();
        });
        
        // Zoom functionality
        btnZoomIn.addEventListener('click', () => {
            state.zoom = Math.min(state.zoom + 0.25, 8.0);
            updateZoomDisplay();
        });
        
        btnZoomOut.addEventListener('click', () => {
            state.zoom = Math.max(state.zoom - 0.25, 0.25);
            updateZoomDisplay();
        });
        
        btnZoomReset.addEventListener('click', () => {
            fitZoomToViewport();
        });
        
        // Mouse/Key bindings for pan/zoom & crop
        window.addEventListener('keydown', (e) => {
            if (e.code === 'Space') {
                // Prevent standard scrolling when focused inside editor
                if (document.activeElement.tagName !== 'INPUT' && document.activeElement.tagName !== 'TEXTAREA') {
                    e.preventDefault();
                    state.spacePressed = true;
                    canvasViewport.style.cursor = 'grab';
                }
            }
        });
        
        window.addEventListener('keyup', (e) => {
            if (e.code === 'Space') {
                state.spacePressed = false;
                canvasViewport.style.cursor = 'crosshair';
            }
        });
        
        canvasViewport.addEventListener('wheel', (e) => {
            e.preventDefault();
            const zoomFactor = 0.1;
            const direction = e.deltaY < 0 ? 1 : -1;
            
            // Mouse client pos
            const mX = e.clientX - canvasViewport.getBoundingClientRect().left;
            const mY = e.clientY - canvasViewport.getBoundingClientRect().top;
            
            // Relative coordinates on wrap before scaling
            const relativeX = (mX - state.panX) / state.zoom;
            const relativeY = (mY - state.panY) / state.zoom;
            
            const prevZoom = state.zoom;
            state.zoom = Math.max(0.25, Math.min(state.zoom + direction * zoomFactor, 8.0));
            
            // Re-adjust pan values to center zoom on mouse
            state.panX = mX - relativeX * state.zoom;
            state.panY = mY - relativeY * state.zoom;
            
            updateZoomDisplay();
        });
        
        // CANVAS DRAGGING & RESIZING EVENTS
        mainCanvas.addEventListener('mousedown', (e) => {
            const mouse = getCanvasCoords(e);
            
            // 1. Check if Space/Middle click is panning
            if (state.spacePressed || e.button === 1) {
                state.isPanning = true;
                state.panStartX = e.clientX - state.panX;
                state.panStartY = e.clientY - state.panY;
                canvasViewport.style.cursor = 'grabbing';
                return;
            }
            
            // 2. Check if clicked on a handle of the ACTIVE sprite
            if (state.activeSpriteId) {
                const rect = state.sprites[state.activeSpriteId];
                const handle = getHandleAtCoord(rect, mouse.x, mouse.y);
                if (handle !== -1) {
                    state.isDragging = true;
                    state.dragMode = handle;
                    state.dragStartMouse = { x: mouse.x, y: mouse.y };
                    state.dragStartRect = { ...rect };
                    return;
                }
            }
            
            // 3. Check if clicked INSIDE any sprite box to select/move
            let clickedSpriteId = null;
            for (const [id, rect] of Object.entries(state.sprites)) {
                if (isPointInRect(mouse.x, mouse.y, rect)) {
                    clickedSpriteId = id;
                    // Prioritize clicking current selection if overlapped
                    if (id === state.activeSpriteId) break;
                }
            }
            
            if (clickedSpriteId) {
                state.activeSpriteId = clickedSpriteId;
                state.isDragging = true;
                state.dragMode = 'move';
                state.dragStartMouse = { x: mouse.x, y: mouse.y };
                state.dragStartRect = { ...state.sprites[clickedSpriteId] };
                
                // Highlight item in list
                highlightSpriteListItem(clickedSpriteId);
                updateUI();
                drawEditor();
                return;
            }
            
            // 4. Draw a new sprite box
            state.isDragging = true;
            state.dragMode = 'create';
            const snappedX = getSnappedVal(mouse.x, true);
            const snappedY = getSnappedVal(mouse.y, false);
            
            state.dragStartMouse = { x: mouse.x, y: mouse.y };
            state.dragStartRect = { x: snappedX, y: snappedY, w: 0, h: 0 };
            
            // Deselect active
            state.activeSpriteId = null;
            updateUI();
            drawEditor();
        });
        
        mainCanvas.addEventListener('mousemove', (e) => {
            const mouse = getCanvasCoords(e);
            coordsDisplay.textContent = `X: ${mouse.x} | Y: ${mouse.y}`;
            
            // Handle active cursor styling when hovered over handles
            if (!state.isDragging && state.activeSpriteId) {
                const rect = state.sprites[state.activeSpriteId];
                const handle = getHandleAtCoord(rect, mouse.x, mouse.y);
                if (handle !== -1) {
                    const cursors = ['nwse-resize', 'ns-resize', 'nesw-resize', 'ew-resize', 'nwse-resize', 'ns-resize', 'nesw-resize', 'ew-resize'];
                    mainCanvas.style.cursor = cursors[handle];
                    return;
                } else if (isPointInRect(mouse.x, mouse.y, rect)) {
                    mainCanvas.style.cursor = 'move';
                    return;
                }
            }
            
            if (!state.isDragging) {
                mainCanvas.style.cursor = 'crosshair';
            }
            
            if (state.isPanning) {
                state.panX = e.clientX - state.panStartX;
                state.panY = e.clientY - state.panStartY;
                updateZoomDisplay();
                return;
            }
            
            if (!state.isDragging) return;
            
            const dx = mouse.x - state.dragStartMouse.x;
            const dy = mouse.y - state.dragStartMouse.y;
            
            const rect = state.sprites[state.activeSpriteId];
            
            if (state.dragMode === 'create') {
                // Drawing new box
                let x1 = state.dragStartRect.x;
                let y1 = state.dragStartRect.y;
                let x2 = mouse.x;
                let y2 = mouse.y;
                
                if (state.grid.snap) {
                    x2 = getSnappedVal(x2, true);
                    y2 = getSnappedVal(y2, false);
                }
                
                state.dragStartRect.w = x2 - x1;
                state.dragStartRect.h = y2 - y1;
                
                drawEditor();
            } else if (state.dragMode === 'move') {
                // Moving box
                let newX = state.dragStartRect.x + dx;
                let newY = state.dragStartRect.y + dy;
                
                if (state.grid.snap) {
                    newX = getSnappedVal(newX, true);
                    newY = getSnappedVal(newY, false);
                }
                
                rect.x = Math.max(0, Math.min(mainCanvas.width - rect.w, newX));
                rect.y = Math.max(0, Math.min(mainCanvas.height - rect.h, newY));
                
                updateUI();
                drawEditor();
            } else if (typeof state.dragMode === 'number') {
                // Resizing Box via Handles
                let rX = state.dragStartRect.x;
                let rY = state.dragStartRect.y;
                let rW = state.dragStartRect.w;
                let rH = state.dragStartRect.h;
                
                // Active handles mapping modifications
                switch (state.dragMode) {
                    case 0: // Top-Left
                        let newLeftX = getSnappedVal(rX + dx, true);
                        let newTopY = getSnappedVal(rY + dy, false);
                        rect.w = rW + (rX - newLeftX);
                        rect.h = rH + (rY - newTopY);
                        rect.x = newLeftX;
                        rect.y = newTopY;
                        break;
                    case 1: // Top-Center
                        let newTopY2 = getSnappedVal(rY + dy, false);
                        rect.h = rH + (rY - newTopY2);
                        rect.y = newTopY2;
                        break;
                    case 2: // Top-Right
                        rect.w = getSnappedVal(rX + rW + dx, true) - rX;
                        let newTopY3 = getSnappedVal(rY + dy, false);
                        rect.h = rH + (rY - newTopY3);
                        rect.y = newTopY3;
                        break;
                    case 3: // Middle-Right
                        rect.w = getSnappedVal(rX + rW + dx, true) - rX;
                        break;
                    case 4: // Bottom-Right
                        rect.w = getSnappedVal(rX + rW + dx, true) - rX;
                        rect.h = getSnappedVal(rY + rH + dy, false) - rY;
                        break;
                    case 5: // Bottom-Center
                        rect.h = getSnappedVal(rY + rH + dy, false) - rY;
                        break;
                    case 6: // Bottom-Left
                        let newLeftX2 = getSnappedVal(rX + dx, true);
                        rect.w = rW + (rX - newLeftX2);
                        rect.x = newLeftX2;
                        rect.h = getSnappedVal(rY + rH + dy, false) - rY;
                        break;
                    case 7: // Middle-Left
                        let newLeftX3 = getSnappedVal(rX + dx, true);
                        rect.w = rW + (rX - newLeftX3);
                        rect.x = newLeftX3;
                        break;
                }
                
                // Enforce minimum size and fix negative dimensions
                if (rect.w < MIN_BOX_SIZE) rect.w = MIN_BOX_SIZE;
                if (rect.h < MIN_BOX_SIZE) rect.h = MIN_BOX_SIZE;
                
                updateUI();
                drawEditor();
            }
        });
        
        window.addEventListener('mouseup', () => {
            if (state.isPanning) {
                state.isPanning = false;
                canvasViewport.style.cursor = 'crosshair';
                return;
            }
            
            if (!state.isDragging) return;
            
            if (state.dragMode === 'create') {
                let rect = state.dragStartRect;
                // If drawn in negative directions, fix values
                if (rect.w < 0) {
                    rect.x += rect.w;
                    rect.w = Math.abs(rect.w);
                }
                if (rect.h < 0) {
                    rect.y += rect.h;
                    rect.h = Math.abs(rect.h);
                }
                
                // Only add if box size is valid
                if (rect.w >= MIN_BOX_SIZE && rect.h >= MIN_BOX_SIZE) {
                    const name = getUniqueSpriteName();
                    state.sprites[name] = rect;
                    state.activeSpriteId = name;
                }
            }
            
            state.isDragging = false;
            state.dragMode = null;
            
            updateUI();
            drawEditor();
            saveToLocalStorage();
        });
        
        // Double-click grid mapping addition
        mainCanvas.addEventListener('dblclick', (e) => {
            const mouse = getCanvasCoords(e);
            
            // Find coordinate cell under double click
            const gw = state.grid.w;
            const gh = state.grid.h;
            const ox = state.grid.ox;
            const oy = state.grid.oy;
            const gx = state.grid.gx;
            const gy = state.grid.gy;
            
            const cellCol = Math.floor((mouse.x - ox) / (gw + gx));
            const cellRow = Math.floor((mouse.y - oy) / (gh + gy));
            
            const cellX = ox + cellCol * (gw + gx);
            const cellY = oy + cellRow * (gh + gy);
            
            if (cellX + gw <= mainCanvas.width && cellY + gh <= mainCanvas.height) {
                const name = `cell_${cellRow}_${cellCol}`;
                
                // Add if not already existing
                if (!state.sprites[name]) {
                    state.sprites[name] = { x: cellX, y: cellY, w: gw, h: gh };
                }
                state.activeSpriteId = name;
                updateUI();
                drawEditor();
                saveToLocalStorage();
            }
        });
        
        // ANIMATION CONTROLS
        btnNewAnim.addEventListener('click', () => {
            const name = prompt("Enter new animation name:", "walk");
            if (!name) return;
            const formatted = name.trim().toLowerCase().replace(/\s+/g, '_');
            if (!formatted) return;
            
            if (state.animations[formatted]) {
                alert("Animation name already exists.");
                return;
            }
            
            state.animations[formatted] = {
                loop: true,
                frames: []
            };
            state.activeAnimId = formatted;
            
            updateUI();
            saveToLocalStorage();
        });
        
        btnDeleteAnim.addEventListener('click', () => {
            if (!state.activeAnimId) return;
            if (confirm(`Are you sure you want to delete animation '${state.activeAnimId}'?`)) {
                delete state.animations[state.activeAnimId];
                
                // Select another animation
                const keys = Object.keys(state.animations);
                state.activeAnimId = keys.length > 0 ? keys[0] : null;
                
                updateUI();
                saveToLocalStorage();
            }
        });
        
        checkAnimLoop.addEventListener('change', (e) => {
            if (!state.activeAnimId) return;
            state.animations[state.activeAnimId].loop = e.target.checked;
            updateUI();
            saveToLocalStorage();
        });
        
        animSelector.addEventListener('change', (e) => {
            state.activeAnimId = e.target.value;
            state.previewFrameIdx = 0;
            state.previewTimer = 0;
            updateUI();
        });
        
        // PREVIEW PLAYBACK BUTTONS
        btnPrevPlay.addEventListener('click', () => {
            state.isPlaying = !state.isPlaying;
            updatePlayPauseButton();
        });
        
        previewScaleInput.addEventListener('input', (e) => {
            state.previewScale = parseInt(e.target.value) || 4;
            previewScaleLabel.textContent = `${state.previewScale}x`;
        });
        
        bgPickerContainer.addEventListener('click', (e) => {
            if (e.target.classList.contains('bg-btn')) {
                bgPickerContainer.querySelectorAll('.bg-btn').forEach(btn => btn.classList.remove('active'));
                e.target.classList.add('active');
                
                const bgType = e.target.dataset.bg;
                previewDisplayContainer.className = `preview-display ${bgType}`;
                state.previewBg = bgType;
            }
        });
        
        // JSON COPY, DOWNLOAD & IMPORT
        btnCopyJson.addEventListener('click', () => {
            jsonTextarea.select();
            document.execCommand('copy');
            btnCopyJson.textContent = "Copied!";
            setTimeout(() => {
                btnCopyJson.innerHTML = `<svg viewBox="0 0 24 24" width="16" height="16" stroke="currentColor" stroke-width="2" fill="none" stroke-linecap="round" stroke-linejoin="round"><path d="M16 4h2a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2h2"/><rect x="8" y="2" width="8" height="4" rx="1" ry="1"/></svg> Copy JSON`;
            }, 1500);
        });
        
        btnDownloadJson.addEventListener('click', () => {
            const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(jsonTextarea.value);
            const downloadAnchor = document.createElement('a');
            downloadAnchor.setAttribute("href", dataStr);
            downloadAnchor.setAttribute("download", "spritesheet_config.json");
            document.body.appendChild(downloadAnchor);
            downloadAnchor.click();
            downloadAnchor.remove();
        });
        
        btnImportJson.addEventListener('click', () => {
            const fileInput = document.createElement('input');
            fileInput.type = 'file';
            fileInput.accept = '.json';
            fileInput.onchange = (e) => {
                const file = e.target.files[0];
                if (file) {
                    const reader = new FileReader();
                    reader.onload = (evt) => {
                        try {
                            const parsed = JSON.parse(evt.target.result);
                            importJSONConfig(parsed);
                        } catch (err) {
                            alert("Failed to parse JSON file: " + err.message);
                        }
                    };
                    reader.readAsText(file);
                }
            };
            fileInput.click();
        });
        
        // Real-time editor two-way sync
        jsonTextarea.addEventListener('blur', () => {
            try {
                const parsed = JSON.parse(jsonTextarea.value);
                importJSONConfig(parsed, false); // Import without fully resetting textures
            } catch (err) {
                // Ignore temporary parsing errors during editing
                console.warn("JSON textarea contains invalid syntax:", err.message);
            }
        });
    }

    // Dynamic selection ID generator
    function getUniqueSpriteName() {
        let idx = 1;
        while (state.sprites[`sprite_${idx}`]) {
            idx++;
        }
        return `sprite_${idx}`;
    }

    function updatePlayPauseButton() {
        if (state.isPlaying) {
            btnPrevPlay.innerHTML = `<svg viewBox="0 0 24 24" width="14" height="14" stroke="currentColor" stroke-width="2" fill="currentColor"><rect x="6" y="4" width="4" height="16"></rect><rect x="14" y="4" width="4" height="16"></rect></svg>`;
        } else {
            btnPrevPlay.innerHTML = `<svg viewBox="0 0 24 24" width="14" height="14" stroke="currentColor" stroke-width="2" fill="currentColor"><polygon points="5 3 19 12 5 21 5 3"/></svg>`;
        }
    }

    // Load custom image PNG file
    function loadCustomImage(file) {
        const reader = new FileReader();
        reader.onload = (e) => {
            const newImg = new Image();
            newImg.onload = () => {
                state.image = newImg;
                state.imageLoaded = true;
                state.texturePath = `assets/textures/${file.name}`;
                filenameDisplay.textContent = file.name;
                
                mainCanvas.width = newImg.width;
                mainCanvas.height = newImg.height;
                
                fitZoomToViewport();
                updateUI();
                drawEditor();
                saveToLocalStorage();
            };
            newImg.src = e.target.result;
        };
        reader.readAsDataURL(file);
    }

    // GRID SLICING ENGINE
    function autoSliceGrid() {
        if (!state.imageLoaded) return;
        
        const gw = state.grid.w;
        const gh = state.grid.h;
        const ox = state.grid.ox;
        const oy = state.grid.oy;
        const gx = state.grid.gx;
        const gy = state.grid.gy;
        
        const cols = Math.floor((mainCanvas.width - ox) / (gw + gx));
        const rows = Math.floor((mainCanvas.height - oy) / (gh + gy));
        
        for (let r = 0; r < rows; r++) {
            for (let c = 0; c < cols; c++) {
                const x = ox + c * (gw + gx);
                const y = oy + r * (gh + gy);
                const name = `cell_${r}_${c}`;
                
                // Overwrite or create
                state.sprites[name] = { x, y, w: gw, h: gh };
            }
        }
        
        updateUI();
        drawEditor();
        saveToLocalStorage();
    }

    // RENDER SPRITE SIDEBAR ITEMS
    function renderSpriteList() {
        spriteListContainer.innerHTML = '';
        
        const query = spriteSearch.value.toLowerCase().trim();
        const sortedKeys = Object.keys(state.sprites).sort();
        
        sortedKeys.forEach((key) => {
            if (query && !key.toLowerCase().includes(query)) return;
            
            const rect = state.sprites[key];
            const isActive = (key === state.activeSpriteId);
            
            const card = document.createElement('div');
            card.className = `sprite-item ${isActive ? 'active' : ''}`;
            card.dataset.spriteId = key;
            
            // Draw thumbnail canvas
            const thumbCanvas = document.createElement('canvas');
            thumbCanvas.className = 'sprite-preview-canvas';
            thumbCanvas.width = 32;
            thumbCanvas.height = 32;
            const thumbCtx = thumbCanvas.getContext('2d');
            
            if (state.imageLoaded) {
                // Keep pixelated scaling
                thumbCtx.imageSmoothingEnabled = false;
                thumbCtx.drawImage(
                    state.image,
                    rect.x, rect.y, rect.w, rect.h,
                    0, 0, 32, 32
                );
            }
            
            // Build text fields & actions
            card.innerHTML = `
                <div class="sprite-info-inputs">
                    <input type="text" class="sprite-name-input" value="${key}" spellcheck="false">
                    <span class="sprite-coords-label">X:${rect.x} Y:${rect.y} (${rect.w}x${rect.h})</span>
                </div>
                <div class="sprite-actions">
                    <button class="sprite-btn append-btn" title="Append to Timeline">
                        <svg viewBox="0 0 24 24" width="14" height="14" stroke="currentColor" stroke-width="2" fill="none"><polyline points="9 18 15 12 9 6"/></svg>
                    </button>
                    <button class="sprite-btn delete-btn" title="Delete Sprite">
                        <svg viewBox="0 0 24 24" width="14" height="14" stroke="currentColor" stroke-width="2" fill="none"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
                    </button>
                </div>
            `;
            
            // Insert thumbnail canvas at start
            card.insertBefore(thumbCanvas, card.firstChild);
            
            // Card selection click
            card.addEventListener('click', (e) => {
                if (e.target.closest('input') || e.target.closest('button')) return;
                
                state.activeSpriteId = key;
                updateSpriteListSelection();
                drawEditor();
            });
            
            // Rename input listener
            const nameInput = card.querySelector('.sprite-name-input');
            nameInput.addEventListener('change', (e) => {
                const newName = e.target.value.trim().toLowerCase().replace(/\s+/g, '_');
                if (!newName || newName === key) {
                    nameInput.value = key;
                    return;
                }
                if (state.sprites[newName]) {
                    alert("Sprite name already exists!");
                    nameInput.value = key;
                    return;
                }
                
                // Update key in state
                state.sprites[newName] = state.sprites[key];
                delete state.sprites[key];
                
                // Update referenced animations
                Object.values(state.animations).forEach((anim) => {
                    anim.frames.forEach((frame) => {
                        if (frame.sprite === key) {
                            frame.sprite = newName;
                        }
                    });
                });
                
                if (state.activeSpriteId === key) {
                    state.activeSpriteId = newName;
                }
                
                updateUI();
                drawEditor();
                saveToLocalStorage();
            });
            
            // Timeline Append Button
            const appendBtn = card.querySelector('.append-btn');
            appendBtn.addEventListener('click', () => {
                appendFrameToAnimation(key);
            });
            
            // Delete button
            const deleteBtn = card.querySelector('.delete-btn');
            deleteBtn.addEventListener('click', () => {
                deleteSprite(key);
            });
            
            // Double click timeline append
            card.addEventListener('dblclick', (e) => {
                if (e.target.closest('input') || e.target.closest('button')) return;
                appendFrameToAnimation(key);
            });
            
            spriteListContainer.appendChild(card);
        });
    }

    function updateSpriteListSelection() {
        spriteListContainer.querySelectorAll('.sprite-item').forEach((item) => {
            if (item.dataset.spriteId === state.activeSpriteId) {
                item.classList.add('active');
            } else {
                item.classList.remove('active');
            }
        });
    }

    function highlightSpriteListItem(spriteId) {
        const item = spriteListContainer.querySelector(`[data-sprite-id="${spriteId}"]`);
        if (item) {
            item.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
        }
    }

    function deleteSprite(key) {
        if (confirm(`Delete sprite '${key}'? This will remove it from all animations.`)) {
            delete state.sprites[key];
            
            // Remove from animations
            Object.values(state.animations).forEach((anim) => {
                anim.frames = anim.frames.filter(frame => frame.sprite !== key);
            });
            
            if (state.activeSpriteId === key) {
                state.activeSpriteId = null;
            }
            
            updateUI();
            drawEditor();
            saveToLocalStorage();
        }
    }

    // ANIMATION TIMELINE BUILDER
    function appendFrameToAnimation(spriteKey) {
        if (!state.activeAnimId) {
            // Auto create an animation if none exists
            state.animations["default"] = { loop: true, frames: [] };
            state.activeAnimId = "default";
        }
        
        const anim = state.animations[state.activeAnimId];
        anim.frames.push({
            sprite: spriteKey,
            duration: 0.12 // Default 120ms duration
        });
        
        updateUI();
        saveToLocalStorage();
    }

    function renderTimeline() {
        timelineFrames.innerHTML = '';
        
        if (!state.activeAnimId || !state.animations[state.activeAnimId]) {
            frameCountDisplay.textContent = "0 frames";
            return;
        }
        
        const anim = state.animations[state.activeAnimId];
        frameCountDisplay.textContent = `${anim.frames.length} frames`;
        checkAnimLoop.checked = anim.loop;
        
        anim.frames.forEach((frame, idx) => {
            const rect = state.sprites[frame.sprite];
            if (!rect) return; // Skip missing sprites
            
            const card = document.createElement('div');
            card.className = 'frame-card';
            
            // Canvas thumbnail for frame
            const frameCanvas = document.createElement('canvas');
            frameCanvas.className = 'frame-card-canvas';
            frameCanvas.width = 32;
            frameCanvas.height = 32;
            const fCtx = frameCanvas.getContext('2d');
            
            if (state.imageLoaded) {
                fCtx.imageSmoothingEnabled = false;
                fCtx.drawImage(
                    state.image,
                    rect.x, rect.y, rect.w, rect.h,
                    0, 0, 32, 32
                );
            }
            
            card.innerHTML = `
                <div class="frame-index-badge">${idx}</div>
                <input type="number" step="0.01" min="0.01" class="frame-duration-input" value="${frame.duration}" title="Frame duration (seconds)">
                <div class="frame-card-actions">
                    <button class="frame-mini-btn btn-move-left" title="Move frame left">
                        <svg viewBox="0 0 24 24" width="10" height="10" stroke="currentColor" stroke-width="2" fill="none"><polyline points="15 18 9 12 15 6"/></svg>
                    </button>
                    <button class="frame-mini-btn delete-btn" title="Delete frame">
                        <svg viewBox="0 0 24 24" width="10" height="10" stroke="currentColor" stroke-width="2" fill="none"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>
                    </button>
                    <button class="frame-mini-btn btn-move-right" title="Move frame right">
                        <svg viewBox="0 0 24 24" width="10" height="10" stroke="currentColor" stroke-width="2" fill="none"><polyline points="9 18 15 12 9 6"/></svg>
                    </button>
                </div>
            `;
            
            card.insertBefore(frameCanvas, card.children[1]);
            
            // Duration input change
            const durationInput = card.querySelector('.frame-duration-input');
            durationInput.addEventListener('change', (e) => {
                const val = parseFloat(e.target.value) || 0.1;
                frame.duration = val;
                updateUI();
                saveToLocalStorage();
            });
            
            // Delete frame
            card.querySelector('.delete-btn').addEventListener('click', () => {
                anim.frames.splice(idx, 1);
                // Adjust active preview bounds
                if (state.previewFrameIdx >= anim.frames.length) {
                    state.previewFrameIdx = Math.max(0, anim.frames.length - 1);
                }
                updateUI();
                saveToLocalStorage();
            });
            
            // Move left
            card.querySelector('.btn-move-left').addEventListener('click', () => {
                if (idx > 0) {
                    const temp = anim.frames[idx];
                    anim.frames[idx] = anim.frames[idx - 1];
                    anim.frames[idx - 1] = temp;
                    updateUI();
                    saveToLocalStorage();
                }
            });
            
            // Move right
            card.querySelector('.btn-move-right').addEventListener('click', () => {
                if (idx < anim.frames.length - 1) {
                    const temp = anim.frames[idx];
                    anim.frames[idx] = anim.frames[idx + 1];
                    anim.frames[idx + 1] = temp;
                    updateUI();
                    saveToLocalStorage();
                }
            });
            
            timelineFrames.appendChild(card);
        });
    }

    // ANIMATION PLAYBACK ENGINE LOOP
    function animationLoop(timestamp) {
        if (!state.lastTime) state.lastTime = timestamp;
        const dt = (timestamp - state.lastTime) / 1000.0; // Seconds elapsed
        state.lastTime = timestamp;
        
        if (state.isPlaying && state.activeAnimId && state.animations[state.activeAnimId]) {
            const anim = state.animations[state.activeAnimId];
            if (anim.frames.length > 0) {
                state.previewTimer += dt;
                
                let currentFrame = anim.frames[state.previewFrameIdx];
                // Handle fallback if active frame sprite got deleted
                if (!currentFrame || !state.sprites[currentFrame.sprite]) {
                    state.previewFrameIdx = 0;
                    state.previewTimer = 0;
                } else {
                    if (state.previewTimer >= currentFrame.duration) {
                        state.previewTimer -= currentFrame.duration;
                        state.previewFrameIdx++;
                        
                        if (state.previewFrameIdx >= anim.frames.length) {
                            if (anim.loop) {
                                state.previewFrameIdx = 0;
                            } else {
                                state.previewFrameIdx = anim.frames.length - 1;
                                state.isPlaying = false;
                                updatePlayPauseButton();
                            }
                        }
                    }
                }
            }
        }
        
        drawPreview();
        requestAnimationFrame(animationLoop);
    }

    // Draw frame on preview viewport canvas
    function drawPreview() {
        previewCtx.clearRect(0, 0, previewCanvas.width, previewCanvas.height);
        
        if (!state.activeAnimId || !state.animations[state.activeAnimId]) return;
        const anim = state.animations[state.activeAnimId];
        if (anim.frames.length === 0) return;
        
        const frame = anim.frames[state.previewFrameIdx];
        if (!frame) return;
        
        const rect = state.sprites[frame.sprite];
        if (!rect) return;
        
        previewCtx.save();
        previewCtx.imageSmoothingEnabled = false;
        
        // Scale and center the sprite in the preview canvas
        const scaledW = rect.w * state.previewScale;
        const scaledH = rect.h * state.previewScale;
        const drawX = (previewCanvas.width - scaledW) / 2;
        const drawY = (previewCanvas.height - scaledH) / 2;
        
        if (state.imageLoaded) {
            previewCtx.drawImage(
                state.image,
                rect.x, rect.y, rect.w, rect.h,
                drawX, drawY, scaledW, scaledH
            );
        }
        previewCtx.restore();
    }

    // GENERATE AND RENDER REAL-TIME JSON CONFIG
    function serializeStateToJSON() {
        const config = {
            texturePath: state.texturePath,
            sprites: state.sprites,
            animations: state.animations
        };
        jsonTextarea.value = JSON.stringify(config, null, 2);
    }

    // IMPORT JSON CONFIG LOADER
    function importJSONConfig(config, loadSampleTexture = true) {
        if (!config) return;
        
        if (config.sprites) {
            state.sprites = config.sprites;
        }
        if (config.animations) {
            state.animations = config.animations;
        }
        
        if (config.texturePath && loadSampleTexture) {
            state.texturePath = config.texturePath;
            // Check if loading custom texture, else keep default
            const assetName = config.texturePath.split('/').pop();
            if (assetName && assetName !== 'sample_spritesheet.png') {
                filenameDisplay.textContent = assetName;
                // If it is inside textures, attempt to load local file
                // Usually browser blocks loading from local path, so fall back to template
            }
        }
        
        // Pick first anim if current is now invalid
        const keys = Object.keys(state.animations);
        if (keys.length > 0 && (!state.activeAnimId || !state.animations[state.activeAnimId])) {
            state.activeAnimId = keys[0];
        }
        
        state.activeSpriteId = null;
        state.previewFrameIdx = 0;
        state.previewTimer = 0;
        
        updateUI();
        drawEditor();
        saveToLocalStorage();
    }

    // SYNC FULL UI ELEMENTS
    function updateUI() {
        renderSpriteList();
        renderTimeline();
        serializeStateToJSON();
        
        // Sync animation selector dropdown
        const prevSelected = state.activeAnimId;
        animSelector.innerHTML = '';
        
        const animKeys = Object.keys(state.animations);
        if (animKeys.length === 0) {
            const opt = document.createElement('option');
            opt.textContent = "-- No Animations --";
            opt.disabled = true;
            animSelector.appendChild(opt);
            btnDeleteAnim.style.display = 'none';
        } else {
            btnDeleteAnim.style.display = 'inline-flex';
            animKeys.sort().forEach((key) => {
                const opt = document.createElement('option');
                opt.value = key;
                opt.textContent = key;
                if (key === prevSelected) {
                    opt.selected = true;
                    state.activeAnimId = key;
                }
                animSelector.appendChild(opt);
            });
        }
    }

    // Run Studio
    init();
});
