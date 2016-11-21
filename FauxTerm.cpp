static void *bridge(void *state);

#include <cassert>
#include <cstdio>
#include <errno.h>
#include "FauxTerm.h"
#include <pthread.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <system_error>
#include <unistd.h>
#include <vector>

using namespace std;

namespace { class AutoLock {

public:
    AutoLock(pthread_mutex_t *mutex) : m_mutex(mutex) {
        if (pthread_mutex_lock(mutex) < 0)
            throw system_error(errno, system_category());
    }

    ~AutoLock() {
        pthread_mutex_unlock(m_mutex);
    }

private:
    pthread_mutex_t *m_mutex;

}; };

static void *bridge(void *state) {
    FauxTerm *parent = (FauxTerm*)state;

    const int nfds = (parent->m_pipe_fd[0] > parent->m_sig_fd ?
        parent->m_pipe_fd[0] : parent->m_sig_fd) + 1;

    for (;;) {
        fd_set fds;
        FD_ZERO(&fds);

        FD_SET(parent->m_pipe_fd[0], &fds);
        FD_SET(parent->m_sig_fd, &fds);
        
        if (select(nfds, &fds, nullptr, nullptr, nullptr) < 0)
            break;

        if (FD_ISSET(parent->m_pipe_fd[0], &fds)) {
            // TODO: transfer to m_screen
        }

        if (FD_ISSET(parent->m_sig_fd, &fds))
            break;
    }

    return nullptr;
}

FauxTerm::FauxTerm() {

    /* Figure out the size of the terminal. We assume it won't be resized during
     * this object's lifetime.
     */
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0)
        throw system_error(errno, system_category());

    m_width = unsigned(ws.ws_col);
    m_height = unsigned(ws.ws_row);

    m_screen = new TermChar[m_width * m_height]();

    if (pipe(m_pipe_fd) < 0)
        throw system_error(errno, system_category());

    if ((m_sig_fd = eventfd(0, 0)) < 0)
        throw system_error(errno, system_category());

    if (pthread_mutex_init(&m_screen_lock, 0) < 0)
        throw system_error(errno, system_category());

    if (pthread_create(&m_child, nullptr, bridge, this) < 0)
        throw system_error(errno, system_category());

}

FauxTerm::~FauxTerm() {
    // Signal the child that we're exiting.
    uint64_t indicator = 1;
    int ret __attribute__((unused)) = write(m_sig_fd, &indicator,
        sizeof(indicator));
    assert(ret == sizeof(indicator));
    pthread_join(m_child, nullptr);

    pthread_mutex_destroy(&m_screen_lock);

    close(m_sig_fd);
    close(m_pipe_fd[0]);
    close(m_pipe_fd[1]);

    delete[] m_screen;
}

static TermChar blank_char() {
    return TermChar { 0, TC_BLACK, TC_BLACK, 0, 0 };
}

TermChar FauxTerm::get_char(unsigned x, unsigned y) {
    if (x >= m_width)
        return blank_char();
    if (y >= m_height)
        return blank_char();
    // FIXME: taking this lock for every character is going to perform terribly
    AutoLock lock(&m_screen_lock);
    return m_screen[y * m_width + x];
}

vector<TermChar> FauxTerm::get_line(unsigned y) {
    vector<TermChar> line;
    for (unsigned i = 0; i < m_width; i++)
        line.push_back(get_char(i, y));
    return line;
}
