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

echo "Download watchface ${FILENAME}"
wget $1
kotlinc bin2lvgl.kt -include-runtime -d bin2lvgl.jar && java -jar bin2lvgl.jar ${FILENAME} && mkdir -p ${ZSWATCH_ROOT}/app/src/ui/watchfaces/${FILENAME_WITHOUT_ENDING} && cp ${FILENAME_WITHOUT_ENDING}/* ${ZSWATCH_ROOT}/app/src/ui/watchfaces/${FILENAME_WITHOUT_ENDING}/