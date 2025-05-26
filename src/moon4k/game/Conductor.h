#pragma once
#include <vector>
#include "Song.h"

struct BPMChangeEvent {
    int stepTime;
    float songTime;
    int bpm;
};

class Conductor {
public:
    static int bpm;
    static float crochet; // beats in milliseconds
    static float stepCrochet; // steps in milliseconds
    static float songPosition;
    static float lastSongPos;
    static float offset;

    static int safeFrames;
    static float safeZoneOffset; // is calculated in create(), is safeFrames in milliseconds

    static std::vector<BPMChangeEvent> bpmChangeMap;

    Conductor();
    
    static void mapBPMChanges(const SwagSong& song);
    static void changeBPM(int newBpm);
};
