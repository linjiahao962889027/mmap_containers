#pragma once
#include <string>
#include <utility>
#include <functional>
#include <sys/stat.h>
#include <boost/interprocess/allocators/node_allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <type_traits>
#include <memory>

namespace mmap {
	namespace map {
		namespace bipc = boost::interprocess;
		typedef bipc::managed_mapped_file managed_mapped_file_t;
		typedef bipc::managed_mapped_file::segment_manager mapped_segment_manager_t;

		template <typename T>
		using allocator_t = bipc::node_allocator<T, mapped_segment_manager_t>;

		template<typename _key, typename _value>
		using pair_t = std::pair<_key, _value>;

		template<typename T>
		using less_t = std::less<T>;

		template <typename _key, typename _value>
		using map_t = bipc::map<_key, _value, less_t<_key>, allocator_t<pair_t<const _key, _value>>>;

		template <typename _key, typename _value>
		using map_iter_t = map_t<_key, _value>::iterator;

		template <typename _key, typename _value>
		class map : public map_t<_key, _value> {
		public:
			map(managed_mapped_file_t& file) :map_t<_key, _value>(less_t<_key>(), allocator_t<_value>(file.get_segment_manager())) {}
			_value& operator[](const _key& key) {
				return map_t<_key, _value>::operator[](key);
			}
		};

		template <typename _key, typename _value>
		class map_mmap
		{
		public:
			map_mmap(std::string file_name, std::string tag_name, size_t size = 0) {
				this->file_name = file_name;
				this->tag_name = tag_name;
				if (!size) {
					struct _stat info;
					_stat(file_name.c_str(), &info);
					size = info.st_size ? info.st_size : 1024ull * 1024ull;
				}
				this->m_file = managed_mapped_file_t(bipc::open_or_create, file_name.c_str(), size);
				this->p_map = this->m_file.find_or_construct<map_t<_key, _value>>(tag_name.c_str())(less_t<_key>(), this->m_file.get_segment_manager());
				if (!p_map)
				{
					throw std::exception("construct msg_map failed");
				}
			}
			_value& operator[](_key key) {
				map_iter_t<_key, _value> itr = this->p_map->find(key);
				if (itr == this->p_map->end()) {
					int i = 0;
				INSERT_AGAIN:
					try {
						if constexpr (std::is_constructible<_value, managed_mapped_file_t&>::value) {
							this->p_map->insert(pair_t<_key, _value>(key, _value(this->m_file)));
						}
						else {
							this->p_map->insert(pair_t<_key, _value>(key, _value()));
						}
					}
					catch (...) {
						this->expand_file();
						i++;
						if (i > 3) {
							throw std::exception("insert failed");
						}
						else {
							goto INSERT_AGAIN;
						}
					}
					return this->p_map->at(key);
				}
				else {
					return itr->second;
				}
			}

			void erase(_key key) {
				this->p_map->erase(key);
			}

			auto begin() {
				return this->p_map->begin();
			}

			auto end() {
				return this->p_map->end();
			}

			void expand_file() {
				this->m_file.flush();
				auto size = this->m_file.get_size();
				this->m_file.grow(this->file_name.c_str(), size * 2);
				this->m_file.~basic_managed_mapped_file();
				new (&this->m_file) managed_mapped_file_t(bipc::open_or_create, this->file_name.c_str(), size * 3);
				this->p_map = this->m_file.find_or_construct<map_t<_key, _value>>(this->tag_name.c_str())(less_t<_key>(), this->m_file.get_segment_manager());
				if (!p_map)
				{
					throw std::exception("construct map failed");
				}
			}

			~map_mmap() {}

		private:
			managed_mapped_file_t m_file;
			map_t<_key, _value>* p_map = nullptr;
			std::string file_name;
			std::string tag_name;
		};
	}
}
