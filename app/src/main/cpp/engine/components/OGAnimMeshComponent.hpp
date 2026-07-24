//
// Created by Mr Steven J Baldwin on 20/07/2026.
//

#ifndef OXYOUS_2026_OGANIMMESHCOMPONENT_HPP
#define OXYOUS_2026_OGANIMMESHCOMPONENT_HPP


#include "engine/entity/OGObject.hpp"
#include "engine/entity/OGEntity.hpp"

class OGAnimMeshComponent : public OGComponent {
public:
    GET_UNIQUE_TYPE(OGAnimMeshComponent)
public:
    OGAnimMeshComponent() {
        m_owner = nullptr;
    }
    virtual ~OGAnimMeshComponent() = default;
public:

};


#endif //OXYOUS_2026_OGANIMMESHCOMPONENT_HPP
