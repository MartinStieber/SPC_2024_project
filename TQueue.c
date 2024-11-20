/** \file TQueue.c
 *  \brief Implementace API pro typ fronta (realizace pomocí lineárního jednosměrně vázaného seznamu)
 *  \author Petyovský
 *  \version 2022
 *  $Id: TQueue.c 1597 2022-02-24 17:07:05Z petyovsky $
 */

#include "TQueue.h"

 /** \brief Úplná definice privátního typu QueueNode
  *  \details Privátní typ QueueNode (nedostupný mimo funkce ze souboru TQueue.c) reprezentuje typ pro jednotlivé uzly lineárního jednosměrně vázaného seznamu.
  */
struct TQueueNode
{
	struct TQueueNode* iNext;				///< Ukazatel na následující uzel lineárního jednosměrně vázaného seznamu
	TQueueElement iValue;					///< Element fronty uložený v uzlu lineárního jednosměrně vázaného seznamu
};

void queue_init(struct TQueue* aQueue)
{
	// Prvotní nastavení vnitřních proměnných fronty.
	// Pokud parametr typu ukazatel na TQueue není NULL,
	// nastav vnitřní složky odkazující na počáteční a koncový uzel na hodnotu NULL.
	if (aQueue) {
		aQueue->iBack = NULL;
		aQueue->iFront = NULL;
	}

}

bool queue_is_empty(const struct TQueue* aQueue)
{
	// Test, zda je fronta prázdná - (tj. fronta neobsahuje žádné elementy).
	// Pokud parametr typu ukazatel na TQueue není NULL a
	// pokud má vnitřní složka odkazující na počáteční uzel hodnotu různou od NULL (tj. fronta není prázdná), vrať false,
	// jinak vrať true.
	if (aQueue && aQueue->iFront) {
		return false;
	}
	return true;
}

bool /* TQueueIterator */ queue_front(const struct TQueue* aQueue, TQueueElement* aValue)
{
	// Do paměti předané pomocí druhého parametru zapíše kopii elementu z čela fronty.
	// Pokud fronta existuje, není prázdná a druhý parametr není NULL,
	// zkopíruj hodnotu elementu z čela fronty do paměti předané pomocí ukazatele aValue a vrať true,
	// jinak vrať false.
	if (!queue_is_empty(aQueue) && aValue) {
		*aValue = aQueue->iFront->iValue;
		return true;
	}
	return false;
}

bool /* TQueueIterator */ queue_back(const struct TQueue* aQueue, TQueueElement* aValue)
{
	// Do paměti předané pomocí druhého parametru zapíše kopii elementu z konce fronty.
	// Pokud fronta existuje, není prázdná a druhý parametr není NULL,
	// zkopíruj hodnotu elementu z konce fronty do paměti předané pomocí ukazatele aValue a vrať true,
	// jinak vrať false.
	if (!queue_is_empty(aQueue) && aValue) {
		*aValue = aQueue->iBack->iValue;
		return true;
	}
	return false;

}

bool queue_push(struct TQueue* aQueue, TQueueElement aValue)
{
	// Vkládá element na konec fronty (tj. na konec seznamu).
	// Pokud parametr typu ukazatel na TQueue není NULL,
	// alokuj nový uzel TQueueNode, pokud se alokace nepovedla, vrať false.
	// Do nového TQueueNode vlož předanou hodnotu elementu a ukazatel na další (iNext) nastav na NULL.
	// Pokud je fronta prázdná, zapiš adresu nového uzlu do obou složek (proměnných) fronty (iFront a iBack),
	// jinak, zařaď nový uzel za současný konec seznamu.
	// Nastav ukazatel iBack na nový (posledně přidaný) uzel.
	// Pokud operace skončila úspěšně, vrať true,
	// jinak vrať false.
	if (aQueue) {
		struct TQueueNode* newnode = NULL;
		newnode = malloc(sizeof(struct TQueueNode));
		if (newnode == NULL) {
			return false;
		}
		newnode->iNext = NULL;
		newnode->iValue = aValue;
		if (queue_is_empty(aQueue)) {
			aQueue->iBack = newnode;
			aQueue->iFront = newnode;
		}
		else {
			aQueue->iBack->iNext = newnode;
			aQueue->iBack = newnode;
		}
		return true;
	}
	return false;
}

bool queue_pop(struct TQueue* aQueue)
{
	// Odebere element z čela fronty (tj. ze začátku seznamu).
	// Pokud parametr typu ukazatel na TQueue není NULL a fronta není prázdná,
	// zapamatuj si v dočasné proměnné odkaz na počáteční uzel seznamu,
	// jako nový počátek fronty zapiš druhý uzel seznamu.
	// Pokud fronta obsahovala pouze seznam s jedním uzlem (počátek je nyní NULL) vynuluj i ukazatel konce seznamu,
	// dealokuj paměť zapamatovaného bývalého prvního uzlu.
	// Pokud operace skončila úspěšně, vrať true,
	// jinak vrať false.

	if (aQueue && !queue_is_empty(aQueue)) {
		struct TQueueNode* firstnode = aQueue->iFront;
		aQueue->iFront = firstnode->iNext;
		if (aQueue->iFront == NULL) {
			aQueue->iBack = NULL;
		}
		free(firstnode);
		return true;
	}
	return false;
}

void queue_destroy(struct TQueue* aQueue)
{
	// Korektně zruší všechny elementy fronty a uvede ji do základního stavu prázdné fronty (jako po queue_init).
	// Pokud parametr typu ukazatel na TQueue není NULL,
	// zapamatuj si v dočasné proměnné odkaz na počáteční uzel seznamu,
	// vynuluj ukazatel na počáteční a koncový uzel fronty.
	// Pro všechny uzly seznamu, opakuj:
	// zapamatuj si odkaz na rušený uzel,
	// nastav aktuální uzel na následující uzel,
	// odalokuj rušený uzel pomocí zapamatovaného odkazu.
	if (aQueue) {
		struct TQueueNode* actnode = aQueue->iFront;
		struct TQueueNode* desnode = NULL;
		aQueue->iBack = NULL;
		aQueue->iFront = NULL;
		while (actnode != NULL) {
			desnode = actnode;
			actnode = actnode->iNext;
			free(desnode);
		}
	}
}

struct TQueueIterator queue_iterator_begin(const struct TQueue* aQueue)
{
	// Inicializace a asociace/propojení iterátoru s frontou - zapíše odkaz na frontu a nastaví pozici v iterátoru na počátek fronty.
	// Pokud předaná fronta existuje (ukazatel není NULL) a není prázdná, ulož do iterátoru adresu asociované fronty,
	// nastav iterátor na element na čele fronty (na čele je element, který se bude první odebírat),
	// vrať hodnotu vytvořeného iterátoru.
	// Jinak vrať iterátor s vynulovanými vnitřními složkami.
	if (aQueue && !queue_is_empty(aQueue)) {
		return (struct TQueueIterator) { .iQueue = aQueue, .iActual = aQueue->iFront };
	}
	return (struct TQueueIterator) { .iQueue = NULL, .iActual = NULL };
}

bool queue_iterator_is_valid(const struct TQueueIterator* aIter)
{
	// Zjistí, zda iterátor odkazuje na platný element asociované fronty.
	// Pokud parametr typu ukazatel na TQueueIterator není NULL a
	// pokud je iterátor asociován s platnou frontou (tj. má platnou adresu TQueue) a tato fronta není prázdná, pokračuj.
	// Vrať true, pokud odkaz v iterátoru na aktuální uzel je platný (tj. nebyl dosažen konec seznamu),
	// jinak vrať false.
	if (aIter && !queue_is_empty(aIter->iQueue) && aIter->iActual) {
		return true;
	}
	return false;
}

bool queue_iterator_to_next(struct TQueueIterator* aIter)
{
	// Přesune odkaz v iterátoru z aktuálního elementu na následující element fronty.
	// Je-li iterátor validní pokračuj, jinak zruš propojení iterátoru s frontou a vrať false.
	// Posuň aktuální odkaz na další element.
	// Vrať true, když nově odkazovaný element existuje,
	// jinak zruš propojení iterátoru s frontou a vrať false.

	if (aIter) {
		if (queue_iterator_is_valid(aIter)) {
			aIter->iActual = aIter->iActual->iNext;
			if (queue_iterator_is_valid(aIter))
			{
				return true;
			}
			else
				goto aiter_NULL_handling;
		}
		else {
		aiter_NULL_handling:
			aIter->iActual = NULL;
			aIter->iQueue = NULL;
			return false;
		}
	}
	/*
	bool valid = queue_iterator_is_valid(aIter);
	if (valid) {
		aIter->iActual = aIter->iActual->iNext;
		valid = aIter->iActual != NULL;
	}
	return valid;
	*/
}

TQueueElement queue_iterator_value(const struct TQueueIterator* aIter)
{
	// Vrátí hodnotu elementu, na kterou odkazuje iterátor.
	// Pokud je iterátor validní, vrať hodnotu aktuálního elementu,
	// jinak vrať nulový element.
	if (aIter) {
		if (queue_iterator_is_valid(aIter)) {
			return aIter->iActual->iValue;	return (TQueueElement) { aIter->iActual->iValue };
		}
		else {
			return aIter->iActual->iValue;	return (TQueueElement) { 0 };
		}
	}
}

bool queue_iterator_set_value(const struct TQueueIterator* aIter, TQueueElement aValue)
{
	// Nastaví element, na který odkazuje iterátor, na novou hodnotu.
	// Pokud je iterátor validní,
	// zapiš do aktuálního elementu hodnotu předanou pomocí druhého parametru a vrať true,
	// jinak vrať false.
	if (queue_iterator_is_valid(aIter)) {
		aIter->iActual->iValue = aValue;
		return true;
	}
	return false;
}
