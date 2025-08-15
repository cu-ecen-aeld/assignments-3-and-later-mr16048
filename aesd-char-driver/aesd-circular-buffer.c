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
#include <linux/slab.h>
#include "aesd-circular-buffer.h"
#include "aesdchar.h"

struct aesd_buffer_entry tmp_entry;

static void aesd_init_entry(struct aesd_buffer_entry *);

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

int aesd_circular_buffer_find_entry_offset_and_index_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
	int i;
	int total_len;

	total_len = 0;
	PDEBUG("aesd_circular_buffer_find_index(): out_ofs = %d, in_ofs = %d, char_offset = %d", buffer->out_offs, buffer->in_offs, char_offset);
	
	i = buffer->in_offs;
	while(1){
		if(total_len + buffer->entry[i].size > char_offset){
			*entry_offset_byte_rtn = char_offset - total_len;
			PDEBUG("aesd_circular_buffer_find_index(): find %d", i);
			return i;
		}

		total_len += buffer->entry[i].size;
		i++;
		if(i >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
			i = 0;
		}
	}	

	return -1;
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

	char *current_str;
	size_t new_size;
	struct aesd_buffer_entry *target;

	PDEBUG("aesd_circular_buffer_add_entry(): 1");
	
	current_str = tmp_entry.buffptr;	
	PDEBUG("aesd_circular_buffer_add_entry current buffer: %s", current_str);
	new_size = tmp_entry.size + add_entry->size;
	tmp_entry.buffptr = kmalloc(new_size, GFP_KERNEL);
	memset(tmp_entry.buffptr, 0, new_size);
	if(current_str != NULL){
		memcpy(tmp_entry.buffptr, current_str, tmp_entry.size);
	}
	memcpy(tmp_entry.buffptr + tmp_entry.size, add_entry->buffptr, add_entry->size);
	tmp_entry.size = new_size;
	if(current_str != NULL){
		kfree(current_str);
	}

	PDEBUG("aesd_circular_buffer_add_entry adding buffer: %s", add_entry->buffptr);
	PDEBUG("aesd_circular_buffer_add_entry result buffer: %s", tmp_entry.buffptr);

	// buffer->entry[buffer->in_offs] = *add_entry;
	char last_char = add_entry->buffptr[add_entry->size - 1];
	if(last_char == '\n'){

		target = &buffer->entry[buffer->in_offs];
		if(target->buffptr != NULL){
			kfree(target->buffptr);
		}
		*target = tmp_entry;
		PDEBUG("aesd_circular_buffer_add_entry add buffered to entry");
		aesd_init_entry(&tmp_entry);
		
		buffer->in_offs += 1;	
		// buffer->out_offs += 1;
		if(buffer->in_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
			buffer->in_offs = 0;
		}
		if(buffer->in_offs == buffer->out_offs){
			buffer->full = true;
		}
	}

	PDEBUG("aesd_circular_buffer_add_entry() in_ofs: %d", buffer->in_offs);
	PDEBUG("aesd_circular_buffer_add_entry() out_ofs: %d", buffer->out_offs);
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
int aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));

	aesd_init_entry(&tmp_entry);

	return 0;
}

size_t aesd_circular_buffer_raed(struct aesd_circular_buffer *buffer, char *result_buf, size_t len, loff_t *f_pos){

	size_t read_len = 0;
	size_t next_size;
	size_t start_byte_ofs;
	struct aesd_buffer_entry next_entry;
	int startp;
	int copy_start;
	size_t copy_len;

	PDEBUG("aesd_circular_buffer_raed(): 0");

	if(!buffer->full && (buffer->out_offs == buffer->in_offs)){
		PDEBUG("aesd_circular_buffer_raed(): 1 no data to read");
		return 0;
	}

	startp = aesd_circular_buffer_find_entry_offset_and_index_for_fpos(buffer, *f_pos, &start_byte_ofs);
	if(startp < 0){
		return 0;
	}
	buffer->out_offs = startp;
	PDEBUG("aesd_circular_buffer_raed() out_ofs: %d", buffer->out_offs);
	copy_start = strlen(result_buf);

	while(1){
		next_entry = buffer->entry[buffer->out_offs]; 
		next_size = next_entry.size;

		PDEBUG("aesd_circular_buffer_raed(): 1");
		if(next_entry.buffptr == NULL){
			PDEBUG("aesd_circular_buffer_raed(): 2.1");
			break;
		}
		
		if(read_len + next_size >= len){
			copy_len = len - read_len;
		}
		else{
			copy_len = next_size;
		}

		memcpy(result_buf + copy_start + read_len, next_entry.buffptr, copy_len);

		PDEBUG("aesd_circular_buffer_raed(): 2.5");
		read_len += copy_len;

		buffer->out_offs += 1;
		if(buffer->out_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
			buffer->out_offs = 0;        
		}
		PDEBUG("aesd_circular_buffer_raed() adding buffer: %s", next_entry.buffptr);
		PDEBUG("aesd_circular_buffer_raed() res_buffer: %s", result_buf);
		// if(buffer->out_offs == startp){
		// 	buffer->full = false;
		// 	PDEBUG("aesd_circular_buffer_raed(): read all data, break");
		// 	break;
		// }
		if(buffer->full){
			buffer->full = false;
		}
		if(read_len >= len){
			PDEBUG("aesd_circular_buffer_raed(): read specified length of data, break");
			break;	
		}
		if(buffer->out_offs == buffer->in_offs){
			// PDEBUG("aesd_circular_buffer_raed(): break outp: %d, inp: %d", buffer->out_offs, buffer->in_offs);
			PDEBUG("aesd_circular_buffer_raed(): read all data, break");
			break;
		}

		PDEBUG("aesd_circular_buffer_raed(): 3");
	}

	*f_pos = buffer->out_offs;

	PDEBUG("aesd_circular_buffer_raed(): read %dbytes", read_len);
	PDEBUG("aesd_circular_buffer_raed() in_ofs: %d", buffer->in_offs);
	PDEBUG("aesd_circular_buffer_raed() out_ofs: %d", buffer->out_offs);

	return read_len;
}

void aesd_circular_buffer_free(struct aesd_circular_buffer *buffer){

	int i;

	for(i = 0; i< AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++){
		if((buffer->entry[i]).buffptr != NULL){
			kfree((buffer->entry[i]).buffptr);
		}
	}
	buffer->in_offs = 0;
	buffer->out_offs = 0;
}

static void aesd_init_entry(struct aesd_buffer_entry *entry){

	if(entry->buffptr != NULL){
		kfree(entry->buffptr);
	}
	entry->size = 0;
}