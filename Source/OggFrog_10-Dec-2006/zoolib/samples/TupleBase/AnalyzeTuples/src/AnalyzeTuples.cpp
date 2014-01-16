#include "AnalyzeTuples.h"
#include "ZStrim.h"

#include <algorithm>

static inline size_t sRoundUp(size_t iSize, size_t iRound)
	{
	return iRound * ((iSize + iRound-1) / iRound);
	}

static inline size_t sStringSizeUsed(size_t iSize)
	{
	const size_t kAlignment = ZCONFIG(Processor, PPC) ? 16 : 4;

	if (iSize <= 11)
		return 12 + 4;

	// 12 bytes of tuplevalue, plus 4 bytes of propname.
	return 12 + 4 + sRoundUp(iSize, kAlignment);
	}

AnalyzeTuples::AnalyzeTuples()
	{
	fTotalTuples = 0;
	fTotalProperties = 0;
	fTotalStrings = 0;
	fTotalStringSpace = 0;	
	fTotalStringSpaceUsed = 0;
	}

void AnalyzeTuples::Clear()
	{
	fTotalTuples = 0;
	fTotalProperties = 0;
	fTotalStrings = 0;
	fTotalStringSpace = 0;	
	fTotalStringSpaceUsed = 0;

	fPropertyCounts.clear();
	fPropertyNames.clear();
	fTypeCounts.clear();
	fStringCounts.clear();
	fStringSizes.clear();
	}

static void sAddAllPropNames(const ZTuple& iTuple, set<string>& ioPropNames)
	{
	for (ZTuple::const_iterator i = iTuple.begin(); i != iTuple.end(); ++i)
		{
		ioPropNames.insert(iTuple.NameOf(i));
		if (eZType_Tuple == iTuple.TypeOf(i))
			sAddAllPropNames(iTuple.GetTuple(i), ioPropNames);
		}
	}

void AnalyzeTuples::AddOne(const ZTuple& iTuple)
	{
	++fTotalTuples;
	++fPropertyCounts[iTuple.Count()];

	sAddAllPropNames(iTuple, fAllPropertyNames);

	for (ZTuple::const_iterator j = iTuple.begin(); j != iTuple.end(); ++j)
		{
		++fTotalProperties;
		++fPropertyNames[iTuple.NameOf(j)];

		ZType theType = iTuple.TypeOf(j);
		++fTypeCounts[theType];

		if (theType == eZType_String)
			{
			++fTotalStrings;

			const string& theString = iTuple.GetString(j);
			++fStringCounts[theString];

			size_t theSize = theString.size();
			fTotalStringSpace += theSize;
			++fStringSizes[theSize];

			fTotalStringSpaceUsed += sStringSizeUsed(theSize);
			}
		}
	}

template <class P1, class P2>
struct ComparePairsBySecond_T
	{
	ComparePairsBySecond_T(bool iAscending = true) { fAscending = iAscending; }
	
	bool operator()(const pair<P1, P2> first, const pair<P1, P2> second) const
		{
		if (fAscending)
			return first.second < second.second; 
		else
			return first.second > second.second; 
		}
		
	typedef pair<P1, P2> first_argument_type;
	typedef pair<P1, P2> second_argument_type;
	typedef bool result_type;
	bool fAscending;
	};

template <class K, class C>
void sSorted_T(const map<K, C>& iMap, vector<pair<K, C> >& oVector)
	{
	for (typename map<K, C>::const_iterator i = iMap.begin(); i != iMap.end(); ++i)
		oVector.push_back(*i);
	sort(oVector.begin(), oVector.end(), ComparePairsBySecond_T<K, C>(false));
	}

void AnalyzeTuples::Emit(const ZStrimW& s)  const
	{
	s << "\n----------------------------------------\n";
	s.Writef("total number of tuples: %d\n", fTotalTuples);
	s.Writef("total number of properties: %d\n", fTotalProperties);
	s.Writef("propertyCounts.size: %d\n", fPropertyCounts.size());
	s.Writef("propertyNames.size: %d\n", fPropertyNames.size());
	s.Writef("typeCounts.size: %d\n", fTypeCounts.size());
	s.Writef("stringCounts.size: %d\n", fStringCounts.size());
	s.Writef("stringSizes.size: %d\n", fStringSizes.size());
	s.Writef("number of strings: %d\n", fTotalStrings);
	s.Writef("string space: %d\n", fTotalStringSpace);

	s << "----------------------------------------\nCount of properties/tuple\n";
	vector<pair<size_t, size_t> > propertyCountsV;
	sSorted_T(fPropertyCounts, propertyCountsV);
	s << "  #Props   Count\n";
	for (vector<pair<size_t, size_t> >::const_iterator i = propertyCountsV.begin(); i != propertyCountsV.end(); ++i)
		s.Writef("  %6d  %6d\n", (*i).first, (*i).second);

	s << "----------------------------------------\nCount of each type\n";
	vector<pair<ZType, size_t> > typeCountsV;
	sSorted_T(fTypeCounts, typeCountsV);
	s << "    Type   Count\n";
	for (vector<pair<ZType, size_t> >::const_iterator i = typeCountsV.begin(); i != typeCountsV.end(); ++i)
		{
		string theTypeName = ZTypeAsString((*i).first);
		
		if (theTypeName.size() < 8)
			theTypeName = string(8 - theTypeName.size(), ' ') + theTypeName;
		s.Writef("%s  %6d\n", theTypeName.c_str() , (*i).second);
		}

	s << "----------------------------------------\nProperty names:\n";
	vector<pair<string, size_t> > propertyNamesV;
	sSorted_T(fPropertyNames, propertyNamesV);
	s << "   Count  Name\n";
	for (vector<pair<string, size_t> >::const_iterator i = propertyNamesV.begin(); i != propertyNamesV.end(); ++i)
		s.Writef("  %6d  %s\n", (*i).second, (*i).first.c_str());

	s << "----------------------------------------\nLengths of string properties, sorted by count, up to 512 bytes, 3 or more of that size:\n";
	vector<pair<size_t, size_t> > stringSizesV;
	sSorted_T(fStringSizes, stringSizesV);
	s << "    Size   Count\n";
	for (vector<pair<size_t, size_t> >::const_iterator i = stringSizesV.begin(); i != stringSizesV.end(); ++i)
		{
		if ((*i).first <= 512)
			{
			if ((*i).second >= 3)
				s.Writef("  %6d  %6d\n", (*i).first, (*i).second);
			}
		}



	s << "----------------------------------------\nDetailed string lengths, sorted by size, with real and allocated sizes and cumulative totals:\n";
	//    01234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567
	s << "   rsize   count    rs*c   %rs*c   crs*c  %crs*c  ccount %ccount   asize    as*c   %as*c   cas*c  %cas*c  %waste %cwaste\n";
	size_t ccount = 0;
	size_t crsTc = 0;
	size_t casTc = 0;
	for (map<size_t, size_t>::const_iterator i = fStringSizes.begin(); i != fStringSizes.end(); ++i)
		{
		// The real/raw size of the string -- how many characters does it contain.
		const size_t rsize = (*i).first;

		// How many strings of that length.
		const size_t count = (*i).second;

		// The raw space to store all those strings.
		const size_t rsTc = rsize*count;

		// The percentage of the total space used for those strings.
		const float prsTc = (100.0 * rsTc) / fTotalStringSpace;

		// The cumulative space used for all strings up to this size.
		crsTc += rsTc;

		// And the percentage used for all strings up to this size.
		const float pcrsTc = (100.0 * crsTc) / fTotalStringSpace;

		// Cumulative count of all strings so far.
		ccount += count;

		// Cumulative percentage of the total number of strings
		const float pccount = (100.0 * ccount) / fTotalStrings;

		// Same thing, but this time how much space is consumed for the string
		// and its property name, accounting for internal fragmentation.
		const size_t asize = sStringSizeUsed(rsize);
		const size_t asTc = asize*count;
		const float pasTc = (100.0 * asTc) / fTotalStringSpaceUsed;
		casTc += asTc;
		const float pcasTc = (100.0 * casTc) / fTotalStringSpaceUsed;


		// How much space is 'wasted' -- what number of allocated bytes are not
		// being used to store real text. The +4 is to modify the rsize figure to
		// include the space needed to store the pointer to the property name.
		const float pwaste = (100.0 * (asize-(rsize+4))) / (asize);
		const float pcwaste = (100.0 * (casTc-(crsTc+4*ccount))) / (casTc);

		s.Writef(" %7d %7d %7d  %6.2f %7d  %6.2f %7d  %6.2f %7d %7d  %6.2f %7d  %6.2f  %6.2f  %6.2f\n",
			rsize,
			count,
			rsTc,
			prsTc,
			crsTc,
			pcrsTc,
			ccount,
			pccount,
			asize,
			asTc,
			pasTc,
			casTc,
			pcasTc,
			pwaste,
			pcwaste
			);
		}
	
	s << "----------------------------------------\nString properties, at least 3 repeats, first line of each only:\n";
	vector<pair<string, size_t> > stringCountsV;
	sSorted_T(fStringCounts, stringCountsV);
	s << "   Count  String\n";
	for (vector<pair<string, size_t> >::const_iterator i = stringCountsV.begin(); i != stringCountsV.end(); ++i)
		{
		if ((*i).second >= 3)
			{
			const string& theString = (*i).first;
			size_t theEnd = theString.find_first_of("\n\r");
			s.Writef("  %6d  %s\n", (*i).second, theString.substr(0, theEnd).c_str());
			}
		}

	s << "----------------------------------------\nAll property names, suitable for use with ZTuplePropName::sPreRegister\n";
	s << "// Top level names:\n";
	for (map<string, size_t>::const_iterator i = fPropertyNames.begin(); i != fPropertyNames.end(); ++i)
		s << "\"" << (*i).first << "\",\n";

	s << "// Other names:\n";
	for (set<string>::const_iterator i = fAllPropertyNames.begin(); i != fAllPropertyNames.end(); ++i)
		{
		if (fPropertyNames.end() == fPropertyNames.find(*i))
			s << "\"" << *i << "\",\n";
		}
	}

