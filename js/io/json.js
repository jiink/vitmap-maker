// JSON save/load (full-fidelity project files).
import { project } from '../project.js';
import { editor } from '../editor/editor.js';

export function safeFileName(name) {
    return name.replace(/[<>:"/\\|?*\x00-\x1f]/g, "_").replace(/[. ]+$/, "") || "untitled";
}

export function saveToJSON() {
    const name = project.name.trim() || "untitled";
    project.name = name;
    document.getElementById('project-name').value = name;
    const data = { meta: { version: "1.0", name, fps: editor.fps }, actions: project.actions };
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: "application/json" });
    const a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = `${safeFileName(name)}.json`;
    a.click();
    setTimeout(() => URL.revokeObjectURL(a.href), 1000);
    project.isDirty = false;
}

export function loadFromJSON(file) {
    if (!file) return;
    const reader = new FileReader();
    reader.onload = (e) => {
        let data;
        try {
            data = JSON.parse(e.target.result);
        } catch (err) {
            alert("Error parsing JSON");
            return;
        }
        if (!Array.isArray(data.actions)) {
            alert("This JSON file has no actions.");
            return;
        }
        project.loadData({
            actions: data.actions,
            name: data.meta && typeof data.meta.name === 'string'
                ? data.meta.name
                : file.name.replace(/\.json$/i, ''),
            fps: data.meta?.fps,
        });
    };
    reader.readAsText(file);
}
