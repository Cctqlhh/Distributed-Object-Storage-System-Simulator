#pragma once

class Request {
private:
    int request_id;
    int object_id;
    int prev_request_id;
    bool is_done;

public:
    Request(int req_id = 0, int obj_id = 0);
    void link_to_previous(int prev_id);
    bool is_completed() const;
    void mark_as_completed();
    int get_object_id() const;
    int get_prev_id() const;
    void set_object_id(int id);
};