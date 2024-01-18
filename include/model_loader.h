#ifndef ALE_LOADER
#define ALE_LOADER

// ext
#pragma once
#include <vector>
#include <functional>   
#include <string>
#include <unordered_map>
#include <filesystem>
#include <set>
#include <utility>

// int
#include <primitives.h>
#include <re_mesh.h>
#include <tracer.h>

namespace ale {
    class Loader {
    private:
        std::vector<std::string> commandLineTokens;
        
        bool _canReadFile(std::filesystem::path p);
    public: 
        Loader();
        ~Loader();
        int loadModelOBJ(char *model_path, Mesh &_model);
        int loadModelGLTF(const std::string filename, Mesh &_model, Image &_image);
        bool loadTexture(const char* path, Image& img);
        void recordCommandLineArguments(int & argc, char ** argv);
        int getFlaggedArgument(const std::string flag, std::string &result);
        const std::string &getCmdOption(const std::string &option) const;
        bool cmdOptionExists(const std::string &option) const;
        bool isFileValid(std::string file_path);
    };

} // namespace loader

#endif // ALE_LOADER



