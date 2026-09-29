#include <stdio.h>
#include <stdlib.h>

#include "record.h"
#include "coord_query.h"

struct kd_node {
  const struct record *record;
  int axis;
  struct kd_node *left;
  struct kd_node *right;
};

struct kd_data {
  struct kd_node *root;
};

static int current_axis;

static int compare_records(const void *a, const void *b) {
  const struct record *ra = *(const struct record **)a;
  const struct record *rb = *(const struct record **)b;

  double a_value;
  double b_value;

  if (current_axis == 0) {
    a_value = ra->lon;
    b_value = rb->lon;
  } else {
    a_value = ra->lat;
    b_value = rb->lat;
  }

  if (a_value < b_value) {
    return -1;
  }

  if (a_value > b_value) {
    return 1;
  }

  return 0;
}

static struct kd_node* build_tree(const struct record **points,
                                  int n, int depth) {
  if (n == 0) {
    return NULL;
  }

  int axis = depth % 2;

  current_axis = axis;

  qsort(points, n, sizeof(*points), compare_records);

  int median = n / 2;

  struct kd_node *node = malloc(sizeof(struct kd_node));

  if (node == NULL) {
    return NULL;
  }

  node->record = points[median];
  node->axis = axis;

  node->left = build_tree(points, median, depth + 1);
  node->right = build_tree(points + median + 1,
                           n - median - 1,
                           depth + 1);

  return node;
}

static void free_tree(struct kd_node *node) {
  if (node == NULL) {
    return;
  }

  free_tree(node->left);
  free_tree(node->right);
  free(node);
}

struct kd_data* mk_kdtree(const struct record *rs, int n) {
  struct kd_data *data = malloc(sizeof(struct kd_data));

  if (data == NULL) {
    return NULL;
  }

  const struct record **points =
      malloc((size_t)n * sizeof(*points));

  if (points == NULL) {
    free(data);
    return NULL;
  }

  for (int i = 0; i < n; i++) {
    points[i] = &rs[i];
  }

  data->root = build_tree(points, n, 0);

  free(points);

  if (data->root == NULL && n > 0) {
    free(data);
    return NULL;
  }

  return data;
}

void free_kdtree(struct kd_data *data) {
  if (data == NULL) {
    return;
  }

  free_tree(data->root);
  free(data);
}

static double squared_distance(const struct record *r,
                               double lon, double lat) {
  double dlon = r->lon - lon;
  double dlat = r->lat - lat;

  return dlon * dlon + dlat * dlat;
}

static void lookup_tree(struct kd_node *node,
                        double lon, double lat,
                        const struct record **closest,
                        double *closest_distance) {
  if (node == NULL) {
    return;
  }

  double distance = squared_distance(node->record, lon, lat);

  if (*closest == NULL || distance < *closest_distance) {
    *closest = node->record;
    *closest_distance = distance;
  }

  double diff;

  if (node->axis == 0) {
    diff = node->record->lon - lon;
  } else {
    diff = node->record->lat - lat;
  }

  struct kd_node *near;
  struct kd_node *far;

  if (diff >= 0) {
    near = node->left;
    far = node->right;
  } else {
    near = node->right;
    far = node->left;
  }

  lookup_tree(near, lon, lat, closest, closest_distance);

  if (diff * diff <= *closest_distance) {
    lookup_tree(far, lon, lat, closest, closest_distance);
  }
}

const struct record* lookup_kdtree(struct kd_data *data,
                                   double lon, double lat) {
  const struct record *closest = NULL;
  double closest_distance = 0;

  lookup_tree(data->root, lon, lat, &closest, &closest_distance);

  return closest;
}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_kdtree,
                          (free_index_fn)free_kdtree,
                          (lookup_fn)lookup_kdtree);
}