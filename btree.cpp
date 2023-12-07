#include "btree.hpp"
#include <iostream> 

btree::btree(){
	root = new page(LEAF);
	height = 1;
};

void btree::insert(char *key, uint64_t val){
    page *cursor = root;
    page *parent = nullptr;
    page *newPage = nullptr;
    char *median = nullptr;
    uint64_t newKey = 0;
    
    while (cursor != nullptr) {
        parent = cursor;
        if (cursor->get_type() == LEAF) {
            if (cursor->is_full(strlen(key) + 1 + sizeof(uint64_t))) {
                // Leaf node is full, perform split operation
                newPage = cursor->split(key, val, &median);
                if (cursor == root) {
                    // Create a new root
                    root = new page(INTERNAL);
                    root->set_leftmost_ptr(cursor);
                    cursor->set_leftmost_ptr(newPage);
                    root->insert(median, newPage->get_val(newPage->get_key(0)));
                } else {
                    parent->insert(median, newPage->get_val(newPage->get_key(0)));
                    parent->set_leftmost_ptr(cursor);
                    parent->defrag();
                }
            } else {
                // Leaf node has space, insert data
                cursor->insert(key, val);
            }

            // Check if the data was inserted correctly
            uint64_t lookupVal = lookup(key);
            if (lookupVal == val) {
                std::cout << "Insert successful: Key = " << key << ", Value = " << val << std::endl;
            } else {
                std::cout << "Insert failed: Key = " << key << ", Expected Value = " << val
                        << ", Actual Value = " << lookupVal << std::endl;
            }
            
            break;
        } else {
            // Traverse down to leaf level
            cursor = cursor->get_leftmost_ptr();
        }
    }
}




uint64_t btree::lookup(char *key){
    page *cursor = root;
    
    while (cursor != nullptr) {
        if (cursor->get_type() == LEAF) {
            uint64_t val = cursor->find(key);
            return val;
        } else {
            // Traverse down to leaf level
            cursor = cursor->get_leftmost_ptr();
        }
    }
    
    return 0; // Not found
}

