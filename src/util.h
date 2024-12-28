#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;
#define sz(x) ((int)size(x))
#define all(x) begin(x),end(x)
template <typename T> using vec=vector<T>;

#define W 1080
#define H 1920
#define R1 228
#define R2 1682
#define FPS 30

// frame to time
inline float frm2t(int frame) {
    return (float)frame/FPS;
}

// time to frame
inline int t2frm(float t) {
    return t*FPS;
}
