#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

#define list_size(list) ((list)->size)
#define list_head(list) ((list)->head)
#define list_tail(list) ((list)->tail)
#define list_is_head(list, element) ((element) == (list)->head ? 1 : 0)
#define list_is_tail(element) ((element)->next == NULL ? 1 : 0)
#define list_data(element) ((element)->data)
#define list_next(element) ((element)->next)

typedef struct my_list_element {
	void			*data;
	struct my_list_element	*next;
} list_element;

typedef struct my_List {
	int                size;
	list_element           *head;
	list_element           *tail;
} List;

void list_init(List *list);
int list_insert_next(List *list, list_element *element, const void *data);
int list_remove_next(List *list, list_element *element, void **data);

#endif
