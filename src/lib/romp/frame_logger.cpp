/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "pch.hpp"

#if ENABLE_DOG_WATCHERS

#include "frame_logger.hpp"
#include "resource_manager_stats.hpp"
#include "cstdmf/profile.hpp"
#include "romp/geometrics.hpp"

#include <strstream>
#include <fstream>
#include <limits>

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )

extern int getfxcount();
extern double getfxtime();

namespace { // anonymous

// Named constants
const char* LOG_FILENAME = "stats.txt";

std::auto_ptr<std::ostream> f_logFile;

double f_logFrameThreshold = std::numeric_limits<float>::max();
double f_percentThreshold  = 0.005;
double f_slowestFrame2Sec  = 0;
bool   f_useCSVFormat      = false;

/**
 *	Utility struct for keeping track of where we are in the DogWatch tree
 */
struct TreeTrack
{
	/// Constructor.
	TreeTrack(
		DogWatchManager::iterator & iA,
		DogWatchManager::iterator & endA,
		uint64 totA ) : i( iA ), end( endA ), tot( totA ), acc( 0 )
	{ }

	DogWatchManager::iterator	i;		///< @todo Comment
	DogWatchManager::iterator	end;	///< @todo Comment
	uint64						tot;	///< @todo Comment
	uint64						acc;	///< @todo Comment
};

struct LogEntry
{
	std::string name;
	uint64      value;
	int         ident;	
};
typedef std::vector<LogEntry> LogEntryVector;

// Helper functions
void tickLogger();
void logFrame(
	double gameTime, double frameTime, const LogEntryVector & logline);

void updateHistogram(double frameTime);
void renderHistogram();
void resetHistogram();

// The mainlook Task
class FrameLoggerTask : public MainLoopTask
{
	virtual void tick(float time)	
	{
		tickLogger();
	}

	virtual void draw()	
	{
		/*
		if (this->enableDraw != this->lastEnableDraw_)
		{
			resetHistogram();
			this->lastEnableDraw_ = this->enableDraw;
		}
		renderHistogram();
		*/
	}
	
	bool lastEnableDraw_;
};
std::auto_ptr<FrameLoggerTask> f_frameLoogerInstance;

} // namespace anonymous

//---------------------------------------------
// Section : FrameLogger
//---------------------------------------------

void FrameLogger::init()
{
	MF_WATCH( "Debug/Frame Logger/Frame time threshold",
		f_logFrameThreshold, Watcher::WT_READ_WRITE,
		"If a frame takes longer than this value (in seconds) to complete, "
		"a log file will be created with the contets of all DogWatchers. "
		"This can be useful to debug hicups in the frame rate" );

	MF_WATCH( "Debug/Frame Logger/Slowest frame",
		f_slowestFrame2Sec, Watcher::WT_READ_WRITE,
		"Shows the duration (in seconds) of the frame that "
		"took the longest to complete in the last two seconds." );
		
	MF_WATCH( "Debug/Frame Logger/Slice cull threshold",
		f_percentThreshold, Watcher::WT_READ_WRITE,
		"Minimun percentage of a frame a DogWatcher slice must take for "
		"it to be recorded into the log file (in non CSV format only)." );		
	
	MF_WATCH( "Debug/Frame Logger/Use CSV Format",
		f_useCSVFormat, Watcher::WT_READ_WRITE,
		"Set it to true if you want the log to be saved in CSV format. "
		"Set it to false if you prefer a treelike format." );
	
	f_frameLoogerInstance.reset(new FrameLoggerTask);
	MainLoopTasks::root().add(
		f_frameLoogerInstance.get(), 
		"Debug/Frame Logger", ">App", NULL ); 
}

namespace { // anonymous

// Helper functions
void tickLogger()
{
	DogWatchManager::iterator dwmBegin = DogWatchManager::instance().begin();

	double fAcc = 0;
	double secsPerStamp = 1.0 / stampsPerSecondD();
	f_slowestFrame2Sec = 0;
	for (int period = 1; period < DogWatchManager::NUM_SLICES && fAcc < 2; period++)
	{
		double time = dwmBegin.value( period ) * secsPerStamp;
		f_slowestFrame2Sec = std::max(f_slowestFrame2Sec, time);
		fAcc += time;
	}
	
	double frameTime = dwmBegin.value( 1 ) / stampsPerSecondD();

	static double gameTime = 0;
	gameTime += frameTime;

	if (frameTime > f_logFrameThreshold)
	{
		LogEntryVector logline;
	
		if (f_logFile.get() == NULL)
		{
			f_logFile.reset(new std::ofstream( LOG_FILENAME ));
			(*f_logFile).setf(std::ios_base::fixed, std::ios_base::floatfield);
			(*f_logFile).fill(' ');
		}

		DogWatchManager::iterator dwmEnd = DogWatchManager::instance().end();
		TreeTrack treeTrack( dwmBegin, dwmEnd, (uint64)0 );

		std::vector<TreeTrack> stack;
		stack.reserve(10000);
		stack.push_back( treeTrack );
		
		// gather frame data
		int identLevel = 0;
		while (!stack.empty())
		{
			int iIndex = stack.size()-1;
	
			uint64 value = 0;
			std::string name = "";
			TreeTrack & i = stack.back();
			if (i.i == i.end)
			{
				if (i.acc != 0 && i.tot != 0)
				{
					if (i.acc > i.tot) 
					{
						i.acc = i.tot;
					}
					value = i.tot - i.acc;
					name = "    <remainder>";
				}
				stack.pop_back();
				--identLevel;
				MF_ASSERT(identLevel >= -1);
			}
			else
			{
				value = i.i.average( 1, 1 );			
				
				// fix up the stack
				DogWatchManager::iterator iiBegin = i.i.begin();
				DogWatchManager::iterator iiEnd = i.i.end();
				stack.push_back( TreeTrack( iiBegin, iiEnd, value ) );
				name = i.i.name();
				i.acc += value;
				++identLevel;
				++i.i;
			}
			
			if (!name.empty())
			{
				LogEntry entry;
				entry.name = name;
				entry.value = value;
				entry.ident = identLevel;
				logline.push_back(entry);
			}
		}
		
		logFrame(gameTime, frameTime, logline);
	}

	// updateHistogram(frameTime);
}


void logFrame(double gameTime, double frameTime, const LogEntryVector & logline)
{
	// log frame header
	static int lastSize = -1;
	if (f_useCSVFormat && logline.size() != lastSize)
	{
		(*f_logFile) << std::endl;
		LogEntryVector::const_iterator entryIt = logline.begin();
		LogEntryVector::const_iterator entryEnd = logline.end();
		while (entryIt != entryEnd)
		{
			(*f_logFile) << entryIt->name << ";";
			++entryIt;
		}
		lastSize = logline.size();
		(*f_logFile) << std::endl;
	}
	
	// log this frame
	LogEntryVector::const_iterator entryIt = logline.begin();
	LogEntryVector::const_iterator entryEnd = logline.end();
	while (entryIt != entryEnd)
	{
		static double secondsPerStamp = 1.0 / stampsPerSecondD();
		double secondsDouble = ((double)entryIt->value) * secondsPerStamp;
		double percent = secondsDouble/frameTime;
		if (f_useCSVFormat)
		{
			(*f_logFile).precision(5);
			(*f_logFile) << secondsDouble * 1000 << ";";
		}
		else
		{
			if (percent >= f_percentThreshold)
			{
				(*f_logFile).width(5);
				(*f_logFile).precision(2);
				(*f_logFile) << percent * 100;
				(*f_logFile) << "% (";
				(*f_logFile).width(10);
				(*f_logFile).precision(5);
				(*f_logFile) << secondsDouble * 1000 << " ms) : " ;
				(*f_logFile) << std::string(entryIt->ident*2, ' ') + entryIt->name;
				(*f_logFile) << std::endl;
			}
		}
		++entryIt;
	}

	// log directX resource manager statistics
	if (ResourceManagerStats::instance().enabled())	
	{
		ResourceManagerStats::instance().logStatistics( *f_logFile );
	}

	// finish frame
	if (f_useCSVFormat)
	{
		(*f_logFile) << std::endl;
	}
	else
	{
		(*f_logFile) << "---===<<< frame: ";
		(*f_logFile) << gameTime << " >>>===---" << std::endl << std::endl;
	}
	f_logFile->flush();				
}

typedef std::vector<int> IntVector;
IntVector s_histogram(20, 0);
float s_minFrameTimeMs = 75;
float s_maxFrameTimeMs = 300;

void updateHistogram(double frameTime)
{
	int frameTimeIdx = int(
		s_histogram.size() * 
			(float(frameTime) * 1000 - s_minFrameTimeMs) / 
			(s_maxFrameTimeMs - s_minFrameTimeMs));
		
	if (frameTimeIdx >= 0 && frameTimeIdx < int(s_histogram.size()))
	{
		s_histogram[frameTimeIdx] += 1;
	}
}

void renderHistogram()
{
	FontPtr font = FontManager::instance().get( "system_small.font" );
	float screenWidth = Moo::rc().screenWidth();
	float screenHeight = Moo::rc().screenHeight();
	
	Moo::rc().push();
	Moo::rc().world(Matrix::identity);
	Moo::rc().view( Matrix::identity );
	Moo::rc().projection( Matrix::identity );

	IntVector::const_iterator begin = s_histogram.begin();
	IntVector::const_iterator it    = s_histogram.begin();
	IntVector::const_iterator end   = s_histogram.end();
	
	static int s_maxValue;
	
	static const float barwidth = 0.05f;
	static const float zoom     = 1.5f;
	static const float y0       = - zoom / 2.0f;
	const float scale           = float(s_maxValue);
	float totalSlowFrames       = 0;
	s_maxValue  = 1;
	while (it != end)
	{
		int value = *it;
		s_maxValue = std::max(s_maxValue, value);
	
		int index = std::distance(begin, it);
		float x = (zoom * index / float(s_histogram.size())) + y0;
		float y = (zoom * value / scale) + y0;
		Vector3 p1(x, y0, 0);
		Vector3 p2(x, y, 0);

		Moo::Material::setVertexColour();
		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
		//Vector2 p2d1(p1.x - barwidth/2.0f, p1.y);
		//Vector2 p2d2(p2.x + barwidth/2.0f, p2.y);
		//Geometrics::drawRect(p2d1, p2d2, Moo::Colour(1, 0, 0, 1));
		Geometrics::drawLine(p1, p2, Moo::Colour(1, 0, 0, 1));

		float height = font->metrics().clipHeight();
		FontManager::instance().setMaterialActive( font );
		
		int interval = int((s_maxFrameTimeMs - s_minFrameTimeMs) / s_histogram.size());
		int min      = int((index * interval) + s_minFrameTimeMs);
		int max      = min + interval;
		
		std::stringstream minstr;
		minstr << min;
		float minwidth = font->metrics().stringWidth(minstr.str()) / screenWidth;
		font->draw3DString(minstr.str(), p1+Vector3(-minwidth, 0,0));
		
		std::stringstream maxstr;
		maxstr << max;
		float maxwidth = font->metrics().stringWidth(maxstr.str()) / screenWidth;
		font->draw3DString(maxstr.str(), p1+Vector3(-maxwidth, -height,0));

		std::stringstream valuestr;
		valuestr << value;
		float valuewidth = font->metrics().stringWidth(valuestr.str()) / screenWidth;
		font->draw3DString(valuestr.str(), p2+Vector3(-valuewidth, +height, 0));

		totalSlowFrames += value * (min + max)/2.0f;
		++it;
	}
	std::stringstream totalstr;
	totalstr << "Total slow frame time:" << totalSlowFrames << " ms";
	float totalwidth = font->metrics().stringWidth(totalstr.str()) / screenWidth;
	font->draw3DString(totalstr.str(), Vector3(-totalwidth, 0, 0));

	Moo::rc().pop();
}


void resetHistogram()
{
	std::fill(s_histogram.begin(), s_histogram.end(), 0);
}

} // namespace anonymous
// frame_logger.cpp

#endif // ENABLE_DOG_WATCHERS