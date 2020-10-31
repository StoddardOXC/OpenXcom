/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Based on http://www.libsdl.org/projects/flxplay/
 * See also https://github.com/aseprite/flic
 */
#ifdef _MSC_VER
# ifndef _SCL_SECURE_NO_WARNINGS
#  define _SCL_SECURE_NO_WARNINGS
# endif
#endif

#include "FlcPlayer.h"
#include <algorithm>
#include <cassert>
#include <string.h>
#include <SDL_mixer.h>
#include "FileMap.h"
#include "Logger.h"
#include "Screen.h"
#include "Surface.h"
#include "Options.h"
#include "Game.h"

namespace OpenXcom
{


// Taken from: http://www.compuphase.com/flic.htm
enum FileTypes
{
	FLI_TYPE = 0xAF11,
	FLC_TYPE = 0xAF12,
};


enum ChunkTypes
{
	COLOR_256 = 0x04,
	FLI_SS2 = 0x07, // or DELTA_FLC
	COLOR_64 = 0x0B,
	FLI_LC = 0x0C, // or DELTA_FLI
	BLACK = 0x0D,
	FLI_BRUN = 0x0F, // or BYTE_RUN
	FLI_COPY = 0x10,

	AUDIO_CHUNK = 0xAAAA, // This is the only exception, it's from TFTD
	PREFIX_CHUNK = 0xF100,
	FRAME_TYPE = 0xF1FA,
};

enum ChunkOpcodes
{
	PACKETS_COUNT = 0x0000, // 0000000000000000
	LAST_PIXEL = 0x8000, // 1000000000000000
	SKIP_LINES = 0xc000, // 1100000000000000
	MASK = SKIP_LINES
};

enum PlayingState
{
	PLAYING,
	FINISHED,
	SKIPPED
};

FlcPlayer::FlcPlayer() { }
FlcPlayer::~FlcPlayer()  { fini(); }

/**
 * Initialize data structures needed by the player
 * @param buf Video file data
 * @param game Pointer to the Game instance
 * @param mute whether to skip audio
 * @param fps what fps to play at. maybe.
 */
bool FlcPlayer::init(Game *game, SDL_RWops *buf, bool mute)
{
	_game = game;
	_rwops = buf;
	_fileSize = SDL_RWsize(_rwops);
	if (_fileSize < 128) {
		Log(LOG_ERROR) << "Flx file: no header.";
		return false;
	}
	_volume = Game::volumeExponent(Options::musicVolume);
	_mute = mute;

	_frameCount = 0;
	_audioFrameData = 0;
	_hasAudio = false;
	_audioData.loadingBuffer = 0;
	_audioData.playingBuffer = 0;

	// Let's read the first 128 bytes
	_headerFileSize = SDL_ReadLE32(_rwops);
	_headerType = SDL_ReadLE16(_rwops);     // 0xAF12 for FLC, 0xAF11 for .FLI
	_frameCount = SDL_ReadLE16(_rwops);
	_headerWidth = SDL_ReadLE16(_rwops);
	_headerHeight = SDL_ReadLE16(_rwops);
	_headerDepth = SDL_ReadLE16(_rwops);    // should be 0x0008
	/* _flags = */ SDL_ReadLE16(_rwops);    // should be 0x0003
	_headerSpeed = SDL_ReadLE32(_rwops);    // FLI: 1/70  FLC: 1/1000 sec
	/* _created = */ SDL_ReadLE32(_rwops);  // FLC only
	/* _creator = */ SDL_ReadLE32(_rwops);  // FLC only
	/* _updated = */ SDL_ReadLE32(_rwops);  // FLC only
	/* _updator = */ SDL_ReadLE32(_rwops);  // FLC only
	/* _aspectx = */ SDL_ReadLE16(_rwops);  // FLC only
	/* _aspecty = */ SDL_ReadLE16(_rwops);  // FLC only
	/* _reserved = */ SDL_RWseek(_rwops, 38, SEEK_CUR); // skip 38 bytes.
	Uint32 f1_offs =  SDL_ReadLE32(_rwops); // offset to frame 1; FLC only
	/* _oframe_2 = SDL_ReadLE32(_rwops); // offset to frame 2; FLC only */
	/* _reserved = SDL_RWseek(_rwops, 40, SEEK_CUR); // skip 40 bytes */

	// If it's a FLC or FLI file, it's ok
	if ((_headerType != FLI_TYPE) && (_headerType != FLC_TYPE)) {
		Log(LOG_ERROR) << "Flx file failed header check.";
		return false;
	}

	if (_headerType == FLC_TYPE) {
		SDL_RWseek(_rwops, f1_offs, SEEK_SET); // seek to the first frame
	} else {
		_headerSpeed = _headerSpeed * 14.285714285714286; // frame duration conversion from 1/70 sec to ms
	}

	_frameBuffer = SDL_CreateRGBSurfaceWithFormat(0, _headerWidth, _headerHeight, 8, SDL_PIXELFORMAT_INDEX8);
	if (!_frameBuffer) {
		Log(LOG_ERROR) << "FlcPlayer::init(): SDL_CreateRGBSurfaceWithFormat(): " << SDL_GetError();
		return false;
	}

	SDL_FillRect(_frameBuffer, 0, 0);

	Log(LOG_INFO) << "Playing flx, " << _headerWidth << "x" << _headerHeight << ", " << _headerFrames << " frames";

	return true;
}

void FlcPlayer::fini()
{
	if (_frameBuffer != 0) SDL_FreeSurface(_frameBuffer);
	if (_rwops != 0) SDL_RWclose(_rwops);
	_rwops = nullptr;
	_frameBuffer = nullptr;
}

bool FlcPlayer::readChunk() {
	Uint32 frameSize = SDL_ReadLE32(_rwops);
	Uint16 frameType = SDL_ReadLE16(_rwops);

	switch (frameType){
		case FRAME_TYPE:
		{
			Uint16 chunkCount = SDL_ReadLE16(_rwops);
			Uint16 delayOverride = SDL_ReadLE16(_rwops); // 0 = use header
			SDL_ReadLE16(_rwops); // should be 0
			Uint16 widthOverride = SDL_ReadLE16(_rwops);
			Uint16 heightOverride = SDL_ReadLE16(_rwops);
			// read the frame chunks
			for(Uint16 ci = 0; ci < chunkCount ; ci++) {
				if (!readVideoData(delayOverride)) return false;
			}
		}
			break;
		case AUDIO_CHUNK:
			if (!readAudioData()) return false;
			break;
		case PREFIX_CHUNK:
		default:
			SDL_RWseek(_rwops, frameSize - 6, SEEK_CUR); // skip unknown
			break;
	}
	return true;
}

bool FlcPlayer::readVideoData(Uint16 delayOverride) {
	Uint32 chunkSize = SDL_ReadLE32(_rwops);

	// TODO: check if enough data is there here

	Uint16 chunkType = SDL_ReadLE16(_rwops);

	switch(chunkType) {
		case COLOR_256:
			color256();
			break;
		case FLI_SS2:
			fliSS2();
			break;
		case COLOR_64:
			color64();
			break;
		case FLI_LC:
			fliLC();
			break;
		case BLACK:
			black();
			break;
		case FLI_BRUN:
			fliBRun();
			break;
		case FLI_COPY:
			fliCopy();
			break;
		case 18:
		default:
			Log(LOG_WARNING) << "FliPlayer: unknown video chunk type:" << chunkType;
			SDL_RWseek(_rwops, chunkSize - 6, SEEK_CUR);
			break;
	}
	return true;
}

bool FlcPlayer::readAudioData() {
 	Uint16 sampleRate = SDL_ReadLE16(_rwops);
	Uint16 sampleCount = SDL_ReadLE16(_rwops); // TENTATIVE FIXME
}

/** play state:
 * linked list of decoded frames, like this:
 * struct frame {
 *      struct frame *next;
 *      Uint32 tick
 *   	SDL_Surface *frame;
 *  	MIX_Shit *audio; // or null
 * }
 *
 * the object keeps a reference to next unplayed frame
 * the mainloop calls frameAt with current tick
 * the object then either does nothing (ctick < this->tick)
 * or updates given surface and audio state with data from
 * the 'top' frame, then discards it.
 * if it's the last decoded frame, tries to decode more frames.
 * returns false on EOF or error.
 *
 */

/**
 * Advances the play state a frame. Or something.
 */
bool FlcPlayer::frameAt(Uint32 tick, SDL_Surface **frame, Mix_Chunk **sample) {
	if (tick <= _currentTick) return true;

	*frame = NULL;
	*sample = NULL;
	return true;
}

void FlcPlayer::color256()
{
	Uint16 numColorPackets;
	Uint16 numColors = 0;
	Uint8 numColorsSkip;

	numColorPackets = SDL_ReadLE16(_rwops);

#if 0
	while (numColorPackets--)
	{
		numColorsSkip = *(pSrc++) + numColors;
		numColors = *(pSrc++);
		if (numColors == 0)
		{
			numColors = 256;
		}

		for (int i = 0; i < numColors; ++i)
		{
			_colors[i].r = *(pSrc++);
			_colors[i].g = *(pSrc++);
			_colors[i].b = *(pSrc++);
                        _colors[i].a = 255;
		}

		_game->getScreen()->setPalette(_colors, numColorsSkip, numColors, true);
		SDL_SetPaletteColors(_mainScreen->format->palette, _colors, numColorsSkip, numColors);

		if (numColorPackets >= 1)
		{
			++numColors;
		}
	}
#endif
}

void FlcPlayer::fliSS2()
{
#if 0
	Uint8 *pSrc, *pDst, *pTmpDst;
	Sint8 countData;
	Uint8 columSkip, fill1, fill2;
	Uint16 lines;
	Sint16 count;
	bool setLastByte = false;
	Uint8 lastByte = 0;

	pSrc = _chunkData + 6;
	pDst = (Uint8*)_mainScreen->pixels + _offset;
	readU16(lines, pSrc);

	pSrc += 2;

	while (lines--)
	{
		readS16(count, (Sint8 *)pSrc);
		pSrc += 2;

		if ((count & MASK) == SKIP_LINES)
		{
			pDst += (-count)*_mainScreen->pitch;
			++lines;
			continue;
		}

		else if ((count & MASK) == LAST_PIXEL)
		{
			setLastByte = true;
			lastByte = (count & 0x00FF);
			readS16(count, (Sint8 *)pSrc);
			pSrc += 2;
		}

		if ((count & MASK) == PACKETS_COUNT)
		{
			pTmpDst = pDst;
			while (count--)
			{
				columSkip = *(pSrc++);
				pTmpDst += columSkip;
				countData = *(pSrc++);

				if (countData > 0)
				{
					std::copy(pSrc, pSrc + (2 * countData), pTmpDst);
					pTmpDst += (2 * countData);
					pSrc += (2 * countData);

				}
				else
				{
					if (countData < 0)
					{
						countData = -countData;

						fill1 = *(pSrc++);
						fill2 = *(pSrc++);
						while (countData--)
						{
							*(pTmpDst++) = fill1;
							*(pTmpDst++) = fill2;
						}
					}
				}
			}

			if (setLastByte)
			{
				setLastByte = false;
				*(pDst + _mainScreen->pitch - 1) = lastByte;
			}
			pDst += _mainScreen->pitch;
		}
	}
#endif
}

void FlcPlayer::fliBRun()
{
#if 0
	Uint8 *pSrc, *pDst, *pTmpDst, fill;
	Sint8 countData;
	int heightCount;

	heightCount = _headerHeight;
	pSrc = _chunkData + 6; // Skip chunk header
	pDst = (Uint8*)_mainScreen->pixels + _offset;

	while (heightCount--)
	{
		pTmpDst = pDst;
		++pSrc; // Read and skip the packet count value

		int pixels = 0;
		while (pixels != _headerWidth)
		{
			countData = *(pSrc++);
			if (countData > 0)
			{
				fill = *(pSrc++);

				std::fill_n(pTmpDst, countData, fill);
				pTmpDst += countData;
				pixels += countData;
			}
			else
			{
				if (countData < 0)
				{
					countData = -countData;

					std::copy(pSrc, pSrc + countData, pTmpDst);
					pTmpDst += countData;
					pSrc += countData;
					pixels += countData;
				}
			}
		}
		pDst += _mainScreen->pitch;
	}
#endif
}


void FlcPlayer::fliLC()
{
#if 0
	Uint8 *pSrc, *pDst, *pTmpDst;
	Sint8 countData;
	Uint8 countSkip;
	Uint8 fill;
	Uint16 lines, tmp;
	int packetsCount;

	pSrc = _chunkData + 6;
	pDst = (Uint8*)_mainScreen->pixels + _offset;

	readU16(tmp, pSrc);
	pSrc += 2;
	pDst += tmp*_mainScreen->pitch;
	readU16(lines, pSrc);
	pSrc += 2;

	while (lines--)
	{
		pTmpDst = pDst;
		packetsCount = *(pSrc++);

		while (packetsCount--)
		{
			countSkip = *(pSrc++);
			pTmpDst += countSkip;
			countData = *(pSrc++);
			if (countData > 0)
			{
				while (countData--)
				{
					*(pTmpDst++) = *(pSrc++);
				}
			}
			else
			{
				if (countData < 0)
				{
					countData = -countData;

					fill = *(pSrc++);
					while (countData--)
					{
						*(pTmpDst++) = fill;
					}
				}
			}
		}
		pDst += _mainScreen->pitch;
	}
#endif
}

void FlcPlayer::color64()
{
#if 0
	Uint8 *pSrc;
	Uint16 NumColors, NumColorPackets;
	Uint8 NumColorsSkip;

	pSrc = _chunkData + 6;
	readU16(NumColorPackets, pSrc);
	pSrc += 2;

	while (NumColorPackets--)
	{
		NumColorsSkip = *(pSrc++);
		NumColors = *(pSrc++);

		if (NumColors == 0)
		{
			NumColors = 256;
		}

		for (int i = 0; i < NumColors; ++i)
		{
			_colors[i].r = *(pSrc++) << 2;
			_colors[i].g = *(pSrc++) << 2;
			_colors[i].b = *(pSrc++) << 2;
			_colors[i].a = 255;
		}

		_game->getScreen()->setPalette(_colors, NumColorsSkip, NumColors, true);
		SDL_SetPaletteColors(_mainScreen->format->palette, _colors, NumColorsSkip, NumColors);
	}
#endif
}

void FlcPlayer::fliCopy()
{
#if 0
	Uint8 *pSrc, *pDst;
	int Lines = _headerHeight;
	pSrc = _chunkData + 6;
	pDst = (Uint8*)_mainScreen->pixels + _offset;

	while (Lines--)
	{
		memcpy(pDst, pSrc, _headerWidth);
		pSrc += _headerWidth;
		pDst += _mainScreen->pitch;
	}
#endif
}

void FlcPlayer::black()
{
#if 0
	Uint8 *pDst;
	int Lines = _headerHeight;
	pDst = (Uint8*)_mainScreen->pixels + _offset;

	while (Lines-- > 0)
	{
		memset(pDst, 0, _headerHeight);
		pDst += _mainScreen->pitch;
	}
#endif
}

}
