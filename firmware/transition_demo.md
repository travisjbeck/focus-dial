# Timer Arduino - Screen Transition System (Fade Only)

## Summary

Simplified screen transition system that uses only fade transitions for a consistent, smooth user experience.

### Key Features

1. **Multiple Screen Management**
   - Three separate screen objects
   - Better performance for real-time content
   - Lower memory overhead

2. **Single Transition Type**
   - Fade transition only (500ms duration)
   - Smooth, consistent experience
   - No jarring movements

3. **Touch Support**
   - Tap left third: Previous screen
   - Tap right third: Next screen
   - Tap center: Next screen

4. **Visual Indicators**
   - Page dots at bottom of each screen
   - Shows current screen position

5. **Performance Optimizations**
   - Pre-created screens to avoid runtime allocation
   - Transition locking to prevent overlapping animations
   - Memory fallback when PSRAM not available

### Usage

Navigation is done through tap gestures only. You can also programmatically trigger transitions:

```cpp
// Navigate to next screen with fade
go_to_next_screen();

// Navigate to previous screen with fade
go_to_prev_screen();

// Custom transition to specific screen
transition_to_screen(screen2, 500);
```

### Next Steps

To enable PSRAM support (for better performance):
1. In Arduino IDE: Tools → PSRAM → "Enabled"
2. Or if using arduino-cli, add to board options

The current implementation uses regular memory as fallback, which works fine for the demo but PSRAM would allow larger buffers for smoother animations.