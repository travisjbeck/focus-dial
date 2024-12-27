#include "Animation.h"

Animation::Animation(Adafruit_SSD1306* display) : oled(display), animationRunning(false), playInReverse(false) {}

void Animation::start(const byte* frames, int frameCount, bool loop, bool reverse, unsigned long durationMs, int width, int height) {
    animationFrames = frames;
    totalFrames = frameCount;
    loopAnimation = loop;
    playInReverse = reverse; // Set reverse playback flag
    animationRunning = true;

    // Initialize current frame correctly based on direction
    currentFrame = playInReverse ? totalFrames - 1 : 0;

    frameWidth = width;
    frameHeight = height;
    frameDelay = DEFAULT_FRAME_DELAY;

    if (durationMs == 0) {
        animationDuration = totalFrames * frameDelay;
    } else {
        animationDuration = durationMs;
    }

    animationStartTime = millis();
    lastFrameTime = millis();

    frameX = (oled->width() - frameWidth) / 2;
    frameY = (oled->height() - frameHeight) / 2;

    oled->clearDisplay();
    oled->drawBitmap(frameX, frameY, &animationFrames[currentFrame * 288], frameWidth, frameHeight, 1);
    oled->display();
}

void Animation::update() {
    if (!animationRunning) return;

    unsigned long currentTime = millis();

    if (currentTime - animationStartTime >= animationDuration) {
        animationRunning = false;
        return;
    }

    // Check if it's time to advance to the next frame
    if (currentTime - lastFrameTime >= frameDelay) {
        lastFrameTime = currentTime;

        // Adjust current frame based on direction
        if (playInReverse) {
            currentFrame--;
            if (currentFrame < 0) { 
                if (loopAnimation) {
                    currentFrame = totalFrames - 1; // Wrap around to last frame
                } else {
                    animationRunning = false;
                    return;
                }
            }
        } else {
            currentFrame++;
            if (currentFrame >= totalFrames) {
                if (loopAnimation) {
                    currentFrame = 0; // Wrap around to first frame
                } else {
                    animationRunning = false;
                    return;
                }
            }
        }

        // Display the current frame
        oled->clearDisplay();
        oled->drawBitmap(frameX, frameY, &animationFrames[currentFrame * 288], frameWidth, frameHeight, 1);
        oled->display();
    }
}

bool Animation::isRunning() {
    return animationRunning;
}
