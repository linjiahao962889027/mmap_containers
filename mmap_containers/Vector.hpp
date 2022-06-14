#pragma once
#include <string>
#include <sys/stat.h> 
#include <boost/interprocess/allocators/node_allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/containers/vector.hpp>


namespace mmap {
	namespace vector {
		namespace bipc = boost::interprocess;

		typedef bipc::managed_mapped_file managed_mapped_file_t;
		typedef bipc::managed_mapped_file::segment_manager mapped_segment_manager_t;

		template <typename T>
		using allocator_t = bipc::node_allocator<T, mapped_segment_manager_t>;

		template <typename T>
		using vector_t = bipc::vector<T, allocator_t<T>>;

		template <typename T>
		class vector :public vector_t<T> {
		public:
			vector(managed_mapped_file_t& mapped_file) :vector_t<T>(allocator_t<T>(mapped_file.get_segment_manager())) {}
		};

		template <typename T>
		class vector_mmap {
		public:
			vector_mmap(std::string file_name, std::string tag_name, size_t size = 0) {
				this->file_name = file_name;
				this->tag_name = tag_name;
				if (!size) {
					struct _stat info;
					_stat(file_name.c_str(), &info);
					size = info.st_size ? info.st_size : 1024ull * 1024ull;
				}
				this->m_file = managed_mapped_file_t(bipc::open_or_create, file_name.c_str(), size);
				this->p_vector = this->m_file.find_or_construct<vector_t<T>>(tag_name.c_str())(this->m_file.get_segment_manager());
				if (!this->p_vector) {
					throw std::exception("vector construct failed");
				}
			}

			T& operator[](size_t index) {
				return this->p_vector->at(index);
			}

			void push_back(T& value) {
				int i = 0;
			PUSH_BACK_AGAIN:
				try {
					this->p_vector->push_back(value);
				}
				catch (...) {
					i++;
					this->expand_file();
					if (i > 3) {
						throw std::exception("push_back failed");
					}
					else {
						goto PUSH_BACK_AGAIN;
					}
				}
			}

			void push_back(T&& value) {
				int i = 0;
			PUSH_BACK_AGAIN:
				try {
					this->p_vector->push_back(value);
				}
				catch (...) {
					i++;
					this->expand_file();
					if (i > 3) {
						throw std::exception("push_back failed");
					}
					else {
						goto PUSH_BACK_AGAIN;
					}
				}
			}

			void resize(size_t size) {
				try {
					this->p_vector->resize(size);
				}
				catch (...) {
					this->expand_file(size * sizeof(T));
					this->p_vector->resize(size);
				}
			}

			void reserve(size_t size) {
				try {
					this->p_vector->reserve(size);
				}
				catch (...) {
					this->expand_file(size * sizeof(T));
					this->p_vector->reserve(size);
				}
			}

			template <class... _Valty>
			void emplace_back(_Valty&&... _Val) {
				try {
					this->p_vector->emplace_back(_Val);
				}
				catch (...) {
					this->expand_file();
					this->p_vector->emplace_back(_Val);
				}
			}

			void pop_back() {
				this->p_vector->pop_back();
			}

			size_t size() {
				return this->p_vector->size();
			}

			void clear() {
				this->p_vector->clear();
			}

			auto begin() {
				return this->p_vector->begin();
			}

			auto end() {
				return this->p_vector->end();
			}

			void erase(size_t index) {
				this->p_vector->erase(this->p_vector->begin() + index);
			}

			auto get_p_vector() {
				return this->p_vector;
			}
			auto& get_managed_mapped_file() {
				return this->m_file;
			}

			void expand_file() {
				this->expand_file(this->m_file.get_size());
			}

			void expand_file(size_t size) {
				this->m_file.flush();
				auto old_size = this->m_file.get_size();
				this->m_file.grow(this->file_name.c_str(), size);
				this->m_file.~basic_managed_mapped_file();
				new (&this->m_file) managed_mapped_file_t(bipc::open_or_create, this->file_name.c_str(), old_size + size);
				this->p_vector = this->m_file.find_or_construct<vector_t<T>>(this->tag_name.c_str())(this->m_file.get_segment_manager());
				if (!this->p_vector) {
					throw std::exception("vector construct failed");
				}
			}

		private:
			managed_mapped_file_t m_file;
			std::string file_name;
			std::string tag_name;
			vector_t<T>* p_vector;
		};
	}
}
