#ifndef _SINGLE_INSTANCE_H_
#define _SINGLE_INSTANCE_H_

#include <mutex>
#include <memory>

template<typename T>
class Singleton {
public:

    template<typename ...Args>
    static std::shared_ptr<T> getInstance(Args&&... args) {
        if (!m_pSington) {
            std::lock_guard<std::mutex> gLock(m_Mutex);
            if (nullptr == m_pSington) {
                m_pSington = std::make_shared<T>(std::forward<Args>(args)...);
            }
        }
        return m_pSington;
    }

    static void destoryInstance() {
        if (m_pSington) {
            m_pSington.reset();
            m_pSington = nullptr;
        }
    }

private:
    explicit Singleton();
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    ~Singleton();

private:
    static std::shared_ptr<T>   m_pSington;
    static std::mutex           m_Mutex;
};

template<typename T>
std::shared_ptr<T> Singleton<T>::m_pSington = nullptr;

template<typename T>
std::mutex Singleton<T>::m_Mutex;

#endif /* _SINGLE_INSTANCE_H_ */