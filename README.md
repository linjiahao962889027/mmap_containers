# mmap_containers

### 简介

mmap_containers是一个基于boost的interprocess库所构建的一个C++轻量的内存映射文件（mmap）容器库。

依赖C++ 20, Boost::iterprocess

包括下列容器：

- vector
- deque
- list
- slist
- stable_vector
- set
- map

### 用法

使用方法与普通的容器使用方法基本相同。默认创建文件大小为1MB，可以在创建容器的时候，在第三个参数指定。

#### 简单用法

```c++
#include "mmap_containers.hpp"
#include <iostream>
int main(){
    mmap::vector_proxy<int> v(R"(C:\Users\xxx\Desktop\test.mmap)", "vector_proxy");
    for(int i = 0; i < 100; i++){
        v.push_back(i);
        v.emplace_back(i*2);
    }
    for(auto& val : v){
        std::cout << val << " ";
    }
    std::cout << std::endl;
    return 0;
}
```

#### 容器嵌套

```c++
#include "mmap_containers.hpp"
#include <iostream>
int main(){
	mmap::vector_proxy<mmap::vector<int>> v(R"(C:\Users\xxx\Desktop\test.mmap)", "vvector_proxy");
    v.emplace_back(v.get_managed_mapped_file());
    for(int i = 0; i < 100; i++){
        v[0].push_back(i);
    }
    for(auto& val : v[0]){
        std::cout << val << " ";
    }
    std::cout << std::endl;
    return 0;
}
```

#### 自定义数据结构

```c++
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
	mmap::vector_proxy<User_1> users(R"(C:\Users\xxx\Desktop\test.mmap)", "user_1");
	mmap::vector_proxy<User_2> users_2(R"(C:\Users\xxx\Desktop\test.mmap)", "user_2");
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
    return 0;
}
```

### 注意事项

mmap::vector等容器进行push_back等操作时可能会存在容量问题，请确保文件大小足够写入。可以使用proxy类的expand_file进行扩充，每次扩充后，容器内部元素位置可能发生了改变，因此不是特别建议使用引用保存容器内的元素。如下操作可能会出现空悬引用异常：

```c++
mmap::vector_proxy<mmap::vector<int>> v(R"(C:\Users\xxx\Desktop\test.mmap)", "vvector_proxy");
auto& ele = v[0];
v.expand_file();
ele.push_back(1); //error!
ele = v[0] //error！
ele.push_back(1);
```

如下操作是没有问题的：

```c++
mmap::vector_proxy<mmap::vector<int>> v(R"(C:\Users\xxx\Desktop\test.mmap)", "vvector_proxy");
auto ele = &v[0];
v.expand_file();
ele = &v[0];
ele->push_back(1);
```

此外，注意，代理类容器不能成为其他容器的元素：

```c++
mmap::vector_proxy<mmap::vector_proxy<int>> v(R"(C:\Users\xxx\Desktop\test.mmap)", "vvector_proxy"); //error
```

