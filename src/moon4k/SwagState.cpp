#include "SwagState.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include "game/Conductor.h"

const std::string SwagState::soundExt = ".ogg";

SwagState::SwagState()
    : lastBeat(0.0f)
    , lastStep(0.0f)
    , curStep(0)
    , curBeat(0) {
}

SwagState::~SwagState() {
    destroy();
}

void SwagState::create() {
    std::cout << "SwagState::create()" << std::endl;
}

void SwagState::update(float elapsed) {
    int oldStep = curStep;

    updateCurStep();
    updateBeat();

    if (oldStep != curStep && curStep > 0) {
        stepHit();
    }

    if (_subStates.empty()) {
    } else {
        updateSubState(elapsed);
    }
}

void SwagState::render() {
    if (!_subStates.empty()) {
        renderSubState();
    }
}

void SwagState::destroy() {
}

void SwagState::updateBeat() {
    curBeat = static_cast<int>(std::floor(curStep / 4.0f));
}

void SwagState::updateCurStep() {
    BPMChangeEvent lastChange = {
        0,      // stepTime
        0.0f,   // songTime
        0       // bpm
    };

    for (const auto& change : Conductor::bpmChangeMap) {
        if (Conductor::songPosition >= change.songTime) {
            lastChange = change;
        }
    }

    float stepsSinceChange = (Conductor::songPosition - lastChange.songTime) / Conductor::stepCrochet;
    curStep = lastChange.stepTime + static_cast<int>(std::floor(stepsSinceChange));
}

void SwagState::stepHit() {
    if (curStep % 4 == 0) {
        beatHit();
    }
}

void SwagState::beatHit() {
    // become GAY!
}
