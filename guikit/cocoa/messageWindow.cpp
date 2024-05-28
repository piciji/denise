namespace GUIKIT {

auto pMessageWindow::callAlert(MessageWindow::State& state, NSAlertStyle style) -> NSInteger {
    NSAlert* alert = [[[NSAlert alloc] init] autorelease];
    if(!state.title.empty())
        [alert setMessageText:[NSString stringWithUTF8String:state.title.c_str()]];
    
    [alert setInformativeText:[NSString stringWithUTF8String:state.text.c_str()]];
    MessageWindow::Trans trans = MessageWindow::trans;

    switch(state.buttons) {
        case MessageWindow::Buttons::Ok:
            [alert addButtonWithTitle:[NSString stringWithUTF8String:trans.ok.c_str()]];
            break;
        case MessageWindow::Buttons::OkCancel:
            [alert addButtonWithTitle:[NSString stringWithUTF8String:trans.ok.c_str()]];
            [alert addButtonWithTitle:[NSString stringWithUTF8String:trans.cancel.c_str()]];
            break;
        case MessageWindow::Buttons::YesNo:
            [alert addButtonWithTitle:[NSString stringWithUTF8String:trans.yes.c_str()]];
            [alert addButtonWithTitle:[NSString stringWithUTF8String:trans.no.c_str()]];
            break;
        case MessageWindow::Buttons::YesNoCancel:
            [alert addButtonWithTitle:[NSString stringWithUTF8String:trans.yes.c_str()]];
            [alert addButtonWithTitle:[NSString stringWithUTF8String:trans.no.c_str()]];
            [alert addButtonWithTitle:[NSString stringWithUTF8String:trans.cancel.c_str()]];
            break;
    }
    
    [alert setAlertStyle:style];
    return [alert runModal];
}

auto pMessageWindow::message(MessageWindow::State& state, NSAlertStyle style) -> MessageWindow::Response {
    if (!pApplication::appTimer)
        return MessageWindow::Response::Cancel;

    @autoreleasepool {
        __block NSInteger response;
        
        if([NSThread isMainThread])
            response = callAlert(state, style);
        else {
            dispatch_group_t group = dispatch_group_create();
            dispatch_group_enter(group);

            dispatch_async(dispatch_get_main_queue(), ^{
                response = callAlert(state, style);
                dispatch_group_leave(group);
            });
        
            dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
        }
        
        switch(state.buttons) {
            case MessageWindow::Buttons::Ok:
                if(response == NSAlertFirstButtonReturn) return MessageWindow::Response::Ok;
                break;
            case MessageWindow::Buttons::OkCancel:
                if(response == NSAlertFirstButtonReturn) return MessageWindow::Response::Ok;
                if(response == NSAlertSecondButtonReturn) return MessageWindow::Response::Cancel;
                break;
            case MessageWindow::Buttons::YesNo:
                if(response == NSAlertFirstButtonReturn) return MessageWindow::Response::Yes;
                if(response == NSAlertSecondButtonReturn) return MessageWindow::Response::No;
                break;
            case MessageWindow::Buttons::YesNoCancel:
                if(response == NSAlertFirstButtonReturn) return MessageWindow::Response::Yes;
                if(response == NSAlertSecondButtonReturn) return MessageWindow::Response::No;
                if(response == NSAlertThirdButtonReturn) return MessageWindow::Response::Cancel;
                break;
        }
    }

    return MessageWindow::Response::Cancel;
}

auto pMessageWindow::error(MessageWindow::State& state) -> MessageWindow::Response {
    return message(state, NSAlertStyleCritical);
}

auto pMessageWindow::information(MessageWindow::State& state) -> MessageWindow::Response {
    return message(state, NSAlertStyleInformational);
}

auto pMessageWindow::question(MessageWindow::State& state) -> MessageWindow::Response {
    return message(state, NSAlertStyleInformational);
}

auto pMessageWindow::warning(MessageWindow::State& state) -> MessageWindow::Response {
    return message(state, NSAlertStyleWarning);
}

}
