/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_48C47CC604E42661__
#define __JUCE_HEADER_48C47CC604E42661__

//[Headers]     -- You can add your own extra header files here --
#include "element/juce.hpp"

namespace element {
namespace gui {

    class GuiApp;

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PreferencesWidget  : public Component
{
public:
    //==============================================================================
    PreferencesWidget (GuiApp& gui_);
    ~PreferencesWidget();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setPage (const String& uri);

    //[/UserMethods]

    void paint (Graphics& g);
    void resized();



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    GuiApp& gui;

    class PageList;

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<PageList> pageList;
    ScopedPointer<GroupComponent> groupComponent;
    ScopedPointer<Component> pageComponent;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreferencesWidget)
};

//[EndFile] You can add extra defines here...
}}  /* namnespace element::gui */
//[/EndFile]

#endif   // __JUCE_HEADER_48C47CC604E42661__
