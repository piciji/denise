
#define MAX_RADIO_BOXES 8

struct SliderLayoutAlt : GUIKIT::VerticalLayout {
    GUIKIT::Label name;
    GUIKIT::Label valueLabel;
    GUIKIT::HorizontalSlider slider;
    GUIKIT::Button defaultButton;
    GUIKIT::HorizontalLayout hLayout;
    GUIKIT::Widget spacer;
    GUIKIT::RadioBox boxes[MAX_RADIO_BOXES];
    bool withButton = false;
    std::function<float (unsigned position)> updateWidget;
    std::function<float ()> requestDefault;

    unsigned decimalPlaces;
    float minimum;
    float maximum;
    float step;

    SliderLayoutAlt(bool withButton = false) {
        this->withButton = withButton;

        append(name, {0u, 0u}, 0);

        slider.onChange = [this](unsigned position) {
            float value = updateWidget(position);

            valueLabel.setText(GUIKIT::String::formatFloatingPoint(value, decimalPlaces, decimalPlaces == 0));
        };

        defaultButton.onActivate = [this]() {
            float value = requestDefault();

            if (useSlider()) {
                slider.setPosition((float) (value - minimum) / step);
                valueLabel.setText(GUIKIT::String::formatFloatingPoint(value, decimalPlaces, decimalPlaces == 0));
            } else {
                int steps = ((maximum - minimum) / step) + 0.5;
                steps += 1;
                std::vector<float> distances;
                float _minimum = minimum;

                for(int i = 0; i < std::min(steps, MAX_RADIO_BOXES); i++) {
                    distances.push_back( std::fabs(value - _minimum) );
                    _minimum += step;
                }
                auto it = std::min_element(distances.begin(), distances.end());
                boxes[std::distance(std::begin(distances), it)].setChecked();
            }
        };

        for(int i = 0; i < MAX_RADIO_BOXES; i++) {
            boxes[i].onActivate = [i, this]() {
                updateWidget(i);
            };
        }
    }

    auto updateView(float value, float minimum, float maximum, float step, unsigned decimalPlaces) -> void {
        this->minimum = minimum;
        this->maximum = maximum;
        this->step = step;
        this->decimalPlaces = decimalPlaces;

        int steps = ((maximum - minimum) / step) + 0.5;
        steps += 1;
        bool descriptor = (minimum == maximum) || ((maximum == step) && (step <= 0.01));

        hLayout.remove(valueLabel);
        hLayout.remove(slider);
        hLayout.remove(defaultButton);
        hLayout.remove(spacer);
        for (int i = 0; i < MAX_RADIO_BOXES; i++)
            hLayout.remove(boxes[i]);

        hLayout.reset();
        remove(hLayout);

        if (descriptor || (steps == 1) )
            return;

        if (steps <= MAX_RADIO_BOXES) {
            std::vector<GUIKIT::RadioBox*> groupBoxes;
            std::vector<float> distances;
            float _minimum = minimum;

            for(int i = 0; i < steps; i++) {
                hLayout.append( boxes[i], {0u, 0u}, 10 );
                boxes[i].setText( GUIKIT::String::formatFloatingPoint(_minimum, decimalPlaces, decimalPlaces == 0) );
                distances.push_back( std::fabs(value - _minimum) );
                _minimum += step;
                groupBoxes.push_back( &boxes[i] );
            }

            GUIKIT::RadioBox::setGroup(groupBoxes);
            auto it = std::min_element(distances.begin(), distances.end());
            boxes[std::distance(std::begin(distances), it)].setChecked();

            hLayout.append(spacer, {~0u, 0u});
        } else {
            slider.setLength(steps);
            unsigned position = (unsigned) ((value - minimum) / step);
            slider.setPosition(position);
            valueLabel.setText(GUIKIT::String::formatFloatingPoint(value, decimalPlaces, decimalPlaces == 0));

            hLayout.append(valueLabel, {0u, 0u}, 8);
            hLayout.append(slider, {~0u, 0u}, withButton ? 8 : 0 );
        }

        if (withButton)
            hLayout.append(defaultButton, {0u, 0u});

        hLayout.setAlignment(0.5);

        append(hLayout, {~0u, 0u});
    }

    auto useSlider() -> bool {
        return hLayout.has(slider);
    }

    static auto scale( std::vector<SliderLayoutAlt*> sliders, std::string maxValue) -> void {
        GUIKIT::Label test;
        test.setText( maxValue );

        auto valueSize = test.minimumSize();

        for (auto sliderLay : sliders) {

            if (sliderLay->useSlider())
                sliderLay->hLayout.children[ 0 ].size.width = valueSize.width;
        }
    }
};
