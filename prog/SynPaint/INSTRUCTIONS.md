# SynPaint — Installation & Usage

## Dependencies
- SDL2 (`libsdl2`)
- Cairo (`cairo` / `libcairo2`)
- g++ with C++11 support

## Build
```bash
cd prog/SynPaint
make
```

Run:
```bash
./SynPaint
```

Clean:
```bash
make clean
```

## Controls

### Tools (left panel)
| Tool | Shortcut | Description |
|------|----------|-------------|
| Pen | click | 1px freehand line |
| Brush | click | Freehand with adjustable width |
| Eraser | click | Draws with BG color |
| Line | click+drag | Straight line |
| Rect | click+drag | Rectangle outline |
| Circle | click+drag | Circle from center |
| Fill | click | Flood fill area |

### Colors
- **Left click** palette swatch → set FG color
- **Right click** palette swatch → set BG color
- Click **FG/BG box** → open RGB color picker
- Click **swap arrow** ↔ between FG and BG

### Brush Size
Drag the **Size** slider to adjust (1–20).

### Actions
| Button | Hotkey | Description |
|--------|--------|-------------|
| New Canvas | Ctrl+N | Clear canvas (with undo) |
| Save PNG | Ctrl+S | Save as PNG (auto-named) |
| Undo | Ctrl+Z | Undo last stroke |
| About | | Version info |

## Notes
- Maximum 20 undo steps.
- Saved PNGs are written to the current directory.
- File format: `synpaint_YYYYMMDD_HHMMSS.png`
