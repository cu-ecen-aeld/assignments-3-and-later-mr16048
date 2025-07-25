/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif
#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
	int i;
	int total_len;

	total_len = 0;
	if(buffer->in_offs <= buffer->out_offs){
		for (i = buffer->out_offs; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++){
			if(total_len + buffer->entry[i].size > char_offset){
				*entry_offset_byte_rtn = char_offset - total_len;
				return &(buffer->entry[i]);
			}
			total_len += buffer->entry[i].size;
		}
		for (i = 0; i < buffer->in_offs; i++){
			if(total_len + buffer->entry[i].size > char_offset){
				*entry_offset_byte_rtn = char_offset - total_len;
				return &(buffer->entry[i]);
			}
			total_len += buffer->entry[i].size;
		}
	}
	else{
		for (i = buffer->out_offs; i < buffer->in_offs; i++){
			if(total_len + buffer->entry[i].size > char_offset){
				*entry_offset_byte_rtn = char_offset - total_len;
				return &(buffer->entry[i]);
			}
			total_len += buffer->entry[i].size;
		}
	}

	return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */  
    buffer->entry[buffer->in_offs] = *add_entry;
    buffer->in_offs += 1;
	buffer->out_offs += 1;
	if(buffer->in_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
		buffer->in_offs = 0;
		if(!buffer->full){
			buffer->full = true;
		}
  }
	if(buffer->out_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
		buffer->out_offs = 0;        
	}
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}

size_t aesd_circular_buffer_raed(struct aesd_circular_buffer *buffer, char *result_buf, size_t len){

	size_t read_len = 0;
	size_t next_size;
	struct aesd_buffer_entry next_entry;

	while(1){
		next_entry = buffer->entry[buffer->out_offs]; 
		next_size = next_entry.size;
		if(read_len + next_size >= len){
			break;
		}
		strcat(result_buf, next_entry.buffptr);
		read_len += next_size;
		buffer->out_offs += 1;
		if(buffer->out_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
			buffer->out_offs = 0;        
		}
		if(buffer->out_offs >= buffer->in_offs){
			break;
		}
	}

	return read_len;
}
