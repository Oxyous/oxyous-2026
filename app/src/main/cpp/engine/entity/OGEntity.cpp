//
// Created by Mr Steven J Baldwin on 21/06/2026.
//

#include "OGEntity.hpp"
#include "../collision/Collision.hpp"

void OGEntity::setBounds(IVolume *bounds) {
    m_bounds = std::shared_ptr<IVolume>(bounds);
    if (m_bounds) {
        m_bounds->setOwner(this);
    }
}

IVolume* OGEntity::getBounds() {
    return m_bounds.get();
}
