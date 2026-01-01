#include "pathglobals.h"
#include <string>
#include <filesystem>
const std::string assetsPath = "\\assets";
const std::string shadersPath = "\\shaders";

const std::string globalPath = [] {
    std::string path = std::filesystem::current_path().string();
    if (path.length() >= 4) {
        path.erase(path.length() - 4);
    }
    return path;
}();

