// Core data structures. Everything here is plain data so it survives JSON round-trips.

export class Vec2 { constructor(x, y) { this.x = x; this.y = y; } }

export class Shape {
    constructor() {
        this.points = [];
        this.color = { r: 0, g: 170, b: 255, a: 255 };
        this.borderWidth = 0;
        this.borderColor = { r: 255, g: 255, b: 255 };
        this.open = false;
    }
}

export class Frame { constructor() { this.shapes = []; } }

export class Action {
    constructor(name) {
        this.name = name;
        this.frames = [new Frame()];
        this.loop = true;
    }
}
