/**
 * @file main.cpp
 * @author whoami (13003827890@163.com)
 * @brief main source file
 * @version 0.1
 * @date 2024-11-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <iostream>
#include <algorithm>

#include "smart_ptr/shared_ptr.h"
#include "smart_ptr/unique_ptr.h"
#include "functional/function.h"
#include <functional>

int main() {
    std::function<void()> f;
    tiny_std::function<void()> t;
    std::cout << "hello world" << std::endl;
    return 0;
}