//
// Created by Mr Steven J Baldwin on 16/06/2026.
//

#ifndef OXYOUS_2026_OGSINGLETON_HPP
#define OXYOUS_2026_OGSINGLETON_HPP

template<typename T>
class OGSingleton
{
private:
    OGSingleton() {}
    ~OGSingleton() = default;
public:
    static T* getInstance() {
        if (m_instance == nullptr) {
            m_instance = new T();
        }
        return m_instance;
    }
private:
    static T* m_instance;
};

template<class T> T* OGSingleton<T>::m_instance = nullptr;

#endif //OXYOUS_2026_OGSINGLETON_HPP
