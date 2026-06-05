#ifndef TOOLS_H
#define TOOLS_H

enum Tool {
    TOOL_PEN    = 0,
    TOOL_BRUSH  = 1,
    TOOL_ERASER = 2,
    TOOL_LINE   = 3,
    TOOL_RECT   = 4,
    TOOL_CIRCLE = 5,
    TOOL_FILL   = 6,
    TOOL_COUNT  = 7
};

extern const char *tool_names[];

struct ToolState {
    int  tool;
    int  brush_size;
    int  mouse_x, mouse_y;
    int  drawing;
    int  start_x, start_y;
    int  last_x, last_y;
};

#endif
