// Run with: node --test tests/
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { encodeVM1, decodeVM1, toLuaLine, extractVM, luaIdentifier } from '../js/io/vm1.js';

const shape = (points, extra = {}) => ({
    points: points.map(([x, y]) => ({ x, y })),
    color: { r: 10, g: 20, b: 30, a: 255 },
    borderWidth: 0,
    borderColor: { r: 255, g: 255, b: 255 },
    open: false,
    ...extra,
});

const sample = () => ({
    fps: 8,
    actions: [
        {
            name: 'Idle', loop: true, frames: [
                { shapes: [shape([[8, 0], [0, -8], [-8, 0]]), shape([[1, 1], [2, 2], [3, 1]], { open: true, borderWidth: 0.5, borderColor: { r: 1, g: 2, b: 3 } })] },
                { shapes: [] },
            ],
        },
        { name: 'Bøom ✨', loop: false, frames: [{ shapes: [shape([[0, 0], [1, 0], [0, 1]], { color: { r: 255, g: 0, b: 0, a: 128 } })] }] },
    ],
});

test('round-trips integer data exactly with i8 coords', () => {
    const data = sample();
    const enc = encodeVM1(data);
    assert.ok(enc.text.startsWith('VM1:'));
    assert.equal(enc.scaleShift, 0);
    assert.equal(enc.lossy, false);
    assert.deepEqual(decodeVM1(enc.text), data);
});

test('uses a fixed-point scale for fractional coordinates', () => {
    const data = sample();
    data.actions[0].frames[0].shapes[0].points[0] = { x: 0.5, y: -0.25 };
    const enc = encodeVM1(data);
    assert.equal(enc.scaleShift, 2);
    assert.equal(enc.lossy, false);
    assert.deepEqual(decodeVM1(enc.text), data);
});

test('switches to i16 for large coordinates', () => {
    const data = sample();
    data.actions[0].frames[0].shapes[0].points[0] = { x: 300, y: -1000 };
    const enc = encodeVM1(data);
    assert.deepEqual(decodeVM1(enc.text), data);
});

test('irrational coordinates are rounded to 1/16 and reported', () => {
    const data = sample();
    data.actions[0].frames[0].shapes[0].points[0] = { x: Math.SQRT2, y: Math.PI };
    const enc = encodeVM1(data);
    assert.equal(enc.lossy, true);
    assert.equal(enc.scaleShift, 4);
    assert.ok(enc.maxError <= 1 / 32 + 1e-9);
    const p = decodeVM1(enc.text).actions[0].frames[0].shapes[0].points[0];
    assert.equal(p.x, Math.round(Math.SQRT2 * 16) / 16);
});

test('is much smaller than the JSON', () => {
    const circle = Array.from({ length: 32 }, (_, i) => [Math.round(Math.cos(i) * 8), Math.round(Math.sin(i) * 8)]);
    const data = { fps: 12, actions: [{ name: 'Default', loop: true, frames: Array.from({ length: 6 }, () => ({ shapes: [shape(circle), shape(circle)] })) }] };
    const json = JSON.stringify({ meta: { version: '1.0', name: 'x' }, actions: data.actions }, null, 2);
    const enc = encodeVM1(data);
    assert.ok(json.length / enc.text.length > 10, `ratio ${json.length / enc.text.length}`);
});

test('extracts from a Lua line and rejects other versions', () => {
    const enc = encodeVM1(sample());
    const line = toLuaLine('My Ship', enc.text);
    assert.match(line, /^local my_ship = vitmap\[\[VM1:[A-Za-z0-9+/=]+\]\]$/);
    assert.deepEqual(extractVM(line), { vm: enc.text, name: 'my_ship' });
    assert.throws(() => decodeVM1('VM2:AAAA'), /Unsupported format VM2/);
    assert.throws(() => decodeVM1(enc.text.slice(0, 12)), /truncated/);
});

test('matches the test vector used by docs/vm1_decoder.rs', () => {
    const enc = encodeVM1({ fps: 8, actions: [
        { name: 'Idle', loop: true, frames: [{ shapes: [
            shape([[8, 0], [0, -8], [-8, 0]]),
            shape([[0.5, 1], [2, 2]], { color: { r: 255, g: 0, b: 0, a: 128 }, borderWidth: 0.5, borderColor: { r: 1, g: 2, b: 3 }, open: true }),
        ] }] },
        { name: 'Run', loop: false, frames: [{ shapes: [] }, { shapes: [] }] },
    ] });
    assert.equal(enc.text, 'VM1:AAEIAgRJZGxlAQECAAoUHgMQAADw8AAH/wAAgDIAAQIDAgECBAQDUnVuAAIAAA==');
});

test('lua identifiers are valid', () => {
    assert.equal(luaIdentifier('3 ships'), '_3_ships');
    assert.equal(luaIdentifier('end'), 'end_');
    assert.equal(luaIdentifier('!!!'), 'sprite');
});
