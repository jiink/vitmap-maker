// Undo/redo via whole-document snapshots. Vitmaps are small, so a JSON string of all
// actions per step is cheap and avoids having to write an inverse for every operation.
//
// Call `history.commit()` after a change is finished (project.markDirty() does this).
// Commits that don't change the document only refresh the stored view/selection.
import { project } from './project.js';
import { editor } from './editor/editor.js';

const LIMIT = 200;

function snapshot() {
    const frame = project.getCurrentFrame();
    return {
        data: JSON.stringify(project.actions),
        actionIndex: project.currentActionIndex,
        frameIndex: editor.currentFrameIndex,
        selection: editor.selectedShapes.map(s => frame.shapes.indexOf(s)).filter(i => i !== -1),
    };
}

export const history = {
    undoStack: [],
    redoStack: [],
    current: null,

    reset() {
        this.undoStack = [];
        this.redoStack = [];
        this.current = snapshot();
    },

    commit() {
        const snap = snapshot();
        if (this.current && snap.data === this.current.data) {
            this.current = snap;
            return;
        }
        if (this.current) this.undoStack.push(this.current);
        if (this.undoStack.length > LIMIT) this.undoStack.shift();
        this.redoStack = [];
        this.current = snap;
    },

    undo() {
        this.commit(); // flush anything not yet committed so it gets undone too
        if (this.undoStack.length === 0) return false;
        const undone = this.current;
        this.redoStack.push(undone);
        this.current = this.undoStack.pop();
        // Show the place where the undone change happened, not where the older state was made.
        this.restore(this.current, undone);
        return true;
    },

    redo() {
        this.commit();
        if (this.redoStack.length === 0) return false;
        this.undoStack.push(this.current);
        this.current = this.redoStack.pop();
        this.restore(this.current, this.current);
        return true;
    },

    restore(state, location) {
        project.actions = JSON.parse(state.data);
        project.currentActionIndex = Math.min(location.actionIndex, project.actions.length - 1);
        const frames = project.getCurrentAction().frames;
        editor.currentFrameIndex = Math.min(location.frameIndex, frames.length - 1);

        const sameView = state.actionIndex === project.currentActionIndex
            && state.frameIndex === editor.currentFrameIndex;
        const shapes = project.getCurrentFrame().shapes;
        editor.selectedShapes = sameView ? state.selection.map(i => shapes[i]).filter(Boolean) : [];
        editor.selectedPointIndex = -1;
        editor.drawingShape = null;
        editor.circleStart = null;
        editor.isDragging = false;

        // Keep `current` in sync with the view we actually ended up showing.
        this.current = snapshot();
        project.isDirty = true;
        project.refreshUI();
        editor.render();
    },
};
