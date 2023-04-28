#include "open_plc_psin.h"

int main(int argc, char const *argv[]) {
  set_prescaler("SECC_STM_ORG.pib", "SECC_STM.pib");
  return (0);
}
