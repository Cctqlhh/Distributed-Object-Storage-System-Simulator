#include "request.h"

Request::Request(int req_id, int obj_id)
    : request_id(req_id)
    , object_id(obj_id)
    , prev_request_id(0)
    , is_done(false) {
}

void Request::link_to_previous(int prev_id) {
    prev_request_id = prev_id;
}

bool Request::is_completed() const {
    return is_done;
}

void Request::mark_as_completed() {
    is_done = true;
}

int Request::get_object_id() const {
    return object_id;
}

int Request::get_prev_id() const {
    return prev_request_id;
}

void Request::set_object_id(int id) {
    object_id = id;
}