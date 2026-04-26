# TFT Dashboard Layout System

## Overview
The dashboard layout system provides a centralized, grid-based approach to organizing the TFT screen (320x240 landscape mode) for the irrigation control system.

## Screen Layout (Landscape: 320x240)

```
┌─────────────────────────────────────────────────────────┐
│  STATUS BAR (320x28) - State name + Health status      │
├─────────────────────────────────────────────────────────┤
│  HEADER (320x32) - Screen title                         │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  VALUE BOXES ROW (2 boxes: 148x64 each)                │
│  ┌──────────────┐  ┌──────────────┐                   │
│  │   PH BOX     │  │   EC BOX     │                   │
│  │  (Cyan)      │  │  (Green)     │                   │
│  └──────────────┘  └──────────────┘                   │
│                                                         │
├─────────────────────────────────────────────────────────┤
│  MODE LINE - Auto mode + Date/Time                      │
├─────────────────────────────────────────────────────────┤
│  PARCEL LINE - Current parcel + Time remaining          │
├─────────────────────────────────────────────────────────┤
│  PROGRESS BAR (300x24) - Irrigation progress            │
├─────────────────────────────────────────────────────────┤
│  ACTION BUTTONS (3 buttons: START/STOP, MANUAL, SET)   │
└─────────────────────────────────────────────────────────┘
```

## Layout Constants

### Display Dimensions
- **Screen Width**: 320px (landscape)
- **Screen Height**: 240px (landscape)
- **Grid**: 12 columns × 8 rows

### Sections
| Section | Y Position | Height | Purpose |
|---------|------------|--------|---------|
| Status Bar | 0 | 28px | System state & health |
| Header | 28 | 32px | Screen title |
| Content Area | 60 | 140px | Main content |
| Footer | 200 | 40px | Action buttons |

### Value Boxes
| Property | Value | Description |
|----------|-------|-------------|
| Width | 148px | Each value box |
| Height | 64px | Box height |
| Gap | 10px | Between boxes |
| X1 | 10px | PH box position |
| X2 | 168px | EC box position |
| Y | 70px | From content top |

### Buttons
| Property | Value | Description |
|----------|-------|-------------|
| Min Width | 64px | Touch target minimum |
| Min Height | 36px | Touch target minimum |
| Action Btn Width | 100px | Main screen buttons |
| Action Btn Height | 44px | Main screen buttons |
| Gap | 10px | Between buttons |

### Spacing System
| Token | Value | Usage |
|-------|-------|-------|
| XS | 4px | Tight spacing |
| SM | 8px | Component internal |
| MD | 12px | Between elements |
| LG | 16px | Between sections |
| XL | 24px | Major separations |

## Color Palette

### Primary Colors
- **Primary**: Blue (`ILI9341_BLUE`)
- **Primary Dark**: Navy (`ILI9341_NAVY`)
- **Accent**: Cyan (`ILI9341_CYAN`)

### Status Colors
- **Success**: Green (`ILI9341_GREEN`)
- **Warning**: Yellow (`ILI9341_YELLOW`)
- **Error**: Red (`ILI9341_RED`)
- **Info**: Cyan (`ILI9341_CYAN`)

### Value Box Colors
- **pH**: Cyan
- **EC**: Green
- **Temperature**: Orange
- **Humidity**: Blue

### Neutral Colors
- **Background**: Black
- **Background Light**: Dark Gray
- **Text**: White
- **Text Dim**: Gray

## Grid System

The layout uses a 12-column grid for flexible positioning:

```c
// Calculate grid position
#define LAYOUT_GRID_X(col)  (MARGIN + col * (CELL_W + GAP))
#define LAYOUT_GRID_Y(row)  (MARGIN + row * (CELL_H + GAP))
#define LAYOUT_GRID_W(cols) (cols * CELL_W + (cols-1) * GAP)
#define LAYOUT_GRID_H(rows) (rows * CELL_H + (rows-1) * GAP)
```

### Utility Macros
```c
LAYOUT_CENTER_X(width)     // Center horizontally
LAYOUT_CENTER_Y(height)    // Center vertically
LAYOUT_RIGHT_X(width)      // Right-align
LAYOUT_BOTTOM_Y(height)    // Bottom-align
```

## Implementation Files

### `gui_layout.h`
Centralized layout configuration with all constants, dimensions, colors, and utility macros.

### `gui.c` (Refactored)
Main screen drawing functions using layout constants:
- `GUI_DrawMainScreen()` - Main dashboard
- `GUI_DrawStatusbar()` - Top status bar
- `GUI_DrawValueBox()` - pH/EC value displays
- `GUI_DrawMainModeLine()` - Mode & datetime
- `GUI_DrawMainParcelLine()` - Parcel progress
- `GUI_DrawMainProgress()` - Progress bar
- `GUI_DrawMainActions()` - Action buttons

## Benefits

1. **Consistency**: All screens use the same spacing and sizing
2. **Maintainability**: Changes in one place affect all screens
3. **Scalability**: Easy to add new screens with same layout
4. **Readability**: Named constants instead of magic numbers
5. **Flexibility**: Grid system allows adaptive layouts

## Adding New Screens

When adding a new screen, follow these guidelines:

1. Use layout constants for all positions and sizes
2. Maintain consistent header/footer structure
3. Use the spacing system (XS, SM, MD, LG, XL)
4. Follow the color palette for status indication
5. Ensure touch targets are at least 64x36px

### Example: Adding a Settings Screen

```c
void GUI_DrawSettingsScreen(void) {
  LCD_Clear(LAYOUT_COLOR_BG);
  GUI_DrawHeader("SETTINGS");
  GUI_DrawBackButton();
  
  // Use grid system for button placement
  for (uint8_t i = 0; i < SETTINGS_COUNT; i++) {
    uint8_t col = i % 2;
    uint8_t row = i / 2;
    uint16_t x = LAYOUT_GRID_X(col);
    uint16_t y = LAYOUT_CONTENT_Y + LAYOUT_GRID_Y(row);
    
    GUI_DrawButton(x, y, 
                   LAYOUT_SETTINGS_CELL_WIDTH,
                   LAYOUT_SETTINGS_CELL_HEIGHT,
                   settings_labels[i],
                   LAYOUT_COLOR_PRIMARY_DARK);
  }
}
```

## Touch Targets

Minimum touch target sizes:
- **Buttons**: 64×36px minimum
- **Action Buttons**: 100×44px (main screen)
- **Footer Buttons**: 145×28px
- **Settings Buttons**: Grid-calculated size

## Future Improvements

1. Add animation transitions between screens
2. Implement partial screen updates for better performance
3. Add support for additional font sizes
4. Create reusable component library
5. Add screen orientation support (portrait mode)

## Testing Checklist

- [x] Build succeeds without errors
- [x] All layout constants properly defined
- [x] Main screen uses new layout system
- [x] Value boxes correctly positioned
- [x] Buttons properly sized and spaced
- [x] Progress bar centered and sized correctly
- [ ] Test on actual hardware
- [ ] Verify touch targets accuracy
- [ ] Check readability and visual hierarchy
