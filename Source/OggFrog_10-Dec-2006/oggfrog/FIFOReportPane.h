// FIFOReportPane.h

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

#ifndef FIFOREPORTPANE_H
#define FIFOREPORTPANE_H

#include <string>

#include "ZPane.h"

#include "OggFrogConfig.h"

class ZUI_FuelGage;
class ZUITextPane;

class FIFOReportPane: public ZSuperPane, public ZPaneLocator
{
	public:
	
		FIFOReportPane( bool inDisplayHeader,
						std::string const &inChannelName, 
						ZSuperPane *inSuperPane, 
						ZPaneLocator *inLocator );
		virtual ~FIFOReportPane();
		
		virtual void SubmitReport( ZMessage const &inReport );

		virtual bool GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation);
		virtual bool GetPaneSize(ZSubPane* inPane, ZPoint& outSize);
		
		// ZUITextPane::SetText may only be called when its pane locator is
		// fully constructed.  Otherwise GetPaneSize and the like may use pointers to other
		// panes that haven't been constructed yet.
		
		void PaneLocatorReady();
		
		ZPoint GetSize();	// We handle our own size rather than allowing the pane locator to
	
	private:
	
		bool		mDisplayHeader;
		std::string	mChannelName;
		
		ZUITextPane *mChannelHeader;
		ZUITextPane *mChannelLabel;
		
		ZUITextPane		*mFillHeader;
		ZUI_FuelGage	*mFillLevel;
		
		ZUITextPane *mWrittenHeader;		
		ZUITextPane	*mFramesWritten;
		
		ZUITextPane *mReadHeader;
		ZUITextPane	*mFramesRead;
		
		ZUITextPane *mSilentHeader;
		ZUITextPane	*mSilentFrames;
		
		ZUITextPane *mWaitHeader;
		ZUITextPane	*mWriteWaits;
};

#endif // FIFOREPORTPANE_H
		
