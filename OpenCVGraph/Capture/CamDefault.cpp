
#include "..\stdafx.h"
#include "CamDefault.h"

using namespace std;
using namespace cv;
namespace fs = ::boost::filesystem;

namespace openCVGraph
{
    // General image source:
    //   if   "camera_index" is set, use that camera
    //   elif "image_name" is set, use just that image
    //   elif "movie_name" is set, use that movie
    //   elif "image_dir" is set and contains images, use all images in dir
    //   else create a noise image

    CamDefault::CamDefault(std::string name, GraphData& graphData, bool showView, int width, int height)
        : Filter(name, graphData, showView, width, height)
    {
        source = Noise;
        camera_index = -1;
    }

    // keyWait required to make the UI activate
    bool CamDefault::processKeyboard(GraphData& data, int key)
    {
        bool fOK = true;
        if (m_showView) {
            return view.KeyboardProcessor(key);  // Hmm,  what to do here?
        }
        return fOK;
    }

    //Allocate resources if needed
    bool CamDefault::init(GraphData& graphData)
    {
		// call the base to read/write configs
		Filter::init(graphData);

        bool fOK = false;
        
        // CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA CAMERA 
        if (camera_index.length() > 0) {

            std::stringstream(camera_index) >> cameraIndex;
            fOK = cap.open(cameraIndex);
            if (fOK) {
                // set camera specific properties
                fOK = cap.read(graphData.m_imCapture);

                if (!graphData.m_imCapture.data)   // Check for invalid input
                {
                    fOK = false;
                    std::cout << "Could not open capture device #" << camera_index << std::endl;
                }
                else {
                    source = Camera;
                    fOK = true;
                }
            }
        }
        
        // SingleImage SingleImage SingleImage SingleImage SingleImage SingleImage 
        if (!fOK && fileExists(image_name)) {
            Mat image = imread(image_name, CV_LOAD_IMAGE_UNCHANGED);   // Read the file

            if (!image.data)                              // Check for invalid input
            {
                std::cout << "Could not open or find the image" << std::endl;
            }
            else {
                graphData.m_imCapture = image;
                source = SingleImage;
                fOK = true;
            }
        }
        
        // Movie Movie Movie Movie Movie Movie Movie Movie Movie Movie Movie 
        if (!fOK && fileExists(movie_name)) {
            if (cap.open(movie_name)) {
                source = Movie;
                fOK = true;
            }
        }

        // Directory Directory Directory Directory Directory Directory Directory 
        if (!fOK) {
            if (fs::exists(image_dir) && fs::is_directory(image_dir)) {

                fs::recursive_directory_iterator it(image_dir);
                fs::recursive_directory_iterator endit;
                vector<string> extensions = { ".png", ".tif", ".tiff", ".jpg", ".jpeg" };
                while (it != endit)
                {
                    string lower = it->path().extension().string();
                    // tolower and replace those pesky backslashes
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    std::replace(lower.begin(), lower.end(), '\\', '/');
                    cout << it->path().extension() << endl;

                    if (fs::is_regular_file(*it) &&
                        std::find(extensions.begin(), extensions.end(), lower) != extensions.end())
                    {
                        images.push_back(it->path());
                    }
                    ++it;
                }
                source = Directory;
                fOK = true;
            }
        }
		
		// Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise Noise 
		if (!fOK) {
			source = Noise;
			graphData.m_imCapture = Mat::zeros(512, 512, CV_16U);
			fOK = true;
		}

		return fOK;
    }


    bool CamDefault::process(GraphData& graphData)
    {
        m_firstTime = false;
		bool fOK = true;

		switch (source) {
		case Camera:
			fOK = cap.read (graphData.m_imCapture);
			break;
		case SingleImage:
			// nothing to do, already loaded
			break;
		case Movie:
			fOK = cap.read(graphData.m_imCapture);
			break;
		case Directory:
            if (images.size() > 0) {
                string fname = images[imageIndex].string();
                // cout << fname << endl;
                graphData.m_imCapture = imread(fname);
                imageIndex++;
                if (imageIndex >= images.size()) {
                    imageIndex = 0;
                }
            }
			break;
		case Noise:
			 //cv::randu(graphData.m_imCapture, Scalar::all(0), Scalar::all(65536));
            //cv::randu(graphData.m_imCapture, Scalar::all(0), Scalar::all(255));
            //graphData.m_imCapture = Mat::zeros(512, 512, CV_16U);
            cv::randu(graphData.m_imCapture, Scalar::all(0), Scalar::all(65536));
            break;
		}

		if (m_showView && fOK) {
            if (camera_name == "Ximea16") {
                imView = 16 * graphData.m_imCapture;
            }
            else {
                imView = graphData.m_imCapture;
            }
			cv::imshow(m_CombinedName, imView);
		}
        return fOK;
    }

    // deallocate resources
    bool CamDefault::fini(GraphData& graphData)
    {
		if (cap.isOpened()) {
			cap.release();
		}
		return true;
    }



    void  CamDefault::saveConfig(FileStorage fs, GraphData& data)
    {
        //fs << "{"
        //fs << "tictoc" << tictoc;
        //fs << "camera_index" << camera_index;
        //fs << "camera_name" << camera_name;
        //fs << "image_name" << image_name;
        //fs << "movie_name" << movie_name;
        //fs << "image_dir" << image_dir;

    }

    void  CamDefault::loadConfig(FileStorage fs, GraphData& data)
    {
		// cout << m_persistFile << endl;
        
        //fs["tictoc"] >> tictoc;
        //fs["camera_index"] >> camera_index;
        //fs ["camera_name"] >> camera_name;
        //fs["image_name"] >> image_name;
        //fs["movie_name"] >> movie_name;
        //fs["image_dir"] >> image_dir;

    }
}
