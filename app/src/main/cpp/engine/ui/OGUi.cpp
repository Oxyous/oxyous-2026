//
// Created by Mr Steven J Baldwin on 06/07/2026.
//

#include "OGUi.hpp"

bool OGUi::initializeUI() {
    if (!m_fontEngine.initializeFont("Roboto_Condensed-Regular.ttf")) {
        throw std::runtime_error("Error: loading font");
        return false;
    }
    return true;
}