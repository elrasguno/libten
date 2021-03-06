#ifndef COROUTINE_HH
#define COROUTINE_HH
#include <boost/utility.hpp>
#include <vector>
#include <sys/mman.h>
#include "context.hh"
#include "error.hh"

#ifndef NVALGRIND
#include <valgrind/valgrind.h>
#endif

namespace ten {

//! lightweight cooperatively scheduled threads of execution
//
//! each coroutine allocates it own stack with a guard page.
//! it uses context to swap between stacks.
class coroutine : boost::noncopyable {
public:
    typedef void (*proc)(void *);

    //! this represents the main coroutine
    coroutine() : stack_start(0), stack_end(0) { ctxt.init(); }

    //! create a new coroutine
    //
    //! allocates a stack and guard page
    coroutine(proc f, void *arg=NULL, size_t stack_size=4096)
    {
        // add on size for a guard page
        size_t pgs = getpagesize();
#ifndef NDEBUG
        size_t real_size = stack_size + pgs;
#else
        size_t real_size = stack_size;
#endif
        int r = posix_memalign((void **)&stack_end, pgs, real_size);
        THROW_ON_NONZERO(r);
#ifndef NDEBUG
        // protect the guard page
        THROW_ON_ERROR(mprotect(stack_end, pgs, PROT_NONE));
        stack_end += pgs;
#endif
        // stack grows down on x86 & x86_64
        stack_start = stack_end + stack_size;
#ifndef NVALGRIND
        valgrind_stack_id =
            VALGRIND_STACK_REGISTER(stack_start, stack_end);
#endif
        ctxt.init(f, arg, stack_start, stack_size);
    }

    ~coroutine() {
        if (stack_end) {
#ifndef NVALGRIND
            VALGRIND_STACK_DEREGISTER(valgrind_stack_id);
#endif

#ifndef NDEBUG
            size_t pgs = getpagesize();
            THROW_ON_ERROR(mprotect(stack_end-pgs, pgs, PROT_READ|PROT_WRITE));
            free(stack_end-pgs);
#else
            free(stack_end);
#endif
        }
    }

    //! save the state of the current coroutine and swap to another
    void swap(coroutine *to) {
        ctxt.swap(&to->ctxt);
    }

    //! is this the main stack?
    bool main() { return stack_start == 0; }

private:
    //! saved state of this coroutine
    context ctxt;
    //! pointer to stack start
    char *stack_start;
    //! pointer to stack end
    char *stack_end;
#ifndef NVALGRIND
    //! stack id so valgrind doesn't freak when stack swapping happens
    int valgrind_stack_id;
#endif

};

} // end namespace ten 

#endif // COROUTINE_HH
