#include "ImageBuffer.h"
#include "NodeUtils.h"

void ImageBuffer::ShowImage()
{
    if (!texture)
        return;

    UploadTextureToOpenGL(width, height, texture, imageData, true);

    ImGui::Text("pointer = %x", texture);
    ImGui::Text("size = %d x %d", width, height);

    // Get content region size
    ImVec2 contentSize = ImGui::GetContentRegionAvail();

    // Original image size
    ImVec2 imageSize = ImVec2((float)width, (float)height);

    // Calculate scaling factor while preserving aspect ratio
    float scaleX = contentSize.x / imageSize.x;
    float scaleY = contentSize.y / imageSize.y;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    // Clamp to 1.0 if image is smaller than window
    if (scale > 1.0f) scale = 1.0f;

    // Final displayed size
    ImVec2 displaySize = ImVec2(imageSize.x * scale, imageSize.y * scale);

    // Draw the image
    ImGui::Image((ImTextureID)(intptr_t)texture, displaySize);
}

ImageBuffer::~ImageBuffer()
{
    if (imageData) {
        free(imageData);
        imageData = nullptr;
    }
    if (texture) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
}