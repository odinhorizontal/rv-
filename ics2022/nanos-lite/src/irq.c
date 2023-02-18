#include <common.h>
extern void do_syscall(Context *c);

static Context* do_event(Event e, Context* ctx) {

  if (e.event == EVENT_YIELD ) {
    printf("EVENT_YIELD, event ID %d\n", e.event);
  } else if (e.event == EVENT_SYSCALL ) {
    printf("EVENT_SYSCALL, do_event ID %d, count %d", e.event, ctx->GPR4);
    do_syscall( ctx );
  } else {
    panic("PANIC event ID %d", e.event);
  }

	return ctx;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
