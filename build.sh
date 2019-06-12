if [ -d $HOME/JUCE ]; then
    echo "A $HOME/JUCE directory is already present, skipping download"
else
    if [ "$OSTYPE" == "linux-gnu" ]; then
        os=linux
    elif [ "$OSTYPE" == "darwin"* ]; then
        os=osx
    fi
    if [ -f "./juce-5.4.3-${os}.zip" ]; then
        echo "./juce-5.4.3-${os}.zip already on disk, skipping download."
    else
        echo "Downloading Juce for ${os}..."
        wget -c -nv "https://d30pueezughrda.cloudfront.net/juce/juce-5.4.3-${os}.zip"
    fi
    unzip ./juce-5.4.3-${os}.zip -d $HOME
fi

echo "Juce installed in $HOME/JUCE, starting build."

if [ "$OSTYPE" == "linux-gnu" ]; then
    cd ./Builds/LinuxMakefile
    make -j$(nproc) CONFIG=Release
elif [ "$OSTYPE" == "darwin"* ]; then
    brew update
    cd ./Builds/MacOSX
    xcodebuild -project sfizz.xcodeproj -alltargets -configuration Release build
fi
