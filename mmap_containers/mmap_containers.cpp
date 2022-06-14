// mmap_containers.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <vector>
#include <deque>
#include <list>
#include <boost/container/slist.hpp>
#include "mmap_containers.hpp"

int main()
{
    mmap::vector_proxy<int> v("C:\\Users\\linji\\Desktop\\test.mmap", "vec");
    mmap::deque_proxy<int> d("C:\\Users\\linji\\Desktop\\test.mmap", "deq");
    mmap::list_proxy<int> l("C:\\Users\\linji\\Desktop\\test.mmap", "list");
    mmap::slist_proxy<int> sl("C:\\Users\\linji\\Desktop\\test.mmap", "slist");
    mmap::stable_vector_proxy<int> sv("C:\\Users\\linji\\Desktop\\test.mmap", "svec");
    
    v.push_back(1);
    v.push_back(2);
    d.push_back(1);
    d.push_back(2);
    l.push_back(3);
    l.push_back(4);
    sl.push_front(5);
    sl.push_front(6);
    sv.push_back(7);
    sv.push_back(8);
    for (auto& i : v) {
        std::cout << i << std::endl;
    }
    for (auto& i : d) {
        std::cout << i << std::endl;
    }
    for (auto& i : l) {
        std::cout << i << std::endl;
    }
    for (auto& i : sl) {
        std::cout << i << std::endl;
    }
    for (auto& i : sv) {
        std::cout << i << std::endl;
    }
    std::cout << "Hello World!\n";
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
