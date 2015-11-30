#pragma once

#ifndef INCLUDE_FOCUSSOBEL_HPP
#define INCLUDE_FOCUSSOBEL_HPP

#include "..\stdafx.h"

using namespace cv;
using namespace std;

namespace openCVGraph
{
    // Scores an image using sobel filter.  Cannot score astimatism.

    class FocusSobel : public Filter
    {
    public:

        FocusSobel::FocusSobel(std::string name, GraphData& graphData,
            int width = 512, int height = 512)
            : Filter(name, graphData, width, height)
        {
        }

        bool FocusSobel::init(GraphData& graphData) override
        {
            Filter::init(graphData);
            graphData.m_NeedCV_8UC1 = true;

            if (m_Enabled) {
                if (m_showView) {
                    if (m_showViewControls) {
                        createTrackbar("Kernel", m_CombinedName, &m_kSize, 7, SliderCallback, this);
                    }
                }
            }
            return true;
        }

        ProcessResult FocusSobel::process(GraphData& graphData) override
        {
            if (graphData.m_UseCuda) {
                Scalar s;
                graphData.m_imOutGpu16UC1 = graphData.m_imCapGpu16UC1;
                auto nPoints = graphData.m_imCapGpu16UC1.size().area();

                // X
                m_cudaFilter = cv::cuda::createSobelFilter(graphData.m_imCapGpu16UC1.type(), graphData.m_imOutGpu16UC1.type(), 1, 0, m_kSize);
                m_cudaFilter->apply(graphData.m_imCapGpu16UC1, graphData.m_imOutGpu16UC1);
                s = cv::cuda::sum(graphData.m_imOutGpu16UC1);
                meanX = s[0] / nPoints;

                // Y
                m_cudaFilter = cv::cuda::createSobelFilter(graphData.m_imCapGpu16UC1.type(), graphData.m_imOutGpu16UC1.type(), 0, 1, m_kSize);
                m_cudaFilter->apply(graphData.m_imCapGpu16UC1, graphData.m_imOutGpu16UC1);
                s = cv::cuda::sum(graphData.m_imOutGpu16UC1);
                meanY = s[0] / nPoints;

                meanXY = (meanX + meanY) / 2;
            }
            else {
                cv::Sobel(graphData.m_imCapture, m_imSx,
                    graphData.m_imCapture.depth(),
                    1, 0,
                    m_kSize);
                cv::Sobel(graphData.m_imCapture, m_imSy,
                    graphData.m_imCapture.depth(),
                    0, 1,
                    m_kSize);

                meanX = cv::mean(m_imSx)[0];
                meanY = cv::mean(m_imSy)[0];
                meanXY = (meanX + meanY) / 2;
            }

            return ProcessResult::OK;  // if you return false, the graph stops
        }

        void FocusSobel::processView(GraphData& graphData)  override
        {
            ClearOverlayText();

            if (graphData.m_UseCuda) {
                graphData.m_imCapGpu8UC1.download(m_imView);
            }
            else
            {
                m_imView = graphData.m_imCap8UC1;
            }

            std::ostringstream str;

            int posLeft = 10;
            double scale = 1.0;

            str.str("");
            str << "  meanXY    X      Y";
            DrawOverlayTextMono(str.str(), Point(posLeft, 50), scale);

            str.str("");
            str << std::setfill(' ') << setw(7) << (int) meanXY << setw(7) << (int) meanX << setw(7) << (int) meanY;
            DrawOverlayTextMono(str.str(), Point(posLeft, 100), scale);

            Filter::processView(graphData);
        }

        void  FocusSobel::saveConfig(FileStorage& fs, GraphData& data) override
        {
            Filter::saveConfig(fs, data);
            fs << "kernel_size" << m_kSize;
        }

        void  FocusSobel::loadConfig(FileNode& fs, GraphData& data) override
        {
            Filter::loadConfig(fs, data);
            fs["kernel_size"] >> m_kSize;
        }

        void FocusSobel::KernelSize(int kernelSize) {
            m_kSize = kernelSize;
        }

    private:
        Mat m_imSx;
        Mat m_imSy;
        double meanX, meanY, meanXY;
        cv::Ptr<cv::cuda::Filter> m_cudaFilter;
        int m_kSize = 3;

        static void FocusSobel::SliderCallback(int pos, void * userData) {
            FocusSobel* filter = (FocusSobel *)userData;
            if (!(pos & 1)) {
                pos++;      // kernels must be odd
            }
            filter->KernelSize(pos);
        }
    };
}
#endif