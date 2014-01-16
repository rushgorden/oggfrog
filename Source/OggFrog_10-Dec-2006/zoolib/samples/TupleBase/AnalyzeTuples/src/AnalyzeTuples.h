#ifndef __AnalyzeTuples__
#define __AnalyzeTuples__

#include "ZTuple.h"

#include <map>
#include <set>

class ZStrimW;

class AnalyzeTuples
	{
public:
	AnalyzeTuples();

	void Clear();

	void AddOne(const ZTuple& iTuple);
	void Emit(const ZStrimW& s) const;

	size_t fTotalTuples;
	size_t fTotalProperties;
	size_t fTotalStrings;
	size_t fTotalStringSpace;
	size_t fTotalStringSpaceUsed;

	map<size_t, size_t> fPropertyCounts;
	map<string, size_t> fPropertyNames;
	set<string> fAllPropertyNames;
	map<ZType, size_t> fTypeCounts;
	map<string, size_t> fStringCounts;
	map<size_t, size_t> fStringSizes;

	};

#endif // __AnalyzeTuples__
