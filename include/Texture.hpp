#pragma once
#include <vector>
#include <string>
#include <map>
#include <memory>

class Texture2D;
typedef std::shared_ptr< Texture2D > Texture2DPtr;

class TextureManager
{
public:
	static Texture2DPtr CreateTexture2D( const std::wstring& filename );

};

class Texture2D
{
public:
	Texture2D();
	~Texture2D();

	uint32_t width = 0;
	uint32_t height = 0;

	// The number of bytes used to represent a pixel in the texture.
	uint32_t pixelSize = 0;

	uint8_t* data;

};