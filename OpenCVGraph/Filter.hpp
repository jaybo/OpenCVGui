
#pragma once

#ifndef INCLUDE_FILTER_HPP
#define INCLUDE_FILTER_HPP

#include "stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{

	class Filter
	{
	public:
        /// Base class for all filters in the graph
        Filter::Filter(std::string name, GraphData& data, int width = 512, int height=512)
            : m_FilterName(name), m_width(width), m_height(height)
        {
            BOOST_LOG_TRIVIAL(info) << "Filter() " << m_FilterName;

            m_CombinedName = data.m_GraphName + "-" + name;
            m_TickFrequency = cv::getTickFrequency();
            m_imView = Mat::eye(10, 10, CV_16U);
        }

        virtual Filter::~Filter()
        {
            BOOST_LOG_TRIVIAL(info) << "~Filter() " << m_FilterName;
        }

        // Graph is starting up
        // Allocate resources if needed
        // Specify special capture format(s) required
        virtual bool Filter::init(GraphData& data)
        {
            if (m_showView) {
                m_ZoomView = ZoomView(m_CombinedName);
                m_ZoomView.Init(m_width, m_height, m_MouseCallback);
            }
            m_IsInitialized = true;
            return true;
        }

        // All of the work is done here
        virtual bool Filter::process(GraphData& data)
        {
            m_firstTime = false;

            // do all the work here

            return true;
        }

        virtual void UpdateView(GraphData& graphData) {
            if (m_showView) {
                m_ZoomView.UpdateView(m_imView, m_imViewOverlay, graphData, m_ZoomViewLockIndex);
            }
        };

        // Graph is stopping
        // Deallocate resources
        virtual bool Filter::fini(GraphData& data)
        {
            m_IsInitialized = false;
            return true;
        }

        // Process keyboard hits
        virtual bool Filter::processKeyboard(GraphData& data, int key)
        {
            if (m_showView) {
                return m_ZoomView.KeyboardProcessor(key);
            }
            return true;
        }

        // Record time at start of processing
        virtual void Filter::tic()
        {
            m_TimeStart = static_cast<double>(cv::getTickCount());
        }

        // Calc delta at end of processing
        virtual void Filter::toc()
        {
            m_TimeEnd = static_cast<double>(cv::getTickCount());
            m_DurationMS = (m_TimeEnd - m_TimeStart) / m_TickFrequency * 1000;
            m_DurationMSSum += m_DurationMS;
            m_DurationMSMax = max(m_DurationMS, m_DurationMSMax);
            m_DurationMSMin = min(m_DurationMS, m_DurationMSMin);
            BOOST_LOG_TRIVIAL(info) << m_FilterName << "\ttime(MS): " << std::fixed << std::setprecision(1) << m_DurationMS;
        }

        virtual void Filter::saveConfig(FileStorage& fs, GraphData& data)
        {
            fs << "IsEnabled" << m_Enabled;
            fs << "ShowView" << m_showView;
            fs << "ZoomViewLockIndex" << m_ZoomViewLockIndex;

            // Save how long the filter took to process its last sample, mean, min, max
            // Help, take me back to C#!!! or even javascript
            std::stringstream strT;
            std::string tmp;
            strT << fixed << setprecision(1);

            strT << (m_DurationMSSum / (( data.m_FrameNumber == 0) ? 1 : data.m_FrameNumber));
            tmp = strT.str();
            fs << "Duration_MS_Mean" << tmp.c_str();

            strT.str("");
            strT << m_DurationMSMin;
            tmp = strT.str();
            fs << "Duration_MS_Min" << tmp.c_str();

            strT.str("");
            strT << m_DurationMSMax;
            tmp = strT.str();
            fs << "Duration_MS_Max" << tmp.c_str();

            strT.str("");
            strT <<  m_DurationMS;
            tmp = strT.str();
            fs << "Duration_MS_Last" << tmp.c_str();

        }

        virtual void Filter::loadConfig(FileNode& fs, GraphData& data)
        {
            auto en = fs["IsEnabled"];
            if (!en.empty()) {
                en >> m_Enabled;
            }
            fs ["ShowView"] >> m_showView;
            fs ["ZoomViewLockIndex"] >> m_ZoomViewLockIndex;
            if (m_ZoomViewLockIndex >= MAX_ZOOMVIEW_LOCKS) {
                m_ZoomViewLockIndex = -1;
            }
        }

        virtual void Filter::Enable(bool enable)
        {
            m_Enabled = enable;
        }
        
        /// Can only be called before the graph is first started
        virtual void Filter::EnableZoomView(bool enable)
        {
            if (!m_IsInitialized) {
                m_showView = enable;
            }
        }

        bool Filter::IsEnabled() { return m_Enabled; }

        std::string m_FilterName;
		std::string m_CombinedName; // Graph-Filter name

        double m_DurationMS = 0;                // tictoc of last process
        double m_DurationMSSum = 0;             // sum of all durations
        double m_DurationMSMin = 9999;             
        double m_DurationMSMax = 0;

        int m_ZoomViewLockIndex = -1;

	protected:
		bool m_firstTime = true;
		bool m_showView = true;
        bool m_Enabled = true;
        bool m_IsInitialized = false;
        double m_TimeStart;
        double m_TimeEnd;

        double m_TickFrequency;

        int m_width, m_height;

        cv::Mat m_imView;               // image to display
        cv::Mat m_imViewOverlay;        // overlay for that image
        ZoomView m_ZoomView;
        cv::MouseCallback m_MouseCallback = NULL;
	};

    typedef std::shared_ptr<Filter> CvFilter;

}
#endif