#include "disk.h"
#include <cassert>

Disk::Disk(int disk_id, int disk_capacity, int max_tokens) 
    : id(disk_id)
    , capacity(disk_capacity)
    , head_position(1)
    , storage(disk_capacity + 1, 0)
    , max_tokens_(max_tokens) {
        token_manager = new TokenManager(max_tokens);
    }

bool Disk::write(int position, int object_id) {
    assert(position > 0 && position <= capacity);
    storage[position] = object_id;
    return true;
}

void Disk::erase(int position) {
    assert(position > 0 && position <= capacity);
    storage[position] = 0;
}

int Disk::get_head_position() const {
    return head_position;
}

bool Disk::is_free(int position) const {
    assert(position > 0 && position <= capacity);
    return storage[position] == 0;
}

int Disk::get_id() const {
    return id;
}

int Disk::get_capacity() const {
    return capacity;
}

int Disk::get_distance_to_head(int position) const {
    assert(position > 0 && position <= capacity);
    if(position < head_position)
        return capacity - head_position + position;
    else return position - head_position;
}

int Disk::get_need_token_to_head(int position) const {
    assert(position > 0 && position <= capacity);
    int distance = get_distance_to_head(position);
    return 0; ////////
}
int Disk::refresh_token_manager(){
    token_manager->refresh();
    return 0;
}

int Disk::jump(int position){
    if(token_manager->consume_jump()){
        head_position = position;
        return head_position;
    }
    else return 0;
}
int Disk::pass(){
    if(token_manager->consume_pass()){
        head_position += 1;
        if(head_position > capacity)
            head_position = 1;
        return head_position;
    }
    else return 0;
}
int Disk::read(){
    if(token_manager->consume_read()){
        head_position += 1;
        if(head_position > capacity)
            head_position = 1;
        return head_position;
    }
    else return 0;
}