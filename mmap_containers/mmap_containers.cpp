// mmap_containers.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "mmap_containers.hpp"

struct User_1 {
	int id;
	int age;
};

struct User_2 {
public:
	User_2(mmap::managed_mapped_file_t& file) : scores(file) { this->id = 0; }
	int id;
	mmap::vector<int> scores;
};

int main()
{
	mmap::vector_proxy<User_1> users(R"(C:\Users\matridx\Desktop\test.mmap)", "user_1");
	mmap::vector_proxy<User_2> users_2(R"(C:\Users\matridx\Desktop\test.mmap)", "user_2");
	for (int i = 0; i < 10; i++) {
		users.push_back({ i, i });
		users_2.emplace_back(users_2.get_managed_mapped_file());
		users_2.back().id = i;
		users_2.back().scores.push_back(i);
		users_2.back().scores.push_back(i * 2);
	}
	for (int i = 0; i < 10; i++) {
		std::cout << users[i].id << " " << users[i].age << std::endl;
		std::cout << users_2[i].id << " ";
		for (int j = 0; j < users_2[i].scores.size(); j++) {
			std::cout << users_2[i].scores[j] << " ";
		}
		std::cout << std::endl;
	}
	mmap::vector_proxy<int> vp(R"(C:\Users\xxx\Desktop\test.mmap)", "vector_proxy");
	for (int i = 0; i < 100; i++)
	{
		vp.push_back(i);
		vp.emplace_back(i * 2);
	}
	std::cout << "size: " << vp.size() << std::endl;
	std::cout << "capacity: " << vp.capacity() << std::endl;
	std::cout << "at(0): " << vp.at(0) << std::endl;
	std::cout << "at(1): " << vp[1] << std::endl;
	mmap::vector_proxy<mmap::vector<int>> vvp(R"(C:\Users\xxx\Desktop\test.mmap)", "vector_proxy2");
	vvp.emplace_back(vvp.get_managed_mapped_file());
	auto pVint = &vvp[0];
	for (int i = 0; i < 100; i++) {
		pVint->push_back(i);
		pVint->emplace_back(i * 2);
	}
	std::cout << "size: " << vvp.size() << std::endl;
	std::cout << "size: " << vvp[0].size() << std::endl;
	for (auto& v : *pVint) {
		std::cout << v << " ";
	}
	mmap::vector_proxy<mmap::map<int, int>> vmp(R"(C:\Users\xxx\Desktop\test.mmap)", "vmap_proxy");
	vmp.emplace_back(vmp.get_managed_mapped_file());
	auto pVmap = &vmp[0];
	for (int i = 0; i < 100; i++) {
		pVmap->insert(std::make_pair(i, i * 2));
	}
	auto& pVmapRef = *pVmap;
	for (auto& v : pVmapRef) {
		std::cout << v.first << " " << v.second << std::endl;
	}
	mmap::map_proxy<int, mmap::vector<int>> mp(R"(C:\Users\xxx\Desktop\test.mmap)", "mapv_proxy");
	auto& pmpvRef = mp[0];
	for (int i = 0; i < 100; i++) {
		pmpvRef.push_back(i);
		pmpvRef.emplace_back(i * 2);
	}
	std::cout << "size: " << pmpvRef.size() << std::endl;
	for (auto& v : pmpvRef) {
		std::cout << v << " ";
	}
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
