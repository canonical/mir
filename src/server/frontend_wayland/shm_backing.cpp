#include "shm_backing.h"
#include "mir/synchronised.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <system_error>
#include <memory>
#include <atomic>
#include <boost/throw_exception.hpp>

namespace
{
class ShmBufferSIGBUSHandler
{
public:
    static auto get_sigbus_handler() -> std::shared_ptr<ShmBufferSIGBUSHandler>
    {
        static std::mutex construction_mutex;
        std::lock_guard lock{construction_mutex};

        if (auto current_handler = installed_handler.lock())
        {
            return current_handler;
        }

        auto handler = std::shared_ptr<ShmBufferSIGBUSHandler>(new ShmBufferSIGBUSHandler{});
        installed_handler = handler;
        return handler;
    }

    ~ShmBufferSIGBUSHandler()
    {
        /* We're going to free previous_handler, so in order for it to be safe
         * to instantiate a ShmBufferSIGBUSHandler, free it, and instantiate a
         * new one we need to ensure previous_handler is nulled by this destructor.
         */
        if (auto last_handler = previous_handler.exchange(nullptr))
        {
            sigaction(SIGBUS, last_handler, nullptr);
            delete last_handler;
        }
    }

    class AccessProtector
    {
        friend class ShmBufferSIGBUSHandler;
    public:
        AccessProtector(AccessProtector const&) = delete;
        AccessProtector(AccessProtector&&) = delete;
        auto operator=(AccessProtector const&) = delete;
        auto operator=(AccessProtector&&) = delete;

        auto invalid_access_prevented() -> bool
        {
            return used;
        }

        auto within_protected_region(void* access) -> bool
        {
            auto fault_addr = reinterpret_cast<uintptr_t>(access);
            auto protected_addr = reinterpret_cast<uintptr_t>(addr);
            return fault_addr > protected_addr &&
                fault_addr < protected_addr + len;
        }

        auto provide_fallback_mapping() -> bool
        {
            // Replace the existing mapping with a fallback
            if (mmap(
                addr, len,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,
                -1, 0) == addr)
            {
                // We've successfully replaced any existing mapping with a new,
                // all-0, mapping that will not SIGBUS on access.
                used = true;
                return true;
            }
            return false;
        }

        ~AccessProtector()
        {
            if (used)
            {
                munmap(addr, len);
            }
        }
    private:
        AccessProtector(void* addr, size_t len)
            : addr{addr},
              len{len}
        {
        }

        void* addr;
        size_t len;
        std::atomic<bool> used{false};    // Atomic only to ensure signal-safety
    };

    /**
     * Prepare to safely access a region of memory
     *
     * Ensure that accesses within the memory range [addr, addr+len) can be accessed
     * without crashing with SIGBUS.
     *
     * \returns A handle representing this memory access guard. As long as the guard is
     *          live, accesses within the protected range are safe.
     */
    auto static protect_access_to(void* addr, size_t len) -> std::shared_ptr<AccessProtector>
    {
        auto protector = std::shared_ptr<AccessProtector>{new AccessProtector{addr, len}};
        auto locked_list = current_access.lock();
        install_sigbus_handler();
        locked_list->push_back(protector);
        return protector;
    }

private:
    ShmBufferSIGBUSHandler() = default;

    friend class AccessProtector;

    static void install_sigbus_handler()
    {
        struct sigaction sig_handler_desc;
        sigfillset(&sig_handler_desc.sa_mask);
        sig_handler_desc.sa_flags = SA_SIGINFO;
        sig_handler_desc.sa_sigaction = &sigbus_handler;
        auto old_handler = new struct sigaction;

        /* Note: while the documentation for sigaction doesn't explicitly
         * describe it as threadsafe, it's not marked as thread*un*safe,
         * as many things are, and it's explicitly called out that
         * concurrent sigaction/sigwait invokes undefined behaviour.
         *
         * Assume it's threadsafe.
         */
        if (sigaction(SIGBUS, &sig_handler_desc, old_handler))
        {
            using namespace std::string_literals;
            delete old_handler;
            BOOST_THROW_EXCEPTION((
                std::system_error{
                    errno,
                    std::system_category(),
                    "Failed to install SIGBUS handler for Wayland SHM"
                }));
        }
        if (old_handler->sa_sigaction != &sigbus_handler)
        {
            // Because we install the handler every time, we only want
            // to save the old handler when it's not ours!
            auto to_delete = previous_handler.exchange(old_handler);
            delete to_delete;
        }
    }

    static void sigbus_handler(int sig, siginfo_t* info, void* ucontext)
    {
        if (info->si_code == BUS_ADRERR)
        {
            /* The kernel guarantees that si_code >= 0 can only be sent from
             * the kernel, and BUS_ADRERR is > 0.
             *
             * We've been invoked by the kernel, and we've been invoked for an
             * invalid address access - this means that we're in *synchronous*
             * signal context: we're interrupting the thread that is performing
             * the read or write to memory *during* that read or write.
             *
             * That means we *can't* be interrupting an async-signal-unsafe
             * function. (Under the fairly reasonable assumption that we're
             * not doing something absolutely bonkers, like trying to store
             * pthread mutexes in a file-backed mmap()ed region).
             *
             * *That* means that we have access to sensible concurrency primitives!
             * *And* the memory allocator!
             * What bliss!
             *
             * So, even though this is a signal handler, we can use normal
             * code.
             */
            auto locked_list = current_access.lock();
            for (auto& weak_protector : *locked_list)
            {
                if (auto protector = weak_protector.lock())
                {
                    if (protector->within_protected_region(info->si_addr) &&
                        protector->provide_fallback_mapping())
                    {
                        // We've replaced the client-provided mapping with one that will
                        // not fault; it is now safe to continue.
                        return;
                    }
                }
            }
        }

        /* The SIGBUS is not a physical address error, the access is not in our
         * protected range, or the fallback mapping failed.
         *
         * We cannot save you now.
         * Call the previous SIGBUS handler.
         */
        sigaction(SIGBUS, previous_handler.load(), nullptr);
        if (previous_handler.load()->sa_flags & SA_SIGINFO)
        {
            (previous_handler.load()->sa_sigaction)(sig, info, ucontext);
        }
        else
        {
            (previous_handler.load()->sa_handler)(sig);
        }
    }
    static mir::Synchronised<std::vector<std::weak_ptr<AccessProtector>>> current_access;
    static std::atomic<struct sigaction*> previous_handler;
    static std::weak_ptr<ShmBufferSIGBUSHandler> installed_handler;
};
std::weak_ptr<ShmBufferSIGBUSHandler> ShmBufferSIGBUSHandler::installed_handler;
std::atomic<struct sigaction*> ShmBufferSIGBUSHandler::previous_handler;
mir::Synchronised<std::vector<std::weak_ptr<ShmBufferSIGBUSHandler::AccessProtector>>>
    ShmBufferSIGBUSHandler::current_access;

class ShmBacking
{
public:
    ShmBacking(mir::Fd const& backing_store, size_t claimed_size, int prot);
    ~ShmBacking();

    template<typename Mapping, typename Parent>
    auto get_range(size_t start, size_t len, std::shared_ptr<Parent> parent)
        -> std::unique_ptr<Mapping>;

    void resize(size_t new_size);
private:
    std::shared_ptr<ShmBufferSIGBUSHandler> const sigbus_handler;
    void* mapped_address;
    size_t size;
    bool size_is_trustworthy{false};
};

ShmBacking::ShmBacking(mir::Fd const& backing_store, size_t claimed_size, int prot)
    : sigbus_handler{ShmBufferSIGBUSHandler::get_sigbus_handler()},
      size{claimed_size}
{
    mapped_address = mmap(nullptr, claimed_size, prot, MAP_SHARED, backing_store, 0);
    if (mapped_address == MAP_FAILED)
    {
        BOOST_THROW_EXCEPTION((std::system_error{
            errno,
            std::system_category(),
            "Failed to map client-provided SHM pool"}));
    }

    int file_seals = fcntl(backing_store, F_GET_SEALS);
    if (file_seals == -1)
    {
        // Unsupported? Some other error? We don't really care, this is just an optimisation
        // If we can't do that optimisation, we're done.
        return;
    }
    if (file_seals & F_SEAL_SHRINK)
    {
        /* The kernel promises that the underlying file cannot get smaller, so we can
         * usefully check to see if it's big enough now, and if so, we don't have to do the SIGBUS
         * dance
         */
        struct stat file_info;
        if (fstat(backing_store, &file_info) >= 0)
        {
            size_is_trustworthy = static_cast<size_t>(file_info.st_size) >= claimed_size;
        }
    }
}

ShmBacking::~ShmBacking()
{
    munmap(mapped_address, size);
}

template<typename Mapping, typename Parent>
auto ShmBacking::get_range(size_t start, size_t len, std::shared_ptr<Parent> parent)
    -> std::unique_ptr<Mapping>
{
    // This slightly weird comparison is to avoid integer overflow
    if ((start > size) ||
        (size - start < len))
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Attempt to get a range outside the SHM backing"}));
    }
    auto start_addr = static_cast<char*>(mapped_address) + start;
    return std::make_unique<Mapping>(
        start_addr, len,
        std::move(parent),
        size_is_trustworthy ? nullptr : sigbus_handler->protect_access_to(start_addr, len));
}

void ShmBacking::resize(size_t new_size)
{
    mapped_address = ::mremap(mapped_address, size, new_size, MREMAP_MAYMOVE);
    size = new_size;
}

template<typename T>
class ShmBackedMapping : public mir::Mapping<T>
{
public:
    ShmBackedMapping(T* data, size_t size)
        : ptr{data},
          size{size}
    {
    }

    auto data() -> T* override
    {
        return ptr;
    }

    auto len() const -> size_t override
    {
        return size;
    }
private:
    T* const ptr;
    size_t const size;
};

class ROMappableRange : public mir::ReadMappableRange
{
public:
    ROMappableRange(
        void* ptr,
        size_t len,
        std::shared_ptr<mir::shm::ReadOnlyPool> parent,
        std::shared_ptr<ShmBufferSIGBUSHandler::AccessProtector> sigbus_guard)
        : ptr{ptr},
          len{len},
          parent{std::move(parent)},
          sigbus_guard{std::move(sigbus_guard)}
    {
    }

    auto map_ro() -> std::unique_ptr<mir::Mapping<std::byte const>> override
    {
        return std::make_unique<ShmBackedMapping<std::byte const>>(static_cast<std::byte const*>(ptr), len);
    }

    auto access_fault() const -> bool override
    {
        return sigbus_guard ? sigbus_guard->invalid_access_prevented() : false;
    }
private:
    void* const ptr;
    size_t const len;
    std::shared_ptr<mir::shm::ReadOnlyPool> const parent;
    std::shared_ptr<ShmBufferSIGBUSHandler::AccessProtector> const sigbus_guard;
};

class WOMappableRange : public mir::WriteMappableRange
{
public:
    WOMappableRange(
        void* ptr,
        size_t len,
        std::shared_ptr<mir::shm::WriteOnlyPool> parent,
        std::shared_ptr<ShmBufferSIGBUSHandler::AccessProtector> sigbus_guard)
        : ptr{ptr},
          len{len},
          parent{std::move(parent)},
          sigbus_guard{std::move(sigbus_guard)}
    {
    }

    auto map_wo() -> std::unique_ptr<mir::Mapping<std::byte>> override
    {
        return std::make_unique<ShmBackedMapping<std::byte>>(static_cast<std::byte*>(ptr), len);
    }

    auto access_fault() const -> bool override
    {
        return sigbus_guard ? sigbus_guard->invalid_access_prevented() : false;
    }
private:
    void* const ptr;
    size_t const len;
    std::shared_ptr<mir::shm::WriteOnlyPool> const parent;
    std::shared_ptr<ShmBufferSIGBUSHandler::AccessProtector> const sigbus_guard;
};

class RWShmBackedPool;

class RWMappableRange : public mir::RWMappableRange
{
public:
    RWMappableRange(
        void* ptr,
        size_t len,
        std::shared_ptr<mir::shm::ReadWritePool> parent,
        std::shared_ptr<ShmBufferSIGBUSHandler::AccessProtector> sigbus_guard)
        : ptr{ptr},
          len{len},
          parent{std::move(parent)},
          sigbus_guard{std::move(sigbus_guard)}
    {
    }

    auto map_rw() -> std::unique_ptr<mir::Mapping<std::byte>> override
    {
        return std::make_unique<ShmBackedMapping<std::byte>>(static_cast<std::byte*>(ptr), len);
    }

    auto map_ro() -> std::unique_ptr<mir::Mapping<std::byte const>> override
    {
        return std::make_unique<ShmBackedMapping<std::byte const>>(static_cast<std::byte*>(ptr), len);
    }

    auto map_wo() -> std::unique_ptr<mir::Mapping<std::byte>> override
    {
        return std::make_unique<ShmBackedMapping<std::byte>>(static_cast<std::byte*>(ptr), len);
    }

    auto access_fault() const -> bool override
    {
        return sigbus_guard ? sigbus_guard->invalid_access_prevented() : false;
    }
private:
    void* const ptr;
    size_t const len;
    std::shared_ptr<mir::shm::ReadWritePool> const parent;
    std::shared_ptr<ShmBufferSIGBUSHandler::AccessProtector> const sigbus_guard;
};

class RWShmBackedPool : public mir::shm::ReadWritePool, public std::enable_shared_from_this<RWShmBackedPool>
{
public:
    RWShmBackedPool(mir::Fd const& backing, size_t claimed_size)
        : backing_store{backing, claimed_size, PROT_READ | PROT_WRITE}
    {
    }

    auto get_rw_range(size_t start, size_t len) -> std::unique_ptr<mir::RWMappableRange> override
    {
        return backing_store.get_range<RWMappableRange>(start, len, shared_from_this());
    }

    auto get_ro_range(size_t start, size_t len) -> std::unique_ptr<mir::ReadMappableRange> override
    {
        return backing_store.get_range<ROMappableRange>(start, len, shared_from_this());
    }

    auto get_wo_range(size_t start, size_t len) -> std::unique_ptr<mir::WriteMappableRange> override
    {
        return backing_store.get_range<WOMappableRange>(start, len, shared_from_this());
    }

    void resize(size_t new_size) override
    {
        backing_store.resize(new_size);
    }
private:
    ShmBacking backing_store; 
};
}

auto mir::shm::rw_pool_from_fd(mir::Fd const& backing, size_t claimed_size) -> std::shared_ptr<ReadWritePool>
{
    return std::make_shared<RWShmBackedPool>(backing, claimed_size);
}
