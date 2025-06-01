#pragma once

class ImageBuffer
{
public:
	int width = 0, height = 0;
	unsigned int texture = 0;
	unsigned char* imageData = nullptr;

	void ShowImage();

	ImageBuffer() {};
	ImageBuffer(const ImageBuffer&) = delete;
	ImageBuffer& operator=(const ImageBuffer&) = delete;
	~ImageBuffer();
};