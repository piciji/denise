
struct SliderLayout : GUIKIT::HorizontalLayout {    
    GUIKIT::Label name;
    GUIKIT::CheckBox active;
    GUIKIT::Label value;
    GUIKIT::HorizontalSlider slider;
    GUIKIT::Button defaultButton;
    
    std::string unit = "";
    bool withActivator = false;
    bool withButton = false;   
    
    SliderLayout( std::string unit = "%", bool withActivator = false, bool withButton = false) {
        this->withActivator = withActivator;
        this->withButton = withButton;
        this->unit = unit;

        if (withActivator)
            append(active, {0u, 0u}, 10);
        else
            append(name, {0u, 0u}, 10);
            
        append(value, {0u, 0u}, 8);
        append(slider, {~0u, 0u}, withButton ? 8 : 0 );
        
        if (withButton)
            append(defaultButton, {0u, 0u});

        setAlignment(0.5);
    }

    auto setValue(std::string text) -> void {
        value.setText( text + " " + unit );
    }

    auto getLabelMinimumWidth() -> unsigned {
        if (withActivator)
            return active.minimumSize().width;

        return name.minimumSize().width;
    }
    
    auto updateValueWidth( std::string maxValue, unsigned spacing = 10 ) -> void {

        if (withActivator)
            update(active,{0u, 0u}, spacing);
        else
            update(name,{0u, 0u}, spacing);
        
        GUIKIT::Label test;
        test.setText( maxValue ); 
        
        update( value, {test.minimumSize().width, 0u}, 8 );
    }
    
    static auto scale( std::vector<SliderLayout*> sliders, std::string maxValue, unsigned neededWidth = 0) -> unsigned {

        GUIKIT::Label test;
        test.setText( maxValue );      
                
        for (auto slider : sliders)
            neededWidth = std::max(neededWidth, slider->getLabelMinimumWidth());        

        auto valueSize = test.minimumSize();

        for (auto slider : sliders) {
            
            slider->children[ 0 ].size.width = neededWidth;
                        
            slider->children[ 1 ].size.width = valueSize.width;
        }
        
        return neededWidth;
    }
};
