# Vitmap Editor

A browser editor for vitmaps: animated vector sprites made of polygons.

## Running it

- **Online:** the GitHub Pages site for this repo.
- **Windows:** double-click `open.bat`.
- **macOS / Linux:** run `./open.sh` (needs `python3`).

The launchers start a temporary local server, open the editor in your browser, and shut
the server down once the page has loaded. The page keeps working after that. If you
reload it, you'll need to run the launcher again. A server is needed at all because
browsers won't load ES modules from `file://`.

## Getting art into a script

**Copy for script** copies a line like this to your clipboard:

```lua
local ship = vitmap[[VM1:AAEIAgRJZGxl...]]
```

To edit it later, click **Import string** and paste in either the whole line or just the
`VM1:...` part. Make your changes, then copy it again. The format is documented in
[docs/VM1-FORMAT.md](docs/VM1-FORMAT.md), and a reference Rust decoder is in
[docs/vm1_decoder.rs](docs/vm1_decoder.rs).

**Save JSON** and **Load JSON** still work. They are the lossless project format.

## Layout

```
index.html            markup only
css/style.css
js/main.js            entry point
js/models.js          Shape / Frame / Action
js/project.js         document state: actions, frames, loadData
js/editor/            editor state + tools (editor.js), drawing (render.js), selection ops (transform.js)
js/input.js           mouse, keyboard, drag-and-drop
js/ui/                panels (action/frame/layer lists), control bindings, toast
js/io/                JSON save/load, VM1 codec, copy/import UI
js/utils/             color + geometry helpers
tools/serve-once.ps1  one-shot server used by open.bat
tests/                node --test tests/
```
