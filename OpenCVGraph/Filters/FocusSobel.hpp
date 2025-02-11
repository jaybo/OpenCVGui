#pragma once

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
            StreamIn streamIn = StreamIn::CaptureRaw,
            int width = 512, int height = 512)
            : Filter(name, graphData, streamIn, width, height)
        {
        }

        bool FocusSobel::init(GraphData& graphData) override
        {
            Filter::init(graphData);

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
            graphData.EnsureFormatIsAvailable(graphData.m_UseCuda, CV_16UC1, false);

            if (graphData.m_UseCuda) {
#ifdef WITH_CUDA
                Scalar s;
                graphData.m_imOutGpu = graphData.m_CommonData->m_imCaptureGpu16UC1;
                auto nPoints = graphData.m_CommonData->m_imCaptureGpu16UC1.size().area();

                // X
                m_cudaFilter = cv::cuda::createSobelFilter(graphData.m_CommonData->m_imCaptureGpu16UC1.type(), graphData.m_imOutGpu.type(), 1, 0, m_kSize);
                m_cudaFilter->apply(graphData.m_CommonData->m_imCaptureGpu16UC1, graphData.m_imOutGpu);
                s = cv::cuda::sum(graphData.m_imOutGpu);
                meanX = s[0] / nPoints;

                // Y
                m_cudaFilter = cv::cuda::createSobelFilter(graphData.m_CommonData->m_imCaptureGpu16UC1.type(), graphData.m_imOutGpu.type(), 0, 1, m_kSize);
                m_cudaFilter->apply(graphData.m_CommonData->m_imCaptureGpu16UC1, graphData.m_imOutGpu);
                s = cv::cuda::sum(graphData.m_imOutGpu);
                meanY = s[0] / nPoints;

                meanXY = (meanX + meanY) / 2;
#else
                assert(0);
#endif
            }
            else {
                cv::Sobel(graphData.m_CommonData->m_imCapture, m_imSx,
                    graphData.m_CommonData->m_imCapture.depth(),
                    1, 0,
                    m_kSize);
                cv::Sobel(graphData.m_CommonData->m_imCapture, m_imSy,
                    graphData.m_CommonData->m_imCapture.depth(),
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

            m_imView = graphData.m_CommonData->m_imCapture;

            std::ostringstream str;

            int posLeft = 10;
            double scale = 1.0;

            str.str("");
            str << "  meanXY    X      Y";
            DrawOverlayText(str.str(), Point(posLeft, 50), scale);

            str.str("");
            str << std::setfill(' ') << setw(7) << (int) meanXY << setw(7) << (int) meanX << setw(7) << (int) meanY;
            DrawOverlayText(str.str(), Point(posLeft, 100), scale);

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
#ifdef WITH_CUDA
        cv::Ptr<cv::cuda::Filter> m_cudaFilter;
#endif
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
