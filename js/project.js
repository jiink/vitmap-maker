// Project document: the list of actions (animations) and their frames.
import { Action, Frame } from './models.js';
import { editor } from './editor/editor.js';
import { refreshUI } from './ui/panels.js';

export const project = {
    actions: [],
    currentActionIndex: 0,
    name: "animation",
    isDirty: false,

    init() {
        this.actions = [new Action("Default")];
        this.currentActionIndex = 0;
        document.getElementById('project-name').value = this.name;
        this.isDirty = false;
        this.refreshUI();
    },

    refreshUI() { refreshUI(); },

    getCurrentAction() { return this.actions[this.currentActionIndex]; },
    getCurrentFrame() { return this.getCurrentAction().frames[editor.currentFrameIndex]; },

    /** Replace the whole document (used by JSON load and VM1 import). */
    loadData({ name, actions, fps }) {
        actions.forEach(action => {
            if (action.loop === undefined) action.loop = true;
            action.frames.forEach(frame => frame.shapes.forEach(shape => {
                if (shape.open === undefined) shape.open = shape.closed === false;
                delete shape.closed;
            }));
        });
        this.actions = actions;
        this.name = name;
        document.getElementById('project-name').value = name;
        if (fps) editor.setFps(fps);
        this.currentActionIndex = 0;
        editor.currentFrameIndex = 0;
        editor.selectedShapes = [];
        editor.selectedPointIndex = -1;
        this.isDirty = false;
        this.refreshUI();
        editor.render();
    },

    createAction() {
        const nameInput = document.getElementById('new-action-name');
        const name = nameInput.value || "Untitled";
        this.actions.push(new Action(name));
        this.currentActionIndex = this.actions.length - 1;
        editor.currentFrameIndex = 0;
        editor.selectedShapes = [];
        nameInput.value = "";
        this.isDirty = true;
        this.refreshUI();
        editor.render();
    },

    deleteAction(index) {
        if (this.actions.length <= 1) {
            alert("At least one action is required.");
            return;
        }
        if (!confirm(`Delete action "${this.actions[index].name}"?`)) return;

        this.actions.splice(index, 1);
        if (this.currentActionIndex >= this.actions.length) {
            this.currentActionIndex = this.actions.length - 1;
        }
        editor.currentFrameIndex = 0;
        editor.selectedShapes = [];
        this.isDirty = true;
        this.refreshUI();
        editor.render();
    },

    selectAction(index) {
        this.currentActionIndex = index;
        editor.currentFrameIndex = 0;
        editor.selectedShapes = [];
        this.refreshUI();
        editor.render();
    },

    renameAction(index) {
        const newName = prompt("Enter new name for this action:", this.actions[index].name);
        if (newName && newName.trim() !== "") {
            this.actions[index].name = newName.trim();
            this.isDirty = true;
            this.refreshUI();
        }
    },

    selectFrame(index) {
        editor.currentFrameIndex = index;
        editor.selectedShapes = [];
        editor.render();
        this.refreshUI();
    },

    addFrame() {
        this.getCurrentAction().frames.push(new Frame());
        editor.currentFrameIndex = this.getCurrentAction().frames.length - 1;
        editor.selectedShapes = [];
        this.isDirty = true;
        this.refreshUI();
        editor.render();
    },

    duplicateFrame() {
        const action = this.getCurrentAction();
        const currentFrame = action.frames[editor.currentFrameIndex];
        const newFrame = new Frame();
        newFrame.shapes = JSON.parse(JSON.stringify(currentFrame.shapes));

        action.frames.splice(editor.currentFrameIndex + 1, 0, newFrame);
        editor.currentFrameIndex++;
        editor.selectedShapes = [];
        this.isDirty = true;
        this.refreshUI();
        editor.render();
    },

    deleteFrame() {
        const action = this.getCurrentAction();
        if (action.frames.length <= 1) return;
        action.frames.splice(editor.currentFrameIndex, 1);
        if (editor.currentFrameIndex >= action.frames.length) {
            editor.currentFrameIndex = action.frames.length - 1;
        }
        editor.selectedShapes = [];
        this.isDirty = true;
        this.refreshUI();
        editor.render();
    },
};
