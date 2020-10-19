#include <iostream>
#include <string>
#include <glob.h>
#include <random>
#include <vector>
#include <sstream>

#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>

using namespace std;
using namespace cv;


static bool endsWith(const string& str, const string& suffix);
int indexClosestImg(int mean, vector<vector<int>>& ref);
int randomly(vector<int>& indexes);
vector<string> files_in_dir(string& dir);


int main(int argc, char *argv[])
{
	if (argc < 3) {
		cerr << "Not enough arguments given: \n\n" 
		     "Usage: ./main source.jpg image_folder_name block_dimensions\n\n"
			 "source.jpg: the image you want to make a collage of\n"
			 "image_folder_name: relative or absolute path to a folder "
			 "containing the images that should be used for the collage\n"
			 "block_dimensions: (optional, default=25) the size of the "
			 "individual image inside the collage\n\n" 
			 "Example: ./main bird.jpg ../img" << endl;
		return 0;
	}

	vector<Mat1b> images;

	// Define a reference vector; it is used to reference the color mean
	// to the index of the images stored the images vector
	vector<int> indexes(0);
	vector<vector<int>> ref(255, indexes);

	const string srcName = argv[1];
	string dir = (string)argv[2] + "/*";
	
	cout << "Getting the images from: " << dir << endl;

	Mat1b src = imread(srcName, 0); // Read gray image

	if (src.empty()) {
		cerr << "Source image not found!" << endl;
		return 0;
	}

	// Get the block dimensions we should use for the individual images inside the collage
	int blockDim;
	if (argc > 3) {
		istringstream ss(argv[3]);
		ss >> blockDim; 
	} else {
		blockDim = 25; // Default value
	}

	if (blockDim <= 0 || blockDim > src.rows || blockDim > src.cols) {
		cerr << "Invalid block dimension, should be in range: 1-" << min(src.rows,src.cols) << endl;
		return 0;
	}

	vector<string> files = files_in_dir(dir);

	if (files.size() == 0) {
		cerr << "No images found in the directory: " << dir << endl;
		return 0;
	}

	for (unsigned int i=0; i<files.size(); i++) {
		const string file = files.at(i);
		Mat1b img = imread(file, 0);
		resize(img, img, Size(blockDim,blockDim));
		images.push_back(img.clone());
		Scalar mean = cv::mean(img);
		ref.at(round(mean[0])).push_back(i);
	}

	int wBlocks = src.cols / blockDim;
	int hBlocks = src.rows / blockDim;

	Mat1b target(hBlocks*blockDim, wBlocks*blockDim);

	for (int row=0; row<hBlocks; row++) {
		for (int col=0; col<wBlocks; col++) {
			Rect roi(col*blockDim, row*blockDim, blockDim, blockDim);
			Scalar mean = cv::mean(src(roi));
			int index = indexClosestImg(round(mean[0]), ref);
			images.at(index).copyTo(target(roi));
		}
	}

	// Define the target name as source name without file extension + "Collage.jpg"
	string targetName = srcName.substr(0, srcName.find_last_of(".")) + "Collage.jpg";

	imwrite(targetName, target);
	cout << "Collage is successfully stored under: " << targetName << endl;

	namedWindow("Result");
	imshow("Result", target);
	waitKey(0);
}


/**
 * Check if a string ends with a defined suffix
 */
static bool endsWith(const string& str, const string& suffix) {
    return str.size() >= suffix.size() && 
			0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

/**
 * Get the index of an image from our reference vector that 
 * has the closest mean color value to the block we are 
 * trying to replace with an image 
 */
int indexClosestImg(int mean, vector<vector<int>>& ref) {
	if (ref.at(mean).size() > 0) {
		return randomly(ref.at(mean));
	}

	for(int diff=1; diff<255; diff++) {
		if (mean+diff < 256 && ref.at(mean+diff).size() > 0) {
			return randomly(ref.at(mean+diff));
		} else if (mean-diff >=0 && ref.at(mean-diff).size() > 0) {
			return randomly(ref.at(mean-diff));
		}
	}
	return -1;
}

/**
 * Get a random value from a vector 
 */
int randomly(vector<int>& values) {
	int index = rand() % values.size();
	return values.at(index);
}

/**
 * Get all the image file names locatated in a directory. 
 * Supported image formats are: jpg, jpeg, png and webp
 */ 
vector<string> files_in_dir(string& dir) {
	glob_t glob_result;
	
	glob(dir.c_str(), GLOB_TILDE|GLOB_MARK, NULL, &glob_result);
	vector<string> files;
    for(unsigned int i=0; i<glob_result.gl_pathc; i++) {
		string file = string(glob_result.gl_pathv[i]);
		if (endsWith(file, ".jpg") || endsWith(file, ".png") || 
				endsWith(file, ".jpeg") || endsWith(file, ".webp"))
        	files.push_back(file);
    }
    globfree(&glob_result);
	return files;
}
