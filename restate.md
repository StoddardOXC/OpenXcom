## Redesign of rendering and states in the android-pc merge aka the SDL2 port.


All scaling, coordinate and pixel, is performed in Screen / Renderer.

All that the game sees is a logical screen. 320x200 in the simplest case,
size calculated according to scaling/video options in the case of Geo/Battlescape.

Always 8bit paletted. This allows 8bpp screenshots and shaders to receive 8bpp input.

Actions, i.e. mouse events are converted to this coordinate system before
being passed on to the States.

This is done to remove mouse coordinate scaling code from all over the codebase
and keep it in a single place.



## A State has a scaling mode, typically declared in the constructor.



The mode determines how the engine is to scale the pixels and transform the mouse event
coordinates to the logical coordinate system.

For example, `SC_ORIGINAL` declares a 320x200 logical screen, while `SC_GEOSCAPE` declares
something along the lines of 'give me a logical screen that fits the window size and video
options constraints'.

`SC_INFOSCREEN` declares a 320x200 logical screen that should be maximized
to fit the actual window according to the video settings (aspect ratio mostly) that are set.

`SC_INHERIT`, the default, just inherits the scaling mode from the ancestor `State`.

This simplifies each `State` code since it no longer has to consider scaling coefficients,
letterboxing, actual window size, the infoscreen maximize option, etc.



## A `State` has a nominal position and size `(_dx, _dy, _w, _h)`.


A `State` is a de-facto container of `Surface`s but mostly `InteractiveSurface`s
and descendants.

Coordinates of all that the `State`s' contain are relative to the `(_dx, _dy)`.

Width and height are used to position the State on a possibly bigger
logical screen - that was done by `centerAllSurfaces()` calls before.

This is done to remove recentering from all over the codebase and concentrate it
in a single place (`Game`'s resize event handler).

It also has a small benefit of explicitly defining a single place where to track
positioning of the state - and `Surface`s that belong to it relative
to the logical screen.



## Things to keep in mind while designing new `State`s:


 - `(_dx, _dy)` is where the game placed the `State` and do not touch it.
 - All `Surface`s including `InteractiveSurface`s (most of the UI) are placed relative
   to that. They keep a copy of the offset, and it's your job to propagate `setOffset()` calls.
 - Mouse `Action`s arrive with "absolute", that is, logical screen coordinates, so `(_dx,_dy)` need to be subtracted from that



## Work-in-progress:

<https://github.com/StoddardOXC/OpenXcom/tree/android-pc5>


### Known :
 - intro does not work
 - cutscenes do not work
 - inventory is broken
 - clicks on scanner are broken (medikit probably too)


### How to build:

`apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev zlib1g-dev`

`yaml-cpp` as usual.

Install <https://github.com/lxnt/SDL_pnglite>, it's a straightforward cmake build

The rest is same as a regular linux build.

Consider adding `-DEMBED_ASSETS=ON` to the `cmake` call to make sure assets are those that are expected.


Configuration is renamed to `sdl2options.cfg` to not overwrite existing one.


### Farm builds

Look for `SDL2` labeled builds at <https://lxnt.wtf/oxem/#/ExtendedTests>






