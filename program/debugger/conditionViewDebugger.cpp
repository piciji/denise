
#include "conditionViewDebugger.h"
#include "../thread/emuThread.h"
#include "../../program/program.h"
#include "debugger.h"
#include "watcherHelper.h"

ConditionViewDebugger::ConditionViewDebugger(Debugger* debugger) : GUIKIT::Window(GUIKIT::Window::Hints::No_Title) {
    this->debugger = debugger;
}

ConditionViewDebugger::ConditionLayout::Expression::Expression() {
    append( check, {0u, 0u}, 10 );
    append( compareCombo, {0u, 0u}, 10 );
    append( compareVal, {~0u, 0u} );

    setAlignment( 0.5 );
}

ConditionViewDebugger::ConditionLayout::HitCount::HitCount() {
    append( check, {0u, 0u}, 10 );
    append( compareCombo, {0u, 0u}, 10 );
    append( compareVal, {~0u, 0u} );

    setAlignment( 0.5 );
}

ConditionViewDebugger::ConditionLayout::Control::Control() {
    append( spacer, {~0u, 0u} );
    append( closeButton, {0u, 0u} );
}

ConditionViewDebugger::ConditionLayout::ConditionLayout() {
    info.setEditable( false );
    append( expression, {~0u, 0u}, 10 );
    append( hitCount, {~0u, 0u}, 10 );
    append( info, {~0u, ~0u}, 10 );
    append( control, {~0u, 0u});

    setMargin( 10 );
}

auto ConditionViewDebugger::create(DbgWatcher* watcher, GUIKIT::Position position) -> void {
    unfocusTimer.setInterval(100);

    unfocusTimer.onFinished = [this, watcher]() {
        unfocusTimer.setEnabled(false);

        onUnFocus = [this, watcher]() {
            if (this->visible()) {
                emuThread->lock();
                debugger->updateBreakpointVisuals(watcher);
                emuThread->unlock();
                setVisible( false );
            }
        };
    };

    setGeometry( {position.x + 20, position.y + 20, 500, 200} );
    auto* ex = &conditionLayout.expression;
    auto* hc = &conditionLayout.hitCount;

    conditionLayout.control.closeButton.onActivate = [this, watcher]() {
        emuThread->lock();
        debugger->updateBreakpointVisuals(watcher);
        emuThread->unlock();
        setVisible( false );
    };

    hc->check.onToggle = [this, watcher, hc](bool checked) {
        watcher->useHitCount = checked;
        hc->compareCombo.setEnabled( checked );
        hc->compareVal.setEnabled( checked );
        emuThread->lock();
        debugger->updateWatchpointCondition(*watcher);
        emuThread->unlock();
    };

    ex->check.onToggle = [this, watcher, ex](bool checked) {
        watcher->useExpression = checked;
        ex->compareCombo.setEnabled( checked );
        ex->compareVal.setEnabled( checked );
        emuThread->lock();
        if (!debugger->updateWatchpointCondition(*watcher)) {
            ex->compareVal.setForegroundColor( DEBUG_COLOR );
        } else {
            ex->compareVal.resetForegroundColor();
        }
        emuThread->unlock();
    };

    hc->compareCombo.onChange = [this, watcher, hc]() {
        watcher->hitCountCompare = hc->compareCombo.selection();
        emuThread->lock();
        debugger->updateWatchpointCondition(*watcher);
        emuThread->unlock();
    };

    ex->compareCombo.onChange = [this, watcher, ex]() {
        watcher->expressionCompare = ex->compareCombo.selection();
        emuThread->lock();
        if (!debugger->updateWatchpointCondition(*watcher)) {
            ex->compareVal.setForegroundColor( DEBUG_COLOR );
        } else {
            ex->compareVal.resetForegroundColor();
        }
        emuThread->unlock();
    };

    hc->compareVal.onChange = [this, watcher, hc]() {
        std::string _v = hc->compareVal.text();
        watcher->hitCount = GUIKIT::String::convertToNumber( _v, 0 );
        emuThread->lock();
        debugger->updateWatchpointCondition(*watcher);
        emuThread->unlock();
    };

    ex->compareVal.onChange = [this, watcher, ex]() {
        watcher->expression = ex->compareVal.text();
        emuThread->lock();
        if (!debugger->updateWatchpointCondition(*watcher)) {
            ex->compareVal.setForegroundColor( DEBUG_COLOR );
        } else {
            ex->compareVal.resetForegroundColor();
        }
        emuThread->unlock();
    };

    hc->check.setText( trans->getA( trans->getA( "hit count" ) ) );
    ex->check.setText( trans->getA( trans->getA( "expression" ) ) );
    hc->compareCombo.append( "==" );
    hc->compareCombo.append( ">=" );
    ex->compareCombo.append( "true" );
    ex->compareCombo.append( trans->getA( "change" ) );
    hc->check.setChecked( watcher->useHitCount );
    ex->check.setChecked( watcher->useExpression );
    hc->compareCombo.setSelection( watcher->hitCountCompare );
    ex->compareCombo.setSelection( watcher->expressionCompare );
    hc->compareVal.setText( std::to_string( watcher->hitCount ) );
    ex->compareVal.setText( watcher->expression );
    hc->compareCombo.setEnabled( watcher->useHitCount );
    ex->compareCombo.setEnabled( watcher->useExpression );
    hc->compareVal.setEnabled( watcher->useHitCount);
    ex->compareVal.setEnabled( watcher->useExpression);

    std::string placeHolder = "";

    if (debugger->isAmiga()) {
        for (auto& cond: LIBAMI::DebuggerSnapshot::breakConditions)
            placeHolder += " " + (std::string)cond.ident;

        GUIKIT::String::replace( placeHolder, ":", ":$000000" );
    } else {
        if (debugger->mode == Debugger::Mode::SCPU) {
            for (auto& cond: LIBC64::DebuggerSnapshot::breakConditionsSCPU)
                placeHolder += " " + (std::string)cond.ident;
        } else {
            for (auto& cond: LIBC64::DebuggerSnapshot::breakConditions)
                placeHolder += " " + (std::string)cond.ident;
        }

        GUIKIT::String::replace( placeHolder, ":", ":$0000" );
    }

    conditionLayout.info.setText(
        trans->getA("operators") + ": $ | & ^ || &&  == != <= < << >= > >> + - * / % \n" +
        trans->getA("replacements") + ": " + placeHolder
    );

    conditionLayout.control.closeButton.setText( trans->getA( "close" ) );
    append( conditionLayout );

    GUIKIT::Layout::alignChildWidth({hc, ex}, 0);
    GUIKIT::Layout::alignChildWidth({hc, ex}, 1);
}

auto ConditionViewDebugger::open() -> void {
    unfocusTimer.setEnabled();
    setVisible(  );
}
