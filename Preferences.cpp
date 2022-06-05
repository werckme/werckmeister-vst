#include "Preferences.h"


Preferences::Preferences()
{
    int w = 600, h = 250;
    setSize(w, h);
    int row = 15;
    //
    sheetPathLabelText_1.setBounds(5, row, w - 10, 20);
    sheetPathLabelText_1.setText("Specifiy the folder path where the executables \"sheetp\" and \"sheetc\" are located.", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(sheetPathLabelText_1);
    //
    row += 15;
    sheetPathLabelText_2.setBounds(5, row, w - 10, 25);
    sheetPathLabelText_2.setText("If empty, system search PATH will be used", juce::NotificationType::dontSendNotification);
    addAndMakeVisible(sheetPathLabelText_2);

    // 
    row += 30;
    sheetPath.setBounds(5, row, w - 10, 22);
    addAndMakeVisible(sheetPath);

    //
    row += 30;
    findPathBtn.setButtonText("Select");
    findPathBtn.setBounds(w-50 - 5, row, 50, 30);
    findPathBtn.onClick = std::bind(&Preferences::select, this);
    addAndMakeVisible(findPathBtn);

    //
    okBtn.setButtonText("OK");
    okBtn.setBounds(w-50 - 5, h - 35, 50, 30);
    okBtn.onClick = std::bind(&Preferences::close, this);
    addAndMakeVisible(okBtn);
}

void Preferences::close()
{
    auto parentWindow = findParentComponentOfClass<juce::DialogWindow>();
    parentWindow->closeButtonPressed();
}

void Preferences::paint (juce::Graphics&)
{

}

void Preferences::resized() 
{

}

void Preferences::select()
{
    using namespace juce;
    myChooser = std::make_unique<FileChooser> ("Find the Werckmeister binary path",
                                               File());
 
    auto folderChooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;
 
    myChooser->launchAsync (folderChooserFlags, [this] (const FileChooser& chooser)
    {
        File sheetFile (chooser.getResult());
        sheetPath.setText(sheetFile.getFullPathName());
    }); 
}