#include "video.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/freetype.hpp>

struct clip_t {
    int st; // start time
    int cnt; // # frames
};

cv::Ptr<cv::freetype::FreeType2> ft2;

vec<float> video::create(VideoWriter &out, VideoCapture &src, vec<Evt> evts, vec<int> srccut) {
    // Create FreeType object for rendering custom fonts
    ft2 = freetype::createFreeType2();
    ft2->loadFontData("res/font.ttf", 0); // Load the font file

    // prepare src clips
    sort(all(srccut));
    vec<clip_t> clips;
    for (int i=0; i<sz(srccut); ++i) {
        clips.push_back({srccut[i], srccut[i+1]-srccut[i]});
    }
    sort(all(clips),[](clip_t x, clip_t y){return x.cnt<y.cnt;});

    // return value
    vec<float> aud;

    // prepare sweepline
    multiset<pair<int,pair<int,int>>> nxt; // (time, type, evt ind)
    for (int i=0; i<sz(evts); ++i) {
        if (evts[i].type==EvtType::Sfx) {
            system(("cp "+evts[i].sfx_path+" out/"+to_string(sz(aud))+".wav").c_str());
            aud.push_back(frm2t(evts[i].st));
            continue;
        }

        nxt.insert({evts[i].st, {0, i}});
        nxt.insert({evts[i].nd, {1, i}});
    }

    // src cut buffer to prevent repeat clips too often
    vec<int> srcbuf(sz(srccut));

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
                    if (srcbuf[choice]>0) srcbuf[choice]--;
                } while (evts[ind].nd-evts[ind].st > clips[choice].cnt || srcbuf[choice]>0);
                srcbuf[choice]=2;

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

            Mat res=write_evt(src, active, frm, evts, aud);
            out.write(res);
        }
    }
    putchar('\n');

    return aud;
}

void draw_glowing_text(Mat &frame, const string &text,
                       int fontHeight, Scalar textColor, Scalar glowColor, int glowRadius) {
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

void draw_text_left(Mat &frame, const string &text,
                    int fontHeight, Scalar textColor,
                    int borderThickness = 2) {
    // Split the text into lines based on newline characters.
    vector<string> lines;
    istringstream textStream(text);
    string line;
    while (getline(textStream, line)) {
        lines.push_back(line);
    }

    // Define line spacing between lines (adjust as needed).
    int lineSpacing = 10;

    // Determine the total height of the multi-line text block.
    int totalTextHeight = static_cast<int>(lines.size()) * fontHeight +
                          (static_cast<int>(lines.size()) - 1) * lineSpacing;

    // Calculate the vertical start position to center the text block in the frame.
    // (An upward shift of 100 pixels is applied; adjust or remove as desired.)
    int initialY = (frame.rows - totalTextHeight) / 2 + fontHeight - 250;

    // Set the fixed left margin (e.g., starting at column 200).
    int leftMargin = 100;

    // For each line, check for a dynamic color code, then draw the text.
    for (size_t i = 0; i < lines.size(); ++i) {
        // Make a mutable copy of the current line.
        string currentLine = lines[i];

        // Default color is the provided textColor.
        Scalar lineColor = textColor;

        // Check if the line starts with a dynamic color code (e.g., "*0", "*1", etc.)
        if (!currentLine.empty() && currentLine[0] == '*' && currentLine.size() >= 2 && isdigit(currentLine[1])) {
            int code = currentLine[1] - '0';
            // Map the code to a color.
            switch (code) {
                case 0:
                    lineColor = Scalar(255, 255, 255); // White
                    break;
                case 1:
                    lineColor = Scalar(0, 255, 0);       // Green
                    break;
                case 2:
                    lineColor = Scalar(0, 255, 255);     // Yellow
                    break;
                case 3:
                    lineColor = Scalar(0, 0, 255);       // Red
                    break;
                default:
                    // For any unknown code, keep the default textColor.
                    lineColor = textColor;
                    break;
            }
            // Remove the color code from the beginning of the line.
            // This removes the '*' and the digit. Optionally, remove a following space.
            currentLine.erase(0, 2);
            if (!currentLine.empty() && currentLine[0] == ' ')
                currentLine.erase(0, 1);
        }

        // Calculate the text origin for this line.
        Point textOrg(leftMargin, initialY + static_cast<int>(i) * (fontHeight + lineSpacing));

        // Draw the black border around the text by drawing the text several times at slight offsets.
        for (int dx = -borderThickness; dx <= borderThickness; ++dx) {
            for (int dy = -borderThickness; dy <= borderThickness; ++dy) {
                // Skip the center position so we don't overdraw the main text.
                if (dx == 0 && dy == 0)
                    continue;
                Point borderOrg = textOrg + Point(dx, dy);
                ft2->putText(frame, currentLine, borderOrg, fontHeight,
                             Scalar(0, 0, 0), -1, LINE_AA, false);
            }
        }

        // Finally, draw the main text using the chosen color.
        ft2->putText(frame, currentLine, textOrg, fontHeight,
                     lineColor, -1, LINE_AA, false);
    }
}

void draw_text_bottom(Mat &frame, const string &text,
                      int fontHeight, Scalar textColor,
                      int borderThickness = 2) {
    // Split the text into lines based on newline characters
    vector<string> lines;
    istringstream textStream(text);
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
    int initialY = 0.75*H + fontHeight - 100;

    // For each line, render the text with a black border and then with the main text color.
    for (size_t i = 0; i < lines.size(); ++i) {
        const string &currentLine = lines[i];

        // Calculate the size of the current line and center it horizontally
        Size textSize = ft2->getTextSize(currentLine, fontHeight, -1, nullptr);
        Point textOrg((frame.cols - textSize.width) / 2,
                      initialY + static_cast<int>(i) * (fontHeight + lineSpacing));

        // Draw the black border by rendering the text several times at offsets
        for (int dx = -borderThickness; dx <= borderThickness; ++dx) {
            for (int dy = -borderThickness; dy <= borderThickness; ++dy) {
                // Skip the center position where the main text will be drawn.
                if (dx == 0 && dy == 0)
                    continue;
                ft2->putText(frame, currentLine, textOrg + Point(dx, dy),
                             fontHeight, Scalar(0, 0, 0), -1, LINE_AA, false);
            }
        }

        // Render the main (non-border) text on top of the border
        ft2->putText(frame, currentLine, textOrg, fontHeight, textColor,
                     -1, LINE_AA, false);
    }
}

void draw_horizontal_black_band(cv::Mat &frame, int centerY = 960, int fadeSize = 150)
{
    // Define the region that's fully black:
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
    double intensity = 2.5 - (cur-st)*0.15; // Increased initial intensity and decay rate
    intensity = std::max(1., intensity);

    mt.convertTo(mt, -1, intensity, 0);
}

void zoom_frame(Mat &mt, int st, int cur) {
    // Sine ease-out: fast at start, slow at end
    double progress = std::min(1.0, (cur-st) / 6.0); // 0 to 1 over 6 frames (was 10)
    double t = sin(progress * M_PI_2); // ease-out
    double zoom = 1.10 * (1.0 - t) + 1.0 * t;
    
    if (zoom > 1.0) {
        int newWidth = mt.cols * zoom;
        int newHeight = mt.rows * zoom;
        int offsetX = (newWidth - mt.cols) / 2;
        int offsetY = (newHeight - mt.rows) / 2;
        Mat zoomed;
        resize(mt, zoomed, Size(newWidth, newHeight), 0, 0, INTER_LINEAR);
        Rect roi(offsetX, offsetY, mt.cols, mt.rows);
        zoomed(roi).copyTo(mt);
    }
}

Mat video::write_evt(VideoCapture &src, const vec<int> &active, int frm, const vec<Evt> &evts, vec<float> &aud) {
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
                shake_frame(mt, 15, evts[ind].bg_srcst_, srcfrm); // Reduced shake amplitude for better compatibility with zoom
                glitch_frame(mt);
                flash_frame(mt, evts[ind].bg_srcst_, srcfrm);
                zoom_frame(mt, evts[ind].bg_srcst_, srcfrm);
            }

            // copy to res
            mt.copyTo(res);

            // Ensure top and bottom regions stay black
            for (int r = 0; r < R1; r++) {
                for (int c = 0; c < W; c++) {
                    res.at<Vec3b>(r,c) = Vec3b(0,0,0);
                }
            }
            for (int r = R2; r < H; r++) {
                for (int c = 0; c < W; c++) {
                    res.at<Vec3b>(r,c) = Vec3b(0,0,0);
                }
            }
        } else if (evts[ind].type==EvtType::Text) {
            // TEXT
            // draw_glowing_text(res, evts[ind].text_str, "res/font.ttf", 60, Scalar(255,255,255), Scalar(0,0,255), 45);
            draw_glowing_text(res, evts[ind].text_str, evts[ind].text_big ? 90 : 60, Scalar(255,255,255), Scalar(0,0,255), 45);
        } else if (evts[ind].type==EvtType::Region) {
            // REGION
            int srcfrm=evts[ind].region_srcst_ + frm-evts[ind].st;
            src.set(CAP_PROP_POS_FRAMES, srcfrm);

            // copy src frame
            Mat mt;
            if (!src.read(mt)) {
                printf("[write_evt region event] ERROR: frame nonexistent (frame index %d, video frames %d).\n", srcfrm, (int)src.get(CAP_PROP_FRAME_COUNT));
                exit(1);
            }

            // effects
            Point dst=evts[ind].region_dst;
            if (srcfrm-evts[ind].region_srcst_<10) {
                shake_frame(mt, 20, evts[ind].region_srcst_, srcfrm);
                glitch_frame(mt);
                flash_frame(mt, evts[ind].region_srcst_, srcfrm);

                double shake=max(0., 1.-(srcfrm-evts[ind].region_srcst_)*0.1);
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
        } else if (evts[ind].type==EvtType::LeftText) {
            // LEFT TEXT
            draw_text_left(res, evts[ind].left_text_str, 80, Scalar(255,255,255));
        } else if (evts[ind].type==EvtType::Caption) {
            // CAPTION
            draw_text_bottom(res, evts[ind].caption_text, 70, Scalar(255,255,255));
            if (frm==evts[ind].st) {
                string cmd="espeak-ng \""+tts_preproc(evts[ind].caption_text)+"\" -w out/"+to_string(sz(aud))+".wav";
                system(cmd.c_str());
                aud.push_back(frm2t(evts[ind].st));
            }
        } else if (evts[ind].type==EvtType::TimerBar) {
            // TIMER BAR
            int y=0.75*H;
            int h=80;
            int w=0.6*W;
            int x=W/2-w/2;
            float p=(float)(frm-evts[ind].st)/(evts[ind].nd-evts[ind].st);
            int px=x+p*w;
            for (int i=y; i<y+h; ++i) {
                for (int j=x; j<x+w; ++j) {
                    res.at<Vec3b>(i,j) = j>px ? Vec3b(128,128,128) : Vec3b(50,50,255);
                }
            }
        }
    }

    return res;
}
