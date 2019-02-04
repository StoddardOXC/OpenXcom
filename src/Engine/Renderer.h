/*
 * An interface to the rendering infrastructure.
 * This should not be instanced.
 */


#ifndef OPENXCOM_RENDERER_H
#define OPENXCOM_RENDERER_H

#include <SDL.h>
#include <string>

namespace OpenXcom
{

enum RendererType
{
	RENDERER_SDL2,
	RENDERER_OPENGL
};

struct RendererDriver
{
	std::string driverName;
	int driverID;
	std::string pixelFormatName;
	Uint32 pixelFormat;
};

struct RendererFilter
{
	std::string filterName;
	int filterID;
};

class Renderer
{
public:
	Renderer(void);
	virtual ~Renderer(void);
	/// ala glGetTexture()
	virtual unsigned getTexture() = 0;
	/// Sets the size of the expected SDL_Surface. blend: if blit it to the screen respecting transparency.
	virtual void setInternalRect(unsigned item, SDL_Rect *srcRect, bool blend) = 0;
	/// Sets the desired output rectangle.
	virtual void setOutputRect(unsigned item, SDL_Rect *dstRect) = 0;
	/// Sets the surface data
	virtual void updateTexture(unsigned item, SDL_Surface *surface) = 0;
	/// Blits the contents of RenderItems to the screen.
	virtual void flip() = 0;
	/// Saves a screenshot to filename.
	virtual void screenshot(const std::string &filename) = 0;
	/// Returns the renderer type.
	virtual RendererType getRendererType() const = 0;
	/// Returns the output size, real pixels
	virtual void getOutputSize(int& w, int& h) const = 0;
	/// Sets the clear color
	virtual void setClearColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) = 0;
	/// Gets the whatever drivers the renderer has (and pixformats: TBD if worth it)

};
}
#endif
