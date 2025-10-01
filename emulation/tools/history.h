
#pragma once

#include "memchangetracker.h"
#include "serializer.h"

template<typename A, typename V, unsigned tracker>
struct MemState {
    MemChangeTracker<A, V> trackers[tracker];
    Emulator::MemSerializer serializer;
};

template<typename A, typename V, unsigned tracker>
struct History {
    std::vector<MemState<A, V, tracker>*> memStates;
    unsigned curPos;
    unsigned frames;

    unsigned steps;
    unsigned curStep;

    unsigned curSize;
    unsigned maxSize;
    bool rewind;

    History() {
        steps = 0;
        maxSize = 0;
        reset();
        appendHistory();
    }

    ~History() {
        clearMemstate();
    }

    auto clearMemstate() -> void {
        for (auto memState : memStates)
            delete memState;
        memStates.clear();
    }

    auto reset() -> void {
        curStep = steps ? steps - 1 : 0;
        curPos = 0;
        frames = 0;
        curSize = 0;
        rewind = false;
    }

    auto enable() -> bool {
        return steps != 0;
    }

    auto remember() -> MemState<A, V, tracker>* {
        if (++curStep == steps) {
            curStep = 0;
            MemState<A, V, tracker>* memState = getCurMemstate();
            unsigned _c = getCapacity(memState);
            setNextHistoryPos(true);

            if (maxSize >= (curSize + _c)) {
                curSize += _c;
                if (frames++ >= memStates.size())
                    appendHistory();
            } //else
                //fprintf(stderr, "full %i %i %i %i", curSize, maxSize, memState->serializer.capacity(), memStates.size());

            return getCurMemstate();
        }

        return nullptr;
    }

    auto apply() -> MemState<A, V, tracker>* {
        MemState<A, V, tracker>* memState = getCurMemstate();

        if (frames) {
            for (auto& t : memState->trackers)
                t.apply();

            setNextHistoryPos(false);
            if (!--frames)
                curSize = 0;
        }

        memState = getCurMemstate();
        unsigned _c = getCapacity(memState);
        if (curSize > _c)
            curSize -= _c;
        else
            curSize = 0;

        curStep = steps - 1;

        if (memState->serializer.data())
            return memState;

        //fprintf(stderr, "miss history %i %i %i \n", frames, curPos, (int)memStates.size());

        return nullptr;
    }

    auto getCurMemstate() -> MemState<A, V, tracker>* {
        return memStates[curPos];
    }

    auto config(unsigned _steps, unsigned maxSizeInMb) -> void {
        unsigned newSize = maxSizeInMb * 1024 * 1024;
        if (newSize < maxSize) {
            reset();
            clearMemstate();
            appendHistory();
        }
        maxSize = newSize;

        unsigned stepsBefore = steps;
        steps = _steps;

        if (stepsBefore > steps) {
            reset();
            for (auto memState : memStates) {
                for (auto& t : memState->trackers)
                    t.initSize();
            }
        }
    }

    auto setRewind(bool state) -> void {
        if (!steps) {
            reset();
            return;
        }

        if (rewind != state) {
            rewind = state;

            if (state) {
                if (frames)
                    frames--;
            } else
                frames++;
        }
    }

    auto setNextHistoryPos(bool forward) -> void {
        if (forward) {
            if (++curPos >= memStates.size())
                curPos = 0;

        } else if (!curPos)
            curPos = memStates.size() - 1;
        else
            curPos--;
    }

    auto appendHistory() -> void {
        unsigned elements = 100;
        auto& v = memStates;
        v.reserve(v.size() + elements);
        unsigned _pos = (curPos >= v.size()) ? v.size() : curPos;

        while (elements--)
            v.insert(v.begin() + _pos, new MemState<A, V, tracker>);
    }

    auto getCapacity(MemState<A, V, tracker>* memState) -> unsigned {
        unsigned _c = memState->serializer.capacity();
        for (auto& t : memState->trackers)
            _c += t.size;

        return _c;
    }
};
