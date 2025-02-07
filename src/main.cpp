#include "util.h"
#include "video.h"
#include "source.h"
#include <chrono>
#include <thread>
#include <fstream>

int main(int argc, char **argv) {
    srand(time(0));
    system("mkdir -p out");

    // create video
    VideoWriter out("no-audio.mp4", VideoWriter::fourcc('a','v','c','1'), FPS, Size(W,H));

    // 1: bateman edit
    // 2: meme compilation
    // 3: comparison
    int type=3;
    string bgm;
    string title;

    if (argc>1) {
        type=stoi(argv[1]);
    }

    auto st=chrono::high_resolution_clock::now();
    if (type==1) {
        // bateman edit
        printf("video type: bateman edit\n");
        VideoCapture src("res/video/edit/bateman.mp4");
        bgm="derniere-beatdrop";
        video::create(out, src, edit::audsrc_evts(bgm, title), vidsrc_cuts("bateman"));
    } else if (type==2) {
        // meme compilation
        printf("video type: meme compilation\n");
        VideoCapture src("res/video/edit/bateman.mp4");
        bgm="next";
        video::create(out, src, meme::audsrc_evts(title), vidsrc_cuts("bateman"));
    } else if (type==3) {
        // compare
        printf("video type: comparison\n");
        VideoCapture src("res/video/compare/src.mp4");
        bgm="aura-compare";
        video::create(out, src, compare::audsrc_evts(bgm, title), {});
    }
    int dur = chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now()-st).count();
    printf("took %dm %ds\n", dur/60, dur%60);

    out.release();
    while (!ifstream("no-audio.mp4")) {
        printf("no-audio.mp4 not detected\n");
        this_thread::sleep_for(chrono::milliseconds(400));
    }

    // add audio
    printf("adding audio...\n");
    string cmd="ffmpeg -i no-audio.mp4 -i res/audio/"+bgm+".mp3 -c:v copy -c:a aac -strict experimental -shortest -y out/out.mp4 > ffmpeg.log 2>&1";
    system(cmd.c_str());
    system("rm no-audio.mp4");

    // post
    printf("========\n");
    printf("%s\n", title.c_str());
}
