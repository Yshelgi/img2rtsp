#include <iostream>

#include "Media.h"

int main() {
    auto media = new Media("rtsp://localhost:8554/live/0",25);
    media->open();
    media->push();
    return 0;
}
