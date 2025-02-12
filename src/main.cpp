#include "util.h"
#include "video.h"
#include "source.h"
#include <chrono>
#include <thread>
#include <fstream>

int main(int argc, char **argv) {
    if (argc==2 && string(argv[1])=="-h") {
        printf("1: bateman edit\n");
        printf("2: bateman meme compilation\n");
        printf("3: comparison\n");
        printf("4: shelby edit\n");
        printf("5: shelby meme compilation\n");
        printf("6: quiz\n");
        return 0;
    }

    srand(time(0));
    system("mkdir -p out");

    // create video
    VideoWriter out("no-audio.mp4", VideoWriter::fourcc('a','v','c','1'), FPS, Size(W,H));

    int type=6;
    string bgm;
    string title;

    if (argc>1) {
        type=stoi(argv[1]);
    }

    vec<float> aud;
    auto st=chrono::high_resolution_clock::now();
    if (type==1) {
        // bateman edit
        printf("video type: bateman edit\n");
        VideoCapture src("res/video/edit/bateman.mp4");
        bgm="derniere-beatdrop";
        aud=video::create(out, src, edit::audsrc_evts(bgm, title), vidsrc_cuts("bateman"));
    } else if (type==2) {
        // bateman meme compilation
        printf("video type: bateman meme compilation\n");
        VideoCapture src("res/video/edit/bateman.mp4");
        bgm="next";
        aud=video::create(out, src, meme::audsrc_evts(title), vidsrc_cuts("bateman"));
    } else if (type==3) {
        // compare
        printf("video type: comparison\n");
        VideoCapture src("res/video/compare/src.mp4");
        bgm="aura-compare";
        aud=video::create(out, src, compare::audsrc_evts(bgm, title), {});
    } else if (type==4) {
        // shelby edit
        printf("video type: shelby edit\n");
        VideoCapture src("res/video/edit/shelby.mp4");
        bgm="derniere-beatdrop";
        aud=video::create(out, src, edit::audsrc_evts(bgm, title), vidsrc_cuts("shelby"));
    } else if (type==5) {
        // shelby meme compilation
        printf("video type: shelby meme compilation\n");
        VideoCapture src("res/video/edit/shelby.mp4");
        bgm="next";
        aud=video::create(out, src, meme::audsrc_evts(title), vidsrc_cuts("shelby"));
    } else if (type==6) {
        // quiz
        printf("video type: quiz\n");
        VideoCapture src("res/video/parkour.mp4");
        bgm="wiishop";
        aud=video::create(out, src, quiz::audsrc_evts(), vidsrc_cuts("parkour"));
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

    // add audio events
    printf("adding audio events...\n");
    string audcmd = "ffmpeg -i out/out.mp4 ";
    for (int i=0; i<sz(aud); ++i) {
        audcmd += "-i out/"+to_string(i)+".wav ";
    }
    audcmd += "-filter_complex \"";
    for (int i=0; i<sz(aud); ++i) {
        int st=aud[i]*1000;
        audcmd += "["+to_string(i+1)+":a]adelay="+to_string(st)+"|"+to_string(st)+"[a"+to_string(i+1)+"]; ";
    }
    audcmd += "[0:a]";
    for (int i=0; i<sz(aud); ++i) {
        audcmd += "[a"+to_string(i+1)+"]";
    }
    audcmd += "amix=inputs="+to_string(sz(aud)+1)+":duration=first\" -c:v copy -c:a aac out/tmp.mp4 > ffmpeg.log 2>&1";
    system(audcmd.c_str());

    while (!ifstream("out/tmp.mp4")) {
        printf("[audio events] out/tmp.mp4 not detected\n");
        this_thread::sleep_for(chrono::milliseconds(400));
    }
    system("mv out/tmp.mp4 out/out.mp4");
    system("rm out/*.wav");

    // post
    printf("========\n");
    printf("%s\n", title.c_str());
}
