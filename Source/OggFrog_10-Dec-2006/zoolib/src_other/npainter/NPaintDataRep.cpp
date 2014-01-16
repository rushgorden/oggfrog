#include "NPaintDataRep.h"
#include "NPainter_FromPICT.h"

#include "ZDC.h" // For ZDCScratch
#include "ZDC_QD.h" // For ZDC_PICT
#include "ZDCPixmapCoder_BMP.h"
#include "ZDCPixmapCoder_PNG.h"
#include "ZStream_Count.h"
#include "ZString.h"
#include "ZTextUtil.h"
#include "ZUtil_Mac_HL.h" // For sPixmapFromStreamPICT

static void sReadOldPainting(const ZStreamR& inStream,
					ZPoint& outOverallSize,
					ZDCPixmap& outPixmap,
					vector<string>& outStrings, 
 					vector<ZDCFont>& outFonts,
					vector<ZRect>& outBounds,
					vector<ZRGBColor>& outColors)
	{
	// This is the bounds of the painting as a whole
	ZRect paintingBounds;
	paintingBounds.top = inStream.ReadInt16();
	paintingBounds.left = inStream.ReadInt16();
	paintingBounds.bottom = inStream.ReadInt16();
	paintingBounds.right = inStream.ReadInt16();
	outOverallSize.h = paintingBounds.Width();
	outOverallSize.v = paintingBounds.Height();

	inStream.ReadInt32(); // the PICT handle size
	::NPainter_FromPICT(inStream, outPixmap);

	size_t paneCount = inStream.ReadInt16();
	for (size_t i = 0; i < paneCount; ++i)
		{
		unsigned long paneType = inStream.ReadUInt32();

		// Get the pane's bounds
		ZRect paneBounds;
		paneBounds.top = inStream.ReadInt16();
		paneBounds.left = inStream.ReadInt16();
		paneBounds.bottom = inStream.ReadInt16();
		paneBounds.right = inStream.ReadInt16();

		// Compensate for text that seems to have had an origin including the bounding box
		paneBounds += ZPoint(4, 0);

		// And compensate for the painting's overall offset
		paneBounds -= paintingBounds.TopLeft();

		// we shouldn't need the type or tool
		inStream.Skip(6);
		if (paneType == ZFOURCC('H','I','L','T'))
			{
			// Read and discard hilite panes
			ZRGBColor hiliteColor;
			hiliteColor.red = inStream.ReadInt16();
			hiliteColor.green = inStream.ReadInt16();
			hiliteColor.blue = inStream.ReadInt16();
			}
		else if (paneType == ZFOURCC('T','E','X','T'))
			{
			string textString;
			ZString::sFromStream(textString, inStream);

			// Set up default values in case we have an empty style scrap record
			ZRGBColor theTextColor = ZRGBColor::sBlack;
			ZDCFont theTextFont = ZDCFont::sApp9;
			long stylesSize	= inStream.ReadInt32();
			if (stylesSize > 0)
				{
				short styleCount = inStream.ReadInt16();
				for (short x = 0; x < styleCount; ++x)
					{
					// scrpStartChar. Offset of first character the style applies to
					inStream.ReadInt32();

					// scrpHeight. Line height
					inStream.ReadInt16();

					// scrpAscent. Line ascent
					inStream.ReadInt16();

					// scrpFont
					short fontID = inStream.ReadInt16();

					// scrpFont
					short fontStyle = inStream.ReadInt16();

					// scrpSize
					short fontSize = inStream.ReadInt16();

					ZRGBColor tempColor;
					tempColor.red = inStream.ReadUInt16();
					tempColor.green = inStream.ReadUInt16();
					tempColor.blue = inStream.ReadUInt16();

					if (x == 0)
						{
						theTextFont.SetSize(fontSize);

						// Styles are in the high byte in a ScrpSTElement
						theTextFont.SetStyle(fontStyle >> 8);
						#if (ZCONFIG_OS_MacOS7 == ZCONFIG_OS) || (ZCONFIG_OS_Carbon == ZCONFIG_OS)
							Str255 fontName;
							::GetFontName(fontID, fontName);
							::GetFNum(fontName, &fontID);
							if (fontID == 0)
								theTextFont.SetFontID(ZDCFont::sApp9.GetFontID());
							else
								theTextFont.SetFontID(fontID);
						#else
						#endif
						}
					}
				}
			short alignment = inStream.ReadInt16(); // ignored for now

			outBounds.push_back(paneBounds);
			outStrings.push_back(textString);
			outColors.push_back(theTextColor);
			outFonts.push_back(theTextFont);
			} 
		}
	}

// ==================================================
#pragma mark -
#pragma mark * NPaintDataRep

NPaintDataRep::NPaintDataRep()
	{
	fOffset = ZPoint::sZero;
	}

NPaintDataRep::NPaintDataRep(const NPaintDataRep* inOther)
:	fRegion(inOther->fRegion),
	fOffset(inOther->fOffset),
	fPixmap(inOther->fPixmap),
	fStrings(inOther->fStrings), 
	fFonts(inOther->fFonts),
	fBounds(inOther->fBounds),
	fColors(inOther->fColors)
	{}

NPaintDataRep::NPaintDataRep(const ZDCPixmap& iPixmap)
:	fPixmap(iPixmap)
	{
	fRegion = ZRect(fPixmap.Size());
	fOffset = ZPoint::sZero;
	}

NPaintDataRep::NPaintDataRep(ZPoint inSize)
:	fPixmap(inSize, ZDCPixmapNS::eFormatEfficient_Color_24, ZRGBColor::sWhite),
	fOffset(0, 0),
	fRegion(inSize)
	{}

NPaintDataRep::NPaintDataRep(const ZRect& inMaxBounds, const string& inString)
	{
	ZDC localDC(ZDCScratch::sGet());
	localDC.SetFont(ZDCFont::sApp9);
	ZPoint theSize = ZTextUtil::sCalcSize(localDC, inMaxBounds.Width(), inString);
	ZRect theBounds(theSize);
	theBounds = theBounds.Centered(inMaxBounds);
	fStrings.push_back(inString);
	fFonts.push_back(ZDCFont::sApp9);
	fColors.push_back(ZRGBColor::sBlack);
	fBounds.push_back(theBounds);
	fOffset = ZPoint::sZero;
	}

NPaintDataRep::~NPaintDataRep()
	{}

void NPaintDataRep::FromStream_Old(const ZStreamR& inStream)
	{
	ZPoint newSize = ZPoint::sZero;
	// Reset our data to initially constructed condition
	fPixmap = ZDCPixmap();
	fOffset = ZPoint::sZero;
	fRegion = ZDCRgn();
	fStrings.clear();
	fBounds.clear();
	fFonts.clear();
	fColors.clear();
	try
		{
		inStream.ReadInt32(); // Ignored -- blob size
		inStream.ReadInt32(); // Magic number 'PNTR'
		long version = inStream.ReadInt32();
		if (version != 2)
			throw runtime_error("NPaintDataRep::FromStream_Old, bad version");

		// This is version 2.x data
		::sReadOldPainting(inStream, newSize, fPixmap, fStrings, fFonts, fBounds, fColors);
		fRegion = ZRect(newSize);
		}
	catch (...)
		{
		// An exception ocurred, reset any data we we're able to read.
		fPixmap = ZDCPixmap();
		fStrings.clear();
		fBounds.clear();
		fFonts.clear();
		fColors.clear();
		}
	}

void NPaintDataRep::FromStream(const ZStreamR& inStream)
	{
	// Reset our data to initially constructed condition
	fPixmap = ZDCPixmap();
	fOffset = ZPoint::sZero;
	fRegion = ZDCRgn();
	fStrings.clear();
	fBounds.clear();
	fFonts.clear();
	fColors.clear();
	try
		{
		inStream.ReadInt32(); // Ignored -- version number of some kind?
		inStream.ReadInt32(); // Ignored -- blob size
		if (ZFOURCC('P','N','T','R') != inStream.ReadInt32())
			throw runtime_error("NPaintDataRep::FromStream, PNTR magic number missing");

		long version = inStream.ReadInt32();
		if (version == 2)
			{
			// This is version 2.x data
			ZPoint newSize;
			::sReadOldPainting(inStream, newSize, fPixmap, fStrings, fFonts, fBounds, fColors);
			fRegion = ZRect(newSize);
			}
		else if (version == 3 || version == 4 || version == 5)
			{
			if (version < 5)
				{
				ZPoint overallSize;
				overallSize.h = inStream.ReadInt16();
				overallSize.v = inStream.ReadInt16();
				fRegion = ZRect(overallSize);
				}
			else
				{
				ZRect bounds;
				bounds.left = inStream.ReadInt16();
				bounds.top = inStream.ReadInt16();
				bounds.right = inStream.ReadInt16();
				bounds.bottom = inStream.ReadInt16();
				fRegion = bounds;
				}

			size_t stringCount = inStream.ReadUInt32();
			for (size_t x = 0; x < stringCount; ++x)
				{
				string theString;
				ZString::sFromStream(theString, inStream);

				ZDCFont theFont;
				if (version >= 4)
					{
					// Prior to version 4 we did not actually write the
					// font to the stream.
					theFont.FromStream(inStream);
					}

				ZRect theBounds;
				theBounds.left = inStream.ReadInt16();
				theBounds.top = inStream.ReadInt16();
				theBounds.right = inStream.ReadInt16();
				theBounds.bottom = inStream.ReadInt16();

				ZRGBColor theColor;
				theColor.red = inStream.ReadUInt16();
				theColor.green = inStream.ReadUInt16();
				theColor.blue = inStream.ReadUInt16();

				fStrings.push_back(theString);
				fFonts.push_back(theFont);
				fBounds.push_back(theBounds);
				fColors.push_back(theColor);
				}

			// Now the pixmap
			unsigned long format = inStream.ReadUInt32();
			unsigned long pictureDataSize = inStream.ReadUInt32();

			if (false)
				{}
			else if (format == ZFOURCC('B','M','P',' '))
				{
				fPixmap = ZDCPixmapDecoder_BMP::sReadPixmap(inStream, false);
				}
			#if ZCONFIG_SPI_Enabled(libpng)
			else if (format == ZFOURCC('P','N','G',' '))
				{
				fPixmap = ZDCPixmapDecoder_PNG::sReadPixmap(inStream);
				}
			#endif
			else
				{
				throw runtime_error("NPaintDataRep::FromStream, unrecognized pixmap format");
				}
			}
		else
			{
			throw runtime_error("NPaintDataRep::FromStream, PNTR format is too new");
			}
		}
	catch (...)
		{
		// An exception ocurred, reset any data we were able to read.
		fPixmap = ZDCPixmap();
		fStrings.clear();
		fBounds.clear();
		fFonts.clear();
		fColors.clear();
		}
	}

void NPaintDataRep::ToStream(const ZStreamW& inStream)
	{
	inStream.WriteInt32(0); // Legacy cruft
	inStream.WriteInt32(0); // Legacy cruft
	inStream.WriteUInt32(ZFOURCC('P','N','T','R')); // Magic number

	inStream.WriteInt32(5); // Version number

	const ZRect& regionBounds = fRegion.Bounds();
	inStream.WriteInt16(regionBounds.left);
	inStream.WriteInt16(regionBounds.top);
	inStream.WriteInt16(regionBounds.right);
	inStream.WriteInt16(regionBounds.bottom);

	inStream.WriteUInt32(fStrings.size()); // Count of text blocks

	for (size_t x = 0; x < fStrings.size(); ++x)
		{
		ZString::sToStream(fStrings[x], inStream);

		fFonts[x].ToStream(inStream);

		inStream.WriteInt16(fBounds[x].left);
		inStream.WriteInt16(fBounds[x].top);
		inStream.WriteInt16(fBounds[x].right);
		inStream.WriteInt16(fBounds[x].bottom);

		inStream.WriteUInt16(fColors[x].red);
		inStream.WriteUInt16(fColors[x].green);
		inStream.WriteUInt16(fColors[x].blue);
		}

	// Write the picture's streamed format

	uint32 theTag = 0;
	std::auto_ptr<ZDCPixmapEncoder> theEncoder;
	#if ZCONFIG_SPI_Enabled(libpng)
		theTag = ZFOURCC('P','N','G',' ');
		theEncoder.reset(new ZDCPixmapEncoder_PNG);
	#else
		theTag = ZFOURCC('B','M','P',' ');
		theEncoder.reset(new ZDCPixmapEncoder_BMP(false));
	#endif

	inStream.WriteUInt32(theTag);

	// Figure out how big the picture is going to be. 
	ZStreamW_Count theStreamCount;
	theEncoder->Write(theStreamCount, fPixmap);
	
	// Write that size to the stream. If you want to forgo this step, write a size of 0xFFFFFFFF.
	// However that does mean that anyone reading this stream is gonna have to be able to
	// interpet the data in order to be able to skip it.
	inStream.WriteUInt32(theStreamCount.GetCount());

	// And now actually write it out
	theEncoder->Write(inStream, fPixmap);
	}

void NPaintDataRep::Draw(const ZDC& inDC, ZPoint inLocation)
	{
	ZDC localDC = inDC;
	localDC.OffsetOrigin(inLocation);
	ZDCRgn oldClip = localDC.GetClip();
	ZDCRgn pixmapRegion = fRegion + fOffset;
	localDC.SetClip(oldClip & pixmapRegion);
	localDC.Draw(fOffset, fPixmap);

	localDC.SetInk(ZRGBColor::sWhite);
 	for (size_t x = 0; x < fStrings.size(); ++x)
	  	{
  		localDC.SetFont(fFonts[x]);
  		localDC.SetTextColor(fColors[x]);
  		ZDCRgn textRegion = fBounds[x];
  		localDC.SetClip(oldClip & textRegion);
  		// If the text extends beyond the pixmap's bounds,
  		// we fill that area with white.
  		ZDCRgn nonOverlapped = textRegion - pixmapRegion;
		localDC.Fill(nonOverlapped);
		ZTextUtil::sDraw(localDC, fBounds[x].TopLeft(), fBounds[x].Width(), fStrings[x]);
  		}
	}

void NPaintDataRep::DrawPixmap(const ZDC& inDC, ZPoint inLocation)
	{
	ZDC localDC(inDC);
	localDC.Draw(inLocation, fPixmap);
	}

bool NPaintDataRep::HasData()
	{
	return fPixmap;
	}

ZPoint NPaintDataRep::GetSize()
	{
	return fRegion.Bounds().BottomRight() + fOffset;
	}

void NPaintDataRep::FromStream_BMP(const ZStreamR& inStream)
	{
	fPixmap = ZDCPixmapDecoder_BMP::sReadPixmap(inStream, false);
	fRegion = ZRect(fPixmap.Size());
	fOffset = ZPoint::sZero;
	}

void NPaintDataRep::ToStream_BMP(const ZStreamW& inStream)
	{
	ZDCPixmapEncoder_BMP::sWritePixmap(inStream, fPixmap, false);
	}

#if ZCONFIG(API_Graphics, QD)
void NPaintDataRep::FromStream_PICT(const ZStreamR& inStream)
	{
	fPixmap = ZUtil_Mac_HL::sPixmapFromStreamPICT(inStream);
	fRegion = ZRect(fPixmap.Size());
	fOffset = ZPoint::sZero;
	}

void NPaintDataRep::ToStream_PICT(const ZStreamW& inStream)
	{
	// Figure out what boundary we're going to need
	ZRect bounds = fRegion.Bounds() + fOffset;
	for (size_t x = 0; x < fBounds.size(); ++x)
		bounds |= fBounds[x];
	ZDC_PICT theDC(bounds, inStream);
	this->Draw(theDC, ZPoint::sZero);
	}
#endif // ZCONFIG(API_Graphics, QD)

const ZDCPixmap& NPaintDataRep::GetPixmap()
	{ return fPixmap; }
