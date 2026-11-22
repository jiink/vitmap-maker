// Canvas drawing. Mixed into `editor`, so `this` is the editor (this.ctx, this.camera, ...).
import { project } from '../project.js';

export const renderMethods = {
    render() {
        const { ctx, canvas } = this;
        if (!ctx) return;
        ctx.fillStyle = this.backgroundColor;
        ctx.fillRect(0, 0, canvas.width, canvas.height);

        if (this.showRefScale) this.drawRefScale();

        if (this.onionSkin && this.currentFrameIndex > 0) {
            const prevFrame = project.getCurrentAction().frames[this.currentFrameIndex - 1];
            this.drawFrame(prevFrame, 0.3);
        }

        this.drawFrame(project.getCurrentFrame(), 1.0);
        if (this.drawingShape) this.drawInProgressShape();

        // Reference image goes above every shape; only the editing aids (grid,
        // selection handles, cursor) are drawn over it so they stay usable.
        if (this.overlayImage) this.drawOverlay();
        this.drawGrid();

        if (this.tool === 'select') {
            this.selectedShapes.forEach(shape => {
                this.drawSelectionUI(shape, this.selectedShapes.length === 1);
            });
        }

        if (this.tool === 'draw' || this.isDragging) this.drawCursor();
        this.renderPreview();
    },

    /** The small read-only viewport: current frame at a fixed zoom, centered on the origin. */
    renderPreview() {
        const canvas = this.previewCanvas;
        if (!canvas || !this.showPreview) return;
        const previewCtx = canvas.getContext('2d');
        previewCtx.fillStyle = this.backgroundColor;
        previewCtx.fillRect(0, 0, canvas.width, canvas.height);

        // Reuse the normal drawing code by pointing it at the preview's context and camera.
        const saved = { ctx: this.ctx, camera: this.camera };
        this.ctx = previewCtx;
        this.camera = { x: canvas.width / 2, y: canvas.height / 2, zoom: this.previewZoom };
        try {
            this.drawFrame(project.getCurrentFrame(), 1.0);
            if (this.drawingShape) this.drawPoly(this.drawingShape, 1.0, false);
        } finally {
            Object.assign(this, saved);
        }
    },

    drawRefScale() {
        const { ctx } = this;
        ctx.save();
        const topLeft = this.worldToScreen(-8, -8);
        const dim = 16 * this.camera.zoom;
        ctx.strokeStyle = "#00ff00";
        ctx.lineWidth = 1;
        ctx.setLineDash([4, 4]);
        ctx.strokeRect(topLeft.x, topLeft.y, dim, dim);
        const origin = this.worldToScreen(0, 0);
        ctx.fillStyle = "#00ff00";
        ctx.fillRect(origin.x - 2, origin.y - 2, 4, 4);
        ctx.restore();
    },

    drawInProgressShape() {
        const { ctx } = this;
        const pts = this.drawingShape.points;
        if (pts.length === 0) return;
        ctx.beginPath();
        const start = this.worldToScreen(pts[0].x, pts[0].y);
        ctx.moveTo(start.x, start.y);
        ctx.fillStyle = "#fff"; ctx.fillRect(start.x - 3, start.y - 3, 6, 6);
        for (let i = 1; i < pts.length; i++) {
            const p = this.worldToScreen(pts[i].x, pts[i].y);
            ctx.lineTo(p.x, p.y); ctx.fillRect(p.x - 2, p.y - 2, 4, 4);
        }
        if (!this.drawingShape.open) ctx.closePath();
        ctx.strokeStyle = "#00aaff"; ctx.lineWidth = 2; ctx.stroke();
        const lastPt = pts[pts.length - 1];
        const lastScreen = this.worldToScreen(lastPt.x, lastPt.y);
        const mouseScreen = this.worldToScreen(this.mouseWorld.x, this.mouseWorld.y);
        ctx.beginPath(); ctx.moveTo(lastScreen.x, lastScreen.y); ctx.lineTo(mouseScreen.x, mouseScreen.y);
        ctx.strokeStyle = "#00aaff"; ctx.lineWidth = 1; ctx.setLineDash([5, 5]); ctx.stroke(); ctx.setLineDash([]);
    },

    drawOverlay() {
        const { ctx } = this;
        ctx.save();
        ctx.globalAlpha = this.overlayOpacity;
        const screenPos = this.worldToScreen(this.overlayX, this.overlayY);
        const w = this.overlayImage.width * (this.camera.zoom / this.gridSize) * this.overlayScale;
        const h = this.overlayImage.height * (this.camera.zoom / this.gridSize) * this.overlayScale;
        ctx.drawImage(this.overlayImage, screenPos.x - w / 2, screenPos.y - h / 2, w, h);
        ctx.restore();
    },

    drawCursor() {
        const { ctx } = this;
        const s = this.worldToScreen(this.mouseWorld.x, this.mouseWorld.y);
        ctx.strokeStyle = "#fff"; ctx.lineWidth = 1;
        ctx.beginPath(); ctx.arc(s.x, s.y, 3, 0, Math.PI * 2); ctx.stroke();
        ctx.fillStyle = "#fff"; ctx.font = "10px monospace";
        ctx.fillText(`${Math.round(this.mouseWorld.x)}, ${Math.round(this.mouseWorld.y)}`, s.x + 10, s.y);
    },

    drawGrid() {
        if (!this.showGrid) return;
        const { ctx, canvas } = this;
        const zoom = this.camera.zoom;
        const stride = Math.max(1, Math.ceil(12 / zoom));
        const spacing = zoom * stride;
        const offsetX = this.camera.x % spacing;
        const offsetY = this.camera.y % spacing;
        ctx.beginPath(); ctx.fillStyle = "#444";
        for (let x = offsetX; x < canvas.width; x += spacing) {
            for (let y = offsetY; y < canvas.height; y += spacing) {
                ctx.moveTo(x + 1.5, y);
                ctx.arc(x, y, 1.5, 0, Math.PI * 2);
            }
        }
        ctx.fill();
        const origin = this.worldToScreen(0, 0);
        ctx.beginPath(); ctx.strokeStyle = "#444"; ctx.lineWidth = 2;
        ctx.moveTo(origin.x - 10, origin.y); ctx.lineTo(origin.x + 10, origin.y);
        ctx.moveTo(origin.x, origin.y - 10); ctx.lineTo(origin.x, origin.y + 10);
        ctx.stroke();
    },

    drawFrame(frame, alphaMult) {
        frame.shapes.forEach(shape => { if (!shape.hidden) this.drawPoly(shape, alphaMult, false); });
    },

    drawPoly(shape, alphaMult, isOutline) {
        const { ctx } = this;
        if (shape.points.length === 0) return;
        ctx.beginPath();
        const start = this.worldToScreen(shape.points[0].x, shape.points[0].y);
        ctx.moveTo(start.x, start.y);
        for (let i = 1; i < shape.points.length; i++) {
            const p = this.worldToScreen(shape.points[i].x, shape.points[i].y);
            ctx.lineTo(p.x, p.y);
        }
        const isOpen = !!shape.open;
        if (!isOpen) ctx.closePath();
        const c = shape.color;
        ctx.fillStyle = `rgba(${c.r},${c.g},${c.b},${(c.a / 255) * alphaMult})`;
        if (!isOpen && !isOutline) ctx.fill();
        const borderWidth = Math.max(0, Number(shape.borderWidth) || 0);
        if (borderWidth > 0) {
            const border = shape.borderColor || c;
            ctx.strokeStyle = `rgba(${border.r},${border.g},${border.b},${alphaMult})`;
            ctx.lineWidth = borderWidth * this.camera.zoom;
            ctx.lineJoin = "round";
            ctx.stroke();
        }
    },

    drawSelectionUI(shape, showPoints) {
        const { ctx } = this;
        ctx.beginPath();
        if (shape.points.length > 0) {
            const start = this.worldToScreen(shape.points[0].x, shape.points[0].y);
            ctx.moveTo(start.x, start.y);
            for (let i = 1; i < shape.points.length; i++) {
                const p = this.worldToScreen(shape.points[i].x, shape.points[i].y);
                ctx.lineTo(p.x, p.y);
            }
            if (!shape.open) ctx.closePath();
            ctx.strokeStyle = "#00aaff"; ctx.lineWidth = 1.5; ctx.setLineDash([5, 3]); ctx.stroke(); ctx.setLineDash([]);
        }
        if (showPoints) {
            shape.points.forEach((pt, idx) => {
                const s = this.worldToScreen(pt.x, pt.y);
                ctx.beginPath(); ctx.arc(s.x, s.y, 4, 0, Math.PI * 2);
                ctx.fillStyle = (idx === this.selectedPointIndex) ? "#ff0000" : "#fff";
                ctx.strokeStyle = "#000"; ctx.fill(); ctx.stroke();
            });
        }
    },
};
