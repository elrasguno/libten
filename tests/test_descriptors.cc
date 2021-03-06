#define BOOST_TEST_MODULE descriptor test
#include <boost/test/unit_test.hpp>
#include <boost/bind.hpp>
#include "descriptors.hh"

using namespace ten;

#ifdef __GXX_EXPERIMENTAL_CXX0X__
BOOST_AUTO_TEST_CASE(fd_move_test) {
    fd_base fd1(0);
    fd_base fd2(std::move(fd1));
    BOOST_CHECK_EQUAL(fd1.fd, -1);
    BOOST_CHECK_EQUAL(fd2.fd, 0);
}

BOOST_AUTO_TEST_CASE(socketpair_cxx0x_test) {
    std::pair<socket_fd, socket_fd> sp(socket_fd::pair(AF_UNIX, SOCK_STREAM));
    {
        char a = 1;
        BOOST_CHECK_EQUAL(sp.first.write(&a, 1), 1);
        BOOST_CHECK_EQUAL(sp.second.read(&a, 1), 1);
        BOOST_CHECK_EQUAL(a, 1);
    }
}
#endif

BOOST_AUTO_TEST_CASE(socketpair_test) {
    int sv[2];
    BOOST_CHECK_EQUAL(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);
    socket_fd sp1(sv[0]);
    socket_fd sp2(sv[1]);
    char a = 1;
    BOOST_CHECK_EQUAL(sp1.write(&a, 1), 1);
    BOOST_CHECK_EQUAL(sp2.read(&a, 1), 1);
    BOOST_CHECK_EQUAL(a, 1);
}

BOOST_AUTO_TEST_CASE(pipe_test) {
    char a = 1;
    pipe_fd pipe;
    pipe.write(&a, 1);
    BOOST_CHECK_EQUAL(pipe.read(&a, 1), 1);
    BOOST_CHECK_EQUAL(a, 1);
}


BOOST_AUTO_TEST_CASE(timer_fd_test) {
    timer_fd t;
    itimerspec ts;
    itimerspec prev;
    memset(&ts, 0, sizeof(ts));
    ts.it_value.tv_sec = 1;
    ts.it_value.tv_nsec = 50;
    t.settime(0, ts, prev);
}

BOOST_AUTO_TEST_CASE(socket_listen_test) {
    socket_fd s(AF_INET, SOCK_STREAM);
    s.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
    address addr("0.0.0.0", 0);
    s.bind(addr);
    s.listen();
}

BOOST_AUTO_TEST_CASE(udp_socket_test) {
    socket_fd us(AF_INET, SOCK_DGRAM);
    us.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
    address uaddr("0.0.0.0", 0);
    us.bind(uaddr);
}

BOOST_AUTO_TEST_CASE(signal_fd_test) {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    signal_fd sig(sigset);
}
