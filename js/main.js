// Entry point: wire everything up and start the editor.
import { project } from './project.js';
import { editor } from './editor/editor.js';
import { bindControls } from './ui/controls.js';
import { bindCanvasInput } from './input.js';

const canvas = document.getElementById('main-canvas');

bindControls();
bindCanvasInput(canvas);
project.init();
editor.init(canvas, document.getElementById('preview-canvas'));

window.addEventListener('beforeunload', (e) => {
    if (project.isDirty) {
        e.preventDefault();
        e.returnValue = "Unsaved changes";
    }
});
