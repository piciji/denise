

InputLayout::InputLayout() {
    setMargin(10);

    append(driverWrapper, {~0u, 0u}, 10);

	auto selectedDriver = program->getInputDriver();
	unsigned i = 0;
	for (auto& driver : inputDriver->available()) {
		driverLayout.combo.append(driver);
		if (driver == selectedDriver) {
			driverLayout.combo.setSelection(i);
		}
		i++;
	}
    if (driverLayout.combo.rows() > 0) driverWrapper.append(driverLayout, {~0u, 0u});
    if (driverLayout.combo.rows() == 1) driverLayout.setEnabled(false);


	driverLayout.combo.onChange = [this]() {
        emuThread->lock();
		globalSettings->set<std::string>("input_driver", driverLayout.combo.text());
        InputManager::rememberLastDeviceState();
		program->initInput();
        emuThread->unlock();

	};    

}

auto InputLayout::translate() -> void {


	driverLayout.name.setText( trans->get("driver", {}, true) );
}
