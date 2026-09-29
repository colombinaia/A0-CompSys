#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "record.h"
#include "coord_query.h"

struct naive_data {
  struct record *rs;
  int n;
};

struct naive_data* mk_naive(struct record* rs, int n) {
  struct naive_data* data = malloc(sizeof(struct naive_data));

  data->rs = rs;
  data->n = n;

  return data;
}

void free_naive(struct naive_data* data) {
  free(data);
}

const struct record* lookup_naive(struct naive_data *data, double lon, double lat) {
  const struct record* closest = NULL;
  double closest_distance = 0;

  for (int i = 0; i < data->n; i++) {
    double distance =
        (data->rs[i].lon - lon) * (data->rs[i].lon - lon)
      + (data->rs[i].lat - lat) * (data->rs[i].lat - lat);

    if (closest == NULL || distance < closest_distance) {
      closest = &data->rs[i];
      closest_distance = distance;
    }
  }

  return closest;
}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_naive,
                          (free_index_fn)free_naive,
                          (lookup_fn)lookup_naive);
}
