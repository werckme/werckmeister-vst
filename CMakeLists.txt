cmake_minimum_required(VERSION 3.15)
execute_process(COMMAND git --work-tree=. log --pretty=format:%h -n 1 OUTPUT_VARIABLE GIT_VERSION)
execute_process(COMMAND git branch --show-current OUTPUT_VARIABLE GIT_BRANCH_NAME)   

# Project name.
project(werckmeister-vst VERSION 0.0.1)

add_subdirectory(ext/JUCE)  

juce_set_vst2_sdk_path(ext/vstsdk2.4)

juce_add_plugin(WerckmeisterVST
    # VERSION ...                               # Set this if the plugin version is different to the project version
    # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    # ICON_SMALL ...
    COMPANY_NAME werckme.github.io              # Specify the name of the plugin's author
    COMPANY_WEBSITE "https://werckme.github.io"
    IS_SYNTH FALSE                              # Is this a synth or an effect?
    NEEDS_MIDI_INPUT FALSE                      # Does the plugin need midi input?
    NEEDS_MIDI_OUTPUT TRUE                      # Does the plugin need midi output?
    IS_MIDI_EFFECT TRUE
    VST_NUM_MIDI_OUTS 2                         
    PLUGIN_MANUFACTURER_CODE wmvt               # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE Wmvt                            # A unique four-character plugin id with exactly one upper-case character
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
    FORMATS AU VST VST3 Standalone                  # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    PRODUCT_NAME "Werckmeister VST"
    )        # The name of the final executable, which can differ from the target name

target_sources(WerckmeisterVST
    PRIVATE
        PluginEditor.cpp
        PluginProcessor.cpp
        Compiler.cpp
        PluginStateData.cpp
        FilterComponent.cpp
        FileWatcher.cpp)

target_compile_definitions(WerckmeisterVST
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        )

# If your target needs extra binary assets, you can add them here. The first argument is the name of
# a new static library target that will include all the binary resources. There is an optional
# `NAMESPACE` argument that can specify the namespace of the generated binary data class. Finally,
# the SOURCES argument should be followed by a list of source files that should be built into the
# static library. These source files can be of any kind (wav data, images, fonts, icons etc.).
# Conversion to binary-data will happen when your target is built.

# juce_add_binary_data(AudioPluginData SOURCES ...)

target_link_libraries(WerckmeisterVST
    PRIVATE
        # AudioPluginData           # If we'd created a binary data target, we'd link to it here
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)