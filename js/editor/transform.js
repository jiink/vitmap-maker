// Operations on the current selection. Mixed into `editor`, so `this` is the editor.
import { project } from '../project.js';

export const transformMethods = {
    transformSelectedShapes(transform) {
        if (this.selectedShapes.length === 0) return;
        let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;
        this.selectedShapes.forEach(shape => {
            shape.points.forEach(point => {
                minX = Math.min(minX, point.x);
                maxX = Math.max(maxX, point.x);
                minY = Math.min(minY, point.y);
                maxY = Math.max(maxY, point.y);
            });
        });
        if (minX === Infinity) return;

        const centerX = (minX + maxX) / 2;
        const centerY = (minY + maxY) / 2;
        const angle = transform === 'rotate-left'
            ? -Math.PI / 4
            : transform === 'rotate-right' ? Math.PI / 4 : 0;

        this.selectedShapes.forEach(shape => {
            shape.points.forEach(point => {
                const x = point.x - centerX;
                const y = point.y - centerY;
                if (transform === 'mirror-horizontal') {
                    point.x = centerX - x;
                } else if (transform === 'mirror-vertical') {
                    point.y = centerY - y;
                } else {
                    point.x = centerX + x * Math.cos(angle) - y * Math.sin(angle);
                    point.y = centerY + x * Math.sin(angle) + y * Math.cos(angle);
                }
            });
        });

        project.markDirty();
        this.render();
    },

    snapSelectedShapesToGrid() {
        if (this.selectedShapes.length === 0) return;
        this.selectedShapes.forEach(shape => {
            shape.points.forEach(point => {
                point.x = Math.round(point.x);
                point.y = Math.round(point.y);
            });
        });
        project.markDirty();
        this.render();
    },

    deleteSelectedShape() {
        if (this.selectedShapes.length === 0) return;
        const frame = project.getCurrentFrame();
        const selectedIndices = this.selectedShapes
            .map(shape => frame.shapes.indexOf(shape))
            .filter(index => index !== -1);
        if (selectedIndices.length === 0) return;
        const nextListPosition = Math.min(...selectedIndices.map(index => frame.shapes.length - 1 - index));

        this.selectedShapes.forEach(shape => {
            const idx = frame.shapes.indexOf(shape);
            if (idx > -1) frame.shapes.splice(idx, 1);
        });

        const nextIndex = frame.shapes.length - 1 - Math.min(nextListPosition, frame.shapes.length - 1);
        this.selectedShapes = frame.shapes.length > 0 ? [frame.shapes[nextIndex]] : [];
        this.selectedPointIndex = -1;
        project.markDirty();
        this.syncPropertyInputs();
        this.render();
        project.refreshUI();
    },

    moveShapeOrder(dir) {
        // Only reorder a single selection, for simplicity.
        if (this.selectedShapes.length !== 1) return;
        const shape = this.selectedShapes[0];
        const frame = project.getCurrentFrame();
        const idx = frame.shapes.indexOf(shape);
        if (idx === -1) return;

        const newIdx = idx + dir;
        if (newIdx >= 0 && newIdx < frame.shapes.length) {
            [frame.shapes[idx], frame.shapes[newIdx]] = [frame.shapes[newIdx], frame.shapes[idx]];
            project.markDirty();
            this.render();
            project.refreshUI();
        }
    },
};
