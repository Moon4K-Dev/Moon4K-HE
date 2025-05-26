#include "Conductor.h"
#include <iostream>

int Conductor::bpm = 100;
float Conductor::crochet = ((60.0f / bpm) * 1000.0f);
float Conductor::stepCrochet = crochet / 4.0f;
float Conductor::songPosition = 0.0f;
float Conductor::lastSongPos = 0.0f;
float Conductor::offset = 0.0f;

int Conductor::safeFrames = 10;
float Conductor::safeZoneOffset = (safeFrames / 60.0f) * 1000.0f;

std::vector<BPMChangeEvent> Conductor::bpmChangeMap;

Conductor::Conductor() {
}

void Conductor::mapBPMChanges(const SwagSong& song) {
    bpmChangeMap.clear();

    int curBPM = song.bpm;
    int totalSteps = 0;
    float totalPos = 0.0f;

    for (size_t i = 0; i < song.notes.size(); i++) {
        if (song.notes[i].changeBPM && song.notes[i].bpm != curBPM) {
            curBPM = song.notes[i].bpm;
            BPMChangeEvent event{
                totalSteps,
                totalPos,
                curBPM
            };
            bpmChangeMap.push_back(event);
        }

        int deltaSteps = song.notes[i].lengthInSteps;
        totalSteps += deltaSteps;
        totalPos += ((60.0f / curBPM) * 1000.0f / 4.0f) * deltaSteps;
    }

    std::cout << "new BPM map BUDDY ";
    for (const auto& event : bpmChangeMap) {
        std::cout << "{ stepTime: " << event.stepTime 
                  << ", songTime: " << event.songTime 
                  << ", bpm: " << event.bpm << " } ";
    }
    std::cout << std::endl;
}

void Conductor::changeBPM(int newBpm) {
    bpm = newBpm;
    crochet = ((60.0f / bpm) * 1000.0f);
    stepCrochet = crochet / 4.0f;
}
