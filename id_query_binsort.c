#include <stdlib.h>
#include <stdint.h>

#include "record.h"
#include "id_query.h"

struct index_record {
  int64_t osm_id;
  const struct record *record;
};

struct indexed_data {
  struct index_record *irs;
  int n;
};

// Comparison function for qsort: orders two index_records by osm_id.
// It uses < and > instead of subtraction, because subtracting two
// int64_t values can overflow.
static int cmp_index_record(const void *a, const void *b) {
  const struct index_record *ra = a;
  const struct index_record *rb = b;

  if (ra->osm_id < rb->osm_id) {
    return -1;
  }

  if (ra->osm_id > rb->osm_id) {
    return 1;
  }

  return 0;
}

// Builds the index (one entry per record) and sorts it by osm_id.
struct indexed_data* mk_indexed(struct record* rs, int n) {
  struct indexed_data *data = malloc(sizeof(struct indexed_data));

  if (data == NULL) {
    return NULL;
  }

  data->irs = malloc((size_t)n * sizeof(struct index_record));

  if (data->irs == NULL) {
    free(data);
    return NULL;
  }

  data->n = n;

  for (int i = 0; i < n; i++) {
    data->irs[i].osm_id = rs[i].osm_id;
    data->irs[i].record = &rs[i];
  }

  qsort(data->irs, (size_t)n, sizeof(struct index_record), cmp_index_record);

  return data;
}

// Frees the index array and the wrapper struct.  Safe to call with NULL.
// It does NOT free the records themselves: they belong to record.c.
void free_indexed(struct indexed_data* data) {
  if (data == NULL) {
    return;
  }

  free(data->irs);
  free(data);
}

// Binary search in the sorted index array.
const struct record* lookup_indexed(struct indexed_data *data, int64_t needle) {
  int left = 0;
  int right = data->n - 1;

  while (left <= right) {
    int mid = left + (right - left) / 2;
    int64_t mid_id = data->irs[mid].osm_id;

    if (mid_id == needle) {
      return data->irs[mid].record;
    } else if (mid_id < needle) {
      left = mid + 1;
    } else {
      right = mid - 1;
    }
  }

  return NULL;
}

int main(int argc, char** argv) {
  return id_query_loop(argc, argv,
                       (mk_index_fn)mk_indexed,
                       (free_index_fn)free_indexed,
                       (lookup_fn)lookup_indexed);
}