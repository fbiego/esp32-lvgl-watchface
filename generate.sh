#/bin/bash
#  Copyright (c) 2024 Daniel Kampert
# Download script for LVGL based ESP32 watchfaces. See https://github.com/Kampi/esp32-lvgl-watchface for more informations

if [ $# -ne 1 ]; then
    echo $0: usage: generate.sh URL-to-ESP32-Watchface
    exit 1
fi

URL=$1
ZSWATCH_ROOT=/home/daniel/ZSWatch
FILENAME=${URL##*/}
FILENAME_WITHOUT_ENDING=${FILENAME%%.*}

if test -f "download/${FILENAME}"; then
    echo "Watchface ${FILENAME} exists. Use file from download directory"
    mv download/${FILENAME} ${FILENAME} 
else
    echo "Download watchface ${FILENAME}"
    wget ${1}
fi

mkdir -p ${ZSWATCH_ROOT}/app/src/images/assets_watchfaces

# Generate the code
kotlinc bin2lvgl.kt -include-runtime -d bin2lvgl.jar && java -jar bin2lvgl.jar ${FILENAME} && mkdir -p ${ZSWATCH_ROOT}/app/src/ui/watchfaces/${FILENAME_WITHOUT_ENDING} && cp ${FILENAME_WITHOUT_ENDING}/* ${ZSWATCH_ROOT}/app/src/ui/watchfaces/${FILENAME_WITHOUT_ENDING}/
mv ${FILENAME} download/${FILENAME}

# Copy the preview image
mkdir -p previews/${FILENAME_WITHOUT_ENDING}
cp ${FILENAME_WITHOUT_ENDING}/watchface.png previews/${FILENAME_WITHOUT_ENDING}/watchface.png

# Copy all assets into the ZSWatch project
cd assets_watchfaces
mv */* .
rm -r ${FILENAME_WITHOUT_ENDING}
cd ..
mv assets_watchfaces/* ${ZSWATCH_ROOT}/app/src/images/assets_watchfaces