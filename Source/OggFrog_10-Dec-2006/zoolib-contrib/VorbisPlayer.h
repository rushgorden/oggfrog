// VorbisPlayer.h

/* ------------------------------------------------------------
Copyright (c) 2006 Michael David Crawford.
http://www.oggfrog.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------ */
 
#ifndef VORBISPLAYER_H
#define VORBISPLAYER_H
 
#include <memory>
#include <vector>

//#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "ZFile.h"
#include "ZThreadSimple.h"
#include "ZRefCount.h"
#include "ZMessage.h"
#include "ZStream_Buffered.h"

#include "ZPCMSource.h"

class ZPCMQueue;

 class VorbisPlayer: public ZPCMSource
 {
	public:
		VorbisPlayer( ZFileSpec const &inFile, ZMessenger inClient );
		virtual ~VorbisPlayer();

		virtual void SetTime( float newTime );
				
		virtual float TotalTime() const;
		virtual float CurTime() const;
		
	private:

		enum{
			kBufSize = 4096
		};

		struct Decoder{
			Decoder();
			~Decoder();
			
			bool			mOpened;
			OggVorbis_File	mFile;
			int				mChannels;
			long			mSamples;
			ZAtomic_t		mCurTime;
			ZAtomic_t		mNewTime;
			float			mPCMBuf[ kBufSize ];
		};
	
		std::auto_ptr< Decoder >	mDecoder;
		
		static ZRef< ZStreamerRPos > OpenInput( ZFileSpec const &inFile );
		static std::auto_ptr< Decoder > Open( ZRef< ZStreamerRPos > &input );	// by reference NOT value!

		static void DecoderProc( VorbisPlayer &inPlayer );
		void DecoderProc();

		void StreamChange( int inStreamIndex );
		
		static size_t Read(void *ptr, size_t size, size_t nmemb, void *datasource);
		static int Seek(void *datasource, ogg_int64_t offset, int whence);
		static int Close(void *datasource);
		static long Tell(void *datasource);
};

#endif // VORBISPLAYER_H
