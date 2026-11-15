// "Copy for script" / "Import string" UI around the VM1 codec.
import { project } from '../project.js';
import { editor } from '../editor/editor.js';
import { encodeVM1, decodeVM1, toLuaLine, extractVM } from './vm1.js';
import { toast } from '../ui/toast.js';

// Scripts are capped at 16 KiB, so warn when a single line gets close to that.
const SCRIPT_LIMIT = 16 * 1024;

async function writeClipboard(text) {
    try {
        await navigator.clipboard.writeText(text);
        return true;
    } catch {
        return false;
    }
}

export async function copyForScript() {
    let encoded;
    try {
        encoded = encodeVM1({ actions: project.actions, fps: editor.fps });
    } catch (err) {
        alert(`Could not encode: ${err.message}`);
        return;
    }
    const line = toLuaLine(project.name || 'sprite', encoded.text);

    const notes = [`${line.length.toLocaleString()} chars (${encoded.byteLength.toLocaleString()} bytes binary)`];
    if (encoded.lossy) {
        notes.push(`coordinates rounded to 1/${2 ** encoded.scaleShift} unit (max error ${encoded.maxError.toFixed(3)})`);
    }
    const tooBig = line.length > SCRIPT_LIMIT;
    if (tooBig) notes.push('over the 16 KiB script limit!');

    if (await writeClipboard(line)) {
        toast(`Copied for script: ${notes.join(' • ')}`, { warn: encoded.lossy || tooBig, duration: 5000 });
    } else {
        // Clipboard can be blocked (e.g. permissions); fall back to a manual copy box.
        prompt(`Copy this line (${notes.join(' • ')}):`, line);
    }
}

export function importString() {
    const text = prompt('Paste a VM1:... string or the whole Lua line:');
    if (!text) return;
    const found = extractVM(text);
    if (!found) {
        alert('No VM1:... string found in what you pasted.');
        return;
    }
    let data;
    try {
        data = decodeVM1(found.vm);
    } catch (err) {
        alert(`Could not import: ${err.message}`);
        return;
    }
    if (project.isDirty && !confirm('Replace the current vitmap? Unsaved changes will be lost.')) return;
    project.loadData({ ...data, name: found.name || project.name });
    toast(`Imported ${data.actions.length} action(s)`);
}
