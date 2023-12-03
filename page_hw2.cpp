#include "page.hpp"
#include <iostream> 
#include <cstring> 

void put2byte(void *dest, uint16_t data){
	*(uint16_t*)dest = data;
}

uint16_t get2byte(void *dest){
	return *(uint16_t*)dest;
}

page::page(uint16_t type){
	hdr.set_num_data(0);
	hdr.set_data_region_off(PAGE_SIZE-1-sizeof(page*));
	hdr.set_offset_array((void*)((uint64_t)this+sizeof(slot_header)));
	hdr.set_page_type(type);
}

uint16_t page::get_type(){
	return hdr.get_page_type();
}

uint16_t page::get_record_size(void *record){
	uint16_t size = *(uint16_t *)record;
	return size;
}

char *page::get_key(void *record){
	char *key = (char *)((uint64_t)record+sizeof(uint16_t));
	return key;
}

// uint64_t page::get_val(void *key){
// 	uint64_t val= *(uint64_t*)((uint64_t)key+(uint64_t)strlen((char*)key)+1);
// 	return val;
// }

uint64_t page::get_val(void *key) {
    uint64_t val = *(uint64_t *)((uint64_t)key + sizeof(uint16_t) + strlen(get_key(key)) + 1);
    return val;
}

void page::set_leftmost_ptr(page *p){
	leftmost_ptr = p;
}

page *page::get_leftmost_ptr(){
	return leftmost_ptr;	
}

uint64_t page::find(char *key) {
    int num_data = hdr.get_num_data();
    void *offset_array = hdr.get_offset_array();
    void *data_region = nullptr;
    char *stored_key = nullptr;

    for (int i = 0; i < num_data; i++) {
        uint16_t off = *(uint16_t *)((uint64_t)offset_array + i * 2);
        data_region = (void *)((uint64_t)this + (uint64_t)off);

        // Check if stored_key is null
        stored_key = get_key(data_region);
        if (stored_key == nullptr) {
            printf("Stored key is null. Something went wrong.\n");
            return 0;
        }

        if (strcmp(key, stored_key) == 0) {
            return get_val(data_region);
        }
    }

    // Key not found
    return 0;
}


bool page::insert(char *key, uint64_t val) {
    int num_data = hdr.get_num_data();
    void *offset_array = hdr.get_offset_array();
    void *data_region = nullptr;
    char *stored_key = nullptr;

    // Find the correct position to insert while maintaining sorted order
    int insert_position = 0;
    for (int i = 0; i < num_data; i++) {
        uint16_t off = *(uint16_t *)((uint64_t)offset_array + i * 2);
        data_region = (void *)((uint64_t)this + (uint64_t)off);
        stored_key = get_key(data_region);

       // Compare keys to find the right position
        if (strcmp(key, stored_key) < 0) {
            insert_position = i;
            break;
        } else {
            insert_position = i + 1;
        }
    }

    // Check if the page is full
    if (is_full(sizeof(uint16_t) + strlen(key) + 1 + sizeof(uint64_t))) {
        // Page is full, insertion fails
        printf("Page is full. Insertion failed for key=%s, val=%llu\n\n", key, val);
        return false;
    }

    // Insert the new record
    uint16_t record_size = sizeof(uint16_t) + strlen(key) + 1 + sizeof(uint64_t);
    uint16_t new_offset = hdr.get_data_region_off() - record_size;

    // Move existing data to make space for the new record
    for (int i = num_data - 1; i >= insert_position; i--) {
        uint16_t off = *(uint16_t *)((uint64_t)offset_array + i * 2);
        *(uint16_t *)((uint64_t)offset_array + (i + 1) * 2) = off;
    }

    // Insert the new offset at the calculated position
    *(uint16_t *)((uint64_t)offset_array + insert_position * 2) = new_offset;

    // Insert the new offset
    *(uint16_t *)((uint64_t)offset_array + insert_position * 2) = new_offset;

    // Insert the new record data
    data_region = (void *)((uint64_t)this + (uint64_t)new_offset);
    uint16_t actual_record_size = get_record_size(data_region);

    put2byte(data_region, actual_record_size);
    memcpy((void *)((uint64_t)data_region + sizeof(uint16_t)), key, strlen(key) + 1);
    *(uint64_t *)((uint64_t)data_region + sizeof(uint16_t) + strlen(key) + 1) = val;

    // Update page header
    hdr.set_num_data(num_data + 1);
    hdr.set_data_region_off(new_offset);

    return true;  // Insertion successful
}

bool page::is_full(uint64_t inserted_record_size) {
    int num_data = hdr.get_num_data();
    uint16_t data_region_off = hdr.get_data_region_off();
    void *offset_array = hdr.get_offset_array();

    // Calculate the used space in the data region
    int used_space = sizeof(slot_header) + num_data * sizeof(uint16_t);

    // Calculate the available space in the data region
    int available_space = static_cast<int>(data_region_off) - used_space;

    // Ensure that available space is at least 0
    available_space = std::max(available_space, 0);

    // Check if there is enough space for the new record
    bool isFull = available_space < static_cast<int>(inserted_record_size);

    // Return true if full, false if not full
    if (isFull) {
        return true;
    } else {
        return false;
    }
}



void page::defrag(){
	page *new_page = new page(get_type());
	int num_data = hdr.get_num_data();
	void *offset_array=hdr.get_offset_array();
	void *stored_key=nullptr;
	uint16_t off=0;
	uint64_t stored_val=0;
	void *data_region=nullptr;

	for(int i=0; i<num_data/2; i++){
		off= *(uint16_t *)((uint64_t)offset_array+i*2);	
		data_region = (void *)((uint64_t)this+(uint64_t)off);
		stored_key = get_key(data_region);
		stored_val= get_val((void *)stored_key);
		new_page->insert((char*)stored_key,stored_val);
	}	
	new_page->set_leftmost_ptr(get_leftmost_ptr());

	memcpy(this, new_page, sizeof(page));
	hdr.set_offset_array((void*)((uint64_t)this+sizeof(slot_header)));
	delete new_page;

}

page* page::split(char *key, uint64_t val, char** parent_key){
	// Please implement this function in project 2.
	page *new_page;
	return new_page;
}


void page::print(){
	uint32_t num_data = hdr.get_num_data();
	uint16_t off=0;
	uint16_t record_size= 0;
	void *offset_array=hdr.get_offset_array();
	void *stored_key=nullptr;
	uint64_t stored_val=0;

	printf("## slot header\n");
	printf("Number of data :%d\n",num_data);
	printf("offset_array : |");
	for(int i=0; i<num_data; i++){
		off= *(uint16_t *)((uint64_t)offset_array+i*2);	
		printf(" %d |",off);
	}
	printf("\n");

	void *data_region=nullptr;
	for(int i=0; i<num_data; i++){
		off= *(uint16_t *)((uint64_t)offset_array+i*2);	
		data_region = (void *)((uint64_t)this+(uint64_t)off);
        uint16_t key_length = strlen(get_key(data_region)) + 1;  // 키 길이 + 널 문자(\0)
        record_size = sizeof(uint16_t) + key_length + sizeof(uint64_t);
		//record_size = get_record_size(data_region);
		stored_key = get_key(data_region);
		stored_val= get_val((void *)stored_key);
		printf("==========================================================\n");
		printf("| data_sz:%u | key: %s | val :%llu |\n",record_size,(char*)stored_key, stored_val);

	}
}




