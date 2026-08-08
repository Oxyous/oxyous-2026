//
// Created by Mr Steven J Baldwin on 08/08/2026.
//

#ifndef OXYOUS_2026_OGACTION_HPP
#define OXYOUS_2026_OGACTION_HPP

#include "../../includes.hpp"

class OGAction {
public:

    OGAction(const std::string& name, std::function<void()> callback) : m_name(name), callback(callback){

    }

    virtual ~OGAction() = default;

    /** Set Action Name*/
    void setName(const std::string& name) {
        m_name = name;
    }

    /** Action Name */
    const std::string& getName() const {
        return m_name;
    }

    /** Execute Action */
    virtual void execute() {
        if(callback) {
            callback();
        }
    }

private:
    std::string m_name;
    std::function<void()> callback;
};

#endif //OXYOUS_2026_OGACTION_HPP
