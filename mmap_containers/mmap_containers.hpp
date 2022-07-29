#pragma once
#include <string>
#include <sys/stat.h>
#include <functional>
#include <boost/interprocess/allocators/node_allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/segment_manager.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/slist.hpp>
#include <boost/interprocess/containers/stable_vector.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>
#include <functional>
#include <string>


namespace mmap {
	namespace bipc = boost::interprocess;
	using managed_mapped_file_t = bipc::managed_mapped_file;
	using mapped_segment_manager_t = bipc::managed_mapped_file::segment_manager;

	template <typename T>
	using allocator_t = bipc::node_allocator<T, mapped_segment_manager_t>;

	template <typename T>
	using less_t = std::less<T>;

	/*template <typename T>
	class basic_string :public bipc::basic_string <T> {
	public:
		basic_string() :bipc::basic_string <T>() {};
		basic_string(const T* str) :bipc::basic_string <T>() {
			this->assign(str);
		}
		operator std::basic_string<T>() const {
			std::basic_string<T> ret;
			ret.assign(this->c_str(), this->size());
			return ret;
		}
		basic_string<T>& operator =(const std::basic_string<T>& str) {
			this->assign(str.c_str(), str.size());
			return *this;
		}
	};*/

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

	template <typename T>
	class basic_string : public vector<T> {
	public:
		const T* c_str() const {
			return this->data();
		}
		basic_string(managed_mapped_file_t& mapped_file) :vector<T>(mapped_file) {};
		operator std::basic_string<T>() const {
			std::basic_string<T> ret;
			ret.assign(this->c_str(), this->size());
			return ret;
		}
		basic_string<T>& operator =(const std::basic_string<T>& str) {
			this->clear();
			for (auto c : str) {
				this->push_back(c);
			}
			return *this;
		}
		void append(const std::basic_string<T>& str) {
			for (auto c : str) {
				this->push_back(c);
			}
		}
		void append(const basic_string<T>& str) {
			for (auto c : str) {
				this->push_back(c);
			}
		}
		void append(const T& ch) {
			this->push_back(ch);
		}
		basic_string<T> substr(size_t pos, size_t len) {
			basic_string<T> ret;
			for (size_t i = pos; i < pos + len; i++) {
				ret.push_back(this->at(i));
			}
			return ret;
		}
		basic_string<T> substr(size_t pos) {
			return this->substr(pos, this->size() - pos);
		}
		basic_string<T> operator +(const std::basic_string<T>& str) {
			basic_string<T> ret(*this);
			ret.append(str);
			return ret;
		}
		basic_string<T> operator +(const basic_string<T>& str) {
			basic_string<T> ret(*this);
			ret.append(str);
			return ret;
		}
		basic_string<T> operator +(const T& ch) {
			basic_string<T> ret(*this);
			ret.push_back(ch);
			return ret;
		}
		basic_string<T>& operator +=(const std::basic_string<T>& str) {
			for (auto c : str) {
				this->push_back(c);
			}
			return *this;
		}
		basic_string<T>& operator +=(const basic_string<T>& str) {
			this->append(str);
			return *this;
		}
		friend std::ostream& operator << (std::ostream& output, const basic_string<T>& str) {
			output << str.c_str();
			return output;
		}
	};

	using string = basic_string<char>;
	using wstring = basic_string<wchar_t>;

	template <typename T, typename _Pr = less_t<T>>
	using set = basic_liner_container<bipc::set<T, _Pr, allocator_t<T>>>;

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

		template <class... _Valtys>
		constexpr decltype(auto) emplace_back(_Valtys&&... _Vals) {
			try {
				return this->p->emplace_back(std::forward<_Valtys>(_Vals)...);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace_back(std::forward<_Valtys>(_Vals)...);
			}
		}

		template <class... _Valtys>
		constexpr iterator emplace(const_iterator _Where, _Valtys&&... _Vals) {
			try {
				return this->p->emplace(_Where, std::forward<_Valtys>(_Vals)...);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace(_Where, std::forward<_Valtys>(_Vals)...);
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

		template <class... _Valtys>
		constexpr decltype(auto) emplace_front(_Valtys&&... _Val) {
			try {
				return this->p->emplace_front(std::forward<_Valtys>(_Val)...);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace_front(std::forward<_Valtys>(_Val)...);
			}
		}

		constexpr void push_front(const value_type& _Val) {
			try {
				this->p->push_front(_Val);
			}
			catch (...) {
				this->expand_file();
				this->p->push_front(_Val);
			}
		}

		constexpr void push_front(value_type&& _Val) {
			try {
				this->p->push_front(_Val);
			}
			catch (...) {
				this->expand_file();
				this->p->push_front(_Val);
			}
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

		template <class... _Valtys>
		constexpr decltype(auto) emplace_front(_Valtys&&... _Val) {
			return this->p->emplace_front(std::forward<_Valtys>(_Val)...);
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

		template <class... _Valtys>
		constexpr iterator emplace(const_iterator _Where, _Valtys&&... _Val) {
			try {
				return this->p->emplace(_Where, std::forward<_Valtys>(_Val)...);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace(_Where, std::forward<_Valtys>(_Val)...);
			}
		}

		template <class... _Valtys>
		constexpr iterator emplace_after(const_iterator _Where, _Valtys&&... _Val) {
			try {
				return this->p->emplace_after(_Where, std::forward<_Valtys>(_Val)...);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace_after(_Where, std::forward<_Valtys>(_Val)...);
			}
		}

		template <class... _Valtys>
		constexpr iterator emplace_front(_Valtys&&... _Val) {
			try {
				return this->p->emplace_front(std::forward<_Valtys>(_Val)...);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace_front(std::forward<_Valtys>(_Val)...);
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

	template <typename T, typename _Pr = less_t<T>>
	class set_proxy :
		public basic_liner_container_proxy<bipc::set<T, _Pr, allocator_t<T>>>
	{
	public:
		using value_type = T;
		using container = bipc::set<T, _Pr, allocator_t<T>>;
		using iterator = container::iterator;
		using const_iterator = container::const_iterator;

		set_proxy(std::string file_name, std::string tag_name, size_t size = 0)
			:basic_liner_container_proxy<bipc::set<T, _Pr, allocator_t<T>>>
			(file_name, tag_name, size) {};

		constexpr iterator find(const value_type& value) {
			return this->p->find(value);
		}

		constexpr const_iterator find(const value_type& value) const {
			return this->p->find(value);
		}

		template <class... _Valtys>
		constexpr iterator emplace_hint(const_iterator _Where, _Valtys&&... _Vals) {
			try {
				return this->p->emplace_hint(_Where, std::forward<_Valtys>(_Vals)...);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace_hint(_Where, std::forward<_Valtys>(_Vals)...);
			}
		}

		template <class... _Valtys>
		constexpr auto emplace(_Valtys&&... _Vals) {
			try {
				return this->p->emplace(std::forward<_Valtys>(_Vals)...);
			}
			catch (...) {
				this->expand_file();
				return this->p->emplace(std::forward<_Valtys>(_Vals)...);
			}
		}

		constexpr size_t count(const value_type& value) const {
			return this->p->count(value);
		}

		constexpr auto key_comp() const {
			return this->p->key_comp();
		}

		constexpr auto value_comp() const {
			return this->p->value_comp();
		}

		constexpr bool contains(const value_type& value) const {
			return this->p->contains(value);
		}

		constexpr void merge(container& other) {
			try {
				this->p->merge(other);
			}
			catch (...) {
				this->expand_file();
				this->p->merge(other);
			}
		}

		constexpr void merge(container&& other) {
			try {
				this->p->merge(other);
			}
			catch (...) {
				this->expand_file();
				this->p->merge(other);
			}
		}

		constexpr auto equal_range(const value_type& value) {
			return this->p->equal_range(value);
		}

		constexpr auto equal_range(const value_type& value) const {
			return this->p->equal_range(value);
		}

		constexpr auto extract(const value_type& value) {
			return this->p->extract(value);
		}

		constexpr auto extract(const const_iterator itr) {
			return this->p->extract(itr);
		}

		constexpr iterator lower_bound(const value_type& value) {
			return this->p->lower_bound(value);
		}

		constexpr const_iterator lower_bound(const value_type& value) const {
			return this->p->lower_bound(value);
		}

	private:
		constexpr value_type& operator[](size_t index);
		constexpr const value_type& operator[](size_t index) const;
		constexpr value_type& at(size_t index);
		constexpr const value_type& at(size_t index) const;
	};

	template<typename _Key, typename _Value>
	using pair_t = std::pair<_Key, _Value>;

	template<typename Container>
	class map_basic : public Container {
	public:
		using container = Container;
		using key_type = container::key_type;
		using mapped_type = container::mapped_type;
		using value_type = container::value_type;
		using iterator = container::iterator;
		using const_iterator = container::const_iterator;

		map_basic(managed_mapped_file_t& mapped_file)
			:container(allocator_t<value_type>(mapped_file.get_segment_manager())) {
			this->p_mapped_file = &mapped_file;
		};

		constexpr value_type& operator[](const key_type& key) {
			iterator itr = this->find(key);
			if (itr == this->end()) {
				if constexpr (std::is_constructible<value_type, managed_mapped_file_t&>::value) {
					this->emplace(value_type(key, value_type(*(this->p_mapped_file))));
				}
				else {
					this->emplace(value_type(key, value_type()));
				}
				return this->at(key);
			}
			else {
				return itr->second;
			}
		}

		constexpr const value_type& operator[](const key_type& key) const {
			const_iterator itr = this->find(key);
			if (itr == this->end()) {
				throw std::out_of_range("key not found");
			}
			return itr->second;
		}
	private:
		managed_mapped_file_t* p_mapped_file;
	};

	template<typename _Key, typename _Value, typename _Pr = less_t<_Key>>
	using map = map_basic<bipc::map<_Key, _Value, _Pr, allocator_t<pair_t<const _Key, _Value>>>>;

	template<typename _Key, typename _Value, typename _Hash = boost::hash<_Key>, typename _Pr = std::equal_to<_Key>>
	using unordered_map = map_basic<boost::unordered_map<_Key, _Value, _Hash, _Pr, allocator_t<pair_t<const _Key, _Value>>>>;

	template<typename Container>
	class map_basic_proxy {
	public:
		using container = Container;
		using key_type = container::key_type;
		using mapped_type = container::mapped_type;
		using value_type = container::value_type;
		using iterator = container::iterator;
		using const_iterator = container::const_iterator;

		map_basic_proxy(std::string file_name, std::string tag_name, size_t size = 0) {
			this->file_name = std::move(file_name);
			this->tag_name = std::move(tag_name);
			if (!size) {
				struct _stat info;
				_stat(this->file_name.c_str(), &info);
				size = info.st_size ? info.st_size : 1024ull * 1024ull;
			}
			this->m_file = managed_mapped_file_t(bipc::open_or_create, this->file_name.c_str(), size);
			this->p = this->m_file.find_or_construct<container>(this->tag_name.c_str())(this->m_file.get_segment_manager());
		}

		constexpr managed_mapped_file_t& get_managed_mapped_file() noexcept {
			return this->m_file;
		}

		constexpr const managed_mapped_file_t& get_managed_mapped_file() const noexcept {
			return this->m_file;
		}

		constexpr container* get_container() noexcept {
			return this->p;
		}

		constexpr const container* get_container() const noexcept {
			return this->p;
		}

		void expand_file(size_t size) {
			this->m_file.flush();
			auto old_size = this->m_file.get_size();
			this->m_file.grow(this->file_name.c_str(), size);
			this->m_file.~basic_managed_mapped_file();
			new (&this->m_file) managed_mapped_file_t(bipc::open_or_create, this->file_name.c_str(), old_size + size);
			this->p = this->m_file.find_or_construct<container>(this->tag_name.c_str())(this->m_file.get_segment_manager());
			if (!this->p) {
				throw std::exception("construct failed");
			}
		}

		void expand_file() {
			this->expand_file(this->m_file.get_size());
		}

		mapped_type& operator[](const key_type& key) {
			iterator itr = this->p->find(key);
			if (itr == this->p->end()) {
				try {
					if constexpr (std::is_constructible<mapped_type, managed_mapped_file_t&>::value) {
						this->p->emplace(key, this->m_file);
					}
					else {
						this->p->emplace(key, mapped_type());
					}
				}
				catch (...) {
					this->expand_file();
					if constexpr (std::is_constructible<mapped_type, managed_mapped_file_t&>::value) {
						this->p->emplace(key, this->m_file);
					}
					else {
						this->p->emplace(key, mapped_type());
					}
				}
				return this->p->at(key);
			}
			else {
				return itr->second;
			}
		}

		const mapped_type& operator[](const key_type& key) const {
			const_iterator itr = this->p->find(key);
			if (itr == this->p->end()) {
				throw std::out_of_range("key not found");
			}
			return itr->second;
		}

		constexpr value_type& at(key_type key) {
			return this->p->at(key);
		}

		constexpr const value_type& at(key_type key) const {
			return this->p->at(key);
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

		template <class ..._Valtys>
		constexpr auto emplace(_Valtys&&... _Vals) {
			return this->p->emplace(std::forward(_Vals)...);
		}

		constexpr iterator erase(const_iterator _Where) noexcept {
			return this->p->erase(_Where);
		}

		constexpr iterator erase(const_iterator _First, const_iterator _Last) noexcept {
			return this->p->erase(_First, _Last);
		}

		constexpr size_t erase(const key_type& _Keyval) noexcept {
			return this->p->erase(_Keyval);
		}

		constexpr iterator erase(iterator _Where) noexcept {
			return this->p->erase(_Where);
		}

		constexpr auto find(const key_type& _Keyval) noexcept {
			return this->p->find(_Keyval);
		}

		constexpr auto find(const key_type& _Keyval) const noexcept {
			return this->p->find(_Keyval);
		}

		constexpr size_t count(const key_type& _Keyval) const noexcept {
			return this->p->count(_Keyval);
		}

	private:
		managed_mapped_file_t m_file;
		std::string file_name;
		std::string tag_name;
		container* p = nullptr;
	};

	template<typename _Key, typename _Value, typename _Pr = less_t<_Key>>
	using map_proxy = map_basic_proxy<bipc::map<_Key, _Value, _Pr, allocator_t<pair_t<const _Key, _Value>>>>;

	template<typename _Key, typename _Value, typename _Hash = boost::hash<_Key>, typename _Pr = std::equal_to<_Key>>
	using unordered_map_proxy = map_basic_proxy<boost::unordered_map<_Key, _Value, _Hash, _Pr, allocator_t<pair_t<const _Key, _Value>>>>;
}
