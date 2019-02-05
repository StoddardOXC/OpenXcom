/*
 * An interface to the rendering infrastructure.
 * This should not be instanced.
 */


#ifndef OPENXCOM_RENDERER_H
#define OPENXCOM_RENDERER_H

#include <SDL.h>
#include <string>

/* overall concept:
 *
 * A Renderer has one or more drivers and one or more filters.
 * These are orthogonal, as in every driver supports every filter.
 *
 * If this is not possible, create another Renderer subclass.
 *
 */

namespace OpenXcom
{

class Renderer
{
public:
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
	/// Returns the output size, real pixels
	virtual void getOutputSize(int& w, int& h) const = 0;
	/// Sets the clear color
	virtual void setClearColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) = 0;
	/// Sets a filter, this should be possible without recreating the renderer.
	virtual void setFilter(const std::string& filter) = 0;

};
}
#endif
