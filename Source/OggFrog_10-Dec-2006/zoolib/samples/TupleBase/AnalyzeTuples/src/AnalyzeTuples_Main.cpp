#include "ZFile.h"
#include "ZMain.h"
#include "ZStdIO.h"
#include "ZStream_Buffered.h"
#include "ZUtil_Strim.h"
#include "ZUtil_TS.h"

#include "AnalyzeTuples.h"

// =================================================================================================
#pragma mark -
#pragma mark * Sink_AT

class Sink_AT : public ZUtil_TS::Sink
	{
public:
	Sink_AT(AnalyzeTuples& iAT);

// From Sink
	virtual bool Set(uint64 iID, const ZTuple& iTuple);
	virtual void Clear();

private:
	AnalyzeTuples& fAT;
	set<uint64> fIDs;
	};

Sink_AT::Sink_AT(AnalyzeTuples& iAT)
:	fAT(iAT)
	{}

bool Sink_AT::Set(uint64 iID, const ZTuple& iTuple)
	{
	if (fIDs.insert(iID).second)
		{
		fAT.AddOne(iTuple);
		return true;
		}
	return false;
	}

void Sink_AT::Clear()
	{
	fAT.Clear();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMain

int ZMain(int argc, char **argv)
	{
	const ZStrimW& serr = ZStdIO::strim_err;
	const ZStrimW& sout = ZStdIO::strim_out;

	if (argc < 2)
		{
		serr << "Usage: " << argv[0] << " TuplestoreFileName\n";
		return 1;
		}
		

	ZRef<ZStreamerRPos> theStreamerRPos = ZFileSpec(argv[1]).OpenRPos();
	if (!theStreamerRPos)
		{
		serr << "Couldn't open '" << argv[1] << "'\n";
		return 1;
		}

	AnalyzeTuples theAT;
	Sink_AT theSink(theAT);
	uint64 nextUnusedID;
	try
		{
		ZUtil_TS::sRead(theStreamerRPos->GetStreamRPos(), nextUnusedID, theSink);
		theAT.Emit(sout);
		}
	catch (std::exception& ex)
		{
		serr << ex.what();
		}

	return 0;
	}
