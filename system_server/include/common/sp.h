#ifndef _SP_H
#define _SP_H

#include <memory>

template<typename T>
using sp = std::shared_ptr<T>;

template<typename T>
using up = std::unique_ptr<T>;

template<typename T>
using wp = std::weak_ptr<T>;

#endif 
