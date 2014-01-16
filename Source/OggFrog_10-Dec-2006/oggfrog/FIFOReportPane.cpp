// FIFOReportPane.cpp

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

#include "ZUIFactory.h"
#include "ZString.h"

#include "FIFOReportPane.h"
#include "ZUI_FuelGage.h"

using std::string;

FIFOReportPane::FIFOReportPane( bool inDisplayHeader,
								std::string const &inChannelName, 
								ZSuperPane *inSuperPane, 
								ZPaneLocator *inLocator )
	: ZSuperPane( inSuperPane, inLocator ),
		ZPaneLocator( nil ),
		
		mDisplayHeader( inDisplayHeader ),
		mChannelName( inChannelName ),
		
		mChannelHeader( !mDisplayHeader ? nil
						: ZUIFactory::sGet()->Make_TextPane( this, this ) ),
		mChannelLabel( ZUIFactory::sGet()->Make_TextPane( this, this )	),
		
		mFillHeader( !mDisplayHeader ? nil
						: ZUIFactory::sGet()->Make_TextPane( this, this ) ),
		mFillLevel( new ZUI_FuelGage( 0, 0, this, this ) ),
		
		mWrittenHeader( !mDisplayHeader ? nil
						: ZUIFactory::sGet()->Make_TextPane( this, this ) ),
		mFramesWritten( ZUIFactory::sGet()->Make_TextPane( this, this )	),
		
		mReadHeader( !mDisplayHeader ? nil
						: ZUIFactory::sGet()->Make_TextPane( this, this ) ),
		mFramesRead( ZUIFactory::sGet()->Make_TextPane( this, this ) ),
		
		mSilentHeader( !mDisplayHeader ? nil
						: ZUIFactory::sGet()->Make_TextPane( this, this ) ),
		mSilentFrames( ZUIFactory::sGet()->Make_TextPane( this, this ) ),
		
		mWaitHeader( !mDisplayHeader ? nil
						: ZUIFactory::sGet()->Make_TextPane( this, this ) ),
		mWriteWaits( ZUIFactory::sGet()->Make_TextPane( this, this ) )
{	
	return;
}
		
FIFOReportPane::~FIFOReportPane()
{
	return;
}

void FIFOReportPane::PaneLocatorReady()
{
	// ZUITextPane::SetText may only be called when its pane locator is
	// fully constructed.  Otherwise GetPaneSize and the like may use pointers to other
	// panes that haven't been constructed yet.

	mChannelLabel->SetText( mChannelName );

	if ( mDisplayHeader ){
		mChannelHeader->SetText( "Channel" );
		mFillHeader->SetText( "FIFO Fill" );
		mWrittenHeader->SetText( "Frames Written" );
		mReadHeader->SetText( "Frames Read" );
		mSilentHeader->SetText( "Silent Frames" );
		mWaitHeader->SetText( "Write Waits" );
	}

	return;
}

void FIFOReportPane::SubmitReport( ZMessage const &inReport )
{
	mFillLevel->SetFillLevel( inReport.GetInt32( "howFull" ),
								inReport.GetInt32( "capacity" ) );
								
	mFramesWritten->SetText( ZString::sFromInt( inReport.GetInt32( "framesWritten" ) ) );
	mFramesRead->SetText( ZString::sFromInt( inReport.GetInt32( "framesRead" ) ) );
	mSilentFrames->SetText( ZString::sFromInt( inReport.GetInt32( "silentFrames" ) ) );
	mWriteWaits->SetText( ZString::sFromInt( inReport.GetInt32( "writeWaits" ) ) );
	
	return;
}

bool FIFOReportPane::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
{
	outLocation.v = 5;

	if ( mWrittenHeader != nil ){
	
		if ( inPane == mChannelHeader ){
			outLocation.h = mChannelLabel->GetLocation().h;
			return true;
		}
		
		if ( inPane == mFillHeader ){
			outLocation.h = mFillLevel->GetLocation().h;
			return true;
		}
		
		if ( inPane == mWrittenHeader ){
			outLocation.h = mFramesWritten->GetLocation().h;
			return true;
		}
		
		if ( inPane == mReadHeader ){
			outLocation.h = mFramesRead->GetLocation().h;
			return true;
		}
		
		if ( inPane == mSilentHeader ){
			outLocation.h = mSilentFrames->GetLocation().h;
			return true;
		}
		
		if ( inPane == mWaitHeader ){
			outLocation.h = mWriteWaits->GetLocation().h;
			return true;
		}
	
		outLocation.v = 10 + mWrittenHeader->GetSize().v;
	}
	
	if ( inPane == mChannelLabel ){
		outLocation.h = 5;
		return true;
	}else if ( inPane == mFillLevel ){
		outLocation.h = 5 + mChannelLabel->GetLocation().h + mChannelLabel->GetSize().h;
		return true;
	} else if ( inPane == mFramesWritten ){
		outLocation.h = 15 + mFillLevel->GetLocation().h + mFillLevel->GetSize().h;
		return true;
	} else if ( inPane == mFramesRead ){
		outLocation.h = 5 + mFramesWritten->GetLocation().h + mFramesWritten->GetSize().h;
		return true;
	} else if ( inPane == mSilentFrames ){
		outLocation.h = 5 + mFramesRead->GetLocation().h + mFramesRead->GetSize().h;
		return true;
	} else if ( inPane == mWriteWaits ){
		outLocation.h = 5 + mSilentFrames->GetLocation().h + mSilentFrames->GetSize().h;
		return true;
	}
	
	return ZPaneLocator::GetPaneLocation( inPane, outLocation );
}

bool FIFOReportPane::GetPaneSize(ZSubPane* inPane, ZPoint& outSize)
{
	if ( mChannelHeader != nil ){
	
		outSize.v = 30;
	
		if ( inPane == mChannelHeader ){
			outSize.h = mChannelLabel->GetSize().h;
			return true;
		}
		
		if ( inPane == mFillHeader ){
			outSize.h = mFillLevel->GetSize().h;
			return true;
		}
		
		if ( inPane == mWrittenHeader ){
			outSize.h = mFramesWritten->GetSize().h;
			return true;
		}
		
		if ( inPane == mReadHeader ){
			outSize.h = mFramesRead->GetSize().h;
			return true;
		}
		
		if ( inPane == mSilentHeader ){
			outSize.h = mSilentFrames->GetSize().h;
			return true;
		}
		
		if ( inPane == mWaitHeader ){
			outSize.h = mWriteWaits->GetSize().h;
			return true;
		}
	}
	
	outSize.v = 15;

	if ( inPane == mChannelLabel ){
	
		outSize.h = 60;
		return true;
	}
	
	if ( inPane == mFillLevel ){
	
		outSize.h = 100;
		
		return true;
	}
	
	outSize.h = 60;

	return true;
}

ZPoint FIFOReportPane::GetSize()
{
	// We handle our own size rather than allowing the pane locator to
	
	ZCoord width = 10 + mWriteWaits->GetLocation().h + mWriteWaits->GetSize().h;
	
	ZCoord height = 5 + mFillLevel->GetLocation().v + mFillLevel->GetSize().v;
	
	ZPoint result( width, height );
	
	return result;
}




