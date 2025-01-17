#pragma once

void kinit();

WARN_RESULT void *kalloc_page();
void kfree_page(void *);
void *kshare_page(void *pg);

WARN_RESULT void *kalloc(unsigned long long);
void kfree(void *);

// allocate a zero-init, read-only page.
// This can be safely freed by kfree_page.
void *kalloc_zero();
