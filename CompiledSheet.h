#pragma once

#include <set>
#include <string>
#include <boost/icl/interval_map.hpp>
#include <memory>

struct Source
{
    std::string sourceId;
    std::string path;
};

typedef double Ticks;

struct DocumentEventInfo 
{
    Ticks beginTime = -1;
    Ticks endTime = -1;
    int beginPosition = -1;
    int endPosition = -1;
    unsigned sourceId = 0;
    bool operator<(const DocumentEventInfo &b) const { return this->sourceId == b.sourceId ? (this->beginPosition < b.beginPosition) : (this->sourceId < b.sourceId); }
    bool operator==(const DocumentEventInfo &b) const { return this->sourceId == b.sourceId && this->beginPosition == b.beginPosition; }
};

typedef std::set<DocumentEventInfo> EventPositionSet;
typedef boost::icl::interval_map<Ticks, EventPositionSet> EventTimeline;
typedef boost::icl::interval<Ticks> TicksInterval;
typedef EventTimeline::interval_type TimelineIntervalType;

struct CompiledSheet 
{
    std::vector<Source> sources;
    std::vector<unsigned char> midiData;
    EventTimeline eventInfos;
};
typedef std::shared_ptr<CompiledSheet> CompiledSheetPtr;