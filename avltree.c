#include "avltree.h"

struct avl_prio_queue {
    struct avl_root root;
    struct avl_node *min_node;
};

static inline void avl_prio_queue_init(struct avl_prio_queue *queue)
{
    INIT_AVL_ROOT(&queue->root);
    queue->min_node = NULL;
}

static inline void avl_prio_queue_insert_unbalanced(
    struct avl_prio_queue *queue,
    struct avlitem *new_entry)
{
    struct avl_node *parent = NULL;
    struct avl_node **cur_nodep = &queue->root.node;
    struct avlitem *cur_entry;
    int isminimal = 1;

    while (*cur_nodep) {
        cur_entry = avl_entry(*cur_nodep, struct avlitem, avl);

        parent = *cur_nodep;
        if (cmpint(&new_entry->i, &cur_entry->i) <= 0) {
            cur_nodep = &((*cur_nodep)->left);
        } else {
            cur_nodep = &((*cur_nodep)->right);
            isminimal = 0;
        }
    }

    if (isminimal)
        queue->min_node = &new_entry->avl;

    avl_link_node(&new_entry->avl, parent, cur_nodep);
}

static inline struct avlitem *avl_prio_queue_pop_unbalanced(
    struct avl_prio_queue *queue)
{
    struct avlitem *item;
    bool removed_right;

    if (!queue->min_node)
        return NULL;

    item = avl_entry(queue->min_node, struct avlitem, avl);
    queue->min_node = avl_next(queue->min_node);

    avl_erase_node(&item->avl, &queue->root, &removed_right);

    return item;
}

tatic inline void avl_prio_queue_insert_balanced(
    struct avl_prio_queue *queue,
    struct avlitem *new_entry)
{
    struct avl_node *parent = NULL;
    struct avl_node **cur_nodep = &queue->root.node;
    struct avlitem *cur_entry;
    int isminimal = 1;

    while (*cur_nodep) {
        cur_entry = avl_entry(*cur_nodep, struct avlitem, avl);

        parent = *cur_nodep;
        if (cmpint(&new_entry->i, &cur_entry->i) <= 0) {
            cur_nodep = &((*cur_nodep)->left);
        } else {
            cur_nodep = &((*cur_nodep)->right);
            isminimal = 0;
        }
    }

    if (isminimal)
        queue->min_node = &new_entry->avl;

    avl_insert(&new_entry->avl, parent, cur_nodep, &queue->root);
}

static inline struct avlitem *avl_prio_queue_pop_balanced(
    struct avl_prio_queue *queue)
{
    struct avlitem *item;

    if (!queue->min_node)
        return NULL;

    item = avl_entry(queue->min_node, struct avlitem, avl);
    queue->min_node = avl_next(queue->min_node);

    avl_erase(&item->avl, &queue->root);

    return item;
}
