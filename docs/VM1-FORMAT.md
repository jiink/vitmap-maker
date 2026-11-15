# VM1 vitmap string format

The editor's **Copy for script** button produces a line like:

```lua
local ship = vitmap[[VM1:AAEIAgRJZGxlAQECAAoUHgMQAADw8AAH/wAAgDIAAQIDAgECBAQDUnVuAAIAAA==]]
```

**Import string** accepts either that whole line or just the `VM1:...` part. Importing
replaces the current vitmap. If you paste the whole line, the Lua variable name becomes
the vitmap's name. Because the editor can read the string back in, the line in your
script is the only copy you need to keep.

- Encoder/decoder: [`js/io/vm1.js`](../js/io/vm1.js)
- Reference Rust decoder (no dependencies, includes tests): [`vm1_decoder.rs`](vm1_decoder.rs)

## Envelope

`VM1:` + base64 (standard alphabet, with padding) of the binary payload.
Decoders should also accept the URL-safe alphabet (`-` and `_`), missing padding and
embedded whitespace.

The `VM<n>:` prefix is the format version. A decoder must reject any version it does not
know. Any change to the layout gets a new number (`VM2:`), so scripts already using
`VM1:` keep working.

## Payload

All multi-byte integers are little-endian. `varint` is unsigned LEB128 (7 bits per
byte, high bit = more bytes follow, at most 5 bytes). Strings are a `varint` byte length
followed by that many bytes of UTF-8.

```
header
  u8      flags        bit0: coordinates are i16 (else i8). Other bits must be 0.
  u8      scaleShift   coordinate = raw / 2^scaleShift. The encoder writes 0..8.
  u8      fps          playback rate, 1..255
  varint  actionCount

action  (actionCount times)
  string  name
  u8      flags        bit0: loop. Other bits must be 0.
  varint  frameCount

frame  (frameCount times)
  varint  shapeCount

shape  (shapeCount times, back to front: later shapes draw on top)
  u8      flags        bit0: open, bit1: has alpha, bit2: has border. Other bits must be 0.
  u8 r, u8 g, u8 b     fill color
  [u8 a]               only if "has alpha"; otherwise a = 255
  [u16 borderWidth]    only if "has border": width in 1/100 units
  [u8 r, u8 g, u8 b]   only if "has border": border color (opaque)
  varint  pointCount
  pointCount × (x, y)  each i8 or i16, depending on the header flag
```

If a shape has no border, its border width is 0. The border color then doesn't matter;
decoders use white.

### Shape semantics

- **Closed** (`open = 0`): a filled polygon, plus a stroked outline if it has a border.
- **Open** (`open = 1`): a polyline with no closing edge and no fill. Only the border is
  drawn.
- Border alpha follows the editor, which always draws borders fully opaque.
- Coordinates are world units with the origin at the sprite center. The editor's
  "Scale Ref" box covers -8..8 on both axes.

## Fixed-point coordinates

The editor stores coordinates as floats. Snapping gives whole numbers, but circles,
45° rotations and drawing with snap off all produce fractions. The encoder:

1. Picks the smallest `scaleShift` from 0 to 8 at which every coordinate is exactly
   `int / 2^shift` and still fits in i16. Whole-number art gets shift 0. Half-unit
   art gets shift 1, and so on.
2. If no shift is exact (for example, rotated or circle points), it uses shift 4, which
   is 1/16 unit and adds at most 1/32 unit of error. The editor says so when this
   happens. Re-importing gives the rounded points.
3. Uses i8 if every scaled value fits in -128..127, and i16 otherwise.

## Size

The editor pretty-prints its JSON. A shape that is opaque and has no border costs
`1 + 3 + 1 + 2·points` bytes (`4·points` with i16 coordinates). Base64 adds a third
on top. For typical art the string is 10× or more smaller than the pretty-printed
JSON. The toast after copying shows the line length, and warns past 16 KiB.

## What is not stored

These live in the editor or in JSON files, but not in VM1: the project name (taken from
the Lua variable on import), border color on shapes without a border, and editor
settings (grid, overlay, background).
