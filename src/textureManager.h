/**
 * @file textureManager.h
 * @brief A source file defining the texture manager of the application.
 * 
 * @author SJTU-XHW
 * @date   Feb 12, 2024
 */

#pragma once

#include <string>

#include <AppOpenGLWrapper.hpp>

#include <Type/csmVector.hpp>

/**
 * @class TextureManager
 * @brief Texture Management Class.
 *
 * Class for loading and managing images.
 */
class TextureManager {
public:

    /**
     * @struct TextureInfo
     * @brief Image Information Structure.
     */
    struct TextureInfo {
        GLuint id;  /**< texture ID */
        int width;
        int height;
        std::string fileName;
    };

    TextureManager();
    ~TextureManager();

    /**
     * @brief Premultiply processing
     *
     * @param[in] red    Red value of image.
     * @param[in] green  Green value of image.
     * @param[in] blue   Blue value of image.
     * @param[in] alpha  Alpha value of image.
     *
     * @return Color values after pre-multiply processing.
     */
    inline unsigned int Premultiply(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) {
        return static_cast<unsigned>(\
            (red * (alpha + 1) >> 8) | \
            ((green * (alpha + 1) >> 8) << 8) | \
            ((blue * (alpha + 1) >> 8) << 16) | \
            (((alpha)) << 24)   \
            );
    }

    /**
     * @brief Image loading.
     *
     * @param[in] fileName   Image file path name to be read.
     * @return   Image information. Returns NULL on read failure.
     */
    TextureInfo* CreateTextureFromPngFile(std::string fileName);

    /**
     * @brief Release image(s).
     *
     * Deallocate all images present in the array.
     */
    void ReleaseTextures();

    /**
     * @brief Release image.
     *
     * Releases the image with the specified texture ID.
     * 
     * @param[in] textureId  The ID of the texture to be released.
     **/
    void ReleaseTexture(Csm::csmUint32 textureId);

    /**
     * @brief Release image.
     *
     * Releases the image with the specified file name.
     * 
     * @param[in] fileName  The file name of the texture to be released.
     **/
    void ReleaseTexture(std::string fileName);

    /**
     * @brief Obtain texture information from texture ID.
     *
     * @param[in]   textureId   Texture ID to be acquired.
     * @return  TextureInfo is returned if the texture exists.
     */
    TextureInfo* GetTextureInfoById(GLuint textureId) const;

private:
    Csm::csmVector<TextureInfo*> _textures;
};
