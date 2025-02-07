#pragma once
#include "util.h"
#include <algorithm>
#include <random>

inline vec<int> vidsrc_cuts(string name) {
    if (name=="bateman") {
        return {
            0,136,384,454,748,931,1529,1672,1738,1952,2092,2156,2272,2372,2504,2584,2663,
            2916,3001,3140,3365,3495,3760,3826,4005,4093,4209,4259,4446,4492,4564,4660,
            4942,5012,5339,5653,5717,5756,5797,5883,5919,6015,6103,6240,6349,6487,6728,
            6894,7133,7252,7342,7502,7547,7702,7907,8006,8079,8206,8354,8459,8534,8598,
            8735,8810,8867,8989,9073,9146,9175,9214,9343,9418,9568,9768,9877,9984,10316,
            10572
        };
    } else {
        assert(false);
    }
}

enum class EvtType {
    Bg=0,
    Region=1,
    HBar=2,
    Text=3,
    TopText=4,
};

struct Evt {
    EvtType type;
    int st; // start time
    int nd; // end time

    // fields with extra _ at end are deferred fields
    // fields with prefix _ are optional fields

    // bg
    int bg_srcst_=-1; // source video start frame
    int _bg_yoffset=0; // how far down bg should go

    // text
    string text_str; // what's displayed on screen
    bool text_big=false;

    // region
    int region_srcst_=-1; // source video start frame
    Rect region_src;
    Point region_dst; // top left
    float region_scale;

    // top text
    string top_text_str; // what's displayed on screen
};

inline Evt evt_bg(float st, float nd, int yoffset=0) {
    Evt e;
    e.type=EvtType::Bg;
    e.st=t2frm(st);
    e.nd=t2frm(nd);
    e._bg_yoffset=yoffset;
    return e;
}

inline Evt evt_txt(float st, float nd, string s) {
    Evt e;
    e.type=EvtType::Text;
    e.st=t2frm(st);
    e.nd=t2frm(nd);
    e.text_str=s;
    return e;
}

inline Evt evt_region(float st, float nd, Rect src, Point dst, float scale) {
    Evt e;
    e.type=EvtType::Region;
    e.st=t2frm(st);
    e.nd=t2frm(nd);
    e.region_src=src;
    e.region_dst=dst;
    e.region_scale=scale;
    return e;
}

inline Evt evt_toptxt(float st, float nd, string s) {
    Evt e;
    e.type=EvtType::TopText;
    e.st=t2frm(st);
    e.nd=t2frm(nd);
    e.top_text_str=s;
    return e;
}

inline Evt evt_hbar(float st, float nd) {
    Evt e;
    e.type=EvtType::HBar;
    e.st=t2frm(st);
    e.nd=t2frm(nd);
    return e;
}

namespace edit {
    inline map<string,vec<double>> beats={
        {"derniere-beatdrop", {
            0,
            3.893, // major
            4.221, 4.589, // down down
            5.094, 5.317, // up up
            5.773, 6.203, 6.459, 6.939, 7.175, // going down
            8.016, 8.371, 8.835, 9.07, // going up
            9.489, 9.889, 10.224, 10.719, // going down
            10.966, 11.094, 11.211, 11.312, 11.437, // middle
            11.565, 11.802, 12.139, 12.626, 12.802, // going up
            13.330, 13.7, 14.052, 14.536, // going down
            14.772, 15.259,
            15.613, 15.955, 16.434, 16.66,
            17.146, 17.514, 17.867, 18.342,
            18.585, 18.709, 18.827, 18.93, 19.06,
        }},
    };

    inline vec<Evt> audsrc_evts(string name, string &title) {
        vec<Evt> res;
        for (int i=0; i<sz(beats[name])-1; ++i) {
            res.push_back(evt_bg(beats[name][i], beats[name][i+1]));
        }

        if (name=="derniere-beatdrop") {
            // region
            // float scale=(float)(483-60)/W * 1.2;
            // res.push_back(evt_region(10.966, 11.565, Rect(0,R1,W,R2-R1+1), Point(10,R1+10), scale));
            // res.push_back(evt_region(11.094, 11.565, Rect(0,R1,W,R2-R1+1), Point(W/2+10,R1+10), scale));
            // res.push_back(evt_region(11.211, 11.565, Rect(0,R1,W,R2-R1+1), Point(W/2+10,(R1+R2)/2+10), scale));
            // res.push_back(evt_region(11.312, 11.565, Rect(0,R1,W,R2-R1+1), Point(10,(R1+R2)/2+10), scale));

            // text
            vec<pair<float,float>> in={
                {0, 3.893},
                {3.893, 5.773},
                {5.773, 8.016},
                {8.016, 9.489},
                {9.489, 11.565},
                {11.565, 13.330},
                {13.330, 15.259},
                {15.259, 17.146},
                {17.146, 19.06},
            };
            vec<string> s;

            int rng=rand()%100+1;
            if (rng<=50) {
                s.push_back("8 SIGNS YOU MIGHT BE A SIGMA MALE");

                vec<string> signs={
                    "YOU WALK YOUR OWN PATH",
                    "YOU'RE AN INTROVERT",
                    "YOU KNOW DARK PSYCHOLOGY",
                    "YOU WATCH SIGMA EDITS",
                    "YOU LISTEN TO PHONK",
                    "YOU DON'T CARE WHAT OTHER PEOPLE THINK",
                    "YOU'RE FOCUSED ON THE GRIND",
                    "YOUR FAVORITE LUNCHLY IS FIESTA NACHOS",
                    "YOU'RE ALWAYS TWO STEPS AHEAD",
                    "YOU'RE ESCAPING THE MATRIX",
                    "YOU CHOSE TO BE SINGLE",
                    "YOU ENJOY SOLITUDE",
                    "YOU HAVE NO FRIENDS",
                    "YOU HAVE NO RIZZ",
                    "YOU'RE ALWAYS ALONE",
                    "YOU HAVE A BEAST INSIDE YOU",
                    "YOU SEE RED WHEN YOU'RE ANGRY",
                    "NORADRENALINE IS YOUR BIGGEST WEAPON",
                };
                random_device rd;
                mt19937 g(rd());
                shuffle(all(signs),g);

                for (int i=0; i<=6; ++i) {
                    s.push_back(to_string(i+1)+". "+signs[i]);
                }
                s.push_back("8. YOU'RE SUBSCRIBED TO SIGMA CENTRAL");

                title = "8 SIGNS YOU MIGTH BE A SIGMA MALE";
            } else if (rng<=100) {
                s.push_back("DO YOU KNOW ALL THE SIGMA RULES?");

                vec<string> rules={
                    "MARCH TO YOUR OWN BEAT.",
                    "OBSERVE, DON'T SPEAK.",
                    "RESERVED BUT SELF-ASSURED.",
                    "UNCONVENTIONAL AND FREE-SPIRITED.",
                    "MASTER OF MY OWN DESTINY.",
                    "EMBRACE SOLITUDE, FIND STRENGTH.",
                    "I LIVE LIFE ON MY OWN TERMS.",
                    "I'M HERE TO STAND OUT, NOT FIT IN.",
                    "SOCIETY'S LABELS DON'T DEFINE ME.",
                    "SILENT STRENGTH, HIDDEN POTENTIAL.",
                };

                random_device rd;
                mt19937 g(rd());
                shuffle(all(rules),g);

                for (int i=1; i<=8; ++i) {
                    s.push_back(to_string(i)+". "+rules[i-1]);
                }

                title = "DO YOU KNOW ALL THE SIGMA RULES?";
            }

            for (int i=0; i<=8; ++i) {
                res.push_back(evt_txt(in[i].first, in[i].second, s[i]));
            }

            return res;
        } else {
            assert(false);
        }
    }
}

namespace meme {
    inline vec<Evt> audsrc_evts(string &title) {
        vec<Evt> res;
        vec<string> captions={
            "\"WHY ARE YOU ALWAYS SO QUIET?\"\nMY HONEST REACTION:",
            "POV: YOU'RE A SIGMA",
            "POV: YOU JUST LEARNED ABOUT MEWING",
            "POV: YOU JUST LEARNED ABOUT\nCARROTMAXXING",
            "POV: YOU JUST LEARNED ABOUT\nBONESMASHING",
            "ME AFTER A YEAR OF MEWING:",
            "POV: YOU MOG YOUR ENTIRE CLASS",
            "ME AFTER WATCHING SIGMA EDITS:",
            "ME ON MY WAY TO WATCH SIGMA EDITS:",
            "ME BC I HAVE NO FRIENDS\n(I'M A SIGMA):",
            "POV: YOU LOCKED IN AND\nBECAME A SIGMA",
            "POV: YOU GOT ON THE SIGMA GRIND",
            "ME AFTER I LISTEN TO PHONK:",
            "ME BECAUSE I SUBSCRIBED TO\nSIGMA CENTRAL:",
            "ME WALKING INTO CLASS KNOWING\nI'M A MYSTERIOUS SIGMA MALE:",
            "\"YOU CAN'T JUST WATCH\nSIGMA EDITS ALL DAY!\"\nMY HONEST REACTION:",
            "\"GET A JOB AND STOP\nWATCHING SIGMA CENTRAL!\"\nMY HONEST REACTION:",
            "\"WHEN ARE YOU GOING TO GET A JOB?\"\nI'M ON A DIFFERENT PATH NOW BROTHER.",
            "ME AFTER A PRODUCTIVE DAY OF\nARGUING ABOUT SIGMA MALES ON REDDIT:",
            "ME AFTER SUBSCRIBING TO\nSIGMA CENTRAL:",
            "\"WHY DO YOU HAVE NO FRIENDS?\"\nME, A SIGMA MALE:",
        };
        random_device rd;
        mt19937 g(rd());
        shuffle(all(captions),g);

        int cnt=8;
        float t=0;
        for (int i=0; i<cnt; ++i) {
            float tp=t + 3./25*sz(captions[i]);
            res.push_back(evt_bg(t, tp, 100));
            res.push_back(evt_toptxt(t, tp, captions[i]));
            t=tp;
        }

        title = "Relatable Sigma Meme Compilation";
        return res;
    }
}

namespace compare {
    inline map<string,vec<double>> beats={
        {"aura-compare", {
            4.832, 5.499, 6.166, 6.824, 7.485, 8.158, 8.837, 9.498, // character 1
            10.165, 10.825, 11.457, 12.171, 12.796, 13.457, 14.166, 14.827, // character 2
            15.512, 16.201, 16.892, 17.534, 18.225, 18.896, 19.548, 20.239, // rand
            20.880, 21.552, // last one (tiebreaker)
            22.223, 23.536, 26.271, // winner
        }},
    };

    inline vec<Evt> audsrc_evts(string name, string &title) {
        vec<Evt> res;
        vec<string> cats={
            "IQ", "BATTLE IQ", "AGILITY", "STRENGTH", "ENDURANCE", "SPEED", "EXPERIENCE",
            "SKILL", "WEAPONS", "POWER", "COMBAT", "STAMINA", "FEATS", "DEFENSE", "STEALTH",
            "TACTICS", "FOCUS", "WILLPOWER", "HAX", "LUCK", "INTIMIDATION", "RESOURCEFULNESS",
        };
        random_device rd;
        mt19937 g(rd());
        shuffle(all(cats),g);

        // normal clips
        vec<int> srcs={
            0, // bateman
            198, // shelby
            374, // walter
            564, // salesman
        };

        // clips on win
        vec<int> srcs2={
            1161, // bateman
            1348, // shelby
            855, // walter
            719, // salesman
        };

        vec<string> names={
            "PATRICK BATEMAN",
            "THOMAS SHELBY",
            "WALTER WHITE",
            "THE SALESMAN",
        };
        int i1=rand()%sz(srcs), i2=rand()%sz(srcs);
        if (i1==i2) {
            (++i2)%=sz(srcs);
        }
        printf("comparing %s and %s\n", names[i1].c_str(), names[i2].c_str());
        int score1=0, score2=0;

        vec<vec<int>> patterns={
            {i1,i2,i1,i2},
            {i1,i1,i2,i2},
            {i1,i2,i2,i1},
            {i1,i1,i2,i1},
        };

        int r1=417;
        int h=1920/2-100;

        // intro
        // res.push_back(evt_bg(0, 4.832));
        res.push_back(evt_region(0, beats[name][0], Rect(0, r1, 1080, h), Point(0,100), 1.));
        res.push_back(evt_region(0, beats[name][0], Rect(0, r1, 1080, h), Point(0,1920/2), 1.));
        // res.push_back(evt_hbar(0, beats[name][0]));
        res.push_back(evt_txt(0, beats[name][0], names[i1]+"\nVS\n"+names[i2]));
        res.back().text_big=true;
        res[0].region_srcst_ = srcs[i1];
        res[1].region_srcst_ = srcs[i2];

        auto trait_scene = [&](float t0, float t1, float t2, string trait, int winner) {
            // trait
            res.push_back(evt_region(t0, t1, Rect(0, r1, 1080, h), Point(0,100), 1.));
            res.back().region_srcst_ = srcs[i1];
            res.push_back(evt_region(t0, t1, Rect(0, r1, 1080, h), Point(0,100+h), 1.));
            res.back().region_srcst_ = srcs[i2];
            res.push_back(evt_txt(t0, t1, trait));
            res.back().text_big=true;

            // who gets it
            res.push_back(evt_bg(t1, t2));
            res.back().bg_srcst_ = srcs2[winner];
            (winner==i1?score1:score2)++;
            res.push_back(evt_txt(t1, t2, to_string(score1)+"-"+to_string(score2)));
            res.back().text_big=true;
        };

        // character 1
        for (int i=0; i<4; ++i) {
            trait_scene(beats[name][2*i], beats[name][2*i+1], beats[name][2*i+2], cats[i], i1);
        }

        // character 2
        for (int i=4; i<8; ++i) {
            trait_scene(beats[name][2*i], beats[name][2*i+1], beats[name][2*i+2], cats[i], i2);
        }

        // random (pattern based)
        int pat = rand()%sz(patterns);
        for (int i=8; i<12; ++i) {
            trait_scene(beats[name][2*i], beats[name][2*i+1], beats[name][2*i+2], cats[i], patterns[pat][i-8]);
        }

        // final for odd
        trait_scene(beats[name][24], beats[name][25], beats[name][26], cats[12], rand()%2 ? i1 : i2);

        // winner question
        res.push_back(evt_region(beats[name][26], beats[name][27], Rect(0, r1, 1080, h), Point(0,100), 1.));
        res.back().region_srcst_ = srcs[i1];
        res.push_back(evt_region(beats[name][26], beats[name][27], Rect(0, r1, 1080, h), Point(0,100+h), 1.));
        res.back().region_srcst_ = srcs[i2];
        // res.push_back(evt_hbar(st, nd));
        res.push_back(evt_txt(beats[name][26], beats[name][27], "WINNER?"));
        res.back().text_big=true;

        // verdict
        float st=beats[name][sz(beats[name])-2], nd=beats[name][sz(beats[name])-1];
        if (score1<score2) {
            res.push_back(evt_bg(st, nd));
            res.back().bg_srcst_ = srcs2[i2];
            res.push_back(evt_txt(st, nd, names[i2]+" WINS"));
            res.back().text_big=true;
        } else if (score1>score2) {
            res.push_back(evt_bg(st, nd));
            res.back().bg_srcst_ = srcs2[i1];
            res.push_back(evt_txt(st, nd, names[i1]+" WINS"));
            res.back().text_big=true;
        } else {
            res.push_back(evt_region(st, nd, Rect(0, r1, 1080, h), Point(0,100), 1.));
            res.back().region_srcst_ = srcs[i1];
            res.push_back(evt_region(st, nd, Rect(0, r1, 1080, h), Point(0,100+h), 1.));
            res.back().region_srcst_ = srcs[i2];
            // res.push_back(evt_hbar(st, nd));
            res.push_back(evt_txt(st, nd, "TIE"));
            res.back().text_big=true;
        }

        title = names[i1] + " VS " + names[i2] + " | ULTIMATE SIGMA BATTLE";
        return res;
    }
}
