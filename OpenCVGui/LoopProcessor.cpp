
#include "stdafx.h"

// Boost
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

#include "LoopProcessor.h"

namespace openCVGui
{
    // Keep a vector of FrameProcessors and call each in turn to crunch images
    // (or perform other work)
    // States: Stop, Pause (can only Step if Paused), and Run

    LoopProcessor::LoopProcessor(const std::string name, bool abortOnESC)
    {
        Name = name;
		gd.GraphName = Name;
		gd.abortOnESC = abortOnESC;
        state = Stop;
        isInitialized = false;
    }

	void LoopProcessor::StartThread()
	{
		thread = boost::thread(&LoopProcessor::ProcessLoop, this);
	}

	void LoopProcessor::JoinThread()
	{
		thread.join();
	}

	bool LoopProcessor::ProcessOne()
	{
		bool fOK = true;
		for (int i = 0; i < Processors.size(); i++) {
			Processors[i]->tic();
			// Q: Bail only at end of loop or partway through?
			// Currently, complete the loop
			fOK &= Processors[i]->process(gd);
			Processors[i]->toc();
		}
		return fOK;
	}

    bool LoopProcessor::ProcessLoop()
    {
		bool fOK = true;
		stepping = false;

		// Init everybody
		for (int i = 0; i < Processors.size(); i++) {
			fOK &= Processors[i]->init(gd);
			if (!fOK) {
				cout << "ERROR: " + Processors[i]->CombinedName << " failed init()" << endl;
			}
		}

		// main processing loop
        while (fOK) {
			switch (state) {
			case Stop:
				// Snooze.  But this should be a mutex or awaitable object
				boost::this_thread::sleep(boost::posix_time::milliseconds(33));
				break;
			case Pause:
				if (stepping) {
					fOK &= ProcessOne();
					stepping = false;
				}
				// Snooze.  But this should be a mutex or awaitable object
				boost::this_thread::sleep(boost::posix_time::milliseconds(5));
				break;
			case Run:
				fOK &= ProcessOne();
				break;
			}
			auto key = cv::waitKey(1);
			if (gd.abortOnESC && (key == 27)) {
				fOK = false;
			}
        }

		// clean up
		for (int i = 0; i < Processors.size(); i++) {
			Processors[i]->fini(gd);
		}

		return fOK;
	}

    bool LoopProcessor::Step()
    {
        if (state == GraphState::Pause)
        {
            stepping = true;
			return true;
        }
        return false;
    }

    bool LoopProcessor::GotoState(GraphState newState)
    {
        state = newState;

        return true;
    }
}
