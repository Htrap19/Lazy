//
// Created by htrap19 on 11/22/22.
//

#include <iostream>
#include <lazy/list.h>

#define print(...) std::cout << "[" << __FUNCTION__ << "]: " << __VA_ARGS__ << std::endl

lazy::list<uint32_t> do_something()
{
    for (uint32_t i = 0; i < 10; i++)
        co_yield i * 10;
}

int main()
{
    auto list = do_something();
    for (const auto& data : list)
        print(list.size() << " -> " << data);

    print(list.size());
    std::cin.get();
    return 0;
}
