
auto pStepButton::minimumSize() -> Size {
    Size size = getMinimumSize();
    return {size.width, size.height};
	
//	auto context = gtk_widget_get_style_context (gtkWidget);
//    auto state = gtk_widget_get_state_flags (gtkWidget);
//	GtkBorder border;
//
//	gtk_style_context_get_border (context, state, &border);
//
//    return {size.width + 4 + border.left + border.right + 60,
//		size.height + border.top + border.bottom + 10};
		
}

auto pStepButton::updateRange() -> void {    
	
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(gtkWidget), stepButton.state.minValue, stepButton.state.maxValue);
}

auto pStepButton::setValue( int16_t value ) -> void {
	locked = true;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtkWidget), value);
    setFont( widget.font() );    
	locked = false;
}

auto pStepButton::create() -> void {
    destroy();
    gtkWidget = gtk_spin_button_new_with_range(0, 1, 1);	
	gtk_spin_button_set_wrap( GTK_SPIN_BUTTON(gtkWidget), true );
	
	g_signal_connect(G_OBJECT(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(gtkWidget))), "value-changed", G_CALLBACK(pStepButton::onChange), (gpointer)&stepButton);
	
	pSystem::addCssClass(gtkWidget, "removePadding");	
	pSystem::applyCss( gtkWidget, ".removePadding button { padding-left: 2px; padding-right: 2px; padding-top: 0px; padding-bottom: 0px;}" );
}

auto pStepButton::init() -> void {
    create();
    setText(widget.text());
}


auto pStepButton::onChange(GtkAdjustment* adjustment, StepButton* self) -> void {
	
	if (Application::isQuit)
		return;
	
	StepButton& stepButton = *self;
	
	int newValue = gtk_adjustment_get_value( adjustment );

	stepButton.state.value = newValue;
	stepButton.Widget::state.text = std::to_string(stepButton.state.value);

	if (!stepButton.p.locked && stepButton.onChange)
		stepButton.onChange();

}
