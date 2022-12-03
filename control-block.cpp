#include "control-block.h"

void control_block::inc_weak() {
  weak_ref++;
}

void control_block::inc_strong() {
  strong_ref++;
  inc_weak();
}

void control_block::dec_weak() {
  if (--weak_ref == 0) {
    delete this;
  }
}

void control_block::dec_strong() {
  if (--strong_ref == 0) {
    unlink();
  }
  dec_weak();
}


