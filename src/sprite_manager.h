#ifndef SPRITE_MANAGER_H
#define SPRITE_MANAGER_H

#define GL_SILENCE_DEPRECATION  // silences macOS OpenGL deprecation warnings

#include "basic_transform.h"
#include "texture_atlas.h"
#include "vector_util.h"
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#define glCheck(expr)                                                                              \
    expr;                                                                                          \
    if (int e = glGetError()) {                                                                    \
        std::cerr << "OpenGL error: " << e << " in file " __FILE__ << ", line number " << __LINE__ \
                  << '\n';                                                                         \
        abort();                                                                                   \
    }

using SpriteID = size_t;

class SpriteManager {
public:
    SpriteManager(const sf::Vector2u& atlasSize, unsigned padding)
        : mVertices(sf::PrimitiveType::Triangles),
          mNumSprites{0},
          mAtlas(atlasSize, padding),
          mShader(std::filesystem::path{"main_sfml.vert"}, std::filesystem::path{"main.frag"}),
          mBatchShader(std::filesystem::path{"main.vert"}, std::filesystem::path{"main.frag"}),
          mTextureUniformLocation{-1},
          mTransformsUniformLocation{-1},
          mNumTransformsUniformLocation{-1},
          mPositionAttribLocation{-1},
          mTexCoordsAttribLocation{-1},
          mSpriteNumberAttribLocation{-1},
          mVerticesVBO{0},
          mSpriteNumberVBO{0},
          mTransformsTexture{0} {

        // get shader locations
        GLuint program = mBatchShader.getNativeHandle();

        mTextureUniformLocation = glGetUniformLocation(program, "texture");
        mTransformsUniformLocation = glGetUniformLocation(program, "transforms");
        mNumTransformsUniformLocation = glGetUniformLocation(program, "numTransforms");

        mPositionAttribLocation = glGetAttribLocation(program, "position");
        mTexCoordsAttribLocation = glGetAttribLocation(program, "texCoords");
        mSpriteNumberAttribLocation = glGetAttribLocation(program, "spriteNumber");

        // create buffers
        glGenBuffers(1, &mVerticesVBO);
        glGenBuffers(1, &mSpriteNumberVBO);
        glGenTextures(1, &mTransformsTexture);
    }

    // TODO
    TextureID registerTexture(const std::string& filepath) { 
        //return {}; 
        return mAtlas.registerTexture(filepath);
    
    };

    // TODO
    // creating sprite w texture
    SpriteID createSprite(TextureID textureID) { 
        // get texture size + uv bounds
        //return {}; 
        sf::Vector2u size = mAtlas.getSizePixels(textureID);
        sf::FloatRect uv = mAtlas.getBoundsUV(textureID);

        float w = static_cast<float>(size.x);
        float h = static_cast<float>(size.y);

        // get atlas size
        sf::Vector2u atlasSize = mAtlas.getTexture().getSize();

        // convert uv bounds to sfml pixel coordinates
        float left = uv.position.x * atlasSize.x;
        float right = (uv.position.x + uv.size.x) * atlasSize.x;

        float top = (1.f - uv.position.y - uv.size.y) * static_cast<float>(atlasSize.y);

        float bottom = (1.f - uv.position.y) * static_cast<float>(atlasSize.y);

        // create two triangles
        mVertices.append(sf::Vertex({0.f, 0.f}, sf::Color::White, {left, bottom}));
        mVertices.append(sf::Vertex({w, 0.f}, sf::Color::White, {right, bottom}));
        mVertices.append(sf::Vertex({w, h}, sf::Color::White, {right, top}));

        mVertices.append(sf::Vertex({0.f, 0.f}, sf::Color::White, {left, bottom}));
        mVertices.append(sf::Vertex({w, h}, sf::Color::White, {right, top}));
        mVertices.append(sf::Vertex({0.f, h}, sf::Color::White, {left, top}));
        
        // add default transform
        mTransforms.emplace_back();

        // store sprite number for all six vertices
        for (size_t i = 0; i < 6; ++i) {
            mSpriteNumbers.push_back(static_cast<float>(mNumSprites));
        }

        // return new sprite id
        return mNumSprites++;
    };

    // TODO
    // changing sprite texture
    void setTexture(SpriteID sprite, TextureID newTexture) {
        assert(sprite < mNumSprites);

        // get new texture size and uv bounds
        sf::Vector2u size = mAtlas.getSizePixels(newTexture);
        sf::FloatRect uv = mAtlas.getBoundsUV(newTexture);

        float w = static_cast<float>(size.x);
        float h = static_cast<float>(size.y);

        // get atlas size
        sf::Vector2u atlasSize = mAtlas.getTexture().getSize();

        // convert uv bounds to sfml pixel coordinates
        float left = uv.position.x * atlasSize.x;
        float right = (uv.position.x + uv.size.x) * atlasSize.x;

        float bottom = (1.f - uv.position.y) * atlasSize.y;
        float top = (1.f - (uv.position.y + uv.size.y)) * atlasSize.y;

        // get first vertex of sprite
        size_t start = sprite * 6;

        // update vertex positions and uv coordinates
        mVertices[start + 0] = sf::Vertex({0.f, 0.f}, sf::Color::White, {left, bottom});
        mVertices[start + 1] = sf::Vertex({w, 0.f}, sf::Color::White, {right, bottom});
        mVertices[start + 2] = sf::Vertex({w, h}, sf::Color::White, {right, top});

        mVertices[start + 3] = sf::Vertex({0.f, 0.f}, sf::Color::White, {left, bottom});
        mVertices[start + 4] = sf::Vertex({w, h}, sf::Color::White, {right, top});
        mVertices[start + 5] = sf::Vertex({0.f, h}, sf::Color::White, {left, top});
    };

    // TODO
    // draw all sprites in one draw call
    void draw(sf::RenderTarget& target, sf::RenderStates states) const {
        // copy all sprite vertices
        sf::VertexArray vertices = mVertices;

        // apply each sprite transform
        for (size_t i = 0; i < mNumSprites; ++i) {
            const BasicTransform& transform = mTransforms[i];

            // get first vertex of sprite
            size_t start = i * 6;

            // transform all six vertices
            for (size_t j = 0; j < 6; ++j) {
                vertices[start + j].position = transform.applyPoint(vertices[start + j].position);
            }
        }

        // use atlas texture and shader
        states.texture = &mAtlas.getTexture();
        states.shader = &mShader;

        // use atlas texture in shader
        mShader.setUniform("texture", sf::Shader::CurrentTexture);

        // draw all sprites together
        target.draw(vertices, states);
    };

    // TODO: implement batched rendering pt 2
    void drawGL() {
        if (mNumSprites == 0) {
        return;
        }

        // get atlas dimensions
        sf::Vector2u atlasSize = mAtlas.getTexture().getSize();

        // prepare vertex data
        std::vector<float> vertexData;
        vertexData.reserve(mVertices.getVertexCount() * 4);

        for (size_t i = 0; i < mVertices.getVertexCount(); ++i) {
            const sf::Vertex& vertex = mVertices[i];

            // convert sfml pixel coordinates to normalized opengl coordinates
            float u = vertex.texCoords.x / static_cast<float>(atlasSize.x);
            float v = 1.f - vertex.texCoords.y / static_cast<float>(atlasSize.y);

            vertexData.push_back(vertex.position.x);
            vertexData.push_back(vertex.position.y);
            vertexData.push_back(u);
            vertexData.push_back(v);
        }

        // prepare transform data
        std::vector<float> transformData;
        transformData.reserve(mNumSprites * 4);

        for (const BasicTransform& transform : mTransforms) {
            transformData.push_back(transform.scale.x);
            transformData.push_back(transform.scale.y);
            transformData.push_back(transform.translation.x);
            transformData.push_back(transform.translation.y);
        }

        // upload vertex data
        glBindBuffer(GL_ARRAY_BUFFER, mVerticesVBO);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertexData.size() * sizeof(float)),
            vertexData.data(),
            GL_DYNAMIC_DRAW
        );

        // upload sprite numbers
        glBindBuffer(GL_ARRAY_BUFFER, mSpriteNumberVBO);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(mSpriteNumbers.size() * sizeof(float)),
            mSpriteNumbers.data(),
            GL_DYNAMIC_DRAW
        );

        // update transform texture
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, mTransformsTexture);

        glTexImage1D(
            GL_TEXTURE_1D,
            0,
            GL_RGBA32F_ARB,
            static_cast<GLsizei>(mNumSprites),
            0,
            GL_RGBA,
            GL_FLOAT,
            transformData.data()
        );

        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

        // bind atlas texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mAtlas.getTexture().getNativeHandle());

        // use batch shader
        GLuint program = mBatchShader.getNativeHandle();
        glUseProgram(program);

        // set shader uniforms
        glUniform1i(mTextureUniformLocation, 0);
        glUniform1i(mTransformsUniformLocation, 1);
        glUniform1i(mNumTransformsUniformLocation, static_cast<GLint>(mNumSprites));

        // set vertex position and texture coordinates
        glBindBuffer(GL_ARRAY_BUFFER, mVerticesVBO);

        glEnableVertexAttribArray(mPositionAttribLocation);
        glVertexAttribPointer(
            mPositionAttribLocation, 2, GL_FLOAT, GL_FALSE,
            4 * sizeof(float), nullptr
        );

        glEnableVertexAttribArray(mTexCoordsAttribLocation);
        glVertexAttribPointer(
            mTexCoordsAttribLocation, 2, GL_FLOAT, GL_FALSE,
            4 * sizeof(float),
            reinterpret_cast<void*>(2 * sizeof(float))
        );

        // set sprite number attribute
        glBindBuffer(GL_ARRAY_BUFFER, mSpriteNumberVBO);

        glEnableVertexAttribArray(mSpriteNumberAttribLocation);
        glVertexAttribPointer(
            mSpriteNumberAttribLocation, 1, GL_FLOAT, GL_FALSE,
            sizeof(float), nullptr
        );

        // draw all sprites in one call
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mVertices.getVertexCount()));

        // disable vertex attributes
        glDisableVertexAttribArray(mPositionAttribLocation);
        glDisableVertexAttribArray(mTexCoordsAttribLocation);
        glDisableVertexAttribArray(mSpriteNumberAttribLocation);

        // clean up bindings
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    };

    // TODO
    BasicTransform& getTransform(SpriteID id) {
        assert(id < mNumSprites);
        return mTransforms[id];
    }

    // TODO
    const TextureAtlas& getAtlas() const {
        return mAtlas;

    };

private:
    sf::VertexArray mVertices;
    std::vector<BasicTransform> mTransforms;
    size_t mNumSprites;
    TextureAtlas mAtlas;
    mutable sf::Shader mShader;
    sf::Shader mBatchShader;

    // Use the below only in Batched rendering (part 2):
    std::vector<float> mSpriteNumbers;

    GLint mTextureUniformLocation;
    GLint mTransformsUniformLocation;
    GLint mNumTransformsUniformLocation;

    GLint mPositionAttribLocation;
    GLint mTexCoordsAttribLocation;
    GLint mSpriteNumberAttribLocation;

    GLuint mVerticesVBO;
    GLuint mSpriteNumberVBO;
    GLuint mTransformsTexture;
};

#endif  // SPRITE_MANAGER_H
