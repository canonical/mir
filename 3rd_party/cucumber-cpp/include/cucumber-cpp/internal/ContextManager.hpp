#ifndef CUKE_CONTEXTMANAGER_HPP_
#define CUKE_CONTEXTMANAGER_HPP_

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace cuke {
namespace internal {

using boost::shared_ptr;
using boost::weak_ptr;

typedef std::vector<shared_ptr<void> > contexts_type;

class ContextManager {
public:
    void purgeContexts();
    template<class T> weak_ptr<T> addContext();

protected:
    static contexts_type contexts;
};

template<class T>
class SessionContextPtr {
public:
    SessionContextPtr();

    T& operator*();
    T* operator->();

private:
    ContextManager contextManager;
    shared_ptr<T> context;
    static weak_ptr<T> contextReference;
};


template<class T>
weak_ptr<T> ContextManager::addContext() {
    shared_ptr<T> shared(new T);
    contexts.push_back(shared);
    return weak_ptr<T> (shared);
}

template<class T>
weak_ptr<T> SessionContextPtr<T>::contextReference;

template<class T>
SessionContextPtr<T>::SessionContextPtr() {
     if (contextReference.expired()) {
          contextReference = contextManager.addContext<T> ();
     }
     context = contextReference.lock();
}

template<class T>
T& SessionContextPtr<T>::operator*() {
    return *(context.get());
}

template<class T>
T* SessionContextPtr<T>::operator->() {
    return (context.get());
}

}
}

#endif /* CUKE_CONTEXTMANAGER_HPP_ */
