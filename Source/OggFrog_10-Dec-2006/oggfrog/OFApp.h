// OFApp.h

/* Ogg Frog: Free Music Ripping, Encoding and Backup Program
 * http://www.oggfrog.com
 *
 * Copyright (C) 2006 Michael David Crawford. 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef OFFAPP_H
#define OFFAPP_H

#include <vector>

#include "ZApp.h"
#include "ZFile.h"

#include "OggFrogConfig.h"

#include "ZPCMQueue.h"
#include "ZPCMSink.h"
#include "ZPCMSource.h"
#include "ZFIFOVector.h"

class OFDiagnosticWindow;

class OFApp: public ZApp
{
	public:
		OFApp();
		virtual ~OFApp();

		inline static OFApp &sGet();
		
		virtual void RunStarted();
		virtual void RunFinished();
		virtual void WindowSupervisorInstallMenus(ZMenuInstall& inMenuInstall);
		virtual void ReceivedMessage(const ZMessage& inMessage);
		
		void Subscribe( ZMessenger inSubscriber );
		void Unsubscribe( ZMessenger inSubscriber );
		

	private:
	
		typedef std::vector< ZMessenger > MessengerVector;

		bool					mWaitingToQuit;
		OFDiagnosticWindow		*mDiagnosticWindow;
		MessengerVector			mSubscribers;

		void PlayFile();
		void Play();
		void Pause();
		void Stop();
		void SetPosition( float newTime );
		void PlayerFinished( ZPCMSource *inPlayer );
		void DrainFIFO();
		
		void Publish( ZMessage const &inMessage );
		
		void ShutDown();
};

inline OFApp &OFApp::sGet()
{
	try{
	
		return dynamic_cast< OFApp& >( *ZApp::sGet() );
		
	}catch(...){
		
		ZAssert( false );		// ZApp wasn't an OFApp; cast failed
		
		throw;
	}
}

#endif // OFAPP_H

