// OFMain.cpp

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

#include "ZMain.h"
#include <stdio.h>
#include "ZApp.h"

#include "ZUtil_UI.h"
#include "ZUtil_Win.h"
#include "ZThread.h"

#include "OFApp.h"

void DeadlockHandler(const std::string& iString)
{
	ZAssert( false );

	return;
}

int ZMain(int argc, char** argv)
{	
	ZThread::sSetDeadlockHandler( DeadlockHandler );

	ZUtil_UI::sInitializeUIFactories();

#if ZCONFIG( OS, Win32 )
	ZUtil_Win::sDisallowWAPI();	// It doesn't seem to actually work...
#endif
	
	try{
		OFApp theApp;

		theApp.Run();
	}catch (...){
	}
	
	ZUtil_UI::sTearDownUIFactories();

	return 0;
}
