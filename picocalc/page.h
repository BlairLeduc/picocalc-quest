/* page.h — the pages shown in place of the selector when something it
 * needs is missing (design.md §13.1, EL §8.1): never a blank screen.
 *
 * Core 0 finds the failure and names the page; core 1, which owns the
 * LCD, draws it over the whole panel in the 8x10 font and leaves it
 * there. The PSRAM's page cannot go through the frame pool, which lives
 * in the PSRAM, so all of them are drawn this way.
 */
#ifndef PICOCALC_QUEST_PAGE_H
#define PICOCALC_QUEST_PAGE_H

typedef enum {
    PAGE_NONE = 0,
    PAGE_NO_PSRAM,     /* the PSRAM failed its check (§11.3)      */
    PAGE_NO_CARD,      /* card detect says the slot is empty      */
    PAGE_CARD_FAILED,  /* a card is in, but did not mount         */
    PAGE_NO_QUEST,     /* the card has no /quest                  */
    PAGE_GAME_ERROR,   /* upstream's error() ended the game       */
} page_t;

/* The longest message a page shows; longer is cut. */
#define PAGE_MESSAGE_MAX 320

/* Core 0: show a page. Core 1 draws it on its next loop. */
void page_show(page_t p);

/* Core 0: show a page with a message of its own, wrapped to the panel's
 * width. The message is read when core 1 draws it, so it must stay. */
void page_show_message(page_t p, const char *message);

/* Core 1: the page core 0 asked for, and drawing it. */
page_t page_wanted(void);
void page_draw(page_t p);

#endif /* PICOCALC_QUEST_PAGE_H */
