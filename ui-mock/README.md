# Muji Japanese Minimal Timer UI - 466x466 Round Display

A minimalist timer interface inspired by Japanese design philosophy and Muji's aesthetic principles. Features warm, natural tones and embraces Ma (negative space).

## Design Principles

### Japanese Minimalism
- **Ma (間)**: Generous negative space throughout
- **Kanso (簡素)**: Essential elements only - no decorative buttons
- **Wabi-Sabi (侘寂)**: Natural, warm imperfections over sterile perfection

### Interface Design
- **No buttons**: Pure touch interaction, no pause/stop/start controls
- **Single accent color**: Muted sage green (#c2e189) throughout
- **Typography-focused**: Light weights with natural spacing
- **Automatic theme switching**: Demonstrates both modes seamlessly

## Structure

- `index.html` - Four screen layouts with Japanese minimal aesthetic
- `css/styles.css` - Warm, natural color palette with AMOLED-optimized dark mode
- `theme_info.md` - Detailed explanation of Japanese design philosophy
- Each display is 466x466px with circular boundary

## Color Palette

### Light Mode - Natural Paper Tones
```css
var(--light-bg-primary)     /* #FAFAF8 - Warm off-white */
var(--light-bg-secondary)   /* #F5F4F0 - Subtle warm gray */
var(--light-text-primary)   /* #57544F - Deep charcoal */
var(--light-text-secondary) /* #A8A5A0 - Soft warm gray */
var(--light-accent)         /* #c2e189 - Muted sage green */
var(--light-border)         /* #E8E6E1 - Subtle warm border */
```

### Dark Mode - AMOLED Optimized
```css
var(--dark-bg-primary)      /* #000000 - Pure black */
var(--dark-bg-secondary)    /* #0A0A0A - Near black */
var(--dark-text-primary)    /* #F5F4F0 - Warm off-white */
var(--dark-text-secondary)  /* #B8B5B0 - Muted warm gray */
var(--dark-accent)          /* #c2e189 - Same sage green */
var(--dark-border)          /* #1A1A1A - Subtle border */
```

## Screen Designs

### 1. Idle Screen
- **Central time display** (14:32) with ultra-light typography
- **Subtle status message** "No active timers"
- **Maximum Ma (negative space)** for peaceful rest state

### 2. Project Selection  
- **Clean vertical list** with perfect rhythm
- **Active selection** highlighted with sage green accent
- **Natural spacing** between project names

### 3. Timer Running
- **Large countdown display** as hero element
- **Minimal progress ring** (single pixel stroke)
- **Small project context** label above

### 4. Duration Selection
- **Prominent time selection** display
- **Text-only preset buttons** in horizontal layout
- **Active state** shown through color and subtle underline

## Typography Hierarchy

### Weight-Based Hierarchy
- **Ultra-light (200)**: Large time displays for maximum readability
- **Light (300)**: Body text and supporting information
- **Regular (400)**: Active states and emphasis only

### Size Scale
- **72px-96px**: Primary time displays
- **16px-18px**: Comfortable body text
- **14px**: Supporting labels and context

## Implementation Features

- **No interactive buttons**: Embraces pure touch/rotary navigation
- **Automatic theme demonstration**: Switches every 5 seconds
- **Responsive scaling**: Maintains proportions across screen sizes
- **AMOLED optimization**: Pure black backgrounds in dark mode
- **Natural transitions**: Soft, organic animation timing

## Design Philosophy

See `theme_info.md` for detailed explanation of Japanese design principles including Ma (negative space), Wabi-Sabi (imperfect beauty), and Kanso (simplicity).

## File Structure

```
ui_samples_v3_2/
├── index.html          # Four screen implementations
├── css/
│   └── styles.css      # Japanese minimal theme styles
├── theme_info.md       # Design philosophy documentation
└── README.md           # This file
```