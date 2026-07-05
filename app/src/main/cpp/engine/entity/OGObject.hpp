//
// Created by Mr Steven J Baldwin on 21/06/2026.
//

#ifndef OXYOUS_2026_OGOBJECT_HPP
#define OXYOUS_2026_OGOBJECT_HPP

#include "../../includes.hpp"

using ComponentID = std::size_t;
using ComponentType = std::size_t;

constexpr std::size_t maxComponents = 32;

#define DEFINE_TYPE template<class T> bool isTypeOf() {return mTypeID.isTypeOf<T>();} template<class T>bool isDervied() {    return mTypeID.isDerived<T>();} static const OGObjectType mTypeID;

inline std::size_t UUID() noexcept {
    static size_t uid = 0;
    return uid++;
}

template<typename T>
inline ComponentType GetTypeId() {
    static ComponentID typeId = UUID();
    return reinterpret_cast<ComponentType>(&typeId);
}

#define GET_UNIQUE_TYPE(Class) \
    public: \
    static ComponentType GetType() { \
        return GetTypeId<Class>(); \
    }

class OGObjectType {
public:
    OGObjectType(std::string typeName, ComponentID uid, const OGObjectType *baseType) :
            mTypeName(typeName), mUID(uid), mBaseType(baseType) {
    }

    template<class T>
    bool isTypeOf() {
        T *cast = dynamic_cast<T *>(this);
        return cast != nullptr;
    }

    template<class T>
    bool isDerived() {
        if (mBaseType == nullptr) {
            return false;
        }
        return mBaseType->isDerived<T>();
    }

protected:
    std::string mTypeName;
    ComponentID mUID;
    const OGObjectType *mBaseType;
};

class OGObject : public OGObjectType {
public:
    OGObject(std::string typeName) : OGObjectType(typeName, UUID(), this) {

    }
protected:
    template<typename T>
    static ComponentType GetType() {
        return GetTypeId<T>();
    }
};

#endif //OXYOUS_2026_OGOBJECT_HPP