#include "link_layer/timer.h"
#include "log.h"
#include <sys/signal.h>
#include <sys/time.h>

void timer_handler(union sigval val) {
    LLConnection *connection = val.sival_ptr;

    if (connection->n_retransmissions_sent ==
        connection->params.n_retransmissions) {
        ERROR("Max retries achieved, endpoints are probably disconnected, "
              "closing connection!\n");
        // TODO
        // llclose(FALSE);
        exit(-1);
    }

    ALARM("Acknowledgement not received, retrying (c = %02x)\n",
          connection->last_command_frame->command);

    write_frame(connection, connection->last_command_frame);
    connection->n_retransmissions_sent++;
}

void timer_setup(LLConnection *connection) {
    struct sigevent event = {.sigev_notify = SIGEV_THREAD,
                             .sigev_value.sival_ptr = connection,
                             .sigev_notify_function = timer_handler};

    timer_create(CLOCK_REALTIME, &event, &connection->timer);
}

void timer_destroy(LLConnection *connection) {
    timer_delete(connection->timer);
}

void timer_arm(LLConnection *connection) {
    struct itimerspec ts = {
        .it_value = {.tv_sec = connection->params.timeout, .tv_nsec = 0},
        .it_interval = {.tv_sec = connection->params.timeout, .tv_nsec = 0}};

    timer_settime(connection->timer, 0, &ts, 0);
}

void timer_disarm(LLConnection *connection) {
    struct itimerspec ts = {.it_value = {.tv_sec = 0, .tv_nsec = 0},
                            .it_interval = {.tv_sec = 0, .tv_nsec = 0}};

    timer_settime(connection->timer, 0, &ts, 0);
}

void timer_force(LLConnection *connection) {
    union sigval val;
    val.sival_ptr = connection;
    timer_handler(val);

    timer_arm(connection);
}
