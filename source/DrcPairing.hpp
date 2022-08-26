#pragma once

#include "Gfx.hpp"
#include "Text.hpp"

#include <coreinit/time.h>
#include <coreinit/im.h>

class SceneMgr;

class DrcPairing {
public:
    DrcPairing(SceneMgr* sceneMgr);
    virtual ~DrcPairing();

    void Update();

    void DrawScene(Gfx* gfx, Gfx::Target target);

private:
    static void SyncButtonCallback(IOSError error, void* arg);
    static inline bool cancelPairing;

    IOSHandle imHandle;
    IMRequest* imRequest;
    IMRequest* imCancelRequest;
    IMEventMask imEventMask;

    SceneMgr* sceneMgr;
    uint32_t frameCount;

    Sprite* background;
    Text titleText;
    Text pinText;
    Text timeoutText;
    Text doneText;
    Text errorText;
    Text hintText;
    Text hintText2;

    enum State {
        STATE_START,
        STATE_PAIRING,
        STATE_ERROR,
        STATE_DONE,
    };
    State state;
    OSTime startTime;
};
