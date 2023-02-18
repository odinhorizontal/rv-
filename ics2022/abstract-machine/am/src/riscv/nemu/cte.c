#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *ctx) {
  if (user_handler) {
    Event ev = {0};
    if( ctx->mcause >= 0 && ctx->mcause < 20 )
			ev.event = EVENT_SYSCALL;
		else if ( ctx->mcause == -1 ) {
      ev.event = EVENT_YIELD;
    } else {
      ev.event = EVENT_ERROR;
    }
    
		ctx = user_handler(ev, ctx);

		assert(ctx != NULL);
  }

  return ctx;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

void yield() {
  asm volatile("li a7, -1; ecall");
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
