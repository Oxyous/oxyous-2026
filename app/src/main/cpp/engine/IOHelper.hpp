//
// Created by Mr Steven J Baldwin on 07/07/2026.
//

#ifndef OXYOUS_2026_IOHELPER_HPP
#define OXYOUS_2026_IOHELPER_HPP

#include <string>

class IOHelper {
public:
    static std::string getFileExtension(const std::string& filePath) {
        size_t dotPos = filePath.find_last_of('.');
        if (dotPos == std::string::npos) {
            return ""; // No extension found
        }
        return filePath.substr(dotPos + 1);
    }

    static std::string getFilePath(const std::string& filePath) {
        size_t lastSlash = filePath.find_last_of("/\\");
        if (lastSlash == std::string::npos) {
            return "";
        }
        return filePath.substr(0, lastSlash);
    }
};

#endif //OXYOUS_2026_IOHELPER_HPP
