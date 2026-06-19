#ifndef TPPPP_ARENA_HPP
#define TPPPP_ARENA_HPP

#include <cstddef>
#include <memory>
#include <new>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

class Arena
{
public:
    Arena(const Arena &)            = delete;
    Arena &operator=(const Arena &) = delete;
    Arena(Arena &&)                 = delete;
    Arena &operator=(Arena &&)      = delete;

public:
    explicit Arena(size_t bytes)
    {
        size_t page_size = sysconf(_SC_PAGESIZE);
        m_size           = (bytes + page_size - 1) & ~(page_size - 1);

        m_cur = &m_head;
        refreshBlock();
    }

    ~Arena()
    {
        for (auto it = m_destructors.rbegin(); it != m_destructors.rend(); ++it) {
            it->destruct(it->obj);
        }
        
        if (m_head.buffer != MAP_FAILED)
            munmap(m_head.buffer, m_size);

        Block *cur_block = m_head.next;
        while (cur_block != nullptr)
        {
            Block *next_block = cur_block->next;

            if (cur_block->buffer && cur_block->buffer != MAP_FAILED)
                munmap(cur_block->buffer, m_size);

            delete cur_block;
            cur_block = next_block;
        }
    }

    template <typename T, typename... Args>
    T *alloc(Args &&...args)
    {
        size_t alignment = alignof(T);
        size_t size      = sizeof(T);

        void *ptr    = m_ptr;
        size_t space = m_end - m_ptr;

        if (!std::align(alignment, size, ptr, space))
        {
            m_cur = m_cur->next = new Block{};
            refreshBlock();

            ptr   = m_ptr;
            space = m_end - m_ptr;
            std::align(alignment, size, ptr, space);
        }

        m_ptr = static_cast<char *>(ptr) + size;

        auto object = ::new (ptr) T(std::forward<Args>(args)...);

        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            m_destructors.push_back({object,
                                     [](void *p)
                                     {
                                         static_cast<T *>(p)->~T();
                                     }});
        }

        return object;
    }

private:
    void refreshBlock()
    {
        m_cur->buffer = mmap(nullptr, m_size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (m_cur->buffer == MAP_FAILED)
            throw std::bad_alloc();

        m_ptr = static_cast<char *>(m_cur->buffer);
        m_end = m_ptr + m_size;
    }

private:
    struct DestructorNode
    {
        void *obj;
        void (*destruct)(void *);
    };
    std::vector<DestructorNode> m_destructors;

private:
    struct Block
    {
        void *buffer{};
        Block *next{};
    } m_head, *m_cur{};
    char *m_ptr{};
    char *m_end{};
    size_t m_size{};
};

#endif