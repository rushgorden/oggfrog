// OFDiagnosticWindow.cpp

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

#include "OFDiagnosticWindow.h"

#include <stdio.h>

#include "ZUIFactory.h"
#include "ZOSWindow.h"
#include "ZApp.h"
#include "ZFile.h"
#include "ZThreadSimple.h"
#include "OFApp.h"
#include "OFDebug.h"
#include "ZAudioPlayer.h"

#include "FIFOReportPane.h"

OFDiagnosticWindow::OFDiagnosticWindow( ZWindowSupervisor *inSupervisor, ZOSWindow *inOSWindow )
	: ZWindow( inSupervisor, inOSWindow ),
		ZPaneLocator( nil ),
		mContentPane( new ZWindowPane( this, this ) ),
		mPlayFileButton( ZUIFactory::sGet()->Make_ButtonPush( mContentPane, this, this, "Play File" ) ),

		mPlayCaption( new ZUICaption_Text( "Play", ZUIAttributeFactory::sGet()->GetFont_Application(), 0 ) ),
		mPauseCaption( new ZUICaption_Text( "Pause", ZUIAttributeFactory::sGet()->GetFont_Application(), 0 ) ),
		mPlayButton( ZUIFactory::sGet()->Make_ButtonPush( mContentPane, this, this, mPlayCaption ) ),
		
		mStopButton( ZUIFactory::sGet()->Make_ButtonPush( mContentPane, this, this, "Stop" ) ),
		// mDecodeButton( ZUIFactory::sGet()->Make_ButtonPush( mContentPane, this, this, "Decode" ) ),
		mQuitButton( ZUIFactory::sGet()->Make_ButtonPush( mContentPane, this, this, "Quit" ) ),
		mFileName( ZUIFactory::sGet()->Make_TextPane( mContentPane, this ) ),
		mPosition( ZUIFactory::sGet()->Make_ScrollBar( mContentPane, this ) ),
		mAppMessenger( GetAppMessenger() ),
		mLeftStats( new FIFOReportPane( true, "Left", mContentPane, this ) ),
		mRunPulser(),
		mPlayer( ZAudioPlayer::sGet() )
{
#if ZCONFIG_Debug > 0
	AssertPanePointers();
#endif

	mPosition->AddResponder( this );

	mLeftStats->PaneLocatorReady();

	SetTitle( "Ogg Frog 10-Dec-2006" );
	
	OFApp &app = OFApp::sGet();

	{
		ZLocker locker( app.GetLock() );
		app.Subscribe( GetMessenger() );
	}

	{
		ZFIFOMonitor_PCM &monitor = mPlayer.GetFIFOMonitor();

		ZLocker locker( monitor.GetLock() );
		
		ZMessenger me = GetMessenger();
		monitor.Add( me, ZAudioPlayer::kLeft );
		monitor.Add( me, ZAudioPlayer::kRight );

	}

	mRunPulser.fValue = kRun;

	ZThreadSimple< OFDiagnosticWindow& > *pulser =
		new ZThreadSimple< OFDiagnosticWindow& >( &Pulser, *this, "Pulser" );
	
	pulser->Start();
	
	return;
}
		
OFDiagnosticWindow::~OFDiagnosticWindow()
{
	ZAtomic_Set( &mRunPulser, kStop );
	
	while( kDone != ZAtomic_Get( &mRunPulser ) ){
		ZThread::sSleep( 50 );
	}

	return;
}
		
ZOSWindow *OFDiagnosticWindow::sCreateOSWindow( ZApp *inApp )
{
	ZOSWindow::CreationAttributes attr;
	
	attr.fFrame = ZRect( 0, 0, 525, 200 );
	attr.fLook = ZOSWindow::lookDocument;
	attr.fLayer = ZOSWindow::layerDocument;
	attr.fResizable = true;
	attr.fHasSizeBox = true;
	attr.fHasCloseBox = true;
	attr.fHasZoomBox = true;
	attr.fHasMenuBar = false;
	attr.fHasTitleIcon = false;
	
	return inApp->CreateOSWindow( attr );
}

#if 0
bool OFDiagnosticWindow::WindowQuitRequested()
{
	delete this;
	return true;
}
#endif

void OFDiagnosticWindow::ReceivedMessage(const ZMessage& inMessage)
{
#if ZCONFIG_Debug > 0
	AssertPanePointers();
#endif

	string theWhat;
	
	if ( inMessage.GetString( "what", theWhat ) ){
		
		if ( "OggFrog:track-filename" == theWhat ){
		
			mFileName->SetText( inMessage.GetString( "filename" ) );

		}else if ( theWhat == "OggFrog:playing" ){
		
			mPlayButton->SetCaption( mPauseCaption, true );
			mStopButton->Refresh();
			
		}else if ( theWhat == "OggFrog:paused" ){
	
			mPlayButton->SetCaption( mPlayCaption, true );
			mStopButton->Refresh();

		}else if ( theWhat == "OggFrog:stopped" ){

			mStopButton->Refresh();
			mPlayButton->SetCaption( mPlayCaption, true );
			
		}else if ( theWhat == "OggFrog:player-finished" ){
		
			mStopButton->Refresh();
			mPlayButton->SetCaption( mPlayCaption, true );
			
			return;
		
		}else if ( theWhat == "OggFrog:ZPCMQueue:report" ){
		
			int fifo = inMessage.GetInt32( "channel-index" );
			
			if ( fifo == ZAudioPlayer::kLeft ){
			
				mLeftStats->SubmitReport( inMessage );
				
			}
			
			return;
		}else if ( theWhat == "OggFrog:pulse" ){
		
			mPosition->Refresh();
			
			return;
		}else if ( theWhat == "OggFrog:shutdown" ){
		
			ShutDown();
			
			return;
		}
	}
	
	ZWindow::ReceivedMessage( inMessage );
	
	return;
}

bool OFDiagnosticWindow::GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult)
{
#if ZCONFIG_Debug > 0
	AssertPanePointers();
#endif
	
	if ( inPane == mPosition ){
	
		if ( "ZUIRange::Value" == inAttribute ){
		
			ZLocker locker( mPlayer.GetLock() );
									
			float totalTime = mPlayer.TotalTime();
			
			if ( totalTime != 0.0 )
				*reinterpret_cast< double* >( outResult ) = mPlayer.CurTime() / totalTime;
			else
				*reinterpret_cast< double* >( outResult ) = 0.0;
				
			return true;
		}
	}
	
	return ZPaneLocator::GetPaneAttribute( inPane, inAttribute, inParam, outResult );
}

bool OFDiagnosticWindow::GetPaneSize(ZSubPane* inPane, ZPoint& outSize)
{
#if ZCONFIG_Debug > 0
	AssertPanePointers();
#endif

	if ( inPane == mContentPane ){
		outSize = GetWindowSize();
		return true;
	}
	
	if ( inPane == mFileName ){
		outSize.h = mContentPane->GetSize().h - 10;
		outSize.v = 15;
		return true;
	}
	
	if ( inPane == mPosition ){
		outSize.h = mContentPane->GetSize().h - ( mPosition->GetLocation().h + 5 );
		outSize.v = mPosition->GetPreferredThickness();
		return true;
	}
	
	return ZPaneLocator::GetPaneSize( inPane, outSize );
}

bool OFDiagnosticWindow::GetPaneEnabled( ZSubPane* inPane, bool& outEnabled )
{
#if ZCONFIG_Debug > 0
	AssertPanePointers();
#endif

	bool selected;
	bool playing;

	{
		ZLocker locker( mPlayer.GetLock() );
			
		selected = mPlayer.TrackSelected();
		playing = mPlayer.Playing();
	}
	
	if ( inPane == mPosition ){
	
		if ( selected != false )
			outEnabled = true;
		else
			outEnabled = false;
			
		return true;
	}
	
	if ( inPane == mPlayButton ){
	
		if ( selected != false )
			outEnabled = true;
		else
			outEnabled = false;
		
		return true;
	}
	
	if ( inPane == mStopButton ){
		
		if ( playing )
			outEnabled = true;
		else
			outEnabled = false;
		
		return true;
	}
	
	return ZPaneLocator::GetPaneEnabled( inPane, outEnabled );
}

bool OFDiagnosticWindow::GetPaneLocation( ZSubPane *inPane, ZPoint &outLocation )
{
#if ZCONFIG_Debug > 0
	AssertPanePointers();
#endif

	if ( inPane == mLeftStats ){
		outLocation = ZPoint( 75, 80 );
		return true;
	}
	
	if ( inPane == mFileName ){
		outLocation = ZPoint( 5, 5 );
		return true;
	}
	
	if ( inPane == mPosition ){
		ZPoint loc( mPlayFileButton->GetLocation() );
		ZPoint sz( mPlayFileButton->GetSize() );
		
		outLocation.h = loc.h + sz.h + 5;
		outLocation.v = loc.v;
		
		return true;
	}
	
	if ( inPane == mQuitButton ){
		ZPoint sz( mContentPane->GetSize() );
		ZPoint bSz( mQuitButton->GetSize() );
		
		outLocation.h = 5;
		outLocation.v = sz.v - ( 5 + bSz.v );
		
		return true;
	}
	
	if ( inPane == mPlayFileButton ){
		ZPoint loc( mFileName->GetLocation() );
		ZPoint sz( mFileName->GetSize() );
		
		outLocation = ZPoint( 5, loc.v + sz.v + 5 );
		
		return true;
	}
	
	if ( inPane == mPlayButton ){
		ZPoint loc( mPlayFileButton->GetLocation() );
		ZPoint sz( mPlayFileButton->GetSize() );
		
		outLocation = ZPoint( 5, loc.v + sz.v + 5 );
		
		return true;
	}

	if ( inPane == mStopButton ){
		ZPoint loc( mPlayButton->GetLocation() );
		ZPoint sz( mPlayButton->GetSize() );
		
		outLocation = ZPoint( 5, loc.v + sz.v + 5 );
		
		return true;
	}
		
	return ZPaneLocator::GetPaneLocation( inPane, outLocation );
}

bool OFDiagnosticWindow::HandleUIRangeEvent(ZUIRange* inRange, ZUIRange::Event inEvent, ZUIRange::Part inRangePart)
{
	if ( inRange != mPosition )
		return false;

	switch( inEvent ){
		case ZUIRange::EventDown:
		case ZUIRange::EventStillDown:
		
			switch ( inRangePart ){

				case ZUIRange::PartStepUp:
					Advance( -0.01 );
					break;
					
				case ZUIRange::PartPageUp:
					Advance( -0.1 );
					break;
					
				case ZUIRange::PartStepDown:
					Advance( 0.01 );
					break;
					
				case ZUIRange::PartPageDown:
					Advance( 0.1 );
					break;
			
			}
			break;
			
		case ZUIRange::EventStillUp:
		case ZUIRange::EventUp:
		case ZUIRange::EventReleasedWhileUp:
		case ZUIRange::EventAboutToBeReleasedWhileDown:
		case ZUIRange::EventReleasedWhileDown:
			break;
	}

	return true;
}

void OFDiagnosticWindow::Advance( float howMuch )
{
	float totalTime;
	float curTime;
	
	{
		ZLocker locker( mPlayer.GetLock() );
			
		totalTime = mPlayer.TotalTime();

		curTime = mPlayer.CurTime();
	}
	
	float newTime = curTime + ( totalTime * howMuch );
	
	if ( newTime < 0 )
		newTime = 0;
	else if ( newTime > totalTime )
		newTime = totalTime;
		
	ZMessage msg;
	
	msg.SetString( "what", "OggFrog:set-position" );
	
	msg.SetFloat( "time", newTime );
	
	mAppMessenger.PostMessage( msg );

	return;
}	

bool OFDiagnosticWindow::HandleUIRangeSliderEvent(ZUIRange* inRange, double inNewValue)
{
	if ( inRange != mPosition )
		return false;
	
	float newTime;
	
	{
		ZLocker locker( mPlayer.GetLock() );
				
		float totalTime = mPlayer.TotalTime();

		newTime = totalTime * inNewValue;
	}
	
	ZMessage msg;
	
	msg.SetString( "what", "OggFrog:set-position" );
	
	msg.SetFloat( "time", newTime );
	
	mAppMessenger.PostMessage( msg );

	return true;
}
	
bool OFDiagnosticWindow::HandleUIButtonEvent( ZUIButton *inButton, ZUIButton::Event inButtonEvent )
{
#if ZCONFIG_Debug > 0
	AssertPanePointers();
#endif

	if ( inButton == mPlayFileButton ){
		if ( ZUIButton::ButtonAboutToBeReleasedWhileDown == inButtonEvent ){

			ZMessage msg;

			msg.SetString( "what", "OggFrog:play-file" );
			
			mAppMessenger.PostMessage(msg);		
		}
	}

	if ( inButton == mPlayButton ){
		if ( ZUIButton::ButtonAboutToBeReleasedWhileDown == inButtonEvent ){

			ZMessage msg;
			
			{
				ZLocker locker( mPlayer.GetLock() );
						
				bool playing = mPlayer.Playing();

				if ( playing )
					msg.SetString( "what", "OggFrog:pause" );
				else
					msg.SetString( "what", "OggFrog:play" );
			}
			
			mAppMessenger.PostMessage(msg);		
		}
	}

	if ( inButton == mStopButton ){
		if ( ZUIButton::ButtonAboutToBeReleasedWhileDown == inButtonEvent ){

			ZMessage msg;

			msg.SetString( "what", "OggFrog:stop" );
			
			mAppMessenger.PostMessage(msg);		
		}
	}
	
	if ( inButton == mQuitButton ){
		if ( ZUIButton::ButtonAboutToBeReleasedWhileDown == inButtonEvent ){

			ZMessage msg;
			
			msg.SetString( "what", "zoolib:RunFinished" );
			
			ZApp::sGet()->DispatchMessage( msg );
		}
	}
	
	return false;
}
		
void OFDiagnosticWindow::WindowCloseByUser()
{
#if ZCONFIG_Debug > 0
	AssertPanePointers();
#endif

	ZMessage msg;
	
	msg.SetString( "what", "zoolib:RunFinished" );
	
	ZApp::sGet()->DispatchMessage( msg );
	
	return;
}

void OFDiagnosticWindow::Pulser( OFDiagnosticWindow &me )
{	
	ZMessenger mercury;
	
	{
		ZLocker locker( me.GetLock() );
		mercury = me.GetMessenger();
	}
	
	while( true ){
	
		int run = ZAtomic_Get( &me.mRunPulser );
		
		ZAssert( run >= kRun && run < kDone );
		
		if ( run == kStop ){
			ZAtomic_Set( &me.mRunPulser, kDone );
			return;
		}
		
		ZThread::sSleep( 333 );
		
		ZMessage msg;
		
		msg.SetString( "what", "OggFrog:pulse" );
		
		mercury.PostMessage( msg );
	}
	
	return;
}

#if ZCONFIG_Debug > 0
void OFDiagnosticWindow::AssertPanePointers()
{
	ZAssert( GetLock().IsLocked() );

	OFAssertValidDynamicCast( mContentPane, ZWindowPane, ZEventHr );
	OFAssertValidDynamicCast( mPlayFileButton, ZUIButton, ZEventHr );
	OFAssertValidDynamicCast( mPlayButton, ZUIButton, ZEventHr );
	OFAssertValidDynamicCast( mStopButton, ZUIButton, ZEventHr );
	OFAssertValidDynamicCast( mQuitButton, ZUIButton, ZEventHr );
	OFAssertValidDynamicCast( mFileName, ZUITextPane, ZEventHr );
	OFAssertValidDynamicCast( mPosition, ZUIScrollBar, ZEventHr );
	OFAssertValidDynamicCast( mLeftStats, FIFOReportPane, ZEventHr );

	return;
}
#endif

ZMessenger OFDiagnosticWindow::GetAppMessenger()
{
	OFApp &app = OFApp::sGet();
	
	ZLocker locker( app.GetLock() );
	
	return app.GetMessenger();
}

void OFDiagnosticWindow::ShutDown()
{
	OFApp &app = OFApp::sGet();
	
	{
		ZLocker locker( app.GetLock() );
		app.Unsubscribe( GetMessenger() );
	}

	return;
}

