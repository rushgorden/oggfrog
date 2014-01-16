// OFApp.cpp

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

#include "OFApp.h"
#include <cstring>
#include <algorithm>
#include <stdio.h>

#include "ZUtil_File.h"

#include "ZAudioPlayer.h"
#include "OFDiagnosticWindow.h"

using std::find;

OFApp::OFApp()
	: ZApp( "application/x-oggfrog-ogg-frog" ), 
		mSubscribers(),
		mWaitingToQuit( false ),
		mDiagnosticWindow( NULL )
{
	return;
}
	
OFApp::~OFApp()
{
	return;
}

void OFApp::RunStarted()
{
	ZApp::RunStarted();

	mDiagnosticWindow = new OFDiagnosticWindow( this, OFDiagnosticWindow::sCreateOSWindow( this ) );

	mDiagnosticWindow->Center();
	mDiagnosticWindow->BringFront();
	mDiagnosticWindow->GetLock().Release();

	return;
}

void OFApp::RunFinished()
{
//	mDiagnosticWindow->CloseAndDispose();
	
	QueueRequestQuitMessage();
	
	return;
}

void OFApp::ReceivedMessage(const ZMessage& inMessage)
{
	string theWhat;
	
	if ( inMessage.GetString( "what", theWhat ) ){
		
		if ( theWhat == "OggFrog:play-file" ){

			PlayFile();
						
			return;
		}else if ( theWhat == "OggFrog:play" ){

			Play();
					
			return;
			
		}else if ( theWhat == "OggFrog:pause" ){
			
			Pause();
			
			return;
						
		}else if ( theWhat == "OggFrog:stop" ){
			
			Stop();
			
			return;
		}else if ( theWhat == "OggFrog:set-position" ){
		
			SetPosition( inMessage.GetFloat( "time" ) );
			
			return;
			
		}else if ( theWhat == "OggFrog:player-finished" ){
		
			PlayerFinished( reinterpret_cast< ZPCMSource* >( inMessage.GetPointer( "player" ) ) );

			return;
		
		}else if ( theWhat == "OggFrog:shutdown" ){
		
			ShutDown();
			
			return;
		}
	}
	
	ZApp::ReceivedMessage( inMessage );
	
	return;
}
	
void OFApp::WindowSupervisorInstallMenus(ZMenuInstall& inMenuInstall)
{
#if 0
	if (ZRef<ZMenu> appleMenu = inMenuInstall.GetAppleMenu()){
		appleMenu->RemoveAll();
		appleMenu->Append(mcAbout, "About " + this->GetAppName() + "...");
	}
#endif
}

void OFApp::PlayFile()
{
	ZFileSpec musicFile;
	
#if ZCONFIG_API_Enabled(File_MacClassic)
	if ( !ZUtil_File::sOpenFileDialog( "Select an Ogg Vorbis audio file:", 
										vector<OSType>(), 
										musicFile ) ){
		return;
	}
	
#elif  ZCONFIG_API_Enabled(File_Win)

	vector< pair< string, string> > typeList;
	
	typeList.push_back( pair< string, string >( "Audio Files", "*.ogg;*.mp3" ) );
	// typeList.push_back( pair< string, string >( "Ogg Vorbis Audio Files", "*.ogg" ) );
	// typeList.push_back( pair< string, string >( "MP3 Audio Files", "*.mp3" ) );
	
	if ( !ZUtil_File::sOpenFileDialog( "Select an audio file:", 
										typeList, 
										musicFile ) ){
		return;
	}

#else
#error Need a file open dialog
#endif

	ZMessage ssMsg;
	
	ssMsg.SetString( "what", "OggFrog:track-filename" );
	
	ssMsg.SetString( "filename", musicFile.Name() );
	
	Publish( ssMsg );
	
	{
		ZAudioPlayer &player = ZAudioPlayer::sGet();
		
		ZLocker locker( player.GetLock() );
		
		player.SetTrack( musicFile, GetMessenger() );
	}
	
	Play();

	return;
}

void OFApp::Play()
{
	bool playing = false;
	
	{
		ZAudioPlayer &player = ZAudioPlayer::sGet();
		
//		ZLocker locker( player.GetLock() );

		if ( player.TrackSelected() ){
	
			player.Play();
			playing = true;
		}
	}
	
	if ( playing ){	
		ZMessage msg;
		
		msg.SetString( "what", "OggFrog:playing" );
		
		Publish( msg );
		
	}

	return;
}

void OFApp::Pause()
{
	{
		ZAudioPlayer &player = ZAudioPlayer::sGet();
		
		ZLocker locker( player.GetLock() );
		
		player.Pause();
	}

	ZMessage msg;
	
	msg.SetString( "what", "OggFrog:paused" );
	
	Publish( msg );
		
	return;
}

void OFApp::SetPosition( float newTime )
{
	ZAudioPlayer &player = ZAudioPlayer::sGet();
		
	ZLocker locker( player.GetLock() );
	
	player.SetPosition( newTime );

	return;
}

void OFApp::Stop()
{
	{
		ZAudioPlayer &player = ZAudioPlayer::sGet();
		
		ZLocker locker( player.GetLock() );
	
		player.Stop();
	}

	ZMessage msg;
	
	msg.SetString( "what", "OggFrog:stopped" );
	
	Publish( msg );
	
	return;
}

void OFApp::PlayerFinished( ZPCMSource *inPlayer )
{
	{
		ZAudioPlayer &player = ZAudioPlayer::sGet();
		
		ZLocker locker( player.GetLock() );
	
		player.PlayerFinished();
	}

	ZMessage msg;
	
	msg.SetString( "what", "OggFrog:player-finished" );
	
	Publish( msg );
	
	return;
}

void OFApp::Subscribe( ZMessenger inSubscriber )
{
	ZAssert( GetLock().IsLocked() );
	
	mSubscribers.push_back( inSubscriber );
	
	return;
}

void OFApp::Unsubscribe( ZMessenger inSubscriber )
{
	ZAssert( GetLock().IsLocked() );
	
	MessengerVector::iterator it( find( mSubscribers.begin(), mSubscribers.end(), inSubscriber ) );
	
	ZAssert( it != mSubscribers.end() );
	
	mSubscribers.erase( it );
		
	return;
}

void OFApp::Publish( ZMessage const &inMessage )
{	
	for ( MessengerVector::iterator it( mSubscribers.begin() );
			it != mSubscribers.end(); ++it ){
			
			it->PostMessage( inMessage );
	}
	
	return;
}

void OFApp::ShutDown()
{
	mWaitingToQuit = true;

	ZMessage msg;
	
	msg.SetString( "what", "OggFrog:shutdown" );
	
	Publish( msg );
	
	return;
}


