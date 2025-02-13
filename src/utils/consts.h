/**
 * @file consts.h
 * @brief Constants for the application.
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

#define appName "Live2Desktop"

#define RESOURCE_ROOT_DIR "Resources/"
#define CONFIG_FILE_PATH  RESOURCE_ROOT_DIR"config.json"
#define TRAY_ICON_PATH CONFIG_FILE_PATH

#define DATETIME_FORMAT "%04d-%02d-%02d %02d:%02d:%02d"

#define DEFAULT_FADE_IN_TIME 0.5
#define DEFAULT_FADE_OUT_TIME 0.5

#define IDLE_SUFFIX " (idle)"

/* --- Toast Bar Parameters --- */

/* Unit: Millisecond */
const int defaultDuration = 2000;
const int scrollInDuration = 300;
const int scrollOutDuration = 300;
const int popupHeight = 60;
const double maxOpacity = 1.0;
