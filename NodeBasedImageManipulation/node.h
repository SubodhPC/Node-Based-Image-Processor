#pragma once

#include <vector>
#include <unordered_set>
#include <string>
#include "imnodes.h"
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

using namespace std;

class ImageBuffer 
{
public:
	int width = 0, height = 0;
	GLuint texture = 0;
	unsigned char* imageData = nullptr;

	void ShowImage();

	ImageBuffer() {};
	ImageBuffer(const ImageBuffer&) = delete;
	ImageBuffer& operator=(const ImageBuffer&) = delete;
	~ImageBuffer();
};

enum class ChannelType
{
	Input, Output
};
enum class ChannelDataType
{
	Image
};

struct Channel {
	int id;
	std::string name;
	ChannelType type;
	ChannelDataType dataType;
	unordered_set<void*> attachedLinks;
	void* data;  // pointer to actual data

	Channel(int id, string channelname, ChannelType channelType, ChannelDataType channelDataType) 
		: id(id), name(channelname), type(channelType), dataType(channelDataType) {
		data = nullptr;
	};
	
	~Channel()
	{
		if (data)
		{
			if (type == ChannelType::Output && dataType == ChannelDataType::Image)
				delete (ImageBuffer*)data;
			data = nullptr;
		}
	}
};

enum class NodeType
{
	Input, 
	Output,
	BrightnessContrast,
	ColourSplitter,
	Blur
};

class Node
{
	bool dirty = false;
public: 
	unsigned int id = 0;
	NodeType type;
	vector<Channel*> inputs;
	vector<Channel*> outputs;

	virtual ~Node();
	virtual string GetName() = 0;
	virtual void CreateImNode() = 0;
	virtual void CreateImNodeProperties() = 0;
	virtual bool Evaluate() = 0;
	virtual ImageBuffer* GetImageBuffer() = 0;

	void MarkDirty();
	void MarkClean() { dirty = false; }
	bool IsDirty() { return dirty; }
};

class InputNode : public Node
{
	string filePath = "";
	string fileExt = "nil";
	ImageBuffer* data = nullptr;
public:
	InputNode(int id);
	void CreateImNode() override;
	void CreateImNodeProperties() override;
	bool Evaluate() override;
	string GetName() override { return "Input"; }
	ImageBuffer* GetImageBuffer() override;
};

class OutputNode : public Node
{
	string saveFilePath = "";
	string saveFileExt = "";
	const char* availableExt[3] = { ".png", ".jpg", ".bmp" };
	int selectedExt = 0;
public:
	OutputNode(int id);
	void CreateImNode() override;
	void CreateImNodeProperties() override;
	bool Evaluate() override;
	string GetName() override { return "Output"; }
	ImageBuffer* GetImageBuffer() override;
};

class BrightnessContrastNode : public Node
{
	float brightness = 0.0;
	float contrast = 1.0;
public:
	BrightnessContrastNode(int id);
	void CreateImNode() override;
	void CreateImNodeProperties() override;
	bool Evaluate() override;
	string GetName() override { return "Brightness & Contrast"; }
	ImageBuffer* GetImageBuffer() override;
};

class ColorChannelSplitterNode : public Node
{
	bool greyFlags[4]{ false, false, false, false };
	string greyFlagName = "GreyScale";
	string greyFlagNames[4]{ "R Greyscale", "G Greyscale", "B Greyscale", "A Greyscale" };
public:
	ColorChannelSplitterNode(int id);
	void CreateImNode() override;
	void CreateImNodeProperties() override;
	bool Evaluate() override;
	string GetName() override { return "Color Splitter"; }
	ImageBuffer* GetImageBuffer() override;
};

class BlurNode : public Node
{
	enum class BlurDirection { Uniform, Horizontal, Vertical };
	BlurDirection direction = BlurDirection::Uniform;
	int blurRadius = 0;
public:
	BlurNode(int id);
	void CreateImNode() override;
	void CreateImNodeProperties() override;
	bool Evaluate() override;
	string GetName() override { return "Blur"; }
	ImageBuffer* GetImageBuffer() override;
private:
	vector<float> GenerateGaussianKernel(int radius);
	void ApplyGaussianBlur(const unsigned char* input, unsigned char* output, int width, int height, bool horizontal);
};

class ThresholdNode : public Node
{
	enum class ThresholdMethod { Binary, Adaptive, Otsu	};
	ThresholdMethod thresholdMethod = ThresholdMethod::Binary;
	int thresholdValue = 128;
public:
	ThresholdNode(int id);
	void CreateImNode() override;
	void CreateImNodeProperties() override;
	bool Evaluate() override;
	string GetName() override { return "Blur"; }
	ImageBuffer* GetImageBuffer() override;
private:
	void ComputeHistogram(const unsigned char* imgData, int width, int height, float* histogram, float& maxValue);
	int ComputeOtsuThreshold(const unsigned char* hist, int width, int height);
};


