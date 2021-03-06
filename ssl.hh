#include <openssl/ssl.h>
#include <openssl/bio.h>
#include "net.hh"
#include "error.hh"

namespace ten {

struct sslerror : public backtrace_exception {
    char errstr[128];
    long err;

    sslerror();
    const char *what() const throw() { return errstr; }
};

BIO_METHOD *BIO_s_netfd(void);
BIO *BIO_new_netfd(int fd, int close_flag);

class sslsock : public sockbase {
public:
    SSL_CTX *ctx;
    BIO *bio;

    sslsock(int fd=-1) throw (errno_error);
    sslsock(int domain, int type, int protocol=0) throw (errno_error);

    ~sslsock();

    //! false for server mode
    void initssl(SSL_CTX *ctx_, bool client);
    void initssl(SSL_METHOD *method, bool client);

#ifdef __GXX_EXPERIMENTAL_CXX0X__
    // C++0x move stuff
    sslsock(netsock &&other) : sockbase(), ctx(0), bio(0) {
        std::swap(s, other.s);
    }

    sslsock(sslsock &&other) : sockbase(), ctx(0), bio(0) {
        std::swap(s, other.s);
        std::swap(ctx, other.ctx);
        std::swap(bio, other.bio);
    }
    sslsock &operator = (sslsock &&other) {
        if (this != &other) {
            std::swap(s, other.s);
            std::swap(ctx, other.ctx);
            std::swap(bio, other.bio);
        }
        return *this;
    }
#endif

    //! dial requires a large 8MB stack size for getaddrinfo
    int dial(const char *addr,
            uint16_t port, unsigned timeout_ms=0) __attribute__((warn_unused_result));

    int connect(const address &addr,
            unsigned ms=0) __attribute__((warn_unused_result))
    {
        return netconnect(s.fd, addr, ms);
    }

    int accept(address &addr,
            int flags=0, unsigned timeout_ms=0) __attribute__((warn_unused_result))
    {
        return netaccept(s.fd, addr, flags, timeout_ms);
    }

    ssize_t recv(void *buf,
            size_t len, int flags=0, unsigned timeout_ms=0) __attribute__((warn_unused_result))
    {
        return BIO_read(bio, buf, len);
    }

    ssize_t send(const void *buf,
            size_t len, int flags=0, unsigned timeout_ms=0) __attribute__((warn_unused_result))
    {
        return BIO_write(bio, buf, len);
    }

    void handshake();

};



} // end namespace ten

