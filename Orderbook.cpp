#include<cstdint>
#include<limits>
const uint32_t NULL_INDEX = UINT32_MAX;
const uint32_t MAX_PRICE_TICKS = 1000000;
struct Order
{
    uint64_t time_stamp;
    uint64_t Order_id;
    uint32_t quantity;
    uint32_t next;
    uint32_t prev;
    uint8_t side;/* 1 for sell, 0 for buy */
    uint8_t _pad[3]; 
};
Order pool[MAX_PRICE_TICKS];
uint32_t Buy_Head[MAX_PRICE_TICKS];
uint32_t Buy_Tail[MAX_PRICE_TICKS];
uint32_t head_free = 0;

void order_init(){
    for(int i = 0; i < 99999; i ++){
        pool[i].next = i+1;
    }
    pool[99999].next = NULL_INDEX;
}

uint32_t allocate_order(Order order1){
    if(head_free == NULL_INDEX) return NULL_INDEX;
    uint32_t alloc_index = head_free;
    head_free = pool[alloc_index].next;
    return head_free;
}

void free_order(uint32_t index){
    pool[index].next = head_free;
    head_free = index;
}

void add_buy_order(uint32_t price_tick,uint32_t new_index){
    pool[new_index].next = NULL_INDEX;
    if(Buy_Head[price_tick] == NULL_INDEX){
        pool[new_index].prev = NULL_INDEX;
        Buy_Head[price_tick] = new_index;
        Buy_Tail[price_tick] = new_index;
    }
    else{
        uint32_t old_tail = Buy_Tail[price_tick];
        pool[old_tail].next = new_index;
        pool[new_index].prev = old_tail;
        Buy_Tail[price_tick] = new_index;
    }   
}

void cancel_buy_order(uint32_t price_tick,uint32_t target_index){
    if(pool[target_index].prev == NULL_INDEX){      //if the order is head 
        Buy_Head[price_tick] = pool[target_index].next;
        if(pool[target_index].next != NULL_INDEX){
            uint32_t new_index = pool[target_index].next;
            pool[new_index].prev = NULL_INDEX;
        }
        else{
            Buy_Tail[price_tick] = NULL_INDEX;
        }
    }
    else if(pool[target_index].next == NULL_INDEX){    //if the order is tail
        uint32_t new_tail = pool[target_index].prev;
        Buy_Tail[target_index] = new_tail;
        pool[new_tail].next = NULL_INDEX;
    }
    else{                                               // if order is in between
        uint32_t prev_index = pool[target_index].prev;
        uint32_t next_index = pool[target_index].next;
        pool[prev_index].next = next_index;
        pool[next_index].prev = prev_index;
    }
    free_order(target_index);
}