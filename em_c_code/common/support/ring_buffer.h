/*
 * ring_buffer.h
 *
 *  Created on: Dec 25, 2017
 *      Author: adam jones
 */
// since ring_buffer is probably used between isr and not, everything is defined as volatile

#ifndef ring_buffer_defined
#define ring_buffer_defined

#include <stdint.h>
#include <stdbool.h>

class ring_buffer {
    public:
        //init
        ring_buffer();
        bool init_alloc(uint16_t size) volatile;

        //destruct
        ~ring_buffer();
        void dealloc_buffer() volatile;

        //empty in-place
        void reset() volatile;

        //read
        bool read(uint16_t &read_value) volatile;
        bool read(const uint16_t count, uint16_t *const array) volatile;

        //write
        bool write(const uint16_t write_value) volatile;
        bool write(const uint16_t count, const uint16_t *const array) volatile;

        //status
        bool is_full() volatile const {return in_use_ == size_;}
        bool is_empty() volatile const {return in_use_ == 0;}
        uint16_t in_use() volatile const {return in_use_;}
        uint16_t available() volatile const {return size_ - in_use_;}
        uint16_t write_index() volatile const {return write_index_;}  //used by delayprintf
        uint16_t size() volatile const {return size_;}

    private:
        bool initialized_;
        uint16_t size_;         // buffer size
        uint16_t in_use_;       // number in use
        uint16_t read_index_;   // read index
        uint16_t write_index_;  // write index
        uint16_t *buffer_;      // the buffer
};

#endif
