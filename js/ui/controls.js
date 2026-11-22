// Wires the static HTML controls (buttons, inputs) to project/editor methods.
import { project } from '../project.js';
import { editor } from '../editor/editor.js';
import { saveToJSON, loadFromJSON } from '../io/json.js';
import { copyForScript, importString } from '../io/script-string.js';
import { history } from '../history.js';

const $ = (id) => document.getElementById(id);

const actions = {
    'save-json': () => saveToJSON(),
    'load-json': () => $('file-input').click(),
    'copy-script': () => copyForScript(),
    'import-string': () => importString(),
    'snap-selection': () => editor.snapSelectedShapesToGrid(),
    'clear-overlay': () => editor.clearOverlay(),
    'create-action': () => project.createAction(),
    'delete-shape': () => editor.deleteSelectedShape(),
    'shape-down': () => editor.moveShapeOrder(-1),
    'shape-up': () => editor.moveShapeOrder(1),
    'toggle-play': () => editor.togglePlay(),
    'add-frame': () => project.addFrame(),
    'duplicate-frame': () => project.duplicateFrame(),
    'delete-frame': () => project.deleteFrame(),
    'undo': () => history.undo(),
    'redo': () => history.redo(),
    'preview-zoom-in': () => editor.setPreviewZoom(editor.previewZoom + 1),
    'preview-zoom-out': () => editor.setPreviewZoom(editor.previewZoom - 1),
    'toggle-preview': () => editor.togglePreview(),
};

/** Bind an input event to a handler receiving the element. */
function on(id, event, handler) {
    $(id).addEventListener(event, (e) => handler(e.currentTarget));
}

/** Bind an input to an editor setting and re-render. */
function setting(id, event, apply) {
    on(id, event, (input) => { apply(input); editor.render(); });
}

export function bindControls() {
    document.querySelectorAll('[data-action]').forEach(button => {
        const handler = actions[button.dataset.action];
        if (!handler) throw new Error(`No handler for data-action="${button.dataset.action}"`);
        button.addEventListener('click', handler);
    });
    document.querySelectorAll('[data-tool]').forEach(button => {
        button.addEventListener('click', () => editor.setTool(button.dataset.tool));
    });
    document.querySelectorAll('[data-transform]').forEach(button => {
        button.addEventListener('click', () => editor.transformSelectedShapes(button.dataset.transform));
    });

    on('file-input', 'change', (input) => { loadFromJSON(input.files[0]); input.value = ''; });
    on('project-name', 'input', (input) => { project.name = input.value; project.isDirty = true; });
    $('new-action-name').addEventListener('keydown', (e) => { if (e.key === 'Enter') project.createAction(); });

    // Properties
    on('color-picker', 'input', (input) => editor.setColor(input.value));
    on('color-hex', 'input', (input) => editor.setColor(input.value));
    on('editor-background-color', 'input', (input) => editor.setBackgroundColor(input.value));
    on('alpha-slider', 'input', (input) => editor.setAlpha(input.value));
    on('border-width', 'input', (input) => editor.setBorderWidth(input.value));
    on('border-color', 'input', (input) => editor.setBorderColor(input.value));
    on('shape-open', 'change', (input) => editor.setOpen(input.checked));
    // These fire on every drag/keystroke; record a single undo step once the edit is done.
    for (const id of ['color-picker', 'color-hex', 'alpha-slider', 'border-width', 'border-color']) {
        $(id).addEventListener('change', () => { if (project.isDirty) history.commit(); });
    }

    // Grid
    setting('grid-size', 'change', (input) => { editor.gridSize = parseInt(input.value) || 16; });
    setting('grid-visible', 'change', (input) => { editor.showGrid = input.checked; });
    on('snap-check', 'change', (input) => { editor.snapToGrid = input.checked; });
    setting('ref-check', 'change', (input) => { editor.showRefScale = input.checked; });

    // Overlay
    setting('overlay-opacity', 'input', (input) => { editor.overlayOpacity = parseFloat(input.value); });
    on('overlay-scale', 'input', (input) => editor.setOverlayScale(2 ** parseFloat(input.value)));
    on('overlay-scale-value', 'change', (input) => editor.setOverlayScale(input.value));

    // Preview: wheel zooms it (view only, no editing).
    $('preview-canvas').addEventListener('wheel', (e) => {
        e.preventDefault();
        editor.setPreviewZoom(editor.previewZoom + (e.deltaY > 0 ? -1 : 1));
    });
    setting('overlay-x', 'input', (input) => { editor.overlayX = parseInt(input.value); });
    setting('overlay-y', 'input', (input) => { editor.overlayY = parseInt(input.value); });

    // Timeline
    setting('onion-skin', 'change', (input) => { editor.onionSkin = input.checked; });
    on('fps-input', 'change', (input) => editor.setFps(input.value));
}
