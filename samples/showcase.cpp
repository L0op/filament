/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// TODO: trim this list of includes

#include <filament/Engine.h>
#include <filament/IndirectLight.h>
#include <filament/LightManager.h>
#include <filament/RenderableManager.h>
#include <filament/Scene.h>
#include <filament/TransformManager.h>
#include <filament/View.h>

#include <filameshio/MeshReader.h>

#include <gltfio/AssetLoader.h>
#include <gltfio/FilamentAsset.h>
#include <gltfio/BindingHelper.h>

#include "app/Config.h"
#include "app/FilamentApp.h"
#include "app/IBL.h"

#include <stb_image.h>
#include <getopt/getopt.h>

#include <fstream>
#include <iomanip>
#include <iostream>

#include "generated/resources/resources.h"
#include "generated/resources/textures.h"

using namespace filament;
using namespace filamesh;
using namespace gltfio;
using namespace math;
using namespace utils;

struct App {
    Config config;
    AssetLoader* loader;
    FilamentAsset* asset;
    bool shadowPlane = false;
    MeshReader::Mesh mesh;
    mat4f transform;
};

static const char* DEFAULT_IBL = "envs/venetian_crossroads";

static void printUsage(char* name) {
    std::string exec_name(Path(name).getName());
    std::string usage(
            "SHOWCASE renders the specified glTF file, or a built-in file if none is specified\n"
            "Usage:\n"
            "    SHOWCASE [options] <gltf file>\n"
            "Options:\n"
            "   --help, -h\n"
            "       Prints this message\n\n"
            "   --api, -a\n"
            "       Specify the backend API: opengl (default) or vulkan\n\n"
            "   --ibl=<path to cmgen IBL>, -i <path>\n"
            "       Override the built-in IBL\n\n"
            "   --shadow-plane, -p\n"
            "       Enable shadow plane\n\n"
    );
    const std::string from("SHOWCASE");
    for (size_t pos = usage.find(from); pos != std::string::npos; pos = usage.find(from, pos)) {
        usage.replace(pos, from.length(), exec_name);
    }
    std::cout << usage;
}

static int handleCommandLineArgments(int argc, char* argv[], App* app) {
    static constexpr const char* OPTSTR = "ha:pi:";
    static const struct option OPTIONS[] = {
            { "help",       no_argument,       nullptr, 'h' },
            { "api",        required_argument, nullptr, 'a' },
            { "ibl",        required_argument, nullptr, 'i' },
            { "shadow-plane", no_argument,     nullptr, 'p' },
            { nullptr, 0, nullptr, 0 }  // termination of the option list
    };
    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, OPTSTR, OPTIONS, &option_index)) >= 0) {
        std::string arg(optarg ? optarg : "");
        switch (opt) {
            default:
            case 'h':
                printUsage(argv[0]);
                exit(0);
            case 'a':
                if (arg == "opengl") {
                    app->config.backend = Engine::Backend::OPENGL;
                } else if (arg == "vulkan") {
                    app->config.backend = Engine::Backend::VULKAN;
                } else {
                    std::cerr << "Unrecognized backend. Must be 'opengl'|'vulkan'." << std::endl;
                }
                break;
            case 'i':
                app->config.iblDirectory = arg;
                break;
            case 'p':
                app->shadowPlane = true;
                break;
        }
    }
    return optind;
}

static Texture* loadNormalMap(Engine* engine, const uint8_t* normals, size_t nbytes) {
    int w, h, n;
    unsigned char* data = stbi_load_from_memory(normals, nbytes, &w, &h, &n, 3);
    Texture* normalMap = Texture::Builder()
            .width(uint32_t(w))
            .height(uint32_t(h))
            .levels(0xff)
            .format(driver::TextureFormat::RGB8)
            .build(*engine);
    Texture::PixelBufferDescriptor buffer(data, size_t(w * h * 3),
            Texture::Format::RGB, Texture::Type::UBYTE,
            (driver::BufferDescriptor::Callback) &stbi_image_free);
    normalMap->setImage(*engine, 0, std::move(buffer));
    normalMap->generateMipmaps(*engine);
    return normalMap;
}

static std::ifstream::pos_type getFileSize(const char* filename) {
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

int main(int argc, char** argv) {
    App app;

    app.config.title = "showcase";
    app.config.iblDirectory = FilamentApp::getRootPath() + DEFAULT_IBL;

    int option_index = handleCommandLineArgments(argc, argv, &app);
    utils::Path filename;
    int num_args = argc - option_index;
    if (num_args >= 1) {
        filename = argv[option_index];
        if (!filename.exists()) {
            std::cerr << "file " << argv[option_index] << " not found!" << std::endl;
            return 1;
        }
    }

    auto setup = [&app, filename](Engine* engine, View* view, Scene* scene) {
        app.loader = AssetLoader::create(engine);

        if (!filename.isEmpty()) {
            long contentSize = static_cast<long>(getFileSize(filename.c_str()));
            if (contentSize <= 0) {
                std::cerr << "Unable to open " << filename << std::endl;
                exit(1);
            }

            std::ifstream in(filename.c_str(), std::ifstream::in);
            std::vector<uint8_t> buffer(static_cast<unsigned long>(contentSize));
            if (!in.read((char*) buffer.data(), contentSize)) {
                std::cerr << "Unable to read " << filename << std::endl;
                exit(1);
            }

            app.asset = app.loader->createAssetFromJson(buffer.data(), buffer.size());
            if (!app.asset) {
                std::cerr << "Unable to parse " << filename << std::endl;
                exit(1);
            }

            BindingHelper::load(app.asset, *engine);
            
            scene->addEntities(app.asset->getEntities(), app.asset->getEntitiesCount());
        }

        auto ibl = FilamentApp::get().getIBL()->getIndirectLight();
        ibl->setIntensity(100000);
        ibl->setRotation(mat3f::rotate(0.5f, float3{ 0, 1, 0 }));
    };

    auto cleanup = [&app](Engine* engine, View*, Scene*) {
        Fence::waitAndDestroy(engine->createFence());
        app.loader->destroyAsset(app.asset);
        app.loader->destroyMaterials();
        AssetLoader::destroy(&app.loader);
    };

    FilamentApp::get().run(app.config, setup, cleanup);

    return 0;
}
