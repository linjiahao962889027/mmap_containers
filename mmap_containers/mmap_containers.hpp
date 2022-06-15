#pragma once
#include <string>
#include <sys/stat.h>
#include <boost/interprocess/allocators/node_allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/segment_manager.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/slist.hpp>
#include <boost/interprocess/containers/stable_vector.hpp>


namespace mmap {
	namespace bipc = boost::interprocess;
	using managed_mapped_file_t = bipc::managed_mapped_file;
	using mapped_segment_manager_t = bipc::managed_mapped_file::segment_manager;

	template <typename T>
	using allocator_t = bipc::node_allocator<T, mapped_segment_manager_t>;

	template <typename T>
	concept _Container = requires (T t) {
		T::value_type;
		T::iterator;
		T::const_iterator;
		t.at(0);
	};
	template <typename _Container>
	class basic_liner_container :public _Container {
	public:
		using value_type = _Container::value_type;
		basic_liner_container(managed_mapped_file_t& mapped_file)
			:_Container(allocator_t<value_type>(mapped_file.get_segment_manager())) {};
	};

	template <typename T>
	using vector = basic_liner_container<bipc::vector<T, allocator_t<T>>>;

	template <typename T>
	using deque = basic_liner_container<bipc::deque<T, allocator_t<T>>>;

	template <typename T>
	using list = basic_liner_container<bipc::list<T, allocator_t<T>>>;

	template <typename T>
	using slist = basic_liner_container<bipc::list<T, allocator_t<T>>>;

	template <typename T>
	using stable_vector = basic_liner_container<bipc::stable_vector<T, allocator_t<T>>>;

	template <typename _Container>
	class basic_liner_container_proxy {
	public:
		using value_type = _Container::value_type;
		using iterator = _Container::iterator;
		using const_iterator = _Container::const_iterator;

		basic_liner_container_proxy(std::string file_name, std::string tag_name, size_t size = 0) {
			this->file_name = std::move(file_name);
			this->tag_name = std::move(tag_name);
			if (!size) {
				struct _stat info;
				_stat(this->file_name.c_str(), &info);
				size = info.st_size ? info.st_size : 1024ull * 1024ull;
			}
			this->m_file = managed_mapped_file_t(bipc::open_or_create, this->file_name.c_str(), size);
			this->p = this->m_file.find_or_construct<_Container>(this->tag_name.c_str())(this->m_file.get_segment_manager());
		}

		constexpr value_type& at(const size_t index) {
			return this->p->at(index);
		}

		constexpr const value_type& at(const size_t index) const {
			return this->p->at(index);
		}

		constexpr value_type& operator[](size_t index) noexcept {
			return this->p->operator[](index);
		}

		constexpr size_t size() const noexcept {
			return this->p->size();
		}

		constexpr void clear() noexcept {
			this->p->clear();
		}

		constexpr iterator begin() noexcept {
			return this->p->begin();
		}

		constexpr const_iterator begin() const noexcept {
			return this->p->begin();
		}

		constexpr const_iterator cbegin() const noexcept {
			return this->p->cbegin();
		}

		constexpr iterator end() noexcept {
			return this->p->end();
		}

		constexpr const_iterator end() const noexcept {
			return this->p->end();
		}

		constexpr const_iterator cend() const noexcept {
			return this->p->cend();
		}

		constexpr bool empty() const {
			return this->p->empty();
		}

		constexpr size_t max_size() const {
			return this->p->max_size();
		}

		constexpr iterator insert(const_iterator _Where, const value_type& _Val) {
			try {
				return this->p->insert(_Where, _Val);
			}
			catch (...) {
				this->expand_file();
				return this->p->insert(_Where, _Val);
			}
		}

		constexpr iterator insert(const_iterator _Where, value_type&& _Val) {
			try {
				return this->p->insert(_Where, _Val);
			}
			catch (...) {
				this->expand_file();
				return this->p->insert(_Where, _Val);
			}
		}

		constexpr iterator insert(const_iterator _Where, const size_t _Count, const value_type& _Val) {
			try {
				return this->p->insert(_Where, _Count, _Val);
			}
			catch (...) {
				this->expand_file();
				return this->p->insert(_Where, _Count, _Val);
			}
		}

		constexpr iterator insert(const_iterator _Where, iterator _First, iterator _Last) {
			try {
				return this->p->insert(_Where, _First, _Last);
			}
			catch (...) {
				this->expand_file();
				return this->p->insert(_Where, _First, _Last);
			}
		}

		constexpr iterator erase(const_iterator _Where) noexcept {
			return this->p->erase(_Where);
		}

		constexpr iterator erase(const_iterator _First, const_iterator _Last) noexcept {
			return this->p->erase(_First, _Last);
		}

		void expand_file(size_t size) {
			this->m_file.flush();
			auto old_size = this->m_file.get_size();
			this->m_file.grow(this->file_name.c_str(), size);
			this->m_file.~basic_managed_mapped_file();
			new (&this->m_file) managed_mapped_file_t(bipc::open_or_create, this->file_name.c_str(), old_size + size);
			this->p = this->m_file.find_or_construct<_Container>(this->tag_name.c_str())(this->m_file.get_segment_manager());
			if (!this->p) {
				throw std::exception("construct failed");
			}
		}

		void expand_file() {
			this->expand_file(this->m_file.get_size());
		}

		constexpr managed_mapped_file_t& get_managed_mapped_file() noexcept {
			return this->m_file;
		}

		constexpr const managed_mapped_file_t& get_managed_mapped_file() const noexcept {
			return this->m_file;
		}

		constexpr _Container* get_container() noexcept {
			return this->p;
		}

		constexpr const _Container* get_container() const noexcept {
			return this->p;
		}

	protected:
		managed_mapped_file_t m_file;
		std::string file_name;
		std::string tag_name;
		_Container* p = nullptr;
	};

	template <typename _Container>
	class basic_liner_container_proxy_extend_1 :
		public basic_liner_container_proxy<_Container> {
	public:
		using value_type = basic_liner_container_proxy<_Container>::value_type;
		using iterator = basic_liner_container_proxy<_Container>::iterator;
		using const_iterator = basic_liner_container_proxy<_Container>::const_iterator;

		basic_liner_container_proxy_extend_1(std::string file_name, std::string tag_name, size_t size = 0)
			:basic_liner_container_proxy<_Container>
			(file_name, tag_name, size) {};

		constexpr void push_back(const value_type& _Val) {
			try {
				this->p->push_back(_Val);
			}
			catch (...) {
				this->expand_file();
				this->p->push_back(_Val);
			}
		}

		constexpr void push_back(value_type&& _Val) {
			try {
				this->p->push_back(_Val);
			}
			catch (...) {
				this->expand_file();
				this->p->push_back(_Val);
			}
		}

		template <class... _Valty>
		constexpr decltype(auto) emplace_back(_Valty&&... _Val) {
			try {
				return this->p->emplace_back(_Val);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace_back(_Val);
			}
		}

		template <class... _Valty>
		constexpr iterator emplace(const_iterator _Where, _Valty&&... _Val) {
			try {
				return this->p->emplace(_Where, _Val);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace(_Where, _Val);
			}
		}

		constexpr void shrink_to_fit() {
			this->p->shrink_to_fit();
		}

		constexpr void resize(const size_t size) {
			try {
				this->p->resize(size);
			}
			catch (...) {
				this->expand_file(sizeof(value_type) * size);
				this->p->resize(size);
			}
		}

		constexpr void resize(const size_t size, const value_type& val) {
			try {
				this->p->resize(size, val);
			}
			catch (...) {
				this->expand_file(sizeof(value_type) * size);
				this->p->resize(size, val);
			}
		}

		constexpr value_type& front() noexcept {
			return this->p->front();
		}

		constexpr const value_type& front() const noexcept {
			return this->p->front();
		}

		constexpr value_type& back() noexcept {
			return this->p->back();
		}

		constexpr const value_type& back() const noexcept {
			return this->p->back();
		}

		constexpr void pop_back() {
			this->pop_back();
		}
	};

	template <typename T>
	class vector_proxy :public
		basic_liner_container_proxy_extend_1<bipc::vector<T, allocator_t<T>>> {
	public:
		vector_proxy(std::string file_name, std::string tag_name, size_t size = 0)
			:basic_liner_container_proxy_extend_1<bipc::vector<T, allocator_t<T>>>
			(file_name, tag_name, size) {};

		constexpr void reserve(size_t size) {
			try {
				this->p->reserve(size);
			}
			catch (...) {
				this->expand_file(sizeof(T) * size);
				this->p->reserve(size);
			}
		}

		constexpr size_t capacity() const noexcept {
			return this->p->capacity();
		}
	};

	template <typename _Container>
	class basic_liner_container_proxy_extend_2 :public
		basic_liner_container_proxy_extend_1<_Container> {
	public:
		using value_type = basic_liner_container_proxy<_Container>::value_type;
		using iterator = basic_liner_container_proxy<_Container>::iterator;
		using const_iterator = basic_liner_container_proxy<_Container>::const_iterator;

		basic_liner_container_proxy_extend_2(std::string file_name, std::string tag_name, size_t size = 0)
			:basic_liner_container_proxy_extend_1<_Container>
			(file_name, tag_name, size) {};

		template <class... _Valty>
		constexpr decltype(auto) emplace_front(_Valty&&... _Val) {
			return this->p->emplace_front(_Val);
		}

		constexpr void push_front(const value_type& _Val) {
			this->p->push_front(_Val);
		}

		constexpr void push_front(value_type&& _Val) {
			this->p->push_front(_Val);
		}

		constexpr void pop_front() noexcept {
			this->p->pop_front();
		}
	};

	template <typename T>
	using deque_proxy = basic_liner_container_proxy_extend_2<bipc::deque<T, allocator_t<T>>>;

	template <typename T>
	class list_proxy :public
		basic_liner_container_proxy_extend_2<bipc::list<T, allocator_t<T>>> {
	public:
		using value_type = T;
		using iterator = basic_liner_container_proxy_extend_2<bipc::list<T, allocator_t<T>>>::iterator;
		using const_iterator = basic_liner_container_proxy_extend_2<bipc::list<T, allocator_t<T>>>::const_iterator;
		using list = bipc::list<T, allocator_t<T>>;

		list_proxy(std::string file_name, std::string tag_name, size_t size = 0)
			:basic_liner_container_proxy_extend_2<bipc::list<T, allocator_t<T>>>
			(file_name, tag_name, size) {};

		constexpr void sort() {
			this->p->sort();
		}

		template <class _Pr2>
		constexpr void sort(_Pr2 _Pred) {
			this->p->sort(_Pred);
		}

		constexpr void splice(const const_iterator _Where, list& _Right) {
			this->p->splice(_Where, _Right);
		}

		constexpr void splice(const const_iterator _Where, list&& _Right) {
			this->p->splice(_Where, _Right);
		}

		constexpr void splice(const const_iterator _Where, list& _Right, const const_iterator _First) {
			this->p->splice(_Where, _Right, _First);
		}

		constexpr void splice(const const_iterator _Where, list&& _Right, const const_iterator _First) {
			this->p->splice(_Where, _Right, _First);
		}

		constexpr void splice(const const_iterator _Where, list& _Right, const const_iterator _First, const const_iterator _Last) {
			this->p->splice(_Where, _Right, _First, _Last);
		}

		constexpr void splice(const const_iterator _Where, list&& _Right, const const_iterator _First, const const_iterator _Last) {
			this->p->splice(_Where, _Right, _First, _Last);
		}

		constexpr void merge(list& _Right) {
			this->p->merge(_Right);
		}

		constexpr void merge(list&& _Right) {
			this->p->merge(_Right);
		}

		template <class _Pr2>
		constexpr void merge(list& _Right, _Pr2 _Pred) {
			this->p->merge(_Right, _Pred);
		}

		template <class _Pr2>
		constexpr void merge(list&& _Right, _Pr2 _Pred) {
			this->p->merge(_Right, _Pred);
		}

		constexpr auto remove(const value_type& _Val) {
			return this->p->remove(_Val);
		}

		template <class _Pr1>
		constexpr auto remove_if(_Pr1 _Pred) {
			return this->p->remove_if(_Pred);
		}

		constexpr void reverse() noexcept {
			this->p->reverse();
		}

		constexpr auto unique() {
			this->p->unique();
		}

		template <class _Pr2>
		auto unique(_Pr2 _Pred) {
			this->p->unique(_Pred);
		}
	};

	template <typename T>
	class slist_proxy : public
		basic_liner_container_proxy<bipc::slist<T, allocator_t<T>>> {
	public:
		using value_type = T;
		using iterator = basic_liner_container_proxy_extend_2<bipc::list<T, allocator_t<T>>>::iterator;
		using const_iterator = basic_liner_container_proxy_extend_2<bipc::list<T, allocator_t<T>>>::const_iterator;
		using list = bipc::slist<T, allocator_t<T>>;

		slist_proxy(std::string file_name, std::string tag_name, size_t size = 0)
			:basic_liner_container_proxy<bipc::slist<T, allocator_t<T>>>
			(file_name, tag_name, size) {};

		constexpr void sort() {
			this->p->sort();
		}

		template <class _Pr2>
		constexpr void sort(_Pr2 _Pred) {
			this->p->sort(_Pred);
		}

		constexpr void splice(const const_iterator _Where, list& _Right) {
			this->p->splice(_Where, _Right);
		}

		constexpr void splice(const const_iterator _Where, list&& _Right) {
			this->p->splice(_Where, _Right);
		}

		constexpr void splice(const const_iterator _Where, list& _Right, const const_iterator _First) {
			this->p->splice(_Where, _Right, _First);
		}

		constexpr void splice(const const_iterator _Where, list&& _Right, const const_iterator _First) {
			this->p->splice(_Where, _Right, _First);
		}

		constexpr void splice(const const_iterator _Where, list& _Right, const const_iterator _First, const const_iterator _Last) {
			this->p->splice(_Where, _Right, _First, _Last);
		}

		constexpr void splice(const const_iterator _Where, list&& _Right, const const_iterator _First, const const_iterator _Last) {
			this->p->splice(_Where, _Right, _First, _Last);
		}

		constexpr void merge(list& _Right) {
			this->p->merge(_Right);
		}

		constexpr void merge(list&& _Right) {
			this->p->merge(_Right);
		}

		template <class _Pr2>
		constexpr void merge(list& _Right, _Pr2 _Pred) {
			this->p->merge(_Right, _Pred);
		}

		template <class _Pr2>
		constexpr void merge(list&& _Right, _Pr2 _Pred) {
			this->p->merge(_Right, _Pred);
		}

		constexpr auto remove(const value_type& _Val) {
			return this->p->remove(_Val);
		}

		template <class _Pr1>
		constexpr auto remove_if(_Pr1 _Pred) {
			return this->p->remove_if(_Pred);
		}

		constexpr void reverse() noexcept {
			this->p->reverse();
		}

		template <class... _Valty>
		constexpr decltype(auto) emplace_front(_Valty&&... _Val) {
			return this->p->emplace_front(_Val);
		}

		constexpr void push_front(const value_type& _Val) {
			this->p->push_front(_Val);
		}

		constexpr void push_front(value_type&& _Val) {
			this->p->push_front(_Val);
		}

		constexpr void pop_front() noexcept {
			this->p->pop_front();
		}

		constexpr auto unique() {
			this->p->unique();
		}

		template <class _Pr2>
		auto unique(_Pr2 _Pred) {
			this->p->unique(_Pred);
		}

		constexpr iterator before_begin() noexcept {
			return this->p->before_begin();
		}

		constexpr const_iterator before_begin() const noexcept {
			return this->p->before_begin();
		}

		constexpr const_iterator cbefore_begin() const noexcept {
			return this->p->cbefore_begin();
		}

		template <class... _Valty>
		constexpr iterator emplace(const_iterator _Where, _Valty&&... _Val) {
			try {
				return this->p->emplace(_Where, _Val);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace(_Where, _Val);
			}
		}

		template <class... _Valty>
		constexpr iterator emplace_after(const_iterator _Where, _Valty&&... _Val) {
			try {
				return this->p->emplace_after(_Where, _Val);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace_after(_Where, _Val);
			}
		}

		template <class... _Valty>
		constexpr iterator emplace_front(_Valty&&... _Val) {
			try {
				return this->p->emplace_front(_Val);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace_front(_Val);
			}
		}

		constexpr iterator insert_after(const_iterator prev_p, std::initializer_list<value_type> il) {
			try {
				return this->p->insert_after(prev_p, il);
			}
			catch (...) {
				this->expand_file();
				return this->p->insert_after(prev_p, il);
			}
		}

		constexpr void resize(const size_t size) {
			try {
				this->p->resize(size);
			}
			catch (...) {
				this->expand_file(sizeof(value_type) * size);
				this->p->resize(size);
			}
		}
	};

	template <typename T>
	class stable_vector_proxy :public
		basic_liner_container_proxy_extend_1<bipc::stable_vector<T, allocator_t<T>>> {
	public:
		using value_type = T;
		using iterator = basic_liner_container_proxy_extend_1<bipc::stable_vector<T, allocator_t<T>>>::iterator;
		using const_iterator = basic_liner_container_proxy_extend_1<bipc::stable_vector<T, allocator_t<T>>>::const_iterator;

		stable_vector_proxy(std::string file_name, std::string tag_name, size_t size = 0)
			:basic_liner_container_proxy_extend_1<bipc::stable_vector<T, allocator_t<T>>>
			(file_name, tag_name, size) {};

		constexpr void reserve(size_t size) {
			try {
				this->p->reserve(size);
			}
			catch (...) {
				this->expand_file(sizeof(T) * size);
				this->p->reserve(size);
			}
		}

		constexpr size_t capacity() const noexcept {
			return this->p->capacity();
		}

		constexpr size_t index_of(const_iterator itr) const noexcept {
			return this->p->index_of(itr);
		}

		constexpr size_t index_of(iterator itr) noexcept {
			return this->p->index_of(itr);
		}

		constexpr const_iterator nth(size_t n) const noexcept {
			return this->p->nth(n);
		}

		constexpr iterator nth(size_t n) noexcept {
			return this->p->nth(n);
		}
	};
}