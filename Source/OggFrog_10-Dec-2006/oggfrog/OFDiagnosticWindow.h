// OFDiagnosticWindow.h

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

#ifndef OFDIAGNOSTICWINDOW_H
#define OFDIAGNOSTICWINDOW_H

#include <memory>

#include "ZWindow.h"
#include "ZUI.h"
#include "ZThread.h"

#include "OggFrogConfig.h"

#include "ZFIFOMonitor_PCM.h"

class ZOSWindow;
class ZApp;
class ZAudioPlayer;
class FIFOReportPane;

class OFDiagnosticWindow: public ZWindow, 
							public ZPaneLocator, 
							public ZUIButton::Responder,
							public ZUIRange::Responder
{
	public:
		
		OFDiagnosticWindow( ZWindowSupervisor *inSupervisor, ZOSWindow *inOSWindow );
		virtual ~OFDiagnosticWindow();
		
		static ZOSWindow *sCreateOSWindow( ZApp *inApp );

		void ReceivedMessage(const ZMessage& inMessage);

		// virtual bool WindowQuitRequested();
		
		virtual bool GetPaneSize(ZSubPane* inPane, ZPoint& outSize);
		virtual bool GetPaneLocation( ZSubPane *inPane, ZPoint &outLocation );
		virtual bool GetPaneEnabled( ZSubPane* inPane, bool& outEnabled );
		virtual bool GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult);
		
		virtual bool HandleUIButtonEvent( ZUIButton *inButton, ZUIButton::Event inButtonEvent );

		virtual bool HandleUIRangeEvent(ZUIRange* inRange, ZUIRange::Event inEvent, ZUIRange::Part inRangePart);
		virtual bool HandleUIRangeSliderEvent(ZUIRange* inRange, double inNewValue);
		
		virtual void WindowCloseByUser();
		
	private:

		enum{
			kRun,
			kStop,
			kDone
		};
		
		ZWindowPane						*mContentPane;
		ZUIButton						*mPlayFileButton;

		ZRef< ZUICaption >				mPlayCaption;
		ZRef< ZUICaption >				mPauseCaption;
		ZUIButton						*mPlayButton;

		ZUIButton						*mStopButton;
		ZUIButton						*mQuitButton;
		
		ZUITextPane						*mFileName;
		ZUIScrollBar					*mPosition;
		
		ZMessenger				mAppMessenger;

		FIFOReportPane					*mLeftStats;
		
		ZAtomic_t						mRunPulser;
		ZAudioPlayer					&mPlayer;
		
		void PlayFile();
		void Play();
		void Pause();
		void Stop();
		void Advance( float howMuch );
		
		void DrainFIFO();
		
		void ShutDown();
		
		static ZMessenger GetAppMessenger();
		
		static void Pulser( OFDiagnosticWindow &me );

#if ZCONFIG_Debug > 0
	void OFDiagnosticWindow::AssertPanePointers();
#endif

};

#endif // OFDIAGNOSTICWINDOW_H
		