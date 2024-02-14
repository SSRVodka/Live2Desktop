#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "textureManager.h"
#include "tools.h"

TextureManager::TextureManager() {
}

TextureManager::~TextureManager() {
    ReleaseTextures();
}

TextureManager::TextureInfo* TextureManager::CreateTextureFromPngFile(std::string fileName) {
    /* search loaded texture already. */
    for (Csm::csmUint32 i = 0; i < _textures.GetSize(); i++) {
        if (_textures[i]->fileName == fileName) {
            return _textures[i];
        }
    }

    GLuint textureId;
    int width, height, channels;
    unsigned int size;
    unsigned char* png;
    unsigned char* address;

    address = ToolFunctions::LoadFileAsBytes(fileName, &size);

    if(address == NULL)
        return NULL;
    /* Get png information. */
    png = stbi_load_from_memory(
        address,
        static_cast<int>(size),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha);
    {

#ifdef PREMULTIPLIED_ALPHA_ENABLE
        unsigned int* fourBytes = reinterpret_cast<unsigned int*>(png);
        for (int i = 0; i < width * height; i++)
        {
            unsigned char* p = png + i * 4;
            fourBytes[i] = Premultiply(p[0], p[1], p[2], p[3]);
        }
#endif
    }

    /* Generate textures for OpenGL. */
    AppOpenGLWrapper::get()->glGenTextures(1, &textureId);
    AppOpenGLWrapper::get()->glBindTexture(GL_TEXTURE_2D, textureId);
    AppOpenGLWrapper::get()->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, png);
    AppOpenGLWrapper::get()->glGenerateMipmap(GL_TEXTURE_2D);
    AppOpenGLWrapper::get()->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    AppOpenGLWrapper::get()->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    AppOpenGLWrapper::get()->glBindTexture(GL_TEXTURE_2D, 0);

    /* Release current images. */
    stbi_image_free(png);
    ToolFunctions::ReleaseBytes(address);

    TextureManager::TextureInfo* textureInfo = new TextureManager::TextureInfo();
    if (textureInfo != NULL) {
        textureInfo->fileName = fileName;
        textureInfo->width = width;
        textureInfo->height = height;
        textureInfo->id = textureId;

        _textures.PushBack(textureInfo);
    }

    return textureInfo;

}

void TextureManager::ReleaseTextures() {
    for (Csm::csmUint32 i = 0; i < _textures.GetSize(); i++) {
        delete _textures[i];
    }

    _textures.Clear();
}

void TextureManager::ReleaseTexture(Csm::csmUint32 textureId) {
    for (Csm::csmUint32 i = 0; i < _textures.GetSize(); i++) {
        if (_textures[i]->id != textureId) {
            continue;
        }
        delete _textures[i];
        _textures.Remove(i);
        break;
    }
}

void TextureManager::ReleaseTexture(std::string fileName) {
    for (Csm::csmUint32 i = 0; i < _textures.GetSize(); i++) {
        if (_textures[i]->fileName == fileName) {
            delete _textures[i];
            _textures.Remove(i);
            break;
        }
    }
}

TextureManager::TextureInfo* TextureManager::GetTextureInfoById(GLuint textureId) const {
    for (Csm::csmUint32 i = 0; i < _textures.GetSize(); i++) {
        if (_textures[i]->id == textureId) {
            return _textures[i];
        }
    }

    return NULL;
}
