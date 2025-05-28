#pragma once
#include <Windows.h>
#include <algorithm>
#include "node.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include "Link.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void UploadTextureToOpenGL(const int width, const int height, const GLuint texture, unsigned char *& imageData, const bool update = false)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    if (update)
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromMemory(const void* data, size_t data_size, ImageBuffer*& buffer)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

    buffer->texture = image_texture;
    buffer->width = image_width;
    buffer->height = image_height;
    buffer->imageData = image_data;

    //stbi_image_free(image_data);
    return true;
}

// Open and read a file, then forward to LoadTextureFromMemory()
bool LoadTextureFromFile(const char* file_name, ImageBuffer*& buffer)
{
    FILE* f = nullptr;
    auto err = fopen_s(&f, file_name, "rb");
    if (f == NULL)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == -1)
        return false;
    fseek(f, 0, SEEK_SET);
    void* file_data = IM_ALLOC(file_size);
    fread(file_data, 1, file_size, f);
    fclose(f);
    bool ret = LoadTextureFromMemory(file_data, file_size, buffer);
    IM_FREE(file_data);
    return ret;
}

template<typename T>
T Clamp(T val, T lo, T hi)
{
    if (val < lo || val > hi)
        val = abs(val - lo) <= abs(val - hi) ? lo : hi;
    return val;
}

std::string OpenFileDialog() {
    OPENFILENAMEA ofn;
    CHAR szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // or your main window handle
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(ofn.lpstrFile);
    }
    return "";
}

std::string SaveFileDialog(const char* defaultExt = "png") {
    char szFile[MAX_PATH] = { 0 };

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "PNG Files\0*.png\0JPEG Files\0*.jpg;*.jpeg\0Bitmap Files\0*.bmp";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrTitle = "Save Image As";

    if (GetSaveFileNameA(&ofn)) {
        return std::string(ofn.lpstrFile);
    }

    return "";
}

ImageBuffer* CreateBuffer(const string& path)
{
    ImageBuffer* buffer = new ImageBuffer();
    bool result = LoadTextureFromFile(&path[0], buffer);
    if (!result)
        return nullptr;

    return buffer;
}

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
        stbi_image_free(imageData);
        imageData = nullptr;
    }
    if (texture) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
}

Node::~Node()
{
    for (Channel* ch : inputs)
    {
        delete ch;
    }
    for (Channel* ch : outputs)
    {
        delete ch;
    }
}

void Node::MarkDirty()
{
    if (!dirty) {
        dirty = true;
        // Propagate dirtiness to downstream connected nodes
        for (Channel* output : outputs) {
            for (void* link : output->attachedLinks) {  // Assuming Socket tracks its own outgoing links
                Link* l = (Link*)link;
                l->to_node->MarkDirty();
            }
        }
    }
}

InputNode::InputNode(int id)
{
    this->id = id;
    outputs.push_back(new Channel(id + 1, "Image", ChannelType::Output, ChannelDataType::Image));
};

static int InputTextCallback(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        std::string* str = (std::string*)data->UserData;
        str->resize(data->BufTextLen);
        data->Buf = &(*str)[0];
    }
    return 0;
}

void InputNode::CreateImNode()
{
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(GetName().c_str());
    ImNodes::EndNodeTitleBar();

    int width = 0, height = 0;
    if (outputs[0]->data)
    {
        auto buffer = (ImageBuffer*)(outputs[0]->data);
        width = buffer->width;
        height= buffer->height;
    }

    ImGui::Text("File Extention = %s ", fileExt.c_str());
    ImGui::Text("Size = %d x %d", width, height);

    ImGui::SetNextItemWidth(100.0f);
    static ImGuiInputTextFlags flags = ImGuiInputTextFlags_ElideLeft | ImGuiInputTextFlags_CallbackResize;
    if (ImGui::InputTextWithHint("File Path", "Enter path here...", &filePath[0], filePath.size() + 1, flags,
        InputTextCallback, (void*)&filePath))
    {
        MarkDirty();
    }

    if (ImGui::Button("Open File"))
    {
        filePath = OpenFileDialog();
        MarkDirty();
    }

    for (Channel* c : outputs)
    {
        ImNodes::BeginOutputAttribute(c->id);
        ImGui::Indent(60);
        ImGui::Text(c->name.c_str());
        ImNodes::EndOutputAttribute();
    }

    ImNodes::EndNode();
}

void InputNode::CreateImNodeProperties()
{
    int width = 0, height = 0;
    if (outputs[0]->data)
    {
        auto buffer = (ImageBuffer*)(outputs[0]->data);
        width = buffer->width;
        height = buffer->height;
    }
    ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("table3", 2, flags))
    {
        ImGui::TableNextColumn();
        ImGui::Text("File Path");
        ImGui::TableNextColumn();
        ImGui::Text(filePath.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("File Extention");
        ImGui::TableNextColumn();
        ImGui::Text(fileExt.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("Width");
        ImGui::TableNextColumn();
        ImGui::Text("%d", width);
        ImGui::TableNextColumn();
        ImGui::Text("Height");
        ImGui::TableNextColumn();
        ImGui::Text("%d", height);

        ImGui::EndTable();
    }
    ImGui::PushItemWidth(100);
    ImGui::PopItemWidth();
}

bool InputNode::Evaluate()
{
    if (!IsDirty()) return false;
    ImageBuffer* buffer = CreateBuffer(filePath);

    if (buffer == nullptr)
    {
        MarkClean();
        return false;
    }

    outputs[0]->data = (void*)buffer;
    fileExt = filePath.substr(filePath.find_last_of('.'));
    MarkClean();
    return true;
}

ImageBuffer* InputNode::GetImageBuffer()
{
    if (outputs[0]->data == nullptr)
        return nullptr;

    return static_cast<ImageBuffer*>(outputs[0]->data);
}

OutputNode::OutputNode(int id)
{
    this->id = id;
    inputs.push_back(new Channel(id + 1, "Image", ChannelType::Input, ChannelDataType::Image));
};

void OutputNode::CreateImNode()
{
    ImNodes::BeginNode(id);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(GetName().c_str());
    ImNodes::EndNodeTitleBar();

    for (Channel* c : inputs)
    {
        ImNodes::BeginInputAttribute(c->id);
        ImGui::Text(c->name.c_str());
        ImNodes::EndInputAttribute();
    }

    ImGui::Spacing();
    if (ImGui::Button("Save File"))
    {
        saveFilePath = SaveFileDialog();
        saveFileExt = saveFilePath.substr(saveFilePath.find_last_of('.'));
        MarkDirty();
        Evaluate();
    }
    ImNodes::EndNode();
}

void OutputNode::CreateImNodeProperties()
{
    int width = 0, height = 0;
    if (outputs.size() && outputs[0]->data)
    {
        auto buffer = (ImageBuffer*)(outputs[0]->data);
        width = buffer->width;
        height = buffer->height;
    }
    ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("table3", 2, flags))
    {
        ImGui::TableNextColumn();
        ImGui::Text("Width");
        ImGui::TableNextColumn();
        ImGui::Text("%d", width);
        ImGui::TableNextColumn();
        ImGui::Text("Height");
        ImGui::TableNextColumn();
        ImGui::Text("%d", height);

        ImGui::EndTable();
    }
    ImGui::PushItemWidth(100);
    ImGui::PopItemWidth();
}

bool OutputNode::Evaluate()
{
    if (!IsDirty()) return false;
    if (saveFilePath == "")
        return false;
    if (saveFileExt == ".png")
        stbi_write_png(saveFilePath.c_str(), GetImageBuffer()->width, GetImageBuffer()->height, 4, GetImageBuffer()->imageData, GetImageBuffer()->width * 4);
    if (saveFileExt == ".jpg")
        stbi_write_jpg(saveFilePath.c_str(), GetImageBuffer()->width, GetImageBuffer()->height, 4, GetImageBuffer()->imageData, GetImageBuffer()->width * 4);
    if (saveFileExt == ".bmp")
        stbi_write_bmp(saveFilePath.c_str(), GetImageBuffer()->width, GetImageBuffer()->height, 4, GetImageBuffer()->imageData);

    MarkClean();
    return false;
}

ImageBuffer* OutputNode::GetImageBuffer()
{
    if (!inputs.size() || inputs[0]->data == nullptr)
        return nullptr;

    return static_cast<ImageBuffer*>(inputs[0]->data);
}

BrightnessContrastNode::BrightnessContrastNode(int id)
{
    this->id = id;
    inputs.push_back(new Channel(id + 1, "Image", ChannelType::Input, ChannelDataType::Image));
    outputs.push_back(new Channel(id + 2, "Image", ChannelType::Output, ChannelDataType::Image));
};

void BrightnessContrastNode::CreateImNode()
{
    ImNodes::BeginNode(id);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(GetName().c_str());
    ImNodes::EndNodeTitleBar();

    for (Channel* c : inputs)
    {
        ImNodes::BeginInputAttribute(c->id);
        ImGui::Text(c->name.c_str());
        ImNodes::EndInputAttribute();
    }

    HelpMarker("Click to reset the Brightness");

    ImGui::SameLine();
    ImGui::PushID("Reset Brightness");
    if (ImGui::Button(".."))
    {
        if (brightness != 0.0f)
        {
            brightness = 0.0f;
            MarkDirty();
        }
    }
    ImGui::PopID();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::SliderFloat("Brightness", &brightness, -100.0f, 100.0f, "%.3f"))
    {
        MarkDirty();
    }

    HelpMarker("Click to reset the Contrast");

    ImGui::SameLine();
    ImGui::PushID("Reset Contrast");
    if (ImGui::Button(".."))
    {
        if (contrast != 1.0f)
        {
            contrast = 1.0f;
            MarkDirty();
        }
    }
    ImGui::PopID();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::SliderFloat("Contrast", &contrast, 0.0f, 3.0f, "%.3f"))
    {
        MarkDirty();
    }

    for (Channel* c : outputs)
    {
        ImNodes::BeginOutputAttribute(c->id);
        ImGui::Indent(200);
        ImGui::Text(c->name.c_str());
        ImNodes::EndOutputAttribute();
    }

    ImNodes::EndNode();
}

void BrightnessContrastNode::CreateImNodeProperties()
{
    int width = 0, height = 0;
    if (outputs[0]->data)
    {
        auto buffer = (ImageBuffer*)(outputs[0]->data);
        width = buffer->width;
        height = buffer->height;
    }
    ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("table3", 2, flags))
    {
        ImGui::TableNextColumn();
        ImGui::Text("Brightness");
        ImGui::TableNextColumn();

        ImGui::PushItemWidth(0.0);
        ImGui::PushID("propBrightness");
        if (ImGui::SliderFloat("", &brightness, -100.0f, 100.0f, "%.3f"))
        {
            MarkDirty();
        }
        ImGui::PopID();
        ImGui::TableNextColumn();
        ImGui::Text("Contrast");
        ImGui::TableNextColumn();

        ImGui::PushItemWidth(0.0);
        ImGui::PushID("propContrast");
        if (ImGui::SliderFloat("", &contrast, 0.0f, 3.0f, "%.3f"))
        {
            MarkDirty();
        }
        ImGui::PopID();
        ImGui::TableNextColumn();
        ImGui::Text("Width");
        ImGui::TableNextColumn();
        ImGui::Text("%d", width);
        ImGui::TableNextColumn();
        ImGui::Text("Height");
        ImGui::TableNextColumn();
        ImGui::Text("%d", height);

        ImGui::EndTable();
    }
}


bool BrightnessContrastNode::Evaluate()
{
    if (!IsDirty()) 
        return false;

    ImageBuffer* buffer = (ImageBuffer*)inputs[0]->data;
    if (!buffer) 
    {
        Channel* outChannel = outputs[0];
        if (outChannel->data != nullptr)
        {
            if (outChannel->type == ChannelType::Output && outChannel->dataType == ChannelDataType::Image)
                delete (ImageBuffer*)outChannel->data;
            outChannel->data = nullptr;
        }
        MarkClean();
        return false;
    }

    Channel* outChannel = outputs[0];
    ImageBuffer* outbuffer = nullptr;
    bool isNewImageData = false;
    if (outChannel->data)
    {
        outbuffer = (ImageBuffer*)outChannel->data;

        if (outbuffer->width != buffer->width || outbuffer->height != buffer->height)
        {
            delete[] outbuffer->imageData;
            outbuffer->imageData = new unsigned char[buffer->width * buffer->height * 4];
            outbuffer->width = buffer->width;
            outbuffer->height = buffer->height;
        }
    }
    else
    {
        outbuffer = new ImageBuffer();
        outbuffer->width = buffer->width;
        outbuffer->height = buffer->height;
        outbuffer->imageData = new unsigned char[buffer->width * buffer->height * 4];

        glGenTextures(1, &outbuffer->texture);
        isNewImageData = true;
    }

    memcpy(outbuffer->imageData, buffer->imageData, buffer->width * buffer->height * 4);

    auto width = outbuffer->width;
    auto height = outbuffer->height;
    auto editedData = outbuffer->imageData;

    int num_pixels = width * height;
    for (int i = 0; i < num_pixels; ++i)
    {
        int index = i * 4;  // 4 bytes per pixel (RGBA)
        for (int c = 0; c < 3; ++c) // R, G, B only
        {
            //int value = editedData[index + c] + (int)(brightness * 255.0f);
            /*if (value < 0 || value > 255)
                value = abs(value - 0) <= abs(value - 255) ? 0 : 255;*/
            //editedData[index + c] = (unsigned char)value;
            
            float color = (float)editedData[index + c];
            color += brightness * 2.55f; // 255/100
            color /= 255.0f;
            color = (color - 0.5f) * contrast + 0.5f;
            color *= 255.0f;

            auto color2 = Clamp(color, 0.0f, 255.0f);
            if (color < 0 || color > 255)
                color = abs(color - 0.0f) <= abs(color - 255.0f) ? 0.0f : 255.0f;

            assert(color == color2);
            editedData[index + c] = (unsigned char)color;
        }
    }

    if (isNewImageData)
        UploadTextureToOpenGL(width, height, outbuffer->texture, editedData, false);

    outChannel->data = outbuffer;

    MarkClean();
    return true;
}

ImageBuffer* BrightnessContrastNode::GetImageBuffer()
{
    if (outputs[0]->data == nullptr)
        return nullptr;

    return static_cast<ImageBuffer*>(outputs[0]->data);
}

ColorChannelSplitterNode::ColorChannelSplitterNode(int id)
{
    this->id = id;
    inputs.push_back(new Channel(id + 1, "Image", ChannelType::Input, ChannelDataType::Image));
    outputs.push_back(new Channel(id + 2, "Red", ChannelType::Output, ChannelDataType::Image));
    outputs.push_back(new Channel(id + 3, "Green", ChannelType::Output, ChannelDataType::Image));
    outputs.push_back(new Channel(id + 4, "Blue", ChannelType::Output, ChannelDataType::Image));
    outputs.push_back(new Channel(id + 5, "Alpha", ChannelType::Output, ChannelDataType::Image));
}

void ColorChannelSplitterNode::CreateImNode()
{
    ImNodes::BeginNode(id);

    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(GetName().c_str());
    ImNodes::EndNodeTitleBar();

    for (Channel* c : inputs)
    {
        ImNodes::BeginInputAttribute(c->id);
        ImGui::Text(c->name.c_str());
        ImNodes::EndInputAttribute();
    }

    for (size_t i = 0; i < outputs.size(); i++)
    {
        HelpMarker(
            "Tick the checkbox for greyscale \n"
            "representation of the channel.");

        ImGui::SameLine();
        ImGui::PushID(greyFlagNames[i].c_str());
        if (ImGui::Checkbox(greyFlagName.c_str(), &greyFlags[i]))
            MarkDirty();
        ImGui::PopID();

        ImGui::SameLine();
        ImNodes::BeginOutputAttribute(outputs[i]->id);
        ImGui::Indent(10);
        ImGui::Text(outputs[i]->name.c_str());
        ImNodes::EndOutputAttribute();
    }

    ImNodes::EndNode();
}

void ColorChannelSplitterNode::CreateImNodeProperties()
{
    int width = 0, height = 0;
    if (outputs[0]->data)
    {
        auto buffer = (ImageBuffer*)(outputs[0]->data);
        width = buffer->width;
        height = buffer->height;
    }
    ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("table3", 2, flags))
    {
        for (size_t i = 0; i < outputs.size(); i++)
        {
            ImGui::TableNextColumn();
            ImGui::Text(greyFlagNames[i].c_str());

            ImGui::TableNextColumn();
            ImGui::PushID(greyFlagNames[i].c_str());
            if (ImGui::Checkbox("", &greyFlags[i]))
                MarkDirty();
            ImGui::PopID();
        }
        ImGui::TableNextColumn();
        ImGui::Text("Width");
        ImGui::TableNextColumn();
        ImGui::Text("%d", width);
        ImGui::TableNextColumn();
        ImGui::Text("Height");
        ImGui::TableNextColumn();
        ImGui::Text("%d", height);

        ImGui::EndTable();
    }
}

bool ColorChannelSplitterNode::Evaluate()
{
    if (!IsDirty())
        return false;

    ImageBuffer* inputBuffer = (ImageBuffer*)inputs[0]->data;
    if (!inputBuffer)
    {
        for (Channel* outChannel : outputs)
        {
            if (outChannel->data != nullptr)
            {
                if (outChannel->dataType == ChannelDataType::Image)
                    delete (ImageBuffer*)outChannel->data;
                outChannel->data = nullptr;
            }
        }
        MarkClean();
        return true;
    }

    auto width = inputBuffer->width;
    auto height = inputBuffer->height;
    int num_pixels = width * height;
    unsigned char* srcData = inputBuffer->imageData;

    unsigned char* redImageData = new unsigned char[width * height * 4];
    unsigned char* greenImageData = new unsigned char[width * height * 4];
    unsigned char* blueImageData = new unsigned char[width * height * 4];
    unsigned char* alphaImageData = new unsigned char[width * height * 4];

    // Split channels
    for (int i = 0; i < num_pixels; i++)
    {
        int index = i * 4;

        unsigned char r = srcData[index + 0];
        unsigned char g = srcData[index + 1];
        unsigned char b = srcData[index + 2];
        unsigned char a = srcData[index + 3];

        // Red channel image (only red, keep alpha)
        redImageData[index + 0] = r;
        redImageData[index + 1] = greyFlags[0] ? r : 0;
        redImageData[index + 2] = greyFlags[0] ? r : 0;
        redImageData[index + 3] = a;

        // Green channel image (only green, keep alpha)
        greenImageData[index + 0] = greyFlags[1] ? g : 0;
        greenImageData[index + 1] = g;
        greenImageData[index + 2] = greyFlags[1] ? g : 0;
        greenImageData[index + 3] = a;

        // Blue channel image (only blue, keep alpha)
        blueImageData[index + 0] = greyFlags[2] ? b : 0;
        blueImageData[index + 1] = greyFlags[2] ? b : 0;
        blueImageData[index + 2] = b;
        blueImageData[index + 3] = a;

        // Blue channel image (only blue, keep alpha)
        alphaImageData[index + 0] = greyFlags[3] ? a :0;
        alphaImageData[index + 1] = greyFlags[3] ? a :0;
        alphaImageData[index + 2] = greyFlags[3] ? a :0;
        alphaImageData[index + 3] = greyFlags[2] ? a : 255;
    }

    vector<unsigned char*> splitData{ redImageData, greenImageData, blueImageData, alphaImageData };

    for (size_t i = 0; i < outputs.size(); i++)
    {
        bool newBuffer = false;
        ImageBuffer* outbuffer = static_cast<ImageBuffer*>(outputs[i]->data);
        if (!outbuffer)
        {
            outbuffer = new ImageBuffer();
            outbuffer->width = width;
            outbuffer->height = height;
            glGenTextures(1, &outbuffer->texture);
            outputs[i]->data = outbuffer;
            newBuffer = true;
        }
        else if (outbuffer->width != width || outbuffer->height != height)
        {
            delete[] outbuffer->imageData;
            outbuffer->width = width;
            outbuffer->height = height;
        }

        if (outbuffer->imageData)
            delete[] outbuffer->imageData;

        outbuffer->imageData = splitData[i];
        if (newBuffer)
            UploadTextureToOpenGL(width, height, outbuffer->texture, splitData[i], !newBuffer);
    }

    MarkClean();
    return true;
}

ImageBuffer* ColorChannelSplitterNode::GetImageBuffer()
{
    if (inputs[0]->data == nullptr)
        return nullptr;

    return static_cast<ImageBuffer*>(inputs[0]->data);
}

BlurNode::BlurNode(int id)
{
    this->id = id;
    inputs.push_back(new Channel(id + 1, "Image", ChannelType::Input, ChannelDataType::Image));
    outputs.push_back(new Channel(id + 2, "Blurred", ChannelType::Output, ChannelDataType::Image));
}

void BlurNode::CreateImNode()
{
    ImNodes::BeginNode(id);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(GetName().c_str());
    ImNodes::EndNodeTitleBar();

    for (Channel* c : inputs)
    {
        ImNodes::BeginInputAttribute(c->id);
        ImGui::Text(c->name.c_str());
        ImNodes::EndInputAttribute();
    }

    int currentMode = static_cast<int>(direction);
    const char* modes[] = { "Uniform", "Horizontal", "Vertical" };
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::Combo("Direction", &currentMode, modes, IM_ARRAYSIZE(modes)))
    {
        direction = static_cast<BlurDirection>(currentMode);
        MarkDirty();
    }

    HelpMarker("Click to reset the Blur Radius");

    ImGui::SameLine();
    ImGui::PushID("Reset Blur Radius");
    if (ImGui::Button(".."))
    {
        if (blurRadius != 0.0f)
        {
            blurRadius = 0.0f;
            MarkDirty();
        }
    }
    ImGui::PopID();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::SliderInt("Blur Radius", &blurRadius, 0, 20))
    {
        MarkDirty();
    }

    for (Channel* c : outputs)
    {
        ImNodes::BeginOutputAttribute(c->id);
        ImGui::Indent(150);
        ImGui::Text(c->name.c_str());
        ImNodes::EndOutputAttribute();
    }

    ImNodes::EndNode();
}

void BlurNode::CreateImNodeProperties()
{
    int width = 0, height = 0;
    if (outputs[0]->data)
    {
        auto buffer = (ImageBuffer*)(outputs[0]->data);
        width = buffer->width;
        height = buffer->height;
    }
    ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("table3", 2, flags))
    {
        ImGui::TableNextColumn();
        ImGui::Text("Blur Radius");
        ImGui::TableNextColumn();

        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::PushID("propBlurRadius");
        if (ImGui::SliderInt("", &blurRadius, 0, 20))
        {
            blurRadiusChanged = true;
            MarkDirty();
        }
        ImGui::PopID();
        ImGui::TableNextColumn();
        ImGui::Text("Width");
        ImGui::TableNextColumn();
        ImGui::Text("%d", width);
        ImGui::TableNextColumn();
        ImGui::Text("Height");
        ImGui::TableNextColumn();
        ImGui::Text("%d", height);

        ImGui::EndTable();
    }
}

bool BlurNode::Evaluate()
{
    if (!IsDirty())
        return false;

    ImageBuffer* inputBuffer = (ImageBuffer*)inputs[0]->data;
    if (!inputBuffer)
    {
        for (Channel* outChannel : outputs)
        {
            if (outChannel->data != nullptr)
            {
                if (outChannel->dataType == ChannelDataType::Image)
                    delete (ImageBuffer*)outChannel->data;
                outChannel->data = nullptr;
            }
        }
        MarkClean();
        return true;
    }

    int width = inputBuffer->width;
    int height = inputBuffer->height;

    bool newBuffer = false;
    ImageBuffer* outbuffer = static_cast<ImageBuffer*>(outputs[0]->data);
    if (!outbuffer)
    {
        outbuffer = new ImageBuffer();
        outbuffer->width = width;
        outbuffer->height = height;
        glGenTextures(1, &outbuffer->texture);
        outputs[0]->data = outbuffer;
        newBuffer = true;
    }
    else if (outbuffer->width != width || outbuffer->height != height)
    {
        delete[] outbuffer->imageData;
        outbuffer->width = width;
        outbuffer->height = height;
    }

    if (!outbuffer->imageData)
        outbuffer->imageData = new unsigned char[width * height * 4];

    unsigned char* blurImageData = outbuffer->imageData;
    if (blurRadius == 0)
    {
		memcpy(blurImageData, inputBuffer->imageData, width * height * 4);
    }
    else if (direction == BlurDirection::Uniform) {
        vector<unsigned char> tempVec(width * height * 4);
        unsigned char* temp = tempVec.data();
        ApplyGaussianBlur(inputBuffer->imageData, temp, width, height, true);  // H
        ApplyGaussianBlur(temp, blurImageData, width, height, false); // V
    }
    else 
    {
        bool horiz = direction == BlurDirection::Horizontal;
        ApplyGaussianBlur(inputBuffer->imageData, blurImageData, width, height, horiz);  // H
    }

    if (newBuffer)
        UploadTextureToOpenGL(width, height, outbuffer->texture, blurImageData, !newBuffer);

    MarkClean();
    return true;
}

ImageBuffer* BlurNode::GetImageBuffer()
{
    if (outputs[0]->data)
        return static_cast<ImageBuffer*>(outputs[0]->data);
    return nullptr;
}

vector<float> BlurNode::GenerateGaussianKernel(int radius)
{
    std::vector<float> kernel(2 * radius + 1);
    float sigma = radius / 2.0f;
    float sum = 0.0f;

    for (int i = -radius; i <= radius; ++i)
    {
        float value = expf(-(i * i) / (2.0f * sigma * sigma));
        kernel[i + radius] = value;
        sum += value;
    }

    // Normalize the kernel
    for (float& val : kernel)
        val /= sum;

    return kernel;
}

void BlurNode::ApplyGaussianBlur(
    const unsigned char* input,
    unsigned char* output,
    int width,
    int height,
    bool horizontal)
{
    if (blurRadiusChanged) 
    {
        gaussianKernel = GenerateGaussianKernel(blurRadius);
		blurRadiusChanged = false;
    }

    std::vector<float>& kernel = gaussianKernel;
    int kernelSize = static_cast<int>(kernel.size());

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float sum[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

            for (int i = -blurRadius; i <= blurRadius; ++i)
            {
                int sampleX = horizontal ? Clamp(x + i, 0, width - 1) : x;
                int sampleY = horizontal ? y : Clamp(y + i, 0, height - 1);

                int sampleIndex = (sampleY * width + sampleX) * 4;
                float weight = kernel[i + blurRadius];

                for (int c = 0; c < 4; ++c)
                    sum[c] += weight * input[sampleIndex + c];
            }

            int outIndex = (y * width + x) * 4;
            for (int c = 0; c < 4; ++c)
                output[outIndex + c] = static_cast<unsigned char>(Clamp(sum[c], 0.0f, 255.0f));
        }
    }
}

ThresholdNode::ThresholdNode(int id)
{
    this->id = id;
    inputs.push_back(new Channel(id + 1, "Image", ChannelType::Input, ChannelDataType::Image));
    outputs.push_back(new Channel(id + 2, "Image", ChannelType::Output, ChannelDataType::Image));
}

void ThresholdNode::CreateImNode()
{
    ImNodes::BeginNode(id);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(GetName().c_str());
    ImNodes::EndNodeTitleBar();

    for (Channel* c : inputs)
    {
        ImNodes::BeginInputAttribute(c->id);
        ImGui::Text(c->name.c_str());
        ImNodes::EndInputAttribute();
    }

    int currentMethod = static_cast<int>(thresholdMethod);
    const char* methods[] = { "Binary", "Adaptive", "Otsu" };
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::Combo("Method", &currentMethod, methods, IM_ARRAYSIZE(methods)))
    {
        thresholdMethod = static_cast<ThresholdMethod>(currentMethod);
        MarkDirty();
    }

    HelpMarker("Click to reset the Threshold");

    ImGui::SameLine();
    ImGui::PushID("Reset Threshold");
    if (ImGui::Button(".."))
    {
        thresholdValue = 128;
        if (thresholdValue != 128)
        {
            thresholdValue = 128;
            MarkDirty();
        }
    }
    ImGui::PopID();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::SliderInt("Threshold", &thresholdValue, 0, 256))
    {
        MarkDirty();
    }

    for (Channel* c : outputs)
    {
        ImNodes::BeginOutputAttribute(c->id);
        ImGui::Indent(150);
        ImGui::Text(c->name.c_str());
        ImNodes::EndOutputAttribute();
    }

    float histogram[256];
    float maxValue = 0;
    if (outputs[0]->data)
    {
        ImageBuffer* buffer = ((ImageBuffer*)outputs[0]->data);
        ComputeHistogram(buffer->imageData, buffer->width, buffer->height, histogram, maxValue);
    }
    ImGui::Text("Histogram");
    ImGui::PushID("Threshold Histogram");
    ImGui::SetNextItemWidth(200.0f);
    ImGui::PlotHistogram("", histogram, 256, 0, nullptr, 0.0f, maxValue, ImVec2(0, 80.0f));
    ImGui::PopID();
    ImNodes::EndNode();
}

void ThresholdNode::CreateImNodeProperties()
{
}

bool ThresholdNode::Evaluate()
{
    if (!IsDirty())
        return false;

    ImageBuffer* inputBuffer = (ImageBuffer*)inputs[0]->data;
    if (!inputBuffer)
    {
        for (Channel* outChannel : outputs)
        {
            if (outChannel->data != nullptr)
            {
                if (outChannel->dataType == ChannelDataType::Image)
                    delete (ImageBuffer*)outChannel->data;
                outChannel->data = nullptr;
            }
        }
        MarkClean();
        return true;
    }

    //auto width = inputBuffer->width;
    //auto height = inputBuffer->height;
    //int numPixels = width * height;
    //unsigned char* srcData = inputBuffer->imageData;
    //unsigned char* blurImageData = new unsigned char[numPixels * 4];

    //int threshold = thresholdValue;

    //if (thresholdMethod == ThresholdMethod::Otsu)
    //    threshold = ComputeOtsuThreshold(srcData, width, height);

    //for (int i = 0; i < numPixels; i++)
    //{
    //    int gray = (src[i * 4] + src[i * 4 + 1] + src[i * 4 + 2]) / 3;
    //    int bin = 0;

    //    if (thresholdMethod == ThresholdMethod::Binary || thresholdMethod == ThresholdMethod::Otsu)
    //    {
    //        bin = (gray > threshold) ? 255 : 0;
    //    }
    //    else if (thresholdMethod == ThresholdMethod::Adaptive)
    //    {
    //        bin = ApplyAdaptiveThreshold(src, i, width, height);
    //    }

    //    dst[i * 4 + 0] = dst[i * 4 + 1] = dst[i * 4 + 2] = bin;
    //    dst[i * 4 + 3] = src[i * 4 + 3]; // keep alpha
    //}

    //ImageBuffer* output = new ImageBuffer();
    //output->width = width;
    //output->height = height;
    //output->imageData = dst;
    //glGenTextures(1, &output->texture);
    //UploadTextureToOpenGL(width, height, output->texture, dst);

    //outputs[0]->data = output;
    //MarkClean();
    return true;
}

ImageBuffer* ThresholdNode::GetImageBuffer()
{
    return nullptr;
}

void ThresholdNode::ComputeHistogram(const unsigned char* imgData, int width, int height, float* histogram, float& maxValue)
{
    maxValue = 0;
    std::fill(histogram, histogram + 256, 0);
    for (int i = 0; i < width * height * 4; i += 4)
    {
        // Assuming grayscale from red channel
        unsigned char gray = imgData[i];
        histogram[gray]++;
        if (maxValue < histogram[gray]) maxValue = histogram[gray];
    }
}

int ThresholdNode::ComputeOtsuThreshold(const unsigned char* data, int width, int height)
{
    int histogram[256] = { 0 };
    int total = width * height;

    for (int i = 0; i < total; i++)
    {
        int gray = (data[i * 4] + data[i * 4 + 1] + data[i * 4 + 2]) / 3;
        histogram[gray]++;
    }

    float sum = 0;
    for (int t = 0; t < 256; t++) sum += t * histogram[t];

    float sumB = 0, wB = 0, wF = 0;
    float maxVar = 0;
    int threshold = 0;

    for (int t = 0; t < 256; t++)
    {
        wB += histogram[t];
        if (wB == 0) continue;
        wF = total - wB;
        if (wF == 0) break;

        sumB += t * histogram[t];

        float mB = sumB / wB;
        float mF = (sum - sumB) / wF;

        float varBetween = wB * wF * (mB - mF) * (mB - mF);
        if (varBetween > maxVar)
        {
            maxVar = varBetween;
            threshold = t;
        }
    }

    return threshold;
}