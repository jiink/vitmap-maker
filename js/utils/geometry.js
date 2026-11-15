export function pointInPoly(pt, vs) {
    const { x, y } = pt;
    let inside = false;
    for (let i = 0, j = vs.length - 1; i < vs.length; j = i++) {
        const xi = vs[i].x, yi = vs[i].y, xj = vs[j].x, yj = vs[j].y;
        const intersect = ((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi);
        if (intersect) inside = !inside;
    }
    return inside;
}

export function shapeHitTest(pt, shape, zoom) {
    if (!shape.open) return pointInPoly(pt, shape.points);
    const pts = shape.points;
    const threshold = Math.max(4 / zoom, (Number(shape.borderWidth) || 0) / 2);
    for (let i = 1; i < pts.length; i++) {
        const a = pts[i - 1], b = pts[i];
        const dx = b.x - a.x, dy = b.y - a.y;
        const lengthSquared = dx * dx + dy * dy;
        const t = lengthSquared === 0 ? 0 : Math.max(0, Math.min(1, ((pt.x - a.x) * dx + (pt.y - a.y) * dy) / lengthSquared));
        if (Math.hypot(pt.x - (a.x + t * dx), pt.y - (a.y + t * dy)) <= threshold) return true;
    }
    return false;
}
