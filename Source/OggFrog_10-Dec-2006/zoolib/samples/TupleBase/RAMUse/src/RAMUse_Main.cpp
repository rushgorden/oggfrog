#include "ZCommandLine.h"
#include "ZFile.h"
#include "ZMain.h"
#include "ZStdIO.h"
#include "ZTS_RAM.h"
#include "ZTupleIndex_General.h"
#include "ZTupleIndex_String.h"
#include "ZUtil_Strim.h"
#include "ZUtil_TS.h"
#include "ZUtil_Tuple.h"

// ====================================================================================================
#pragma mark -
#pragma mark *

static const char* const sPropNames[] = {
// Top level names:
	"Link",
	"Object",
	"State",
	"SubType",
	"Type",
	"atk",
	"authors",
	"body",
	"card",
	"clearForward",
	"code",
	"crea",
	"criteria",
	"data",
	"databaseID",
	"display author",
	"display title",
	"emai",
	"end_",
	"ephemeral",
	"file",
	"flag",
	"fnam",
	"frmt",
	"from",
	"frst",
	"fsiz",
	"hdht",
	"hdsb",
	"height",
	"hint",
	"icon size",
	"java",
	"last",
	"level",
	"lnam",
	"lndx",
	"loca",
	"location",
	"lock",
	"mime",
	"mode",
	"modi",
	"next",
	"notification",
	"objects",
	"offsets",
	"ordi",
	"org",
	"pass",
	"period",
	"perm",
	"play",
	"progress",
	"prop",
	"prsd",
	"quotation",
	"rate",
	"realm",
	"researcher",
	"revi",
	"scale-x",
	"scale-y",
	"sha1",
	"stat",
	"strt",
	"styles",
	"subtype",
	"tag!",
	"text",
	"thum",
	"titl",
	"to",
	"to__",
	"trgt",
	"type",
	"unam",
	"userFileName",
	"userID",
	"viewID",
	"viewToFront",
	"viewsidebar",
	"width",
	"window",
	"writers can",
	"xml_",
// Other names:
	"attachment",
	"attachments",
	"browsertimezone",
	"count",
	"create scaffolds",
	"create views",
	"enabled",
	"format",
	"lastANMDate",
	"layout",
	"lock views",
	"mailtype",
	"movies",
	"name",
	"noteID",
	"notes",
	"num_neighbors",
	"publish views",
	"screenheight",
	"screenwidth",
	"sendempty",
	"status",
	"then",
	"timezonedelta",
	"version",
	"views"
	};

static const int fred = ZTuplePropName::sPreRegister(sPropNames, countof(sPropNames));

// =================================================================================================
#pragma mark -
#pragma mark * sGetUsage, sDumpUsage

#ifdef linux
#include <malloc.h>

static int sGetUsage()
	{
	struct mallinfo theStats = mallinfo();
	return theStats.uordblks + theStats.usmblks;
//	return theStats.uordblks - theStats.fordblks;
	}

#else

#include <stdlib.h>
#include <malloc/malloc.h>

static size_t sGetUsage()
	{
	malloc_zone_t* theZone = malloc_default_zone();
	malloc_statistics_t theStats;
	theZone->introspect->statistics(theZone, &theStats);
	return theStats.size_in_use;
	}

#endif

#if ZCONFIG_Tuple_SharedPropNames
size_t PropName_GetCount();
static void sDumpUsage(const string& iName, const ZStrimW& s)
	{
	size_t usage = sGetUsage();
	size_t propNameCount = ::PropName_GetCount();

	s << iName << ", ";
	s.Writef("propNameCount: %d, memory in use: %d\n",
		propNameCount, usage);
	}
#else

static void sDumpUsage(const string& iName, const ZStrimW& s)
	{
	s << iName << ", ";
	s.Writef("memory in use: %d\n", sGetUsage());
	}
#endif

// =================================================================================================
#pragma mark -
#pragma mark *

ZRef<ZTupleIndexFactory> sCreateFactory(const ZTupleValue& iTV)
	{
	if (iTV.TypeOf() == eZType_Vector)
		{
		vector<string> thePropNames;
		iTV.GetVector_T(back_inserter(thePropNames), string());
		if (!thePropNames.empty())
			return new ZTupleIndexFactory_General(thePropNames);
		}
	else if (iTV.TypeOf() == eZType_String)
		{
		return new ZTupleIndexFactory_String(iTV.GetString());
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * CommandLine

namespace ZANONYMOUS {
class CommandLine : public ZCommandLine
	{
public:
	String fFile;
	TupleValue fIndices;

	CommandLine()
	:	fFile("-f", "Tuplestore file to open"),
		fIndices("-i", "Indices to create, in tuple syntax", "[]")
		{}
	};
} // anonymous namespace

// =================================================================================================
#pragma mark -
#pragma mark * ZMain

int ZMain(int argc, char **argv)
	{
	const ZStrimW& serr = ZStdIO::strim_err;
	const ZStrimW& sout = ZStdIO::strim_out;

	CommandLine cmd;
	if (!cmd.Parse(serr, argc, argv))
		{
		serr << "Usage: " << argv[0] << " " << cmd << "\n";
		return 1;
		}

	ZRef<ZStreamerRPos> theStreamerRPos = ZFileSpec(cmd.fFile()).OpenRPos();
	if (!theStreamerRPos)
		{
		serr << "Couldn't open '" << argv[1] << "'\n";
		return 1;
		}

	sDumpUsage("BeforeRead", sout);

	uint64 nextUnusedID;
	map<uint64, ZTuple> theTuples;
	ZUtil_TS::Sink_Map theSink(theTuples);
	try
		{
		sRead(theStreamerRPos->GetStreamRPos(), nextUnusedID, theSink);
		}
	catch (std::exception& ex)
		{
		serr << ex.what();
		return 1;
		}

	sout.Writef("Read %d tuples\n", theTuples.size());
	sout.Writef("sizeof(ZTupleValue): %d\n", sizeof(ZTupleValue));

	sDumpUsage("BeforeTSRAM", sout);

	ZRef<ZTS_RAM> theTS_RAM = new ZTS_RAM(vector<ZRef<ZTupleIndexFactory> >(), nextUnusedID, theTuples, true);

	sDumpUsage("BeforeIndices", sout);

	const vector<ZTupleValue>& vec = cmd.fIndices().GetVector();
	for (vector<ZTupleValue>::const_iterator i = vec.begin(); i != vec.end(); ++i)
		{
		if (ZRef<ZTupleIndexFactory> theFactory = sCreateFactory(*i))
			{
			sout << "Adding factory for " << *i;
			theTS_RAM->AddIndex(theFactory);
			sDumpUsage("", sout);
			}
		}

	return 0;
	}
