
#include "stdafx.h"

#include "GraphManager.h"

namespace openCVGraph
{
    // Keep a vector of Filters and call each in turn to crunch images
    // (or perform other work)
    // States: Stop, Pause (can only Step if Paused), and Run

    GraphManager::GraphManager(const std::string name, bool abortOnESC)
    {
        m_Name = name;
		gd.m_GraphName = m_Name;
		gd.abortOnESC = abortOnESC;
        m_GraphState = GraphState::Stop;

        std::cout << "GraphManager()" << std::endl;
        std::string config("config");
        fs::create_directory(config);

        // The settings file name combines both the GraphName and the Filter together
        m_persistFile = config + "/" + m_Name + ".yml";
        std::cout << m_persistFile << std::endl;
    }

    GraphManager::~GraphManager()
    {
        std::cout << "~GraphManager()" << std::endl;
        cv::destroyAllWindows();
    }


	void GraphManager::StartThread()
	{
		thread = boost::thread(&GraphManager::ProcessLoop, this);
	}

	void GraphManager::JoinThread()
	{
		thread.join();
	}

	bool GraphManager::ProcessOne()
	{
		bool fOK = true;
        
        // first process keyhits, then the frame
        int c = waitKey(1);
        if (c != -1) {
            for (int i = 0; i < Processors.size(); i++) {
                fOK &= Processors[i]->processKeyboard(gd, c);
            }
        }

        // MAKE ONE PASS THROUGH THE GRAPH
		for (int i = 0; i < Processors.size(); i++) {
			Processors[i]->tic();
			// Q: Bail only at end of loop or partway through?
			// Currently, complete the loop
			fOK &= Processors[i]->process(gd);

			Processors[i]->toc();
            gd.m_FrameNumber++;
		}

        // If settings were modified 
		return fOK;
	}

    bool GraphManager::ProcessLoop()
    {
		bool fOK = true;
		m_Stepping = false;

        saveConfig();
        loadConfig();


		// Init everybody
		for (int i = 0; i < Processors.size(); i++) {
			fOK &= Processors[i]->init(gd);
			if (!fOK) {
				cout << "ERROR: " + Processors[i]->m_CombinedName << " failed init()" << endl;
			}
		}

		// main processing loop
        while (fOK) {
			switch (m_GraphState) {
			case GraphState::Stop:
				// Snooze.  But this should be a mutex or awaitable object
				boost::this_thread::sleep(boost::posix_time::milliseconds(33));
				break;
			case GraphState::Pause:
				if (m_Stepping) {
					fOK &= ProcessOne();
					m_Stepping = false;
				}
				// Snooze.  But this should be a mutex or awaitable object
				boost::this_thread::sleep(boost::posix_time::milliseconds(5));
				break;
			case GraphState::Run:
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
        destroyAllWindows();

		return fOK;
	}

    bool GraphManager::Step()
    {
        if (m_GraphState == GraphState::Pause)
        {
            m_Stepping = true;
			return true;
        }
        return false;
    }

    bool GraphManager::GotoState(GraphState newState)
    {
        m_GraphState = newState;

        return true;
    }

    void GraphManager::saveConfig()
    {
        FileStorage fs(m_persistFile, FileStorage::WRITE);
        if (!fs.isOpened()) { std::cout << "ERROR: unable to open file storage!" << m_persistFile << std::endl; return; }

        fs << m_Name << "{";
        fs << "baby" << 1;
        fs << "}";
        for (int i = 0; i < Processors.size(); i++) {
            //fs << "tictoc" << tictoc.c_str();
            Processors[i]->saveConfig(fs, gd);
        }
        fs.release();
    }

    void GraphManager::loadConfig()
    {
        FileStorage fs(m_persistFile, FileStorage::READ);
        if (!fs.isOpened()) { std::cout << "ERROR: unable to open file storage!" << m_persistFile << std::endl; return; }

        string name;
        int baby;
        auto n = fs[m_Name];
        n["baby"] >> baby;

        cout << "name" << name;
        cout << "baby" << baby;

        for (int i = 0; i < Processors.size(); i++) {
            //fs << "tictoc" << tictoc.c_str();
            Processors[i]->loadConfig(fs, gd);
        }
        fs.release();
    }
}
