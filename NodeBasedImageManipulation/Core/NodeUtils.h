#pragma once
#include <string>
#include <GLFW/glfw3.h>
#include "imgui.h"

template<typename T>
T Clamp(T val, T lo, T hi)
{
    if (val < lo || val > hi)
        val = abs(val - lo) <= abs(val - hi) ? lo : hi;
    return val;
}

std::string OpenFileDialog();
std::string SaveFileDialog(const char* defaultExt = "png");
void HelpMarker(const char* desc);
void UploadTextureToOpenGL(int width, int height, GLuint texture, unsigned char*& imageData, bool update = false);


