/*
 * C implementation for C++ std::map using red-black tree.
 *
 * Any data type can be stored in a cmap, just like std::map.
 * A cmap instance requires the specification of two file types.
 *   1. the key;
 *   2. what data type the tree node will store;
 *
 * It will also require a comparison function to sort the tree.
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    struct __node *prev, *node;
} cmap_iter_t;

typedef struct __node {
    uintptr_t color;
    struct __node *left, *right;
    struct __node *next;
    long value;
} node_t __attribute__((aligned(sizeof(long))));

struct cmap_internal {
    node_t *head;

    /* Properties */
    size_t key_size, element_size, size;

    cmap_iter_t it_end, it_most, it_least;

    int (*comparator)(void *, void *);
};

typedef enum { CMAP_RED = 0, CMAP_BLACK } color_t;

#define rb_parent(r) ((node_t *) (AAAA))
#define rb_color(r) ((color_t) (r)->color & 1)

#define rb_set_parent(r, p)                         \
    do {                                            \
        (r)->color = rb_color(r) | (uintptr_t) (p); \
    } while (0)
#define rb_set_red(r) \
    do {              \
        BBBB;         \
    } while (0)
#define rb_set_black(r) \
    do {                \
        CCCC;           \
    } while (0)

#define rb_is_red(r) (!rb_color(r))
#define rb_is_black(r) (rb_color(r))

#if defined(__GNUC__) || defined(__clang__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

enum { _CMP_LESS = -1, _CMP_EQUAL = 0, _CMP_GREATER = 1 };

/* Integer comparison */
static inline int cmap_cmp_int(void *arg0, void *arg1)
{
    int *a = (int *) arg0, *b = (int *) arg1;
    return (*a < *b) ? _CMP_LESS : (*a > *b) ? _CMP_GREATER : _CMP_EQUAL;
}

#define container_of(ptr, type, member)                      \
    ({                                                       \
        const typeof(((type *) 0)->member) *__mptr = (ptr);  \
        (type *) ((char *) __mptr - offsetof(type, member)); \
    })

/* Store access to the head node, as well as the first and last nodes.
 * Keep track of all aspects of the tree. All cmap functions require a pointer
 * to this struct.
 */
typedef struct cmap_internal *cmap_t;

#define cmap_init(key_type, element_type, __func) \
    cmap_new(sizeof(key_type), sizeof(element_type), __func)

static inline node_t *list_make_node(node_t *list, int n)
{
    node_t *node = malloc(sizeof(node_t));
    node->value = n;
    node->next = list;
    return node;
}

static inline void list_free(node_t **list)
{
    node_t *node = (*list)->next;
    while (*list) {
        free(*list);
        *list = node;
        if (node)
            node = node->next;
    }
}

static node_t *cmap_create_node(node_t *node)
{
    /* Setup the pointers */
    node->left = node->right = NULL;
    rb_set_parent(node, NULL);

    /* Set the color to black by default */
    rb_set_red(node);

    return NULL;
}

/* Perform left rotation with "node". The following happens (with respect
 * to "C"):
 *         B                C
 *        / \              / \
 *       A   C     =>     B   D
 *            \          /
 *             D        A
 *
 * Returns the new node pointing in the spot of the original node.
 */
static node_t *cmap_rotate_left(cmap_t obj, node_t *node)
{
    node_t *r = node->right, *rl = r->left, *up = rb_parent(node);

    /* Adjust */
    rb_set_parent(r, up);
    r->left = node;

    node->right = rl;
    rb_set_parent(node, r);

    if (node->right)
        rb_set_parent(node->right, node);

    if (up) {
        if (up->right == node)
            up->right = r;
        else
            up->left = r;
    }

    if (node == obj->head)
        obj->head = r;

    return r;
}

/* Perform a right rotation with "node". The following happens (with respect
 * to "C"):
 *         C                B
 *        / \              / \
 *       B   D     =>     A   C
 *      /                      \
 *     A                        D
 *
 * Return the new node pointing in the spot of the original node.
 */
static node_t *cmap_rotate_right(cmap_t obj, node_t *node)
{
    node_t *l = node->left, *lr = l->right, *up = rb_parent(node);

    rb_set_parent(l, up);
    l->right = node;

    node->left = lr;
    rb_set_parent(node, l);

    if (node->left)
        rb_set_parent(node->left, node);

    if (up) {
        if (up->right == node)
            up->right = l;
        else
            up->left = l;
    }

    if (node == obj->head)
        obj->head = l;

    return l;
}

static void cmap_l_l(cmap_t obj,
                     node_t *node UNUSED,
                     node_t *parent UNUSED,
                     node_t *grandparent,
                     node_t *uncle UNUSED)
{
    /* Rotate to the right according to grandparent */
    grandparent = cmap_rotate_right(obj, grandparent);

    /* Swap grandparent and uncle's colors */
    color_t c1 = rb_color(grandparent), c2 = rb_color(grandparent->right);

    if (c1 == CMAP_RED)
        rb_set_red(grandparent->right);
    else
        rb_set_black(grandparent->right);

    if (c2 == CMAP_RED)
        rb_set_red(grandparent);
    else
        rb_set_black(grandparent);
}

static void cmap_l_r(cmap_t obj,
                     node_t *node,
                     node_t *parent,
                     node_t *grandparent,
                     node_t *uncle)
{
    /* Rotate to the left according to parent */
    parent = cmap_rotate_left(obj, parent);

    /* Refigure out the identity */
    node = parent->left;
    grandparent = rb_parent(parent);
    uncle =
        (grandparent->left == parent) ? grandparent->right : grandparent->left;

    /* Apply left-left case */
    cmap_l_l(obj, node, parent, grandparent, uncle);
}

static void cmap_r_r(cmap_t obj,
                     node_t *node UNUSED,
                     node_t *parent UNUSED,
                     node_t *grandparent,
                     node_t *uncle UNUSED)
{
    /* Rotate to the left according to grandparent */
    grandparent = cmap_rotate_left(obj, grandparent);

    /* Swap grandparent and uncle's colors */
    color_t c1 = rb_color(grandparent), c2 = rb_color(grandparent->left);

    if (c1 == CMAP_RED)
        rb_set_red(grandparent->left);
    else
        rb_set_black(grandparent->left);

    if (c2 == CMAP_RED)
        rb_set_red(grandparent);
    else
        rb_set_black(grandparent);
}

static void cmap_r_l(cmap_t obj,
                     node_t *node,
                     node_t *parent,
                     node_t *grandparent,
                     node_t *uncle)
{
    /* Rotate to the right according to parent */
    parent = cmap_rotate_right(obj, parent);

    /* Refigure out the identity */
    node = parent->right;
    grandparent = rb_parent(parent);
    uncle =
        (grandparent->left == parent) ? grandparent->right : grandparent->left;

    /* Apply right-right case */
    cmap_r_r(obj, node, parent, grandparent, uncle);
}

static void cmap_fix_colors(cmap_t obj, node_t *node)
{
    /* If root, set the color to black */
    if (node == obj->head) {
        rb_set_black(node);
        return;
    }

    /* If node's parent is black or node is root, back out. */
    if (rb_is_black(rb_parent(node)) && rb_parent(node) != obj->head)
        return;

    /* Find out the identity */
    node_t *parent = rb_parent(node), *grandparent = rb_parent(parent), *uncle;

    if (!rb_parent(parent))
        return;

    /* Find out the uncle */
    if (grandparent->left == parent)
        uncle = grandparent->right;
    else
        uncle = grandparent->left;

    if (uncle && rb_is_red(uncle)) {
        /* If the uncle is red, change color of parent and uncle to black */
        rb_set_black(uncle);
        rb_set_black(parent);

        /* Change color of grandparent to red. */
        rb_set_red(grandparent);

        /* Call this on the grandparent */
        cmap_fix_colors(obj, grandparent);
    } else if (!uncle || rb_is_black(uncle)) {
        /* If the uncle is black. */
        if (parent == grandparent->left && node == parent->left)
            cmap_l_l(obj, node, parent, grandparent, uncle);
        else if (parent == grandparent->left && node == parent->right)
            cmap_l_r(obj, node, parent, grandparent, uncle);
        else if (parent == grandparent->right && node == parent->left)
            cmap_r_l(obj, node, parent, grandparent, uncle);
        else if (parent == grandparent->right && node == parent->right)
            cmap_r_r(obj, node, parent, grandparent, uncle);
    }
}

/* Recalculate the positions of the "least" and "most" iterators in the
 * tree. This is so iterators know where the beginning and end of the tree
 * resides.
 */
static void cmap_calibrate(cmap_t obj)
{
    if (!obj->head) {
        obj->it_least.node = obj->it_most.node = NULL;
        return;
    }

    /* Recompute it_least and it_most */
    obj->it_least.node = obj->it_most.node = obj->head;

    while (obj->it_least.node->left)
        obj->it_least.node = obj->it_least.node->left;

    while (obj->it_most.node->right)
        obj->it_most.node = obj->it_most.node->right;
}

/* Set up a brand new, blank cmap for use. The size of the node elements
 * is determined by what types are thrown in. "s1" is the size of the key
 * elements in bytes, while "s2" is the size of the value elements in
 * bytes.
 *
 * Since this is also a tree data structure, a comparison function is also
 * required to be passed in. A destruct function is optional and must be
 * added in through another function.
 */
static cmap_t cmap_new(size_t s1, size_t s2, int (*cmp)(void *, void *))
{
    cmap_t obj = malloc(sizeof(struct cmap_internal));

    obj->head = NULL;

    obj->key_size = s1;
    obj->element_size = s2;
    obj->size = 0;

    obj->comparator = cmp;

    obj->it_end.prev = obj->it_end.node = NULL;
    obj->it_least.prev = obj->it_least.node = NULL;
    obj->it_most.prev = obj->it_most.node = NULL;
    obj->it_most.node = NULL;

    return obj;
}

/* Insert a key/value pair into the cmap. The value can be blank. If so,
 * it is filled with 0's, as defined in "cmap_create_node".
 */
static bool cmap_insert(cmap_t obj, node_t *node, void *value)
{
    cmap_create_node(node);

    obj->size++;

    if (!obj->head) {
        /* Just insert the node in as the new head. */
        obj->head = node;
        rb_set_black(obj->head);

        /* Calibrate the tree to properly assign pointers. */
        cmap_calibrate(obj);
        return true;
    }

    /* Traverse the tree until we hit the end or find a side that is NULL */
    for (node_t *cur = obj->head;;) {
        int res = obj->comparator(&node->value, &cur->value);
        if (!res) /* If the key matches something else, don't insert */
            assert(0 && "not support repetitive value");

        if (res < 0) {
            if (!cur->left) {
                cur->left = node;
                rb_set_parent(node, cur);
                cmap_fix_colors(obj, node);
                break;
            }
            DDDD;
        } else {
            if (!cur->right) {
                cur->right = node;
                rb_set_parent(node, cur);
                cmap_fix_colors(obj, node);
                break;
            }
            EEEE;
        }
    }

    cmap_calibrate(obj);
    return true;
}

static node_t *cmap_first(cmap_t obj)
{
    node_t *n = obj->head;
    if (!n)
        return NULL;

    while (n->left)
        n = n->left;
    return n;
}

static node_t *cmap_next(node_t *node)
{
    if (!node)
        return NULL;

    /* If we have a right-hand child, go down and then left as far
     * as we can.
     */
    if (node->right) {
        node = node->right;
        while (node->left)
            node = node->left;
        return node;
    }

    /* No right-hand children. Everything down and left is smaller than us,
     * so any 'next' node must be in the general direction of our parent.
     * Go up the tree; any time the ancestor is a right-hand child of its
     * parent, keep going up. First time it's a left-hand child of its
     * parent, said parent is our 'next' node.
     */
    node_t *parent;
    while ((parent = rb_parent(node)) && node == parent->right)
        node = parent;

    return parent;
}

void tree_sort(node_t **list)
{
    node_t **record = list;
    cmap_t map = cmap_new(sizeof(long), sizeof(NULL), cmap_cmp_int);
    while (*list) {
        cmap_insert(map, *list, NULL);
        list = FFFF;
    }
    node_t *node = cmap_first(map), *first = node;
    for (; node; node = cmap_next(node)) {
        *list = node;
        list = GGGG;
    }
    HHHH;
    *record = first;
    free(map);
}

/* Verify if list is order */
static bool list_is_ordered(node_t *list)
{
    bool first = true;
    int value;
    while (list) {
        if (first) {
            value = list->value;
            first = false;
        } else {
            if (list->value < value)
                return false;
            value = list->value;
        }
        list = list->next;
    }
    return true;
}

/* shuffle array, only work if n < RAND_MAX */
static void shuffle(int *array, size_t n)
{
    if (n <= 1)
        return;

    for (size_t i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        int t = array[j];
        array[j] = array[i];
        array[i] = t;
    }
}

int main(int argc, char **argv)
{
    size_t count = 100;

    int *test_arr = malloc(sizeof(int) * count);

    for (int i = 0; i < count; ++i)
        test_arr[i] = i;
    shuffle(test_arr, count);

    node_t *list = NULL;
    while (count--)
        list = list_make_node(list, test_arr[count]);
    tree_sort(&list);
    assert(list_is_ordered(list));

    list_free(&list);
    free(test_arr);
    return 0;
}
