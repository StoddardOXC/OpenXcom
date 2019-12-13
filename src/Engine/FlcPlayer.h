#pragma once
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

#pragma once
/*
 * Based on http://www.libsdl.org/projects/flxplay/
 */
#include "SDL.h"
#include "SDL_mixer.h"

namespace OpenXcom
{

class Screen;
class Game;

class FlcPlayer
{
private:

	SDL_RWops *_rwops;
	Sint64 _fileSize;
	Uint8 *_videoFrameData;
	Uint8 *_chunkData;
	Uint8 *_audioFrameData;
	Uint16 _frameCount;    /* Frame Counter */
	Uint32 _headerFileSize;    /* Fli file size */
	Uint16 _headerType;    /* Fli header check */
	Uint16 _headerFrames;  /* Number of frames in flic */
	Uint16 _headerWidth;   /* Fli width */
	Uint16 _headerHeight;  /* Fli height */
	Uint16 _headerDepth;   /* Color depth */
	Uint16 _headerSpeed;   /* Number of video ticks between frame */
	Uint32 _videoFrameSize;     /* Frame size in bytes */
	Uint16 _videoFrameType;
	Uint16 _frameChunks;   /* Number of chunks in frame */
	Uint32 _chunkSize;     /* Size of chunk */
	Uint16 _chunkType;     /* Type of chunk */
	Uint16 _delayOverride; /* FRAME_TYPE extension */
	Uint32 _audioFrameSize;
	Uint16 _audioFrameType;
	Uint32 _currentTick;

	bool isPlaying; 		/* if it's playing */

	SDL_Surface *_frameBuffer;
	SDL_Color _colors[256];
	int _offset;
	int _playingState;
	bool _hasAudio, _mute;
	int _videoDelay;
	double _volume;

	typedef struct AudioBuffer
	{
		Sint16 *samples;
		int sampleCount;
		int sampleBufSize;
		int currSamplePos;
	}AudioBuffer;


	typedef struct AudioData
	{
		int sampleRate;
		AudioBuffer *loadingBuffer;
		AudioBuffer *playingBuffer;
		SDL_sem *sharedLock;

	}AudioData;

	AudioData _audioData;

	Game *_game;

	void decodeVideo(bool skipLastFrame);
	void decodeAudio(int frames);

	void playVideoFrame();
	void color256();
	void fliBRun();
	void fliCopy();
	void fliSS2();
	void fliLC();
	void color64();
	void black();

	/// decodes next frame; false on EOF
	bool readChunk();
	/// decodes video subchunks; false on EOF
	bool readVideoData(Uint16 delayOverride);
	/// decodes audio chunk; false on EOF
	bool readAudioData();

public:

	FlcPlayer();
	~FlcPlayer();

	bool init(Game *game, SDL_RWops *buf, bool useInternalAudio);
	bool frameAt(Uint32 tick, SDL_Surface **frame, Mix_Chunk **sample);
	void fini();

};

}
