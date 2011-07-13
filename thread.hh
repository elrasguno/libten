#ifndef THREAD_HH
#define THREAD_HH

#include <list>
#include <boost/function.hpp>
#include <boost/utility.hpp>
#include <pthread.h>
#include <sys/syscall.h>
#include "error.hh"
#include "coroutine.hh"

#include <iostream>

class coroutine;
class thread;

namespace detail {
extern __thread thread *thread_;
}

class mutex : boost::noncopyable {
public:
    mutex(const pthread_mutexattr_t *mutexattr = NULL) {
        pthread_mutex_init(&m, mutexattr);
    }

    ~mutex() {
        THROW_ON_NONZERO(pthread_mutex_destroy(&m));
    }

    class scoped_lock : boost::noncopyable {
    public:
        scoped_lock(mutex &m_, bool lock_=true) : m(m_) {
            if (lock_) { lock(); }
        }

        void lock() { m.lock(); }
        bool trylock() { return m.trylock(); }
        bool timedlock(const struct timespec &abs_timeout) { return m.timedlock(abs_timeout); }
        void unlock() { m.unlock(); }

        ~scoped_lock() {
            unlock();
        }
    protected:
        friend class condition;
        mutex &m;
    };

protected:
    friend class scoped_lock;

    void lock() {
        THROW_ON_NONZERO(pthread_mutex_lock(&m));
    }

    bool trylock() {
        int r = pthread_mutex_trylock(&m);
        if (r == 0) {
            return true;
        } else if (r == EBUSY) {
            return false;
        }
        THROW_ON_NONZERO(r);
        return false;
    }

    bool timedlock(const struct timespec &abs_timeout) {
        int r = pthread_mutex_timedlock(&m, &abs_timeout);
        if (r == 0) {
            return true;
        } else if (r == ETIMEDOUT) {
            return false;
        }
        THROW_ON_NONZERO(r);
        return false;
    }

    void unlock() {
        THROW_ON_NONZERO(pthread_mutex_unlock(&m));
    }
protected:
    friend class condition;
    pthread_mutex_t m;
};

class condition {
public:
    condition(const pthread_condattr_t *attr=NULL) {
        THROW_ON_NONZERO(pthread_cond_init(&c, attr));
    }

    void signal() {
        THROW_ON_NONZERO(pthread_cond_signal(&c));
    }

    void broadcast() {
        THROW_ON_NONZERO(pthread_cond_signal(&c));
    }

    void wait(mutex::scoped_lock &l) {
        THROW_ON_NONZERO(pthread_cond_wait(&c, &l.m.m));
    }

    ~condition() {
        THROW_ON_NONZERO(pthread_cond_destroy(&c));
    }
private:
    pthread_cond_t c;
};

typedef std::list<coroutine *> coro_list;

class thread : boost::noncopyable {
public:
    pid_t id() { return syscall(SYS_gettid);  }

    static thread *spawn(const coroutine::func_t &f) {
        return new thread(f);
    }

    static thread &self() {
        if (detail::thread_ == NULL) {
            detail::thread_ = new thread;
        }
        return *detail::thread_;
    }

    void detach() {
        detached = true;
        int r = pthread_detach(t);
        THROW_ON_NONZERO(r);
    }

    void *join() {
        void *rvalue = NULL;
        int r = pthread_join(t, &rvalue);
        if (r == 0) {
            delete this;
        }
        THROW_ON_NONZERO(r);
        return rvalue;
    }

    void sleep() {
        mutex::scoped_lock l(mut);
        asleep = true;
        while (asleep) {
            cond.wait(l);
        }
    }

    void wakeup() {
        mutex::scoped_lock l(mut);
        wakeup_nolock();
    }

    void schedule();

protected:
    friend class coroutine;

    // called by coroutine::spawn
    void add_to_runqueue(coroutine *c) {
        mutex::scoped_lock l(mut);
        runq.push_back(c);
        wakeup_nolock();
    }

    // lock must already be held
    void wakeup_nolock() {
        if (asleep) {
            asleep = false;
            cond.signal();
        }
    }

    void set_coro(coroutine *c) { coro = c; }
    coroutine *get_coro() {
        return coro;
    }

private:

    pthread_t t;

    mutex mut;
    condition cond;
    bool asleep;

    bool detached;
    coroutine scheduler;
    coroutine *coro;
    coro_list runq;

    thread() : t(0), asleep(false), detached(false), coro(0) {
    }

    thread(const coroutine::func_t &f) : t(0), asleep(false), detached(false), coro(0) {
        coroutine *c = new coroutine(f);
        add_to_runqueue(c);
        pthread_create(&t, NULL, start, this);
    }

    ~thread() {
    }

    static void *start(void *arg);
};

#endif // THREAD_HH