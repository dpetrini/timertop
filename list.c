#include <stdlib.h>
#include <string.h>
#include "list.h"

void list_init(List *list) {

	list->size = 0;
	list->head = NULL;
	list->tail = NULL;
	return;
}

int list_insert_next(List *list, list_element *element, const void *data) {

	list_element * new_element;

	if ((new_element = (list_element *)malloc(sizeof(list_element))) == NULL)
	   return -1;

	new_element->data = (void *)data;

	if (element == NULL) {
		if (list_size(list) == 0)
			list->tail = new_element;

		new_element->next = list->head;
		list->head = new_element;
	} else {
		if (element->next == NULL)
			list->tail = new_element;
		new_element->next = element->next;
	   	element->next = new_element;
	}

	list->size++;
	return 0;
}


int list_remove_next(List *list, list_element *element, void **data) {

	list_element * old_element;

	if (list_size(list) == 0)
	   return -1;

	if (element == NULL) {
		*data = list->head->data;
	   	old_element = list->head;
	   	list->head = list->head->next;
		
		if (list_size(list) == 1)
			list->tail = NULL;

	} else {

		if (element->next == NULL)
			return -1;

		*data = element->next->data;
		old_element = element->next;
		element->next = element->next->next;

		if (element->next == NULL)
			list->tail = element;

	}

	free(old_element);
	list->size--;

	return 0;
}
