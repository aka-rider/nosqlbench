
/*
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "nosqlbench.h"

extern struct nb nb;

extern volatile sig_atomic_t nb_signaled;

int nb_warmup(void)
{
	int rc = 0;
	struct nb_db db = { nb.db, NULL };

	struct nb_key key;
	nb.key->init(&key, nb.key_dist);

	nb.db->init(&db, nb.opts.value_size);
	if (nb.db->connect(&db, &nb.opts) == -1) {
		rc = 1;
		goto free;
	}

	int missed = 0;
	int i = 0;
	while (i < nb.opts.request_count && !nb_signaled) {
		nb.key->generate_by_id(&key, i);
		nb.db->insert(&db, &key);
		i++;
		if (i % nb.opts.request_batch_count == 0) {
			if (nb.db->recv(&db, nb.opts.request_batch_count, &missed) == -1) {
				rc = 1;
				goto free;
			}
			if (nb.report->progress)
				nb.report->progress(i, nb.opts.request_count);
		}
	}
	if (!nb_signaled && i % nb.opts.request_batch_count)
		if (nb.db->recv(&db, i % nb.opts.request_batch_count, &missed) == -1) {
			rc = 1;
			goto free;
		}
free:
	if (nb.report->progress)
		nb.report->progress(0, 0);

	nb.db->close(&db);
	nb.db->free(&db);
	nb.key->free(&key);
	return rc;
}
