// Editor state: camera, tools, brush properties, selection and playback.
// Rendering and selection transforms live in sibling modules and are mixed in below.
import { Shape } from '../models.js';
import { project } from '../project.js';
import { hexToRgb, normalizeHexColor, rgbToHex } from '../utils/color.js';
import { renderMethods } from './render.js';
import { transformMethods } from './transform.js';

export const editor = {
    canvas: null,
    ctx: null,

    camera: { x: 0, y: 0, zoom: 20 },
    tool: 'select',
    gridSize: 16,
    showGrid: true,
    snapToGrid: true,
    showRefScale: false,
    overlayImage: null,
    overlayOpacity: 0.3,
    overlayScale: 1.0,
    overlayX: 0,
    overlayY: 0,
    backgroundColor: '#121212',
    previewCanvas: null,
    showPreview: true,
    previewZoom: 2,

    currentFrameIndex: 0,
    fps: 12,
    isPlaying: false,
    lastDrawTime: 0,
    onionSkin: false,

    selectedShapes: [],
    clipboard: [],
    selectedPointIndex: -1,
    isDragging: false,
    isPanning: false,
    dragStart: { x: 0, y: 0 },
    mouseWorld: { x: 0, y: 0 },
    drawingShape: null,
    circleStart: null,
    borderWidth: 0,
    borderColor: { r: 255, g: 255, b: 255 },
    open: false,

    init(canvas, previewCanvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.previewCanvas = previewCanvas;
        this.setPreviewZoom(this.previewZoom);
        this.resize();
        window.addEventListener('resize', () => this.resize());
        this.startLoop();
    },

    resize() {
        const container = this.canvas.parentElement;
        this.canvas.width = container.clientWidth;
        this.canvas.height = container.clientHeight;
        this.camera.x = this.canvas.width / 2;
        this.camera.y = this.canvas.height / 2;
        this.render();
    },

    setTool(t) {
        if (this.tool === 'circle' && t !== 'circle') {
            this.drawingShape = null;
            this.circleStart = null;
        }
        this.tool = t;
        if (t === 'draw') {
            if (this.selectedShapes.length === 1) {
                const shape = this.selectedShapes[0];
                this.borderWidth = shape.borderWidth ?? 0;
                const color = shape.borderColor || shape.color;
                this.borderColor = { r: color.r, g: color.g, b: color.b };
                this.open = !!shape.open;
            }
            this.selectedShapes = [];
            this.selectedPointIndex = -1;
        }
        this.updateButtonStates();
        this.syncPropertyInputs();
        this.render();
    },

    setBackgroundColor(color) {
        this.backgroundColor = color;
        this.render();
    },

    setFps(value) {
        this.fps = Math.min(255, Math.max(1, Math.round(Number(value)) || 12));
        document.getElementById('fps-input').value = this.fps;
    },

    updateButtonStates() {
        for (const t of ['select', 'draw', 'circle']) {
            document.getElementById(`btn-${t}`).classList.toggle('active', this.tool === t);
        }
    },

    createShapeFromProperties() {
        const shape = new Shape();
        const rgb = hexToRgb(document.getElementById('color-picker').value);
        const borderRgb = hexToRgb(document.getElementById('border-color').value);
        shape.color = {
            r: rgb.r,
            g: rgb.g,
            b: rgb.b,
            a: parseInt(document.getElementById('alpha-slider').value)
        };
        shape.borderWidth = Math.max(0, Number(document.getElementById('border-width').value) || 0);
        shape.borderColor = { r: borderRgb.r, g: borderRgb.g, b: borderRgb.b };
        shape.open = document.getElementById('shape-open').checked;
        return shape;
    },

    updateCircleShape(endPoint) {
        if (!this.drawingShape || !this.circleStart) return;
        const radius = Math.hypot(endPoint.x - this.circleStart.x, endPoint.y - this.circleStart.y);
        const sidesInput = document.getElementById('circle-sides');
        const sides = Math.min(256, Math.max(3, Math.floor(Number(sidesInput.value) || 32)));
        sidesInput.value = sides;
        this.drawingShape.points = Array.from({ length: sides }, (_, i) => {
            const angle = (i / sides) * Math.PI * 2;
            return {
                x: this.circleStart.x + Math.cos(angle) * radius,
                y: this.circleStart.y + Math.sin(angle) * radius
            };
        });
    },

    setColor(hex) {
        const normalizedHex = normalizeHexColor(hex);
        if (!normalizedHex) return;
        const colorHex = `#${normalizedHex}`;
        document.getElementById('color-picker').value = colorHex;
        document.getElementById('color-hex').value = normalizedHex;
        const rgb = hexToRgb(colorHex);
        const alpha = parseInt(document.getElementById('alpha-slider').value);
        const col = { r: rgb.r, g: rgb.g, b: rgb.b, a: alpha };

        // Only recolor the selection with the select tool; with draw tools the
        // color is just the brush for the next shape.
        if (this.tool === 'select' && this.selectedShapes.length > 0) {
            this.selectedShapes.forEach(s => s.color = { ...col });
            project.isDirty = true;
            this.render();
            project.refreshUI();
        }
    },

    syncColorInputs(color) {
        const hex = typeof color === 'string'
            ? normalizeHexColor(color)
            : rgbToHex(color.r, color.g, color.b).slice(1);
        if (!hex) return;
        document.getElementById('color-picker').value = `#${hex}`;
        document.getElementById('color-hex').value = hex;
    },

    setAlpha(val) {
        if (this.tool === 'select' && this.selectedShapes.length > 0) {
            this.selectedShapes.forEach(s => s.color.a = parseInt(val));
            project.isDirty = true;
            this.render();
            project.refreshUI();
        }
    },

    syncPropertyInputs() {
        const shape = this.selectedShapes.length === 1 ? this.selectedShapes[0] : null;
        this.syncColorInputs(shape ? shape.color : document.getElementById('color-picker').value);
        const width = shape ? (shape.borderWidth ?? 0) : this.borderWidth;
        const color = shape ? (shape.borderColor || shape.color) : this.borderColor;
        document.getElementById('border-width').value = width;
        document.getElementById('border-color').value = rgbToHex(color.r, color.g, color.b);
        document.getElementById('shape-open').checked = shape ? !!shape.open : this.open;
    },

    setBorderWidth(value) {
        const width = Math.max(0, Number(value) || 0);
        if (this.tool === 'select' && this.selectedShapes.length > 0) {
            this.selectedShapes.forEach(shape => shape.borderWidth = width);
            project.isDirty = true;
            this.render();
        } else {
            this.borderWidth = width;
        }
    },

    setBorderColor(hex) {
        const rgb = hexToRgb(hex);
        const color = { r: rgb.r, g: rgb.g, b: rgb.b };
        if (this.tool === 'select' && this.selectedShapes.length > 0) {
            this.selectedShapes.forEach(shape => shape.borderColor = { ...color });
            project.isDirty = true;
            this.render();
        } else {
            this.borderColor = color;
        }
    },

    setOpen(open) {
        if (this.tool === 'select' && this.selectedShapes.length > 0) {
            this.selectedShapes.forEach(shape => shape.open = open);
            project.markDirty();
            this.render();
        } else {
            this.open = open;
        }
    },

    togglePlay() {
        this.isPlaying = !this.isPlaying;
        document.getElementById('play-icon').className = this.isPlaying ? "fas fa-pause" : "fas fa-play";
    },

    startLoop() {
        const loop = (time) => {
            if (this.isPlaying && time - this.lastDrawTime > (1000 / this.fps)) {
                const act = project.getCurrentAction();
                this.currentFrameIndex = (this.currentFrameIndex + 1) % act.frames.length;
                this.lastDrawTime = time;
                project.refreshUI();
                this.render();
            }
            requestAnimationFrame(loop);
        };
        requestAnimationFrame(loop);
    },

    screenToWorld(sx, sy) { return { x: (sx - this.camera.x) / this.camera.zoom, y: (sy - this.camera.y) / this.camera.zoom }; },
    worldToScreen(wx, wy) { return { x: (wx * this.camera.zoom) + this.camera.x, y: (wy * this.camera.zoom) + this.camera.y }; },
    snap(val) { return this.snapToGrid ? Math.round(val) : val; },
    clearOverlay() { this.overlayImage = null; this.render(); },

    /** Overlay scale is shown on a log2 slider so tiny and huge images both fit the range. */
    setOverlayScale(value) {
        const scale = Number(value);
        if (!(scale > 0)) return;
        this.overlayScale = scale;
        document.getElementById('overlay-scale').value = Math.log2(scale);
        document.getElementById('overlay-scale-value').value = +scale.toFixed(3);
        this.render();
    },

    setPreviewZoom(zoom) {
        this.previewZoom = Math.min(16, Math.max(1, Math.round(zoom)));
        document.getElementById('preview-zoom').textContent = `${this.previewZoom}x`;
        this.render();
    },

    togglePreview() {
        this.showPreview = !this.showPreview;
        document.getElementById('preview').classList.toggle('collapsed', !this.showPreview);
        this.render();
    },
};

Object.assign(editor, renderMethods, transformMethods);
