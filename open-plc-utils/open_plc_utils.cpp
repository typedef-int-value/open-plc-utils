#include "open_plc_psin.h"
#include "open_plc_modpib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int file_copy(const char *src, const char *dst) {
  FILE *in, *out;
  char *buf;
  size_t len;

  if (!strcmpi(src, dst))
    return 4; // ������ �纻 ������ �����ϸ� ����

  if ((in = fopen(src, "rb")) == NULL)
    return 1; // ���� ���� ����
  if ((out = fopen(dst, "wb")) == NULL) {
    fclose(in);
    return 2;
  } // ��� ���� �����

  if ((buf = (char *)malloc(16777216)) == NULL) {
    fclose(in);
    fclose(out);
    return 10;
  } // ���� �޸� �Ҵ�

  while ((len = fread(buf, sizeof(char), sizeof(buf), in)) != NULL)
    if (fwrite(buf, sizeof(char), len, out) == 0) {
      fclose(in);
      fclose(out);
      free(buf);
      _unlink(dst); // ������ ���� ����� ����
      return 3;
    }

  fclose(in);
  fclose(out);
  free(buf); // �޸� �Ҵ� ����

  return 0;
}

int main(int argc, char const *argv[]) {
  file_copy("EV_PLC_FULL.pib", "EV_PLC_NOT_FULL.pib");
  prescaler_in("EV_PLC_NOT_FULL.pib", "not_full.txt");

  return (0);
}
