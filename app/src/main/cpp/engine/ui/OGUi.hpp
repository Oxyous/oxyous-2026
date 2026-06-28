//
// Created by Mr Steven J Baldwin on 27/06/2026.
//

#ifndef OXYOUS_2026_OGUI_HPP
#define OXYOUS_2026_OGUI_HPP

#include "../../includes.hpp"
#include "../elements/OGElement.hpp"
#include "../../system/OGSingleton.hpp"

class OGUi {
public:
    OGUi() = default;

public:

    template<typename T>
    T* addElement(T* element) {
        m_elements.emplace_back(element);
        return element;
    }

    void update(float delta) {
        for (auto& element : m_elements) {
            element->update(delta);
        }
    }

    std::vector<std::unique_ptr<OGElement>>& getElements() {
        return m_elements;
    }

protected:
    std::vector<std::unique_ptr<OGElement>> m_elements;
};

#define UI OGSingleton<OGUi>::getInstance()

#endif //OXYOUS_2026_OGUI_HPP
