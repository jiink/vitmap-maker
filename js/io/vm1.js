// VM1: compact, versioned binary encoding of a vitmap, wrapped in base64.
// Pure module (no DOM) so it can be tested under Node. Spec: docs/VM1-FORMAT.md
//
//   "VM1:" + base64(payload), all multi-byte integers little-endian.
//
//   header   u8 flags      bit0 = coordinates are i16 (else i8); other bits must be 0
//            u8 scaleShift coordinate = raw / 2^scaleShift  (0..8)
//            u8 fps        playback rate (1..255)
//            varint actionCount
//   action   varint nameLen, nameLen bytes UTF-8
//            u8 actionFlags bit0 = loop; other bits must be 0
//            varint frameCount
//   frame    varint shapeCount
//   shape    u8 shapeFlags bit0 = open, bit1 = has alpha, bit2 = has border; other bits 0
//            u8 r, g, b
//            [u8 a]                          if has alpha (otherwise a = 255)
//            [u16 borderWidth*100, u8 r,g,b] if has border (otherwise width 0)
//            varint pointCount
//            pointCount * (x, y) as i8 or i16 per header flag

export const VM1_PREFIX = 'VM1:';

const FLAG_WIDE = 0x01;
const ACTION_LOOP = 0x01;
const SHAPE_OPEN = 0x01;
const SHAPE_ALPHA = 0x02;
const SHAPE_BORDER = 0x04;
const MAX_EXACT_SHIFT = 8;
const LOSSY_SHIFT = 4; // 1/16 unit when coordinates can't be represented exactly
const EPS = 1e-6;

// ---------- byte helpers ----------

class Writer {
    constructor() { this.bytes = []; }
    u8(v) { this.bytes.push(v & 0xff); }
    u16(v) { this.u8(v); this.u8(v >> 8); }
    i8(v) { this.u8(v); }
    i16(v) { this.u16(v); }
    varint(v) {
        if (!Number.isInteger(v) || v < 0) throw new Error(`Bad varint ${v}`);
        do {
            let b = v & 0x7f;
            v = Math.floor(v / 128);
            if (v > 0) b |= 0x80;
            this.u8(b);
        } while (v > 0);
    }
    str(s) {
        const enc = new TextEncoder().encode(s);
        this.varint(enc.length);
        enc.forEach(b => this.u8(b));
    }
    result() { return Uint8Array.from(this.bytes); }
}

class Reader {
    constructor(bytes) { this.b = bytes; this.i = 0; }
    need(n) { if (this.i + n > this.b.length) throw new Error('VM1 data is truncated'); }
    u8() { this.need(1); return this.b[this.i++]; }
    u16() { const lo = this.u8(); return lo | (this.u8() << 8); }
    i8() { const v = this.u8(); return v > 127 ? v - 256 : v; }
    i16() { const v = this.u16(); return v > 32767 ? v - 65536 : v; }
    varint() {
        let result = 0, mult = 1;
        for (let n = 0; n < 5; n++) {
            const b = this.u8();
            result += (b & 0x7f) * mult;
            if (!(b & 0x80)) return result;
            mult *= 128;
        }
        throw new Error('VM1 varint too long');
    }
    str() {
        const len = this.varint();
        this.need(len);
        const s = new TextDecoder('utf-8', { fatal: true }).decode(this.b.subarray(this.i, this.i + len));
        this.i += len;
        return s;
    }
}

function toBase64(bytes) {
    let bin = '';
    for (let i = 0; i < bytes.length; i += 0x8000) {
        bin += String.fromCharCode(...bytes.subarray(i, i + 0x8000));
    }
    return btoa(bin);
}

function fromBase64(text) {
    let s = text.replace(/\s+/g, '').replace(/-/g, '+').replace(/_/g, '/');
    if (!/^[A-Za-z0-9+/]*=*$/.test(s)) throw new Error('VM1 payload is not valid base64');
    s = s.replace(/=+$/, '');
    s += '='.repeat((4 - (s.length % 4)) % 4);
    const bin = atob(s);
    return Uint8Array.from(bin, c => c.charCodeAt(0));
}

const clampByte = v => Math.min(255, Math.max(0, Math.round(Number(v) || 0)));

// ---------- coordinate quantization ----------

function chooseScale(coords) {
    const fits = (shift) => coords.every(v => Math.abs(Math.round(v * 2 ** shift)) <= 32767);
    for (let shift = 0; shift <= MAX_EXACT_SHIFT; shift++) {
        const scale = 2 ** shift;
        if (!fits(shift)) break;
        if (coords.every(v => Math.abs(v * scale - Math.round(v * scale)) < EPS)) {
            return { shift, lossy: false };
        }
    }
    for (let shift = LOSSY_SHIFT; shift >= 0; shift--) {
        if (fits(shift)) return { shift, lossy: true };
    }
    throw new Error('Coordinates are too large to encode (max ±32767 units)');
}

// ---------- public API ----------

/**
 * Encode actions into a VM1 string.
 * @returns {{ text: string, byteLength: number, scaleShift: number, lossy: boolean, maxError: number }}
 */
export function encodeVM1({ actions, fps = 12 }) {
    const coords = [];
    actions.forEach(a => a.frames.forEach(f => f.shapes.forEach(s => s.points.forEach(p => coords.push(p.x, p.y)))));
    coords.forEach(v => { if (!Number.isFinite(v)) throw new Error('Shape has a non-numeric coordinate'); });

    const { shift, lossy } = chooseScale(coords);
    const scale = 2 ** shift;
    const quantized = coords.map(v => Math.round(v * scale));
    const wide = quantized.some(q => q < -128 || q > 127);
    const maxError = coords.reduce((m, v, i) => Math.max(m, Math.abs(v - quantized[i] / scale)), 0);

    const w = new Writer();
    w.u8(wide ? FLAG_WIDE : 0);
    w.u8(shift);
    w.u8(Math.min(255, Math.max(1, Math.round(fps) || 12)));
    w.varint(actions.length);

    let qi = 0;
    for (const action of actions) {
        w.str(String(action.name ?? ''));
        w.u8(action.loop === false ? 0 : ACTION_LOOP);
        w.varint(action.frames.length);
        for (const frame of action.frames) {
            w.varint(frame.shapes.length);
            for (const shape of frame.shapes) {
                const c = shape.color;
                const alpha = clampByte(c.a ?? 255);
                const borderHundredths = Math.min(65535, Math.max(0, Math.round((Number(shape.borderWidth) || 0) * 100)));
                let flags = 0;
                if (shape.open) flags |= SHAPE_OPEN;
                if (alpha !== 255) flags |= SHAPE_ALPHA;
                if (borderHundredths > 0) flags |= SHAPE_BORDER;
                w.u8(flags);
                w.u8(clampByte(c.r)); w.u8(clampByte(c.g)); w.u8(clampByte(c.b));
                if (flags & SHAPE_ALPHA) w.u8(alpha);
                if (flags & SHAPE_BORDER) {
                    const bc = shape.borderColor || c;
                    w.u16(borderHundredths);
                    w.u8(clampByte(bc.r)); w.u8(clampByte(bc.g)); w.u8(clampByte(bc.b));
                }
                w.varint(shape.points.length);
                for (let p = 0; p < shape.points.length * 2; p++) {
                    const q = quantized[qi++];
                    if (wide) w.i16(q); else w.i8(q);
                }
            }
        }
    }

    const bytes = w.result();
    return { text: VM1_PREFIX + toBase64(bytes), byteLength: bytes.length, scaleShift: shift, lossy, maxError };
}

/**
 * Decode a VM1 string (with or without the "VM1:" prefix) into editor data.
 * @returns {{ actions: object[], fps: number }}
 */
export function decodeVM1(text) {
    let body = text.trim();
    const versioned = /^VM(\d+):/.exec(body);
    if (versioned) {
        if (versioned[1] !== '1') throw new Error(`Unsupported format VM${versioned[1]} (this editor reads VM1)`);
        body = body.slice(versioned[0].length);
    }
    const r = new Reader(fromBase64(body));

    const flags = r.u8();
    if (flags & ~FLAG_WIDE) throw new Error('VM1 header has unknown flags');
    const wide = !!(flags & FLAG_WIDE);
    const shift = r.u8();
    if (shift > 15) throw new Error('VM1 scale is out of range');
    const scale = 2 ** shift;
    const fps = r.u8() || 12;
    const coord = () => (wide ? r.i16() : r.i8()) / scale;

    const actions = [];
    const actionCount = r.varint();
    for (let a = 0; a < actionCount; a++) {
        const name = r.str();
        const aFlags = r.u8();
        if (aFlags & ~ACTION_LOOP) throw new Error('VM1 action has unknown flags');
        const frames = [];
        const frameCount = r.varint();
        for (let f = 0; f < frameCount; f++) {
            const shapes = [];
            const shapeCount = r.varint();
            for (let s = 0; s < shapeCount; s++) {
                const sFlags = r.u8();
                if (sFlags & ~(SHAPE_OPEN | SHAPE_ALPHA | SHAPE_BORDER)) throw new Error('VM1 shape has unknown flags');
                const color = { r: r.u8(), g: r.u8(), b: r.u8(), a: 255 };
                if (sFlags & SHAPE_ALPHA) color.a = r.u8();
                let borderWidth = 0;
                let borderColor = { r: 255, g: 255, b: 255 };
                if (sFlags & SHAPE_BORDER) {
                    borderWidth = r.u16() / 100;
                    borderColor = { r: r.u8(), g: r.u8(), b: r.u8() };
                }
                const points = [];
                const pointCount = r.varint();
                for (let p = 0; p < pointCount; p++) points.push({ x: coord(), y: coord() });
                shapes.push({ points, color, borderWidth, borderColor, open: !!(sFlags & SHAPE_OPEN) });
            }
            frames.push({ shapes });
        }
        if (frames.length === 0) frames.push({ shapes: [] });
        actions.push({ name, frames, loop: !!(aFlags & ACTION_LOOP) });
    }
    if (r.i !== r.b.length) throw new Error('VM1 data has unexpected trailing bytes');
    if (actions.length === 0) throw new Error('VM1 data contains no actions');
    return { actions, fps };
}

/** Turn a project name into a valid Lua identifier. */
export function luaIdentifier(name) {
    let id = String(name).trim().toLowerCase().replace(/[^a-z0-9_]+/g, '_').replace(/^_+|_+$/g, '');
    if (!id) id = 'sprite';
    if (/^[0-9]/.test(id)) id = `_${id}`;
    const reserved = new Set(['and', 'break', 'do', 'else', 'elseif', 'end', 'false', 'for', 'function', 'goto', 'if',
        'in', 'local', 'nil', 'not', 'or', 'repeat', 'return', 'then', 'true', 'until', 'while']);
    return reserved.has(id) ? `${id}_` : id;
}

/** Build the ready-to-paste Lua line. */
export function toLuaLine(name, vm1Text) {
    return `local ${luaIdentifier(name)} = vitmap[[${vm1Text}]]`;
}

/**
 * Find a VM string inside pasted text: a bare "VM1:..." or a whole Lua line.
 * @returns {{ vm: string, name: string|null } | null}
 */
export function extractVM(text) {
    const match = /VM\d+:[A-Za-z0-9+/=_-]*/.exec(text);
    if (!match) return null;
    const vm = match[0];
    const local = /local\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*vitmap\s*\[/.exec(text);
    return { vm, name: local ? local[1] : null };
}
