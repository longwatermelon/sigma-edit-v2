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

            if (evts[ind].type==EvtType::Bg && evts[ind].bg_srcst_==-1) {
                int choice;
                do {
                    choice=rand()%sz(clips);
                } while (evts[ind].nd-evts[ind].st > clips[choice].cnt);

                int evtlen=evts[ind].nd-evts[ind].st+1;
                evts[ind].bg_srcst_=clips[choice].st+rand()%(clips[choice].cnt-evtlen+1);
                // evts[ind].bg_srcst_=srccut[rand()%sz(srccut)];
            }

            if (evts[ind].type==EvtType::Region && evts[ind].region_srcst_==-1) {
                int choice;
                do {
                    choice=rand()%sz(clips);
                } while (evts[ind].nd-evts[ind].st > clips[choice].cnt);

                int evtlen=evts[ind].nd-evts[ind].st+1;
                evts[ind].region_srcst_=clips[choice].st+rand()%(clips[choice].cnt-evtlen+1);
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

void draw_glowing_text(Mat &frame, const string &text, const string &fontPath,
                       int fontHeight, Scalar textColor, Scalar glowColor, int glowRadius) {
    // Create FreeType object for rendering custom fonts
    Ptr<freetype::FreeType2> ft2 = freetype::createFreeType2();
    ft2->loadFontData(fontPath, 0); // Load the font file

    // Split the text into lines based on newline characters
    vector<string> lines;
    std::istringstream textStream(text);
    string line;
    while (std::getline(textStream, line)) {
        lines.push_back(line);
    }

    // Define line spacing between lines
    int lineSpacing = 10; // Adjust as needed

    // Determine the total height of the multi-line text block
    int totalTextHeight = static_cast<int>(lines.size()) * fontHeight +
                          (static_cast<int>(lines.size()) - 1) * lineSpacing;

    // Calculate the vertical start position to center the text block in the frame.
    // (The original function shifted the text up by 100 pixels; adjust if desired.)
    int initialY = (frame.rows - totalTextHeight) / 2 + fontHeight - 100;

    // Create a glow layer (an image the same size as frame)
    Mat glowLayer = Mat::zeros(frame.size(), frame.type());

    // Render the glow text on the glow layer for each line
    for (size_t i = 0; i < lines.size(); ++i) {
        const string &currentLine = lines[i];

        // Calculate the size of the current line and center it horizontally
        Size textSize = ft2->getTextSize(currentLine, fontHeight, -1, nullptr);
        Point textOrg((frame.cols - textSize.width) / 2,
                      initialY + static_cast<int>(i) * (fontHeight + lineSpacing));

        // Draw the current line in the glow color on the glow layer
        ft2->putText(glowLayer, currentLine, textOrg, fontHeight, glowColor,
                     -1, LINE_AA, false);
    }

    // Apply Gaussian blur to the glow layer to create the glow effect
    GaussianBlur(glowLayer, glowLayer, Size(glowRadius, glowRadius), 0);

    // Add the glow layer to the original frame
    add(frame, glowLayer, frame);

    // Render the main (non-glow) text on top of the glow effect for each line
    for (size_t i = 0; i < lines.size(); ++i) {
        const string &currentLine = lines[i];

        // Again, calculate text size and position for this line
        Size textSize = ft2->getTextSize(currentLine, fontHeight, -1, nullptr);
        Point textOrg((frame.cols - textSize.width) / 2,
                      initialY + static_cast<int>(i) * (fontHeight + lineSpacing));

        // Draw the main text in the specified text color
        ft2->putText(frame, currentLine, textOrg, fontHeight, textColor,
                     -1, LINE_AA, false);
    }
}

void draw_horizontal_black_band(cv::Mat &frame, int centerY = 960, int fadeSize = 150)
{
    // Define the region that’s fully black:
    // for example, centerY - 1, centerY, centerY + 1
    int solidTop    = centerY - 1;  // 959
    int solidBottom = centerY + 1;  // 961

    // Define where the fade regions begin/end
    int fadeTop    = centerY - fadeSize; // 960 - 150 = 810 (by default)
    int fadeBottom = centerY + fadeSize; // 960 + 150 = 1110 (by default)

    // Helper lambda to compute alpha for each row.
    // alpha = 0 => no darkening, alpha = 1 => fully black
    auto computeAlpha = [&](int y) -> float
    {
        // 1) Above the solid band, from fadeTop up to solidTop => alpha 0..1
        if (y < solidTop) {
            // linear from 0 to 1
            float range = static_cast<float>(solidTop - fadeTop); // e.g. 959 - 810 = 149
            float val   = static_cast<float>(y - fadeTop);        // how far we are in that fade region
            return std::clamp(val / range, 0.0f, 1.0f);
        }
        // 2) Within the solid band (solidTop..solidBottom) => alpha = 1
        else if (y <= solidBottom) {
            return 1.0f;
        }
        // 3) Below the solid band, from solidBottom+1 down to fadeBottom => alpha 1..0
        else {
            float range = static_cast<float>(fadeBottom - solidBottom); // e.g. 1110 - 961 = 149
            float val   = static_cast<float>(fadeBottom - y);           // how far we are from bottom fade boundary
            return std::clamp(val / range, 0.0f, 1.0f);
        }
    };

    // Iterate through the fade region (top..bottom) and blend black
    for (int y = fadeTop; y <= fadeBottom; y++) {
        // Make sure we stay within image bounds
        if (y < 0 || y >= frame.rows)
            continue;

        float alpha = computeAlpha(y);

        // For alpha blending, newColor = (1 - alpha)*original + alpha*(0,0,0)
        // That simplifies to newColor = original*(1 - alpha).
        uchar *rowPtr = frame.ptr<uchar>(y);
        for (int x = 0; x < frame.cols; x++) {
            // Assuming 3-channel BGR
            rowPtr[3 * x + 0] = static_cast<uchar>(rowPtr[3 * x + 0] * (1.0f - alpha)); // Blue
            rowPtr[3 * x + 1] = static_cast<uchar>(rowPtr[3 * x + 1] * (1.0f - alpha)); // Green
            rowPtr[3 * x + 2] = static_cast<uchar>(rowPtr[3 * x + 2] * (1.0f - alpha)); // Red
        }
    }
}

// void draw_glowing_text(Mat &frame, const std::string &text, const std::string &fontPath,
//                        int fontHeight, Scalar textColor, Scalar glowColor, int glowRadius) {
//     // Create FreeType object for rendering custom fonts
//     Ptr<cv::freetype::FreeType2> ft2 = cv::freetype::createFreeType2();
//     ft2->loadFontData(fontPath, 0); // Load the font file

//     // Calculate text size to center it
//     Size textSize = ft2->getTextSize(text, fontHeight, -1, nullptr);
//     Point textOrg((frame.cols - textSize.width) / 2, (frame.rows + textSize.height) / 2 - 100);

//     // Create a glow layer
//     Mat glowLayer = Mat::zeros(frame.size(), frame.type());

//     // Render the glow text
//     ft2->putText(glowLayer, text, textOrg, fontHeight, glowColor, -1, LINE_AA, false);

//     // Apply Gaussian blur to create the glow effect
//     GaussianBlur(glowLayer, glowLayer, Size(glowRadius, glowRadius), 0);

//     // Add the glow layer to the original frame (preserving brightness)
//     add(frame, glowLayer, frame); // Add glow to the frame without dimming

//     // Render the main text on top
//     ft2->putText(frame, text, textOrg, fontHeight, textColor, -1, LINE_AA, false);
// }

void draw_top_text(Mat &frame, const std::string &text, const std::string &fontPath, int fontHeight) {
    // Create FreeType object for rendering custom fonts
    Ptr<cv::freetype::FreeType2> ft2 = cv::freetype::createFreeType2();
    ft2->loadFontData(fontPath, 0); // Load the font file

    // Calculate text size to center it
    Size textSize = ft2->getTextSize(text, fontHeight, -1, nullptr);
    Point textOrg((frame.cols - textSize.width) / 2, textSize.height+125);

    // Create a glow layer
    Mat glowLayer = Mat::zeros(frame.size(), frame.type());

    // Render the glow text
    ft2->putText(glowLayer, text, textOrg, fontHeight, Scalar(0,0,0), -1, LINE_AA, false);

    // Apply Gaussian blur to create the glow effect
    GaussianBlur(glowLayer, glowLayer, Size(1, 1), 0);

    // Add the glow layer to the original frame (preserving brightness)
    add(frame, glowLayer, frame); // Add glow to the frame without dimming

    // Render the main text on top
    ft2->putText(frame, text, textOrg, fontHeight, Scalar(255,255,255), -1, LINE_AA, false);
}

void draw_top_text_wrap(cv::Mat &frame, const std::string &text, const std::string &fontPath, int fontHeight) {
    // Create FreeType object for rendering custom fonts
    cv::Ptr<cv::freetype::FreeType2> ft2 = cv::freetype::createFreeType2();
    ft2->loadFontData(fontPath, 0); // Load the font file

    // Split the text into lines based on newline characters
    std::vector<std::string> lines;
    std::istringstream textStream(text);
    std::string line;
    while (std::getline(textStream, line)) {
        lines.push_back(line);
    }

    // Determine the total height of the multi-line text block
    int lineSpacing = 10; // Spacing between lines
    int totalTextHeight = lines.size() * fontHeight + (lines.size() - 1) * lineSpacing;

    // Calculate the vertical start position to center the text block at y = 125
    int initialY = 125 - totalTextHeight / 2 + fontHeight;

    // Create a glow layer
    cv::Mat glowLayer = cv::Mat::zeros(frame.size(), frame.type());

    // Loop through each line of text and render
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string &currentLine = lines[i];

        // Calculate text size and position for each line
        cv::Size textSize = ft2->getTextSize(currentLine, fontHeight, -1, nullptr);
        cv::Point textOrg((frame.cols - textSize.width) / 2, initialY + i * (fontHeight + lineSpacing));

        // Render the glow text on the glow layer
        ft2->putText(glowLayer, currentLine, textOrg, fontHeight, cv::Scalar(0, 0, 0), -1, cv::LINE_AA, false);
    }

    // Apply Gaussian blur to create the glow effect
    cv::GaussianBlur(glowLayer, glowLayer, cv::Size(5, 5), 10);

    // Add the glow layer to the original frame
    cv::add(frame, glowLayer, frame);

    // Render the main text on top of the glow
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string &currentLine = lines[i];

        // Calculate text size and position for each line
        cv::Size textSize = ft2->getTextSize(currentLine, fontHeight, -1, nullptr);
        cv::Point textOrg((frame.cols - textSize.width) / 2, initialY + i * (fontHeight + lineSpacing));

        // Render the main text
        ft2->putText(frame, currentLine, textOrg, fontHeight, cv::Scalar(255, 255, 255), -1, cv::LINE_AA, false);
    }
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
        ch[0](Rect(0, 0, mt.cols-offset, mt.rows)).copyTo(ch[0](Rect(offset, 0, mt.cols-offset, mt.rows)));
    } else {
        ch[0](Rect(-offset, 0, mt.cols+offset, mt.rows)).copyTo(ch[0](Rect(0, 0, mt.cols+offset, mt.rows)));
    }

    merge(ch, 3, mt);

    // dim every 5th row
    for (int i=0; i<mt.rows; i+=5) {
        mt.row(i)*=0.5;
    }
}

void flash_frame(Mat &mt, int st, int cur) {
    double intensity=1.8-(cur-st)*0.1;
    intensity=std::max(1., intensity);

    mt.convertTo(mt, -1, intensity, 0);
}

Mat video::write_evt(VideoCapture &src, const vec<int> &active, int frm, const vec<Evt> &evts) {
    Mat res(H,W,CV_8UC3,cv::Scalar(0,0,0));
    for (int ind:active) {
        if (evts[ind].type==EvtType::Bg) {
            // BACKGROUND
            int srcfrm=evts[ind].bg_srcst_ + frm-evts[ind].st;
            src.set(CAP_PROP_POS_FRAMES, srcfrm);

            // copy src frame
            Mat mt;
            src.read(mt);

            // shift down
            Mat tmat = (Mat_<double>(2,3)<<1, 0, 0, 0, 1, evts[ind]._bg_yoffset);
            Mat shift;
            warpAffine(mt,shift,tmat,mt.size());
            mt=shift;

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
            // draw_glowing_text(res, evts[ind].text_str, "res/font.ttf", 60, Scalar(255,255,255), Scalar(0,0,255), 45);
            draw_glowing_text(res, evts[ind].text_str, "res/font.ttf", evts[ind].text_big ? 90 : 60, Scalar(255,255,255), Scalar(0,0,255), 45);
        } else if (evts[ind].type==EvtType::Region) {
            // REGION
            int srcfrm=evts[ind].region_srcst_ + frm-evts[ind].st;
            src.set(CAP_PROP_POS_FRAMES, srcfrm);

            // copy src frame
            Mat mt;
            if (!src.read(mt)) {
                printf("ERROR: frame nonexistent (frame index %d, video frames %d).\n", srcfrm, (int)src.get(CAP_PROP_FRAME_COUNT));
                exit(1);
            }

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
                    if (resr<0 || resc<0 || resr>=H || resc>=W) continue;

                    res.at<Vec3b>(resr, resc) = mt.at<Vec3b>(r, c);
                }
            }
        } else if (evts[ind].type==EvtType::TopText) {
            // TOP TEXT
            draw_top_text_wrap(res, evts[ind].top_text_str, "res/font.ttf", 60);
        } else if (evts[ind].type==EvtType::HBar) {
            // HORIZONTAL BLACK BAR
            draw_horizontal_black_band(res);
        }
    }

    return res;
}
