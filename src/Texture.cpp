#include "Texture.hpp"
#include "DXSampleHelper.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture2DPtr TextureManager::CreateTexture2D( const std::wstring& filename )
{
	int width = 0, height = 0, channels = 4;
	// stbi_set_flip_vertically_on_load( true );

	const auto filenameStr = WstrToStr( filename );

	unsigned char* bitmap = stbi_load( filenameStr.c_str(), &width, &height, &channels, STBI_rgb_alpha );

	Texture2DPtr spTexture = std::make_shared< Texture2D >();
	spTexture->width = width;
	spTexture->height = height;
	spTexture->pixelSize = 4;
	spTexture->data = bitmap;
	return spTexture;
}

Texture2D::Texture2D() : 
	width( 0 ), 
	height( 0 ), 
	pixelSize( 0 ),
	data( nullptr )
{
}

Texture2D::~Texture2D()
{
	if ( data )
	{
		stbi_image_free( data );
	}
}