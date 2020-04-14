/*!\file cQueue.c
** \author SMFSW
** \copyright BSD 3-Clause License (c) 2017-2019, SMFSW
** \brief Queue handling library (designed in c on STM32)
** \details Queue handling library (designed in c on STM32)
**/
/****************************************************************/
#include <string.h>
#include <stdlib.h>

#include "cQueue.h"
/****************************************************************/


/*!	\brief Increment index
**	\details Increment buffer index \b pIdx rolling back to \b start when limit \b end is reached
**	\param [in,out] pIdx - pointer to index value
**	\param [in] end - counter upper limit value
**	\param [in] start - counter lower limit value
**/
static inline void __attribute__((nonnull, always_inline)) inc_idx(uint16_t * const pIdx, const uint16_t end, const uint16_t start)
{
//	(*pIdx)++;
//	*pIdx %= end;
	if (*pIdx < end - 1)	{ (*pIdx)++; }
	else					{ *pIdx = start; }
}

/*!	\brief Decrement index
**	\details Decrement buffer index \b pIdx rolling back to \b end when limit \b start is reached
**	\param [in,out] pIdx - pointer to index value
**	\param [in] end - counter upper limit value
**	\param [in] start - counter lower limit value
**/
static inline void __attribute__((nonnull, always_inline)) dec_idx(uint16_t * const pIdx, const uint16_t end, const uint16_t start)
{
	if (*pIdx > start)		{ (*pIdx)--; }
	else					{ *pIdx = end - 1; }
}


void * __attribute__((nonnull)) q_init(Queue_t * const q, const uint16_t size_rec, const uint16_t nb_recs, const QueueType type, const bool overwrite)
{
	const uint32_t size = nb_recs * size_rec;

	q->rec_nb = nb_recs;
	q->rec_sz = size_rec;
	q->impl = type;
	q->ovw = overwrite;

	q_kill(q);	// Free existing data (if any)
	q->queue = (uint8_t *) malloc(size);

	if (q->queue == NULL)	{ q->queue_sz = 0; return 0; }	// Return here if Queue not allocated
	else					{ q->queue_sz = size; }

	q->init = QUEUE_INITIALIZED;
	q_flush(q);

	return q->queue;	// return NULL when queue not allocated (beside), Queue address otherwise
}

void __attribute__((nonnull)) q_kill(Queue_t * const q)
{
	if (q->init == QUEUE_INITIALIZED)	{ free(q->queue); }	// Free existing data (if already initialized)
	q->init = 0;
}


void __attribute__((nonnull)) q_flush(Queue_t * const q)
{
	q->in = 0;
	q->out = 0;
	q->cnt = 0;
}


bool __attribute__((nonnull)) q_push(Queue_t * const q, const void * const record)
{
	if ((!q->ovw) && q_isFull(q))	{ return false; }

	uint8_t * const pStart = q->queue + (q->rec_sz * q->in);
	memcpy(pStart, record, q->rec_sz);

	inc_idx(&q->in, q->rec_nb, 0);

	if (!q_isFull(q))	{ q->cnt++; }	// Increase records count
	else if (q->ovw)					// Queue is full and overwrite is allowed
	{
		if (q->impl == FIFO)			{ inc_idx(&q->out, q->rec_nb, 0); }	// as oldest record is overwritten, increment out
		//else if (q->impl == LIFO)	{}										// Nothing to do in this case
	}

	return true;
}

bool __attribute__((nonnull)) q_pop(Queue_t * const q, void * const record)
{
	const uint8_t * pStart;

	if (q_isEmpty(q))	{ return false; }	// No more records

	if (q->impl == FIFO)
	{
		pStart = q->queue + (q->rec_sz * q->out);
		inc_idx(&q->out, q->rec_nb, 0);
	}
	else if (q->impl == LIFO)
	{
		dec_idx(&q->in, q->rec_nb, 0);
		pStart = q->queue + (q->rec_sz * q->in);
	}
	else	{ return false; }

	memcpy(record, pStart, q->rec_sz);
	q->cnt--;	// Decrease records count
	return true;
}

bool __attribute__((nonnull)) q_peek(const Queue_t * const q, void * const record)
{
	const uint8_t * pStart;

	if (q_isEmpty(q))	{ return false; }	// No more records

	if (q->impl == FIFO)
	{
		pStart = q->queue + (q->rec_sz * q->out);
		// No change on out var as it's just a peek
	}
	else if (q->impl == LIFO)
	{
		uint16_t rec = q->in;	// Temporary var for peek (no change on q->in with dec_idx)
		dec_idx(&rec, q->rec_nb, 0);
		pStart = q->queue + (q->rec_sz * rec);
	}
	else	{ return false; }

	memcpy(record, pStart, q->rec_sz);
	return true;
}

bool __attribute__((nonnull)) q_drop(Queue_t * const q)
{
	if (q_isEmpty(q))			{ return false; }	// No more records

	if (q->impl == FIFO)		{ inc_idx(&q->out, q->rec_nb, 0); }
	else if (q->impl == LIFO)	{ dec_idx(&q->in, q->rec_nb, 0); }
	else						{ return false; }

	q->cnt--;	// Decrease records count
	return true;
}

bool __attribute__((nonnull)) q_peekIdx(const Queue_t * const q, void * const record, const uint16_t idx)
{
	const uint8_t * pStart;

	if (idx + 1 > q_getCount(q))	{ return false; }	// Index out of range

	if (q->impl == FIFO)
	{
		pStart = q->queue + (q->rec_sz * ((q->out + idx) % q->rec_nb));
	}
	else if (q->impl == LIFO)
	{
		pStart = q->queue + (q->rec_sz * idx);
	}
	else	{ return false; }

	memcpy(record, pStart, q->rec_sz);
	return true;
}

