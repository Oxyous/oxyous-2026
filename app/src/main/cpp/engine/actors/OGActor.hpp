//
// Created by Mr Steven J Baldwin on 22/06/2026.
//

#ifndef OXYOUS_2026_OGACTOR_HPP
#define OXYOUS_2026_OGACTOR_HPP


#include "../entity/OGEntity.hpp"

class OGActor : public OGEntity {
public:
    OGActor() : OGEntity() {}

public:
    virtual void update(double deltaTime) override {
        OGEntity::update(deltaTime);
    }

    virtual bool initialize() override {
        return true;
    }
};


#endif //OXYOUS_2026_OGACTOR_HPP
