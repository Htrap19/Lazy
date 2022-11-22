//
// Created by htrap19 on 11/22/22.
//

#ifndef LAZY_LIST_H
#define LAZY_LIST_H

#include <coroutine>
#include <algorithm>
#include <stdexcept>

namespace lazy
{
	template <typename T>
	class list;

	template <typename List>
	concept iterable = requires()
	{
		typename List::node_ptr;
		typename List::value_type;
		typename List::reference_type;
		typename List::const_reference_type;
	};

	template<iterable lazy_list>
	class iterator
	{
	public:
		using node_ptr = typename lazy_list::node_ptr;
		using value_type = typename lazy_list::value_type;
		using reference_type = typename lazy_list::reference_type;
		using const_reference_type = typename lazy_list::const_reference_type;
		using list_ptr = lazy_list*;

	public:
		iterator(node_ptr ptr, list_ptr list)
			: m_NodePtr(ptr), m_ListPtr(list) {}

		inline void Next()
		{
			if (m_NodePtr->m_Next == nullptr)
				m_ListPtr->resume();
		}

		inline iterator& operator++()
		{
			Next();
			m_NodePtr = m_NodePtr->m_Next;
			return *this;
		}

		inline iterator& operator++() const
		{
			Next();
			m_NodePtr = m_NodePtr->m_Next;
			return *this;
		}

		inline iterator operator++(int)
		{
			Next();
			iterator temp = *this;
			m_NodePtr = m_NodePtr->m_Next;
			return temp;
		}

		inline iterator operator++(int) const
		{
			Next();
			iterator temp = *this;
			m_NodePtr = m_NodePtr->m_Next;
			return temp;
		}

		inline reference_type operator*() { return m_NodePtr->m_Data; }

		inline const_reference_type operator*() const { return m_NodePtr->m_Data; }

		inline bool operator==(const iterator& other) const noexcept { return other.m_NodePtr == m_NodePtr; }

		inline bool operator!=(const iterator& other) const noexcept { return other.m_NodePtr != m_NodePtr; }

		inline bool operator>(const iterator& other) const noexcept { return m_NodePtr > other.m_NodePtr; }

		inline bool operator<(const iterator& other) const noexcept { return m_NodePtr < other.m_NodePtr; }

		inline bool operator>=(const iterator& other) const noexcept { return m_NodePtr >= other.m_NodePtr; }

		inline bool operator<=(const iterator& other) const noexcept { return m_NodePtr <= other.m_NodePtr; }

		inline iterator operator+(size_t num) const noexcept
		{
			auto currNode = m_NodePtr;
			while (currNode != nullptr && num > 0)
				currNode = currNode->m_Next, --num;
			return iterator(num == 0 ? currNode : nullptr, m_ListPtr);
		}

		inline value_type* operator->() { return &m_NodePtr->m_Data; }

	private:
		[[nodiscard]] inline node_ptr get_ptr() const noexcept { return m_NodePtr; }

	private:
		node_ptr m_NodePtr;
		list_ptr m_ListPtr;
		friend class list<value_type>;
	};

	template<typename T>
	class list // Linked-List Generator
	{
	public:
		struct promise_type;
		struct node;

	public:
		using coro_handle = std::coroutine_handle<promise_type>;
		using value_type = T;
		using reference_type = T&;
		using const_reference_type = const T&;
		using movable_type = T&&;
		using node_ptr = node*;
		using iterator = lazy::iterator<list<T>>;
		using const_iterator = const iterator;

		list() = default;

		~list();

		list(const list&) = delete;

		list(list&& other) noexcept;

		list& operator=(const list&) = delete;

		list& operator=(list&& other) noexcept;

		// coroutine utilities

		void resume();

		[[nodiscard]] inline bool done() const noexcept { return m_Handler.done(); }

		// list utilities

		void push_front(const_reference_type data);

		void push_front(movable_type data);

		void push_back(const_reference_type data);

		void push_back(movable_type data);

		template<typename ... Args>
		reference_type emplace_front(Args &&... args);

		template<typename ... Args>
		reference_type emplace_back(Args &&... args);

		iterator find(const_reference_type data);

		reference_type operator[](size_t index) const;

		void erase(const_iterator position);

		void erase(const_iterator first, const_iterator last);

		void clear();

		[[nodiscard]] inline size_t size() const noexcept { return size(m_RootNode); }

		// iterator utilities

		inline iterator begin()
		{
			if (m_RootNode == nullptr) resume(); return iterator(m_RootNode, this);
		}

		inline iterator begin() const
		{
			if (m_RootNode == nullptr) resume(); return iterator(m_RootNode, this);
		}

		inline iterator end() { return iterator(nullptr, this); }

		inline iterator end() const { return iterator(nullptr, this); }

	private:
		explicit list(const coro_handle& handler)
			: m_Handler(handler) {}

		void place_front(node_ptr node);

		void place_back(node_ptr node);

		reference_type get_yield_value() const;

		size_t size(node_ptr node) const noexcept;

		void remove(node_ptr node);

		node_ptr get_last_node(node_ptr node) const noexcept;

	private:
		coro_handle m_Handler;
		node_ptr m_RootNode = nullptr;
	};

#define PUSH_NODE(where, data)  auto newNode = new node(data); \
                                place_##where(newNode);

#define EMPLACE_NODE(where, ...) auto newNode = (node_ptr) ::operator new(sizeof(node)); \
                                 new(&newNode->m_Data) T(__VA_ARGS__); \
                                 place_##where(newNode);  \
                                 return newNode->m_Data;

	template<typename T>
	struct list<T>::promise_type
	{
		list get_return_object() { return list(coro_handle::from_promise(*this)); }

		auto initial_suspend() { return std::suspend_never(); }

		auto final_suspend() const noexcept { return std::suspend_always(); }

		void return_void() {}

		auto yield_value(const_reference_type data)
		{
			m_Data.~T();
			new(&m_Data) T(data);
			return std::suspend_always();
		}

		auto yield_value(movable_type data)
		{
			m_Data.~T();
			new(&m_Data) T(std::move(data));
			return std::suspend_always();
		}

		void unhandled_exception() {}

		value_type m_Data;
	};

	template<typename T>
	struct list<T>::node
	{
		explicit node(const_reference_type data)
			: m_Data(data) {}
		explicit node(movable_type data)
			: m_Data(std::move(data)) {}

		value_type m_Data;
		node_ptr m_Next = nullptr;
	};

	template<typename T>
	list<T>::~list()
	{
		if (m_Handler)
			m_Handler.destroy();
		clear();
	}

	template<typename T>
	list<T>::list(list&& other) noexcept
		: m_RootNode(other.m_RootNode),
		m_Handler(std::move(other.m_Handler))
	{
		other.m_RootNode = nullptr;
		other.m_Handler = std::coroutine_handle<promise_type>();
	}

	template<typename T>
	list<T>& list<T>::operator=(list&& other) noexcept
	{
		m_RootNode = other.m_RootNode;
		m_Handler = std::move(other.m_Handler);
		other.m_RootNode = nullptr;
		other.m_Handler = std::coroutine_handle<promise_type>();
		return *this;
	}

	template<typename T>
	void list<T>::resume()
	{
		if (done())
			return;

		push_back(std::move(get_yield_value()));
		m_Handler.resume();
	}

	template<typename T>
	void list<T>::push_front(const_reference_type data)
	{
		PUSH_NODE(front, data)
	}

	template<typename T>
	void list<T>::push_front(movable_type data)
	{
		PUSH_NODE(front, std::move(data))
	}

	template<typename T>
	void list<T>::push_back(const_reference_type data)
	{
		PUSH_NODE(back, data)
	}

	template<typename T>
	void list<T>::push_back(movable_type data)
	{
		PUSH_NODE(back, std::move(data))
	}

	template<typename T>
	template<typename ... Args>
	typename list<T>::reference_type list<T>::emplace_front(Args &&... args)
	{
		EMPLACE_NODE(front, std::forward<Args>(args)...)
	}

	template<typename T>
	template<typename ... Args>
	typename list<T>::reference_type list<T>::emplace_back(Args &&... args)
	{
		EMPLACE_NODE(back, std::forward<Args>(args)...)
	}

	template<typename T>
	typename list<T>::iterator list<T>::find(const_reference_type data)
	{
		auto currentIter = begin();
		while (currentIter != end())
		{
			if (*currentIter == data)
				break;
			++currentIter;
		}

		return currentIter;
	}

	template<typename T>
	typename list<T>::reference_type list<T>::operator[](size_t index) const
	{
		auto listSize = size();
		if (index >= listSize)
			throw std::runtime_error("Out of the range!");

		if (index == 0)
			return m_RootNode->m_Data;

		auto currentNode = m_RootNode->m_Next;
		size_t currentIndex = 1;
		while (currentNode->m_Next != nullptr)
		{
			if (currentIndex == index)
				break;
			currentNode = currentNode->m_Next;
		}
		return currentNode->m_Data;
	}

	template<typename T>
	void list<T>::erase(list::const_iterator position)
	{
		remove(position.get_ptr());
	}

	template<typename T>
	void list<T>::erase(list::const_iterator first, list::const_iterator last)
	{
		if (first >= last)
		{
			erase(first); return;
		}

		auto firstNode = first.get_ptr();
		auto lastNode = last.get_ptr();
		while (firstNode != lastNode)
		{
			auto nextNode = firstNode->m_Next;
			erase(iterator(firstNode, this));
			firstNode = nextNode;
		}
	}

	template<typename T>
	void list<T>::clear()
	{
		while (m_RootNode != nullptr)
		{
			auto nextNode = m_RootNode->m_Next;
			m_RootNode->m_Data.~T();
			::operator delete(m_RootNode);
			m_RootNode = nextNode;
		}
		m_RootNode = nullptr;
	}

	template<typename T>
	void list<T>::place_front(list::node_ptr node)
	{
		node->m_Next = m_RootNode;
		m_RootNode = node;
	}

	template<typename T>
	void list<T>::place_back(list::node_ptr node)
	{
		auto lastNode = get_last_node(m_RootNode);
		if (lastNode == nullptr)
		{
			place_front(node); return;
		}

		lastNode->m_Next = node;
	}

	template<typename T>
	typename list<T>::reference_type list<T>::get_yield_value() const { return m_Handler.promise().m_Data; }

	template<typename T>
	size_t list<T>::size(node_ptr node) const noexcept
	{
		return node == nullptr ? 0 : 1 + size(node->m_Next);
	}

	template<typename T>
	void list<T>::remove(list::node_ptr node)
	{
		auto currentNode = m_RootNode;
		node_ptr prevNode = m_RootNode;
		while (currentNode != nullptr)
		{
			if (node == currentNode)
				break;
			prevNode = currentNode;
			currentNode = currentNode->m_Next;
		}

		if (prevNode == currentNode)
			m_RootNode = currentNode->m_Next;
		else
			prevNode->m_Next = currentNode->m_Next;

		currentNode->m_Next = nullptr;
		currentNode->m_Data.~T();
		::operator delete(currentNode);
	}

	template<typename T>
	typename list<T>::node_ptr list<T>::get_last_node(node_ptr node) const noexcept
	{
		if (node == nullptr)
			return m_RootNode;

		return node->m_Next == nullptr ? node : get_last_node(node->m_Next);
	}
}

#endif //LAZY_LIST_H
