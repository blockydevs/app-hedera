/* pb_common.c: Common support functions for pb_encode.c and pb_decode.c.
 *
 * 2014 Petteri Aimonen <jpa@kapsi.fi>
 */

#include "pb_common.h"
#include <os.h>
/* Debug logging support - only if PRINTF is available */
#ifdef PRINTF
/* Forward declaration for mcu_usb_printf - function is provided by Ledger SDK */
/* This declaration is needed when PRINTF is defined as mcu_usb_printf */
#define PB_DEBUG_PRINTF PRINTF
#else
#define PB_DEBUG_PRINTF(...) ((void)0)
#endif

static bool load_descriptor_values(pb_field_iter_t *iter)
{
    uint32_t word0;
    uint32_t data_offset;
    int_least8_t size_offset;

    PB_DEBUG_PRINTF("load_descriptor_values: index=%u, field_count=%u\n", 
                    iter->index, iter->descriptor->field_count);

    if (iter->index >= iter->descriptor->field_count)
        return false;

    word0 = PB_PROGMEM_READU32(((uint32_t *)PIC(((pb_msgdesc_t *)PIC(iter->descriptor))->field_info))[iter->field_info_index]);
    iter->type = (pb_type_t)((word0 >> 8) & 0xFF);
    
    // W pliku app-hedera/vendor/nanopb/pb_common.c, w funkcji load_descriptor_values:

    PB_DEBUG_PRINTF("load_descriptor_values: type=0x%02x\n", iter->type);
    
    PB_DEBUG_PRINTF("load_descriptor_values: word0 & 3 = %u (format type)\n", word0 & 3);

    switch(word0 & 3)
    {
        case 0: {
            PB_DEBUG_PRINTF("load_descriptor_values: case 0 (1-word format)\n");
            /* 1-word format */
            iter->array_size = 1;
            iter->tag = (pb_size_t)((word0 >> 2) & 0x3F);
            size_offset = (int_least8_t)((word0 >> 24) & 0x0F);
            data_offset = (word0 >> 16) & 0xFF;
            iter->data_size = (pb_size_t)((word0 >> 28) & 0x0F);
            PB_DEBUG_PRINTF("load_descriptor_values: case 0 done, tag=%u, data_offset=%u\n", iter->tag, data_offset);
            break;
        }

        case 1: {
            PB_DEBUG_PRINTF("load_descriptor_values: case 1 (2-word format)\n");
            /* 2-word format */
            uint32_t word1 = PB_PROGMEM_READU32(((uint32_t *)PIC(((pb_msgdesc_t *)PIC(iter->descriptor))->field_info))[iter->field_info_index + 1]);
            PB_DEBUG_PRINTF("load_descriptor_values: word1=0x%08x\n", word1);

            iter->array_size = (pb_size_t)((word0 >> 16) & 0x0FFF);
            iter->tag = (pb_size_t)(((word0 >> 2) & 0x3F) | ((word1 >> 28) << 6));
            size_offset = (int_least8_t)((word0 >> 28) & 0x0F);
            data_offset = word1 & 0xFFFF;
            iter->data_size = (pb_size_t)((word1 >> 16) & 0x0FFF);
            PB_DEBUG_PRINTF("load_descriptor_values: case 1 done\n");
            break;
        }

        case 2: {
            PB_DEBUG_PRINTF("load_descriptor_values: case 2 (4-word format)\n");
            /* 4-word format */
            uint32_t word1 = PB_PROGMEM_READU32(((uint32_t *)PIC(((pb_msgdesc_t *)PIC(iter->descriptor))->field_info))[iter->field_info_index + 1]);
            uint32_t word2 = PB_PROGMEM_READU32(((uint32_t *)PIC(((pb_msgdesc_t *)PIC(iter->descriptor))->field_info))[iter->field_info_index + 2]);
            uint32_t word3 = PB_PROGMEM_READU32(((uint32_t *)PIC(((pb_msgdesc_t *)PIC(iter->descriptor))->field_info))[iter->field_info_index + 3]);
            PB_DEBUG_PRINTF("load_descriptor_values: word1=0x%08x, word2=0x%08x, word3=0x%08x\n", word1, word2, word3);

            iter->array_size = (pb_size_t)(word0 >> 16);
            iter->tag = (pb_size_t)(((word0 >> 2) & 0x3F) | ((word1 >> 8) << 6));
            size_offset = (int_least8_t)(word1 & 0xFF);
            data_offset = word2;
            iter->data_size = (pb_size_t)word3;
            PB_DEBUG_PRINTF("load_descriptor_values: case 2 done\n");
            break;
        }

        default: {
            PB_DEBUG_PRINTF("load_descriptor_values: default case (8-word format)\n");
            /* 8-word format */
            uint32_t word1 = PB_PROGMEM_READU32(((uint32_t *)PIC(((pb_msgdesc_t *)PIC(iter->descriptor))->field_info))[iter->field_info_index + 1]);
            uint32_t word2 = PB_PROGMEM_READU32(((uint32_t *)PIC(((pb_msgdesc_t *)PIC(iter->descriptor))->field_info))[iter->field_info_index + 2]);
            uint32_t word3 = PB_PROGMEM_READU32(((uint32_t *)PIC(((pb_msgdesc_t *)PIC(iter->descriptor))->field_info))[iter->field_info_index + 3]);
            uint32_t word4 = PB_PROGMEM_READU32(((uint32_t *)PIC(((pb_msgdesc_t *)PIC(iter->descriptor))->field_info))[iter->field_info_index + 4]);
            PB_DEBUG_PRINTF("load_descriptor_values: word1=0x%08x, word2=0x%08x, word3=0x%08x, word4=0x%08x\n", word1, word2, word3, word4);

            iter->array_size = (pb_size_t)word4;
            iter->tag = (pb_size_t)(((word0 >> 2) & 0x3F) | ((word1 >> 8) << 6));
            size_offset = (int_least8_t)(word1 & 0xFF);
            data_offset = word2;
            iter->data_size = (pb_size_t)word3;
            PB_DEBUG_PRINTF("load_descriptor_values: default case done\n");
            break;
        }
    }
    
    PB_DEBUG_PRINTF("load_descriptor_values: after switch, message=%p\n", iter->message);

    if (!iter->message)
    {
        PB_DEBUG_PRINTF("load_descriptor_values: message is NULL\n");
        /* Avoid doing arithmetic on null pointers, it is undefined */
        iter->pField = NULL;
        iter->pSize = NULL;
    }
    else
    {
        PB_DEBUG_PRINTF("load_descriptor_values: setting pField, data_offset=%u\n", data_offset);
        iter->pField = (char*)iter->message + data_offset;

        if (size_offset)
        {
            iter->pSize = (char*)iter->pField - size_offset;
        }
        else if (PB_HTYPE(iter->type) == PB_HTYPE_REPEATED &&
                 (PB_ATYPE(iter->type) == PB_ATYPE_STATIC ||
                  PB_ATYPE(iter->type) == PB_ATYPE_POINTER))
        {
            /* Fixed count array */
            iter->pSize = &iter->array_size;
        }
        else
        {
            iter->pSize = NULL;
        }

        if (PB_ATYPE(iter->type) == PB_ATYPE_POINTER && iter->pField != NULL)
        {
            iter->pData = *(void**)iter->pField;
        }
        else
        {
            iter->pData = iter->pField;
        }
    }

    if (PB_LTYPE_IS_SUBMSG(iter->type))
    {
        iter->submsg_desc = PIC(((const pb_msgdesc_t * const *)PIC(((pb_msgdesc_t *)PIC(iter->descriptor))->submsg_info))[iter->submessage_index]);
    }
    else
    {
        iter->submsg_desc = NULL;
    }
    
    PB_DEBUG_PRINTF("load_descriptor_values: returning true\n");
    return true;
}

// W pliku app-hedera/vendor/nanopb/pb_common.c, w funkcji advance_iterator (około linii 156):

static void advance_iterator(pb_field_iter_t *iter)
{
    PB_DEBUG_PRINTF("advance_iterator: start, index=%u, field_count=%u\n",
                    iter->index, iter->descriptor ? iter->descriptor->field_count : 0);
    
    iter->index++;

    if (iter->index >= iter->descriptor->field_count)
    {
        PB_DEBUG_PRINTF("advance_iterator: restarting\n");
        /* Restart */
        iter->index = 0;
        iter->field_info_index = 0;
        iter->submessage_index = 0;
        iter->required_field_index = 0;
    }
    else
    {
        PB_DEBUG_PRINTF("advance_iterator: incrementing, field_info_index=%u\n", iter->field_info_index);
        /* Increment indexes based on previous field type.
         * All field info formats have the following fields:
         * - lowest 2 bits tell the amount of words in the descriptor (2^n words)
         * - bits 2..7 give the lowest bits of tag number.
         * - bits 8..15 give the field type.
         */
        uint32_t prev_descriptor = PB_PROGMEM_READU32(((uint32_t *)PIC(((pb_msgdesc_t *)PIC(iter->descriptor))->field_info))[iter->field_info_index]);
        pb_type_t prev_type = (prev_descriptor >> 8) & 0xFF;
        pb_size_t descriptor_len = (pb_size_t)(1 << (prev_descriptor & 3));

        PB_DEBUG_PRINTF("advance_iterator: prev_descriptor=0x%08x, prev_type=0x%02x, descriptor_len=%u\n",
                        prev_descriptor, prev_type, descriptor_len);

        /* Add to fields.
         * The cast to pb_size_t is needed to avoid -Wconversion warning.
         * Because the data is is constants from generator, there is no danger of overflow.
         */
        iter->field_info_index = (pb_size_t)(iter->field_info_index + descriptor_len);
        iter->required_field_index = (pb_size_t)(iter->required_field_index + (PB_HTYPE(prev_type) == PB_HTYPE_REQUIRED));
        iter->submessage_index = (pb_size_t)(iter->submessage_index + PB_LTYPE_IS_SUBMSG(prev_type));
        
        PB_DEBUG_PRINTF("advance_iterator: new field_info_index=%u, submessage_index=%u\n",
                        iter->field_info_index, iter->submessage_index);
    }
}

bool pb_field_iter_begin(pb_field_iter_t *iter, const pb_msgdesc_t *desc, void *message)
{
    PB_DEBUG_PRINTF("pb_field_iter_begin: desc=%p, message=%p\n", desc, message);
    
    if (desc == NULL || desc->field_count == 0)
    {
        PB_DEBUG_PRINTF("pb_field_iter_begin: empty descriptor\n");
        return false;
    }
    
    PB_DEBUG_PRINTF("pb_field_iter_begin: field_count=%u, field_info=%p\n", 
                    desc->field_count, desc->field_info);

    memset(iter, 0, sizeof(*iter));

    iter->descriptor = PIC(desc);
    iter->message = message;

    PB_DEBUG_PRINTF("pb_field_iter_begin: calling load_descriptor_values\n");
    return load_descriptor_values(iter);
}

bool pb_field_iter_begin_extension(pb_field_iter_t *iter, pb_extension_t *extension)
{
    const pb_msgdesc_t *msg = (const pb_msgdesc_t*)extension->type->arg;
    bool status;

    uint32_t word0 = PB_PROGMEM_READU32(msg->field_info[0]);
    if (PB_ATYPE(word0 >> 8) == PB_ATYPE_POINTER)
    {
        /* For pointer extensions, the pointer is stored directly
         * in the extension structure. This avoids having an extra
         * indirection. */
        status = pb_field_iter_begin(iter, msg, &extension->dest);
    }
    else
    {
        status = pb_field_iter_begin(iter, msg, extension->dest);
    }

    iter->pSize = &extension->found;
    return status;
}

bool pb_field_iter_next(pb_field_iter_t *iter)
{
    advance_iterator(iter);
    (void)load_descriptor_values(iter);
    return iter->index != 0;
}

// W pliku app-hedera/vendor/nanopb/pb_common.c, w funkcji pb_field_iter_find (około linii 254):

bool pb_field_iter_find(pb_field_iter_t *iter, uint32_t tag)
{
    PB_DEBUG_PRINTF("pb_field_iter_find: start, tag=%u, iter->tag=%u\n", tag, iter->tag);
    PB_DEBUG_PRINTF("pb_field_iter_find: descriptor=%p, field_info=%p\n",
                    iter->descriptor, iter->descriptor ? iter->descriptor->field_info : NULL);
    PB_DEBUG_PRINTF("pb_field_iter_find: index=%u, field_info_index=%u, field_count=%u\n",
                    iter->index, iter->field_info_index,
                    iter->descriptor ? iter->descriptor->field_count : 0);
    
    if (iter->tag == tag)
    {
        PB_DEBUG_PRINTF("pb_field_iter_find: tag matches, returning true\n");
        return true; /* Nothing to do, correct field already. */
    }
    else if (tag > iter->descriptor->largest_tag)
    {
        PB_DEBUG_PRINTF("pb_field_iter_find: tag %u > largest_tag %u, returning false\n",
                        tag, iter->descriptor->largest_tag);
        return false;
    }
    else
    {
        pb_size_t start = iter->index;
        uint32_t fieldinfo;

        PB_DEBUG_PRINTF("pb_field_iter_find: searching, start=%u\n", start);

        if (tag < iter->tag)
        {
            PB_DEBUG_PRINTF("pb_field_iter_find: tag < iter->tag, resetting index\n");
            /* Fields are in tag number order, so we know that tag is between
             * 0 and our start position. Setting index to end forces
             * advance_iterator() call below to restart from beginning. */
            iter->index = iter->descriptor->field_count;
        }

        do
        {
            PB_DEBUG_PRINTF("pb_field_iter_find: loop, index=%u, field_info_index=%u\n",
                            iter->index, iter->field_info_index);
            
            /* Advance iterator but don't load values yet */
            advance_iterator(iter);

            PB_DEBUG_PRINTF("pb_field_iter_find: after advance, field_info_index=%u\n",
                            iter->field_info_index);
            
            /* Do fast check for tag number match */
            fieldinfo = PB_PROGMEM_READU32(((uint32_t *)PIC(((pb_msgdesc_t *)PIC(iter->descriptor))->field_info))[iter->field_info_index]);

            if (((fieldinfo >> 2) & 0x3F) == (tag & 0x3F))
            {
                PB_DEBUG_PRINTF("pb_field_iter_find: candidate match, loading values\n");
                /* Good candidate, check further */
                (void)load_descriptor_values(iter);

                if (iter->tag == tag &&
                    PB_LTYPE(iter->type) != PB_LTYPE_EXTENSION)
                {
                    PB_DEBUG_PRINTF("pb_field_iter_find: found match, returning true\n");
                    /* Found it */
                    return true;
                }
            }
        } while (iter->index != start);

        PB_DEBUG_PRINTF("pb_field_iter_find: not found, restoring values\n");
        /* Searched all the way back to start, and found nothing. */
        (void)load_descriptor_values(iter);
        return false;
    }
}

bool pb_field_iter_find_extension(pb_field_iter_t *iter)
{
    if (PB_LTYPE(iter->type) == PB_LTYPE_EXTENSION)
    {
        return true;
    }
    else
    {
        pb_size_t start = iter->index;
        uint32_t fieldinfo;

        do
        {
            /* Advance iterator but don't load values yet */
            advance_iterator(iter);

            /* Do fast check for field type */
            fieldinfo = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index]);

            if (PB_LTYPE((fieldinfo >> 8) & 0xFF) == PB_LTYPE_EXTENSION)
            {
                return load_descriptor_values(iter);
            }
        } while (iter->index != start);

        /* Searched all the way back to start, and found nothing. */
        (void)load_descriptor_values(iter);
        return false;
    }
}

static void *pb_const_cast(const void *p)
{
    /* Note: this casts away const, in order to use the common field iterator
     * logic for both encoding and decoding. The cast is done using union
     * to avoid spurious compiler warnings. */
    union {
        void *p1;
        const void *p2;
    } t;
    t.p2 = p;
    return t.p1;
}

bool pb_field_iter_begin_const(pb_field_iter_t *iter, const pb_msgdesc_t *desc, const void *message)
{
    return pb_field_iter_begin(iter, desc, pb_const_cast(message));
}

bool pb_field_iter_begin_extension_const(pb_field_iter_t *iter, const pb_extension_t *extension)
{
    return pb_field_iter_begin_extension(iter, (pb_extension_t*)pb_const_cast(extension));
}

bool pb_default_field_callback(pb_istream_t *istream, pb_ostream_t *ostream, const pb_field_t *field)
{
    if (field->data_size == sizeof(pb_callback_t))
    {
        pb_callback_t *pCallback = (pb_callback_t*)field->pData;

        if (pCallback != NULL)
        {
            if (istream != NULL && pCallback->funcs.decode != NULL)
            {
                return pCallback->funcs.decode(istream, field, &pCallback->arg);
            }

            if (ostream != NULL && pCallback->funcs.encode != NULL)
            {
                return pCallback->funcs.encode(ostream, field, &pCallback->arg);
            }
        }
    }

    return true; /* Success, but didn't do anything */

}

#ifdef PB_VALIDATE_UTF8

/* This function checks whether a string is valid UTF-8 text.
 *
 * Algorithm is adapted from https://www.cl.cam.ac.uk/~mgk25/ucs/utf8_check.c
 * Original copyright: Markus Kuhn <http://www.cl.cam.ac.uk/~mgk25/> 2005-03-30
 * Licensed under "Short code license", which allows use under MIT license or
 * any compatible with it.
 */

bool pb_validate_utf8(const char *str)
{
    const pb_byte_t *s = (const pb_byte_t*)str;
    while (*s)
    {
        if (*s < 0x80)
        {
            /* 0xxxxxxx */
            s++;
        }
        else if ((s[0] & 0xe0) == 0xc0)
        {
            /* 110XXXXx 10xxxxxx */
            if ((s[1] & 0xc0) != 0x80 ||
                (s[0] & 0xfe) == 0xc0)                        /* overlong? */
                return false;
            else
                s += 2;
        }
        else if ((s[0] & 0xf0) == 0xe0)
        {
            /* 1110XXXX 10Xxxxxx 10xxxxxx */
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                (s[0] == 0xe0 && (s[1] & 0xe0) == 0x80) ||    /* overlong? */
                (s[0] == 0xed && (s[1] & 0xe0) == 0xa0) ||    /* surrogate? */
                (s[0] == 0xef && s[1] == 0xbf &&
                (s[2] & 0xfe) == 0xbe))                 /* U+FFFE or U+FFFF? */
                return false;
            else
                s += 3;
        }
        else if ((s[0] & 0xf8) == 0xf0)
        {
            /* 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx */
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                (s[3] & 0xc0) != 0x80 ||
                (s[0] == 0xf0 && (s[1] & 0xf0) == 0x80) ||    /* overlong? */
                (s[0] == 0xf4 && s[1] > 0x8f) || s[0] > 0xf4) /* > U+10FFFF? */
                return false;
            else
                s += 4;
        }
        else
        {
            return false;
        }
    }

    return true;
}

#endif

