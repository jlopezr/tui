#include <stdio.h>

#include "test_support.h"

int main(void)
{
    test_checks = 0;
    test_failures = 0;

    test_listbox_suite();
    test_edit_suite();
    test_button_suite();
    test_window_suite();
    test_menu_suite();
    test_statusbar_suite();
    test_label_suite();
    test_core_suite();

    printf("%d checks, %d failures\n",
           test_checks,
           test_failures);

    return test_failures == 0 ? 0 : 1;
}
