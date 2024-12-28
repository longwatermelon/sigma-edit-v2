#include "util.h"
#include "video.h"
#include "source.h"
#include <chrono>
#include <thread>
#include <fstream>

int main() {
    srand(time(0));
    system("mkdir -p out");

    // create video
    VideoWriter out("no-audio.mp4", VideoWriter::fourcc('a','v','c','1'), FPS, Size(W,H));
    VideoCapture src("res/video/bateman.mp4");

    video::create(out, src, edit::audsrc_evts("derniere-beatdrop"), vidsrc_cuts("bateman"));
    out.release();

    while (!ifstream("no-audio.mp4")) {
        printf("no-audio.mp4 not detected\n");
        this_thread::sleep_for(chrono::milliseconds(400));
    }

    // add audio
    printf("adding audio...\n");
    string cmd="ffmpeg -i no-audio.mp4 -i res/audio/derniere-beatdrop.mp3 -c:v copy -c:a aac -strict experimental -shortest -y out/out.mp4 > ffmpeg.log 2>&1";
    system(cmd.c_str());

    system("rm no-audio.mp4");
}
