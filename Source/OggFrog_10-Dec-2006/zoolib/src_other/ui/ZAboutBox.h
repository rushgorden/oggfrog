/*  @(#) $Id: ZAboutBox.h,v 1.1 2002/02/19 21:02:31 agreen Exp $ */

#ifndef __ZAboutBox__
#define __ZAboutBox__ 1
#include "zconfig.h"

#include "ZWindow.h"


class ZAboutBox : public ZWindow
	{
public:
	ZAboutBox(const ZDCPixmap& inPixmap);
	virtual ~ZAboutBox();
protected:
	class PixmapPane;
	};

#endif // __ZAboutBox__
