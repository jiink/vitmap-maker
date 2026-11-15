//! Reference decoder for VM1 vitmap strings (see VM1-FORMAT.md).
//! Dependency-free; drop into your crate and adapt the types as needed.

#[derive(Debug, Clone, PartialEq)]
pub struct Color { pub r: u8, pub g: u8, pub b: u8, pub a: u8 }

#[derive(Debug, Clone, PartialEq)]
pub struct Shape {
    pub points: Vec<(f32, f32)>,
    pub color: Color,
    /// 0.0 means no border.
    pub border_width: f32,
    /// Alpha is always 255.
    pub border_color: Color,
    /// Open shapes are a polyline: stroke only, no fill, no closing edge.
    pub open: bool,
}

#[derive(Debug, Clone, PartialEq)]
pub struct Frame { pub shapes: Vec<Shape> }

#[derive(Debug, Clone, PartialEq)]
pub struct Action { pub name: String, pub looping: bool, pub frames: Vec<Frame> }

#[derive(Debug, Clone, PartialEq)]
pub struct Vitmap { pub fps: u8, pub actions: Vec<Action> }

#[derive(Debug, Clone, PartialEq)]
pub enum DecodeError {
    MissingPrefix,
    UnsupportedVersion(String),
    BadBase64,
    Truncated,
    BadVarint,
    BadUtf8,
    UnknownFlags,
    BadScale,
    TrailingBytes,
}

const FLAG_WIDE: u8 = 0x01;
const ACTION_LOOP: u8 = 0x01;
const SHAPE_OPEN: u8 = 0x01;
const SHAPE_ALPHA: u8 = 0x02;
const SHAPE_BORDER: u8 = 0x04;

/// Decode a string like `VM1:AAEI...`. Leading/trailing whitespace is ignored.
pub fn decode_vitmap(text: &str) -> Result<Vitmap, DecodeError> {
    let text = text.trim();
    let colon = text.find(':').ok_or(DecodeError::MissingPrefix)?;
    let (prefix, body) = (&text[..colon], &text[colon + 1..]);
    if !prefix.starts_with("VM") { return Err(DecodeError::MissingPrefix); }
    if prefix != "VM1" { return Err(DecodeError::UnsupportedVersion(prefix.to_string())); }

    let bytes = base64_decode(body)?;
    let mut r = Reader { b: &bytes, i: 0 };

    let flags = r.u8()?;
    if flags & !FLAG_WIDE != 0 { return Err(DecodeError::UnknownFlags); }
    let wide = flags & FLAG_WIDE != 0;
    let shift = r.u8()?;
    if shift > 15 { return Err(DecodeError::BadScale); }
    let scale = (1u32 << shift) as f32;
    let fps = match r.u8()? { 0 => 12, n => n };

    let action_count = r.varint()?;
    let mut actions = Vec::new();
    for _ in 0..action_count {
        let name = r.string()?;
        let a_flags = r.u8()?;
        if a_flags & !ACTION_LOOP != 0 { return Err(DecodeError::UnknownFlags); }
        let frame_count = r.varint()?;
        let mut frames = Vec::new();
        for _ in 0..frame_count {
            let shape_count = r.varint()?;
            let mut shapes = Vec::new();
            for _ in 0..shape_count {
                let s_flags = r.u8()?;
                if s_flags & !(SHAPE_OPEN | SHAPE_ALPHA | SHAPE_BORDER) != 0 { return Err(DecodeError::UnknownFlags); }
                let (cr, cg, cb) = (r.u8()?, r.u8()?, r.u8()?);
                let a = if s_flags & SHAPE_ALPHA != 0 { r.u8()? } else { 255 };
                let (border_width, border_color) = if s_flags & SHAPE_BORDER != 0 {
                    let w = r.u16()? as f32 / 100.0;
                    (w, Color { r: r.u8()?, g: r.u8()?, b: r.u8()?, a: 255 })
                } else {
                    (0.0, Color { r: 255, g: 255, b: 255, a: 255 })
                };
                let point_count = r.varint()?;
                let mut points = Vec::new();
                for _ in 0..point_count {
                    let (x, y) = if wide { (r.i16()? as f32, r.i16()? as f32) } else { (r.i8()? as f32, r.i8()? as f32) };
                    points.push((x / scale, y / scale));
                }
                shapes.push(Shape {
                    points,
                    color: Color { r: cr, g: cg, b: cb, a },
                    border_width,
                    border_color,
                    open: s_flags & SHAPE_OPEN != 0,
                });
            }
            frames.push(Frame { shapes });
        }
        actions.push(Action { name, looping: a_flags & ACTION_LOOP != 0, frames });
    }
    if r.i != bytes.len() { return Err(DecodeError::TrailingBytes); }
    Ok(Vitmap { fps, actions })
}

struct Reader<'a> { b: &'a [u8], i: usize }

impl<'a> Reader<'a> {
    fn u8(&mut self) -> Result<u8, DecodeError> {
        let v = *self.b.get(self.i).ok_or(DecodeError::Truncated)?;
        self.i += 1;
        Ok(v)
    }
    fn u16(&mut self) -> Result<u16, DecodeError> { Ok(u16::from_le_bytes([self.u8()?, self.u8()?])) }
    fn i8(&mut self) -> Result<i8, DecodeError> { Ok(self.u8()? as i8) }
    fn i16(&mut self) -> Result<i16, DecodeError> { Ok(self.u16()? as i16) }
    fn varint(&mut self) -> Result<u32, DecodeError> {
        let mut result: u32 = 0;
        for n in 0..5 {
            let b = self.u8()?;
            result |= ((b & 0x7f) as u32).checked_shl(7 * n).ok_or(DecodeError::BadVarint)?;
            if b & 0x80 == 0 { return Ok(result); }
        }
        Err(DecodeError::BadVarint)
    }
    fn string(&mut self) -> Result<String, DecodeError> {
        let len = self.varint()? as usize;
        let end = self.i.checked_add(len).filter(|&e| e <= self.b.len()).ok_or(DecodeError::Truncated)?;
        let s = std::str::from_utf8(&self.b[self.i..end]).map_err(|_| DecodeError::BadUtf8)?;
        self.i = end;
        Ok(s.to_string())
    }
}

/// Standard or URL-safe alphabet, padding optional.
fn base64_decode(s: &str) -> Result<Vec<u8>, DecodeError> {
    fn val(c: u8) -> Option<u32> {
        match c {
            b'A'..=b'Z' => Some((c - b'A') as u32),
            b'a'..=b'z' => Some((c - b'a' + 26) as u32),
            b'0'..=b'9' => Some((c - b'0' + 52) as u32),
            b'+' | b'-' => Some(62),
            b'/' | b'_' => Some(63),
            _ => None,
        }
    }
    let data: Vec<u8> = s.bytes().filter(|c| !c.is_ascii_whitespace()).collect();
    let data = match data.iter().position(|&c| c == b'=') {
        Some(p) if data[p..].iter().all(|&c| c == b'=') => &data[..p],
        Some(_) => return Err(DecodeError::BadBase64),
        None => &data[..],
    };
    if data.len() % 4 == 1 { return Err(DecodeError::BadBase64); }
    let mut out = Vec::with_capacity(data.len() * 3 / 4);
    for chunk in data.chunks(4) {
        let mut acc: u32 = 0;
        for &c in chunk { acc = (acc << 6) | val(c).ok_or(DecodeError::BadBase64)?; }
        acc <<= 6 * (4 - chunk.len() as u32);
        let bytes = acc.to_be_bytes();
        out.extend_from_slice(&bytes[1..chunk.len()]);
    }
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;

    // Produced by the editor's encodeVM1 (see tests/vm1.test.mjs for the source data).
    const SAMPLE: &str = "VM1:AAEIAgRJZGxlAQECAAoUHgMQAADw8AAH/wAAgDIAAQIDAgECBAQDUnVuAAIAAA==";

    #[test]
    fn decodes_sample() {
        let v = decode_vitmap(SAMPLE).unwrap();
        assert_eq!(v.fps, 8);
        assert_eq!(v.actions.len(), 2);
        let idle = &v.actions[0];
        assert_eq!(idle.name, "Idle");
        assert!(idle.looping);
        let tri = &idle.frames[0].shapes[0];
        assert_eq!(tri.points, vec![(8.0, 0.0), (0.0, -8.0), (-8.0, 0.0)]);
        assert_eq!(tri.color, Color { r: 10, g: 20, b: 30, a: 255 });
        assert!(!tri.open);
        let line = &idle.frames[0].shapes[1];
        assert_eq!(line.points, vec![(0.5, 1.0), (2.0, 2.0)]);
        assert_eq!(line.color.a, 128);
        assert_eq!(line.border_width, 0.5);
        assert_eq!(line.border_color, Color { r: 1, g: 2, b: 3, a: 255 });
        assert!(line.open);
        let run = &v.actions[1];
        assert_eq!(run.name, "Run");
        assert!(!run.looping);
        assert_eq!(run.frames.len(), 2);
    }

    #[test]
    fn rejects_bad_input() {
        assert_eq!(decode_vitmap("VM2:AAAA"), Err(DecodeError::UnsupportedVersion("VM2".into())));
        assert_eq!(decode_vitmap(&SAMPLE[..20]), Err(DecodeError::Truncated));
        assert_eq!(decode_vitmap("hello"), Err(DecodeError::MissingPrefix));
    }
}
