#include "video.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/freetype.hpp>

struct clip_t {
    int st; // start time
    int cnt; // # frames
};

void video::create(VideoWriter &out, VideoCapture &src, vec<Evt> evts, vec<int> srccut) {
    // prepare src clips
    sort(all(srccut));
    vec<clip_t> clips;
    for (int i=0; i<sz(srccut); ++i) {
        clips.push_back({srccut[i], srccut[i+1]-srccut[i]});
    }
    sort(all(clips),[](clip_t x, clip_t y){return x.cnt<y.cnt;});

    // prepare sweepline
    multiset<pair<int,pair<int,int>>> nxt; // (time, type, evt ind)
    for (int i=0; i<sz(evts); ++i) {
        nxt.insert({evts[i].st, {0, i}});
        nxt.insert({evts[i].nd, {1, i}});
    }

    printf("%d events\n", sz(evts));
    printf("ready to start\n");

    // process evts in nxt
    // note last evt is a kill (1)
    vec<pair<int,pair<int,int>>> pq(all(nxt));
    vec<int> active;
    for (int i=0; i<sz(pq)-1; ++i) {
        fflush(stdout);

        int ind=pq[i].second.second; // corresponding event ind
        if (pq[i].second.first==0) {
            // spawn
            active.push_back(ind);
            sort(all(active),[&](int x, int y){return evts[x].type<evts[y].type;});

            if (evts[ind].type==EvtType::Bg) {
                int choice;
                do {
                    choice=rand()%sz(clips);
                } while (evts[ind].nd-evts[ind].st > clips[choice].cnt);

                int evtlen=evts[ind].nd-evts[ind].st+1;
                evts[ind].bg_srcst_=clips[choice].st+rand()%(clips[choice].cnt-evtlen);
                // evts[ind].bg_srcst_=srccut[rand()%sz(srccut)];
            }

            if (evts[ind].type==EvtType::Region) {
                int choice;
                do {
                    choice=rand()%sz(clips);
                } while (evts[ind].nd-evts[ind].st > clips[choice].cnt);

                int evtlen=evts[ind].nd-evts[ind].st+1;
                evts[ind].region_srcst_=clips[choice].st+rand()%(clips[choice].cnt-evtlen);
            }
        } else {
            // kill
            active.erase(find(all(active),ind));
        }

        // write segment
        int st=pq[i].first;
        int nd=pq[i+1].first;
        for (int frm=st; frm<nd; ++frm) {
            printf("\rframes: %d/%d (%d%% done)", frm+1, pq.back().first, (int)((float)(frm+1)/pq.back().first*100));
            fflush(stdout);

            Mat res=write_evt(src, active, frm, evts);
            out.write(res);
        }
    }
    putchar('\n');
}

void draw_glowing_text(Mat &frame, const std::string &text, const std::string &fontPath,
                       int fontHeight, Scalar textColor, Scalar glowColor, int glowRadius) {
    // Create FreeType object for rendering custom fonts
    Ptr<cv::freetype::FreeType2> ft2 = cv::freetype::createFreeType2();
    ft2->loadFontData(fontPath, 0); // Load the font file

    // Calculate text size to center it
    Size textSize = ft2->getTextSize(text, fontHeight, -1, nullptr);
    Point textOrg((frame.cols - textSize.width) / 2, (frame.rows + textSize.height) / 2 - 100);

    // Create a glow layer
    Mat glowLayer = Mat::zeros(frame.size(), frame.type());

    // Render the glow text
    ft2->putText(glowLayer, text, textOrg, fontHeight, glowColor, -1, LINE_AA, false);

    // Apply Gaussian blur to create the glow effect
    GaussianBlur(glowLayer, glowLayer, Size(glowRadius, glowRadius), 0);

    // Add the glow layer to the original frame (preserving brightness)
    add(frame, glowLayer, frame); // Add glow to the frame without dimming

    // Render the main text on top
    ft2->putText(frame, text, textOrg, fontHeight, textColor, -1, LINE_AA, false);
}

// st: start frame
// cur: cur frame
// includes motion blur
void shake_frame(Mat &mt, int mxa, int st, int cur) {
    double shake=max(0., 1.-(cur-st)*0.1);
    int a=(int)(mxa*shake); // amplitude
    int dx=rand()%(2*a+1)-a;
    int dy=rand()%(2*a+1)-a;

    Mat tmat=(Mat_<double>(2,3)<<1, 0, dx, 0, 1, dy); // translation
    Mat shakemt; // shake mat
    warpAffine(mt, shakemt, tmat, Size(mt.cols,mt.rows), INTER_LINEAR, BORDER_CONSTANT, Scalar(0,0,0));
    mt=shakemt;

    if (a>0) {
        int kernelsz=a*2;
        kernelsz=max(3, kernelsz|1);
        Mat motion=Mat::zeros(kernelsz, kernelsz, CV_32F);

        if (abs(dx)>abs(dy)) {
            // horizontal blur
            for (int i=0; i<kernelsz; ++i) {
                motion.at<float>(kernelsz/2, i)=1.0/kernelsz;
            }
        } else {
            // vertical blur
            for (int i=0; i<kernelsz; ++i) {
                motion.at<float>(i, kernelsz/2)=1.0/kernelsz;
            }
        }

        filter2D(mt, mt, -1, motion, Point(-1,-1), 0, BORDER_CONSTANT);
    }
}

void glitch_frame(Mat &mt) {
    // channel offset
    Mat ch[3];
    split(mt, ch);

    int offset=rand()%20-10;
    if (offset>0) {
        ch[0](Rect(0, 0, W-offset, H)).copyTo(ch[0](Rect(offset, 0, W-offset, H)));
    } else {
        ch[0](Rect(-offset, 0, W+offset, H)).copyTo(ch[0](Rect(0, 0, W+offset, H)));
    }

    merge(ch, 3, mt);

    // dim every 5th row
    for (int i=0; i<H; i+=5) {
        mt.row(i)*=0.5;
    }
}

void flash_frame(Mat &mt, int st, int cur) {
    double intensity=1.8-(cur-st)*0.1;
    intensity=std::max(1., intensity);

    mt.convertTo(mt, -1, intensity, 0);
}

Mat video::write_evt(VideoCapture &src, const vec<int> &active, int frm, const vec<Evt> &evts) {
    Mat res(H,W,CV_8UC3);
    for (int ind:active) {
        if (evts[ind].type==EvtType::Bg) {
            // BACKGROUND
            int srcfrm=evts[ind].bg_srcst_ + frm-evts[ind].st;
            src.set(CAP_PROP_POS_FRAMES, srcfrm);

            // copy src frame
            Mat mt;
            src.read(mt);

            // effects
            if (srcfrm-evts[ind].bg_srcst_<10) {
                shake_frame(mt, 20, evts[ind].bg_srcst_, srcfrm);
                glitch_frame(mt);
                flash_frame(mt, evts[ind].bg_srcst_, srcfrm);
            }

            // copy to res
            for (int r=0; r<H; ++r) {
                for (int c=0; c<W; ++c) {
                    res.at<Vec3b>(r,c)=mt.at<Vec3b>(r,c);
                }
            }
        } else if (evts[ind].type==EvtType::Text) {
            // TEXT
            draw_glowing_text(res, evts[ind].text_str, "res/font.ttf", 60, Scalar(255,255,255), Scalar(0,0,255), 45);
        } else if (evts[ind].type==EvtType::Region) {
            // REGION
            int srcfrm=evts[ind].region_srcst_ + frm-evts[ind].st;
            src.set(CAP_PROP_POS_FRAMES, srcfrm);

            // copy src frame
            Mat mt;
            src.read(mt);

            // effects
            Point dst=evts[ind].region_dst;
            if (srcfrm-evts[ind].region_srcst_<10) {
                shake_frame(mt, 20, evts[ind].bg_srcst_, srcfrm);
                glitch_frame(mt);
                flash_frame(mt, evts[ind].bg_srcst_, srcfrm);

                double shake=max(0., 1.-(srcfrm-evts[ind].bg_srcst_)*0.1);
                int a=(int)(20.*shake); // amplitude
                int dx=rand()%(2*a+1)-a;
                int dy=rand()%(2*a+1)-a;
                dst.x+=dx;
                dst.y+=dy;
            }

            // copy mt to res
            for (int r=evts[ind].region_src.y; r<=evts[ind].region_src.y+evts[ind].region_src.height-1; ++r) {
                for (int c=evts[ind].region_src.x; c<=evts[ind].region_src.x+evts[ind].region_src.width-1; ++c) {
                    int resr=(int)(evts[ind].region_scale * (r-evts[ind].region_src.y)) + dst.y;
                    int resc=(int)(evts[ind].region_scale * (c-evts[ind].region_src.x)) + dst.x;
                    res.at<Vec3b>(resr, resc) = mt.at<Vec3b>(r, c);
                }
            }
        }
    }

    return res;
}
