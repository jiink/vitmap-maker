// Mouse, wheel, keyboard and drag-and-drop handling for the canvas.
import { project } from './project.js';
import { editor } from './editor/editor.js';
import { shapeHitTest } from './utils/geometry.js';
import { history } from './history.js';

function getMousePos(canvas, evt) {
    const rect = canvas.getBoundingClientRect();
    return { x: evt.clientX - rect.left, y: evt.clientY - rect.top };
}

function snapped(worldPos) {
    return { x: editor.snap(worldPos.x), y: editor.snap(worldPos.y) };
}

function commitDrawingShape() {
    project.getCurrentFrame().shapes.push(editor.drawingShape);
    editor.selectedShapes = [editor.drawingShape];
    editor.drawingShape = null;
    project.markDirty();
    project.refreshUI();
}

function onMouseDownSelect(e, pos, worldPos, snapPos) {
    // 1. Point handles (only with a single selected shape)
    if (editor.selectedShapes.length === 1) {
        const shape = editor.selectedShapes[0];
        for (let i = 0; i < shape.points.length; i++) {
            const pt = shape.points[i];
            const sPt = editor.worldToScreen(pt.x, pt.y);
            if (Math.hypot(sPt.x - pos.x, sPt.y - pos.y) < 8) {
                editor.selectedPointIndex = i;
                editor.isDragging = true;
                editor.render();
                return;
            }
        }
    }

    // 2. Shape hit, topmost first
    const frame = project.getCurrentFrame();
    let clickedShape = null;
    for (let i = frame.shapes.length - 1; i >= 0; i--) {
        if (!frame.shapes[i].hidden && shapeHitTest(worldPos, frame.shapes[i], editor.camera.zoom)) {
            clickedShape = frame.shapes[i];
            break;
        }
    }

    if (clickedShape) {
        if (e.shiftKey) {
            const idx = editor.selectedShapes.indexOf(clickedShape);
            if (idx > -1) editor.selectedShapes.splice(idx, 1);
            else editor.selectedShapes.push(clickedShape);
            editor.selectedPointIndex = -1;
        } else {
            // Keep an existing multi-selection so it can be dragged as a group.
            if (!editor.selectedShapes.includes(clickedShape)) {
                editor.selectedShapes = [clickedShape];
            }
            editor.selectedPointIndex = -1;

            if (editor.selectedShapes.length === 1) {
                const c = editor.selectedShapes[0].color;
                editor.syncColorInputs(c);
                document.getElementById('alpha-slider').value = c.a;
            }
        }
        editor.isDragging = true;
        editor.dragStart = snapPos;
    } else if (!e.shiftKey) {
        editor.selectedShapes = [];
    }
    project.refreshUI();
    editor.render();
}

function onMouseDownDraw(e, snapPos) {
    if (e.button !== 0) return;
    if (!editor.drawingShape) {
        editor.drawingShape = editor.createShapeFromProperties();
        editor.drawingShape.points.push(snapPos);
    } else {
        const startPt = editor.drawingShape.points[0];
        if (editor.drawingShape.points.length >= 2 && snapPos.x === startPt.x && snapPos.y === startPt.y) {
            commitDrawingShape();
        } else {
            editor.drawingShape.points.push(snapPos);
        }
    }
    editor.render();
}

function bindMouse(canvas) {
    canvas.addEventListener('mousedown', (e) => {
        const pos = getMousePos(canvas, e);
        if (e.button === 1 || (e.button === 0 && e.altKey)) {
            editor.isPanning = true; editor.dragStart = pos; return;
        }

        const worldPos = editor.screenToWorld(pos.x, pos.y);
        const snapPos = snapped(worldPos);

        if (editor.tool === 'select') {
            onMouseDownSelect(e, pos, worldPos, snapPos);
        } else if (editor.tool === 'draw') {
            onMouseDownDraw(e, snapPos);
        } else if (editor.tool === 'circle' && e.button === 0) {
            editor.circleStart = snapPos;
            editor.drawingShape = editor.createShapeFromProperties();
            editor.updateCircleShape(snapPos);
            editor.render();
        }
    });

    canvas.addEventListener('mousemove', (e) => {
        const pos = getMousePos(canvas, e);
        editor.mouseWorld = snapped(editor.screenToWorld(pos.x, pos.y));

        if (editor.tool === 'circle' && editor.circleStart) {
            editor.updateCircleShape(editor.mouseWorld);
            editor.render();
            return;
        }

        if (editor.isPanning) {
            editor.camera.x += pos.x - editor.dragStart.x;
            editor.camera.y += pos.y - editor.dragStart.y;
            editor.dragStart = pos;
            editor.render();
            return;
        }

        if (editor.isDragging && editor.selectedShapes.length > 0) {
            if (editor.selectedPointIndex !== -1 && editor.selectedShapes.length === 1) {
                editor.selectedShapes[0].points[editor.selectedPointIndex] = editor.mouseWorld;
            } else {
                const deltaX = editor.mouseWorld.x - editor.dragStart.x;
                const deltaY = editor.mouseWorld.y - editor.dragStart.y;
                if (deltaX !== 0 || deltaY !== 0) {
                    editor.selectedShapes.forEach(shape => {
                        shape.points.forEach(p => { p.x += deltaX; p.y += deltaY; });
                    });
                    editor.dragStart = editor.mouseWorld;
                }
            }
        }
        editor.render();
    });

    canvas.addEventListener('mouseup', (e) => {
        if (editor.tool === 'circle' && editor.circleStart && e.button === 0) {
            const pos = getMousePos(canvas, e);
            const endPoint = snapped(editor.screenToWorld(pos.x, pos.y));
            editor.updateCircleShape(endPoint);
            if (Math.hypot(endPoint.x - editor.circleStart.x, endPoint.y - editor.circleStart.y) > 0) {
                commitDrawingShape();
            }
            editor.drawingShape = null;
            editor.circleStart = null;
            editor.render();
        }
        if (editor.isDragging) project.markDirty();
        editor.isDragging = false;
        editor.isPanning = false;
    });

    canvas.addEventListener('wheel', (e) => {
        e.preventDefault();
        const direction = e.deltaY > 0 ? -1 : 1;
        editor.camera.zoom = Math.min(100, Math.max(1, editor.camera.zoom + direction * 2));
        editor.render();
    });

    canvas.addEventListener('contextmenu', e => e.preventDefault());
}

function popDrawingPoint(e) {
    e.preventDefault();
    editor.drawingShape.points.pop();
    if (editor.drawingShape.points.length === 0) editor.drawingShape = null;
    editor.render();
}

function isTextField(target) {
    if (target.tagName === 'TEXTAREA' || target.isContentEditable) return true;
    return target.tagName === 'INPUT' && ['text', 'number', 'search', ''].includes(target.type);
}

function bindKeyboard() {
    window.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') {
            if (editor.tool === 'draw' && editor.drawingShape && editor.drawingShape.points.length > 0) {
                commitDrawingShape();
            } else {
                editor.drawingShape = null;
            }
            editor.circleStart = null;
            editor.setTool('select');
            return;
        }

        const ctrl = e.ctrlKey || e.metaKey;
        const key = e.key.toLowerCase();

        // Undo/redo work everywhere except text fields, which keep their own undo.
        if (ctrl && (key === 'z' || key === 'y') && !isTextField(e.target)) {
            e.preventDefault();
            const redo = key === 'y' || e.shiftKey;
            if (!redo && editor.tool === 'draw' && editor.drawingShape) {
                popDrawingPoint(e);
            } else if (redo) {
                history.redo();
            } else {
                history.undo();
            }
            return;
        }

        if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') return;

        if (e.key === 'Delete' || e.key === 'Backspace') {
            if (editor.tool === 'draw' && editor.drawingShape) {
                popDrawingPoint(e);
                return;
            }
            if (editor.tool === 'select') {
                if (editor.selectedPointIndex !== -1 && editor.selectedShapes.length === 1 && editor.selectedShapes[0].points.length > 3) {
                    editor.selectedShapes[0].points.splice(editor.selectedPointIndex, 1);
                    editor.selectedPointIndex = -1;
                    project.markDirty();
                    editor.render();
                } else {
                    editor.deleteSelectedShape();
                }
            }
        }

        if (ctrl && key === 'c' && editor.selectedShapes.length > 0) {
            editor.clipboard = JSON.parse(JSON.stringify(editor.selectedShapes));
        }
        if (ctrl && key === 'v' && editor.clipboard.length > 0) {
            const newShapes = JSON.parse(JSON.stringify(editor.clipboard));
            // Offset pasted shapes so they are visible
            newShapes.forEach(s => s.points.forEach(p => { p.x += 8; p.y += 8; }));
            project.getCurrentFrame().shapes.push(...newShapes);
            editor.selectedShapes = newShapes;
            editor.selectedPointIndex = -1;
            project.markDirty();
            project.refreshUI();
            editor.render();
        }

        if (e.key === ' ') { editor.togglePlay(); e.preventDefault(); }
        if (!ctrl && (e.key === 's' || e.key === '1')) editor.setTool('select');
        if (!ctrl && (e.key === 'd' || e.key === '2')) editor.setTool('draw');

        if (e.key === 'ArrowLeft' && editor.currentFrameIndex > 0) {
            project.selectFrame(editor.currentFrameIndex - 1);
        }
        if (e.key === 'ArrowRight' && editor.currentFrameIndex < project.getCurrentAction().frames.length - 1) {
            project.selectFrame(editor.currentFrameIndex + 1);
        }
    });
}

function bindImageDrop(canvas) {
    canvas.addEventListener('dragover', (e) => e.preventDefault());
    canvas.addEventListener('drop', (e) => {
        e.preventDefault();
        const file = e.dataTransfer.files[0];
        if (!file || (file.type !== "image/png" && file.type !== "image/jpeg")) return;
        const reader = new FileReader();
        reader.onload = (evt) => {
            const img = new Image();
            img.onload = () => { editor.overlayImage = img; editor.render(); };
            img.src = evt.target.result;
        };
        reader.readAsDataURL(file);
    });
}

export function bindCanvasInput(canvas) {
    bindMouse(canvas);
    bindKeyboard();
    bindImageDrop(canvas);
}
