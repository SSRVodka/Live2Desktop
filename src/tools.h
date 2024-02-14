/**
 * @file tools.h
 * @brief Cubism Platform Abstraction Layer.
 * 
 * @author Copyright(c) Live2D Inc. && SJTU-XHW
 * @date   Feb 12, 2024
 */

#pragma once

#include <string>

#include <CubismFramework.hpp>

/**
 * @class ToolFunctions
 * @brief Cubism Platform Abstraction Layer, which abstracts platform-dependent functions.
 *
 * Organize platform-dependent functions such as file reading, time acquisition, etc.
 *
 */
class ToolFunctions {
public:
    /**
     * @brief Read a file as byte data
     *
     * @param[in]   filePath    Path of the file to be read
     * @param[out]  outSize     File size
     * @return                  Data in bytes
     */
    static Csm::csmByte* LoadFileAsBytes(const std::string filePath, Csm::csmSizeInt* outSize);


    /**
    * @brief Release byte data
    *
    * @param[in]   byteData    Byte data to be released
    */
    static void ReleaseBytes(Csm::csmByte* byteData);

    /**
    * @brief Obtain delta time (difference from previous frame)
    *
    * @return Delta time [ms]
    */
    static Csm::csmFloat32 GetDeltaTime();

    static void UpdateTime();

private:
    static double s_currentFrame;
    static double s_lastFrame;
    static double s_deltaTime;
};

