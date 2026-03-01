
namespace GUIKIT {

auto pColorChooser::choose(ColorChooser::State& state) -> std::optional<unsigned> {
    NSColorPanel* panel = [NSColorPanel sharedColorPanel];
    [NSColorPanel setPickerMode:NSColorPanelModeWheel];
    
    [panel setColor:pHelper::RGBToNSColor(state.defaultColor)];
    
    [panel setTarget:state.window->p.cocoaWindow];
    [panel setAction:@selector(colorDidChange:)];
    
    [panel makeKeyAndOrderFront:nil];
    
    return std::nullopt;
}

}
