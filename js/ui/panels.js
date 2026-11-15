// Rebuilds the action list, frame timeline and layer list from project/editor state.
import { project } from '../project.js';
import { editor } from '../editor/editor.js';

function el(tag, className, text) {
    const node = document.createElement(tag);
    if (className) node.className = className;
    if (text !== undefined) node.textContent = text;
    return node;
}

function renderActionList() {
    const list = document.getElementById('action-list');
    list.replaceChildren();
    project.actions.forEach((act, idx) => {
        const row = el('div', `action-item ${idx === project.currentActionIndex ? 'active' : ''}`);

        // Names can come from imported files/strings, so never inject them as HTML.
        const name = el('span', 'action-name', `${act.name} `);
        name.appendChild(el('small', '', `(${act.frames.length}f)`));

        const buttons = el('div', 'action-buttons');
        const rename = el('i', 'fas fa-edit');
        rename.title = 'Rename';
        rename.onclick = (e) => { e.stopPropagation(); project.renameAction(idx); };
        const del = el('i', 'fas fa-trash action-delete-btn');
        del.title = 'Delete';
        del.onclick = (e) => { e.stopPropagation(); project.deleteAction(idx); };
        buttons.append(rename, del);

        row.append(name, buttons);
        row.onclick = () => project.selectAction(idx);
        list.appendChild(row);
    });
}

function renderTimeline() {
    const frameCont = document.getElementById('frames-container');
    frameCont.replaceChildren();
    project.getCurrentAction().frames.forEach((frm, idx) => {
        const div = el('div', `frame ${idx === editor.currentFrameIndex ? 'active' : ''}`, String(idx + 1));
        div.onclick = () => project.selectFrame(idx);
        frameCont.appendChild(div);
    });
}

function renderLayerList() {
    const layerList = document.getElementById('layer-list');
    layerList.replaceChildren();
    const currentFrame = project.getCurrentFrame();
    editor.syncPropertyInputs();
    [...currentFrame.shapes].reverse().forEach((shape, revIdx) => {
        const realIdx = currentFrame.shapes.length - 1 - revIdx;
        const isSelected = editor.selectedShapes.includes(shape);
        const row = el('div', `layer-item ${isSelected ? 'selected' : ''}`);

        const c = shape.color;
        const swatch = el('div', 'layer-color');
        swatch.style.background = `rgba(${c.r},${c.g},${c.b},${c.a / 255})`;
        row.append(swatch, el('div', 'layer-name', `Shape ${realIdx} (${shape.points.length} pts)`), el('i', 'fas fa-eye'));

        row.onclick = (e) => {
            editor.tool = 'select';
            if (e.shiftKey) {
                const idx = editor.selectedShapes.indexOf(shape);
                if (idx > -1) editor.selectedShapes.splice(idx, 1);
                else editor.selectedShapes.push(shape);
            } else {
                editor.selectedShapes = [shape];
            }

            if (editor.selectedShapes.length === 1) {
                const s = editor.selectedShapes[0];
                editor.syncColorInputs(s.color);
                document.getElementById('alpha-slider').value = s.color.a;
            }

            editor.updateButtonStates();
            editor.render();
            refreshUI();
        };
        layerList.appendChild(row);
    });
}

export function refreshUI() {
    renderActionList();
    renderTimeline();
    renderLayerList();
}
