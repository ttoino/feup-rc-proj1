#include "link_layer/timer.h"
#include "log.h"
#include <sys/signal.h>
#include <sys/time.h>
#include <unistd.h>

void timer_handler(union sigval val) {
    LLConnection *connection = val.sival_ptr;

    if (connection->n_retransmissions_sent ==
        connection->params.n_retransmissions) {
        ERROR("Max retries achieved, endpoints are probably disconnected, "
              "closing connection!\n");
        timer_disarm(connection);
        kill(getpid(), SIGCONT);
        return;
    }

    ALARM("Acknowledgement not received, retrying (c = %02x)\n",
          connection->last_command_frame->command);

    write_frame(connection, connection->last_command_frame);
    connection->n_retransmissions_sent++;
}

void a() { printf("SIGCONT\n"); }

int timer_setup(LLConnection *connection) {
    struct sigevent event = {.sigev_notify = SIGEV_THREAD,
                             .sigev_value.sival_ptr = connection,
                             .sigev_notify_function = timer_handler};

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = a;
    sa.sa_flags = 0;
    sigaction(SIGCONT, &sa, NULL);

    return timer_create(CLOCK_REALTIME, &event, &connection->timer);
}

int timer_destroy(LLConnection *connection) {
    return timer_delete(connection->timer);
}

int timer_arm(LLConnection *connection) {
    struct itimerspec ts = {
        .it_value = {.tv_sec = connection->params.timeout, .tv_nsec = 0},
        .it_interval = {.tv_sec = connection->params.timeout, .tv_nsec = 0}};

    return timer_settime(connection->timer, 0, &ts, 0);
}

int timer_disarm(LLConnection *connection) {
    struct itimerspec ts = {.it_value = {.tv_sec = 0, .tv_nsec = 0},
                            .it_interval = {.tv_sec = 0, .tv_nsec = 0}};

    return timer_settime(connection->timer, 0, &ts, 0);
}

int timer_force(LLConnection *connection) {
    union sigval val;
    val.sival_ptr = connection;
    timer_handler(val);

    return timer_arm(connection);
}
