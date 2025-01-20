extern "C" {
#include <fs/pipe.h>
}

#include "assert.hpp"
#include "pause.hpp"
#include "runner.hpp"
#include <chrono>
#include <condition_variable>
#include <random>
#include <thread>
#include <iostream>

void test_init(void) {
    pipe_init();
}

namespace easy {
#define CNT_MAX 65536

static Pipe *pip;
static void writer() {
    int cnt = 0;
    while (1) {
        isize ret = pipe_write(pip, (char *)&cnt, sizeof(cnt));
        assert_eq(ret, sizeof(cnt));
        if (cnt == CNT_MAX) {
            break;
        }
        cnt++;
    }

    pipe_close_write(pip);
}

static void reader() {
    int cnt = 0;

    while (1) {
        int recv;
        isize ret = pipe_read(pip, (char *)&recv, sizeof(recv));
        if (ret < 0) {
            break;
        }
        assert_eq(ret, sizeof(recv));
        assert_eq(recv, cnt++);
    }

    pipe_close_read(pip);
    assert_eq(cnt, CNT_MAX + 1);
}

void test_rw() {
    pipe_init();
    pip = pipe_open();
    assert_true(pip != (void *)0);

    // single thread test
    for (int i = 0; i < 5; i++) {
        int recv;
        assert_eq(pipe_write(pip, (char *)&i, sizeof(i)), sizeof(i));
        assert_eq(pipe_read(pip, (char *)(&recv), sizeof(i)), sizeof(i));
        assert_eq(recv, i);
    }

    // create two workers, one reader and one writer.
    std::vector<std::thread> workers;

    workers.emplace_back([&] { reader(); });
    workers.emplace_back([&] { writer(); });

    for (auto &worker : workers) {
        worker.join();
    }
}

}; // namespace easy

namespace adhoc {
#define ANS_MIN 0
#define ANS_MAX 65535

static const int TARGET = 54321;

// writer is guesser
static Pipe *gpip;

// writer is judge
static Pipe *jpip;

// The guesser try to guess the answer by
// linear search.
// Each time it generates an answer,
// it send to the pipe.
static void guesser() {
    assert_eq(gpip->nreader, 1);
    assert_eq(gpip->nwriter, 1);
    int sign;
    int guess = 0;

    for (int ans = 0; ans < ANS_MAX; ans++) {
        // notify main thread.
        // write to guess pipe.
        isize ret = pipe_write(gpip, (char *)&ans, sizeof(ans));
        assert_eq(ret, sizeof(ans));
        // read from judge pipe
        ret = pipe_read(jpip, (char *)&sign, sizeof(sign));
        assert_eq(ret, sizeof(sign));

        if (sign == 0) {
            guess = ans;
            break;
        }
    }

    // close pipe.
    pipe_close_write(gpip);
    pipe_close_read(jpip);
    // verify
    assert_eq(guess, TARGET);
}

static void judge(void) {
    assert_eq(jpip->nreader, 1);
    assert_eq(jpip->nwriter, 1);
    int sign;
    int guess;

    while (1) {
        // read the answer
        isize ret = pipe_read(gpip, (char *)&guess, sizeof(guess));
        if (ret < 0) {
            // closed by guesser.
            break;
        }
        assert_eq (ret, sizeof(guess));

        // judge the answer.
        if (guess < TARGET) {
            // Too small.
            sign = -1;
        } else {
            if (guess == TARGET) {
                // correct!
                sign = 0;
            } else {
                // Too large.
                sign = 1;
            }
        }

        assert_eq(pipe_write(jpip, (char *)&sign, sizeof(sign)), sizeof(sign));
    }

    // close the pipe
    pipe_close_read(gpip);
    pipe_close_write(jpip);
}

void test_guess() {
    pipe_init();
    gpip = pipe_open();
    jpip = pipe_open();

    // verify pipe
    assert_ne(gpip, (void *)0);
    assert_ne(jpip, (void *)0);

    assert_eq(gpip->nreader, 1);
    assert_eq(gpip->nwriter, 1);
    assert_eq(jpip->nreader, 1);
    assert_eq(jpip->nwriter, 1);

    // create 2 processes, 
    // one try to guess the target, 
    // the other judge the input.
    std::vector<std::thread> workers;
    workers.emplace_back([&] { guesser(); });
    workers.emplace_back([&] { judge(); });

    for (auto &worker : workers) {
        worker.join();
    }
}

}; // namespace adhoc

auto main(int argc, char **argv) -> int {
    static std::vector<Testcase> tests = {
        {"init", test_init},
        {"easy", easy::test_rw},
        {"guess", adhoc::test_guess},
    };

    Runner(tests).run();
    printf("(info) OK: %zu tests passed.\n", tests.size());
    return 0;
}
