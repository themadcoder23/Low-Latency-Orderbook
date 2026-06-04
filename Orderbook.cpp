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
uint32_t Sell_Head[MAX_PRICE_TICKS];
uint32_t Sell_Tail[MAX_PRICE_TICKS];

uint32_t curr_best_ask = NULL_INDEX;
uint32_t curr_best_bid = 0;

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
    pool[alloc_index] = order1;
    head_free = pool[alloc_index].next;
    return alloc_index;
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
        Buy_Tail[price_tick] = new_tail;
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

void match_buy_order(Order* incoming_order, uint32_t incoming_price) {
    uint32_t incoming_qty = incoming_order->quantity;
    while(incoming_qty > 0 && curr_best_ask < MAX_PRICE_TICKS && curr_best_ask <= incoming_price && curr_best_ask != NULL_INDEX) {

        while(incoming_qty > 0 && Sell_Head[curr_best_ask] != NULL_INDEX) {
            uint32_t resting_idx = Sell_Head[curr_best_ask];
            uint32_t resting_qty = pool[resting_idx].quantity;
            
            if(incoming_qty >= resting_qty) {   // STATE 1: Resting Order is Destroyed
                incoming_qty -= resting_qty;
                uint32_t next_idx = pool[resting_idx].next;
                Sell_Head[curr_best_ask] = next_idx;

                if(next_idx != NULL_INDEX) {
                    pool[next_idx].prev = NULL_INDEX;
                } else {
                    Sell_Tail[curr_best_ask] = NULL_INDEX; // Queue empty
                }
                free_order(resting_idx); // Prevent memory leak
            }
            else {  // STATE 2: Resting Order Survives
                pool[resting_idx].quantity -= incoming_qty;
                incoming_qty = 0;
            }
        }
        
        // Find next active price level
        if(Sell_Head[curr_best_ask] == NULL_INDEX) {
            curr_best_ask++;
            while(curr_best_ask < MAX_PRICE_TICKS && curr_best_ask <= incoming_price && Sell_Head[curr_best_ask] == NULL_INDEX) {
                curr_best_ask++;
            }
            if(curr_best_ask > incoming_price || curr_best_ask >= MAX_PRICE_TICKS) break;
        }
    }

    // Unfilled quantity becomes a resting Maker order
    if(incoming_qty > 0) {
        incoming_order->quantity = incoming_qty;
        uint32_t new_idx = allocate_order(*incoming_order);
        add_buy_order(incoming_price, new_idx);
        
        if(incoming_price > curr_best_bid) {
            curr_best_bid = incoming_price;
        }
    }
}