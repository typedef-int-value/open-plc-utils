/*====================================================================*
 *
 *   Copyright (c) 2013 Qualcomm Atheros, Inc.
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted (subject to the limitations
 *   in the disclaimer below) provided that the following conditions
 *   are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials
 *     provided with the distribution.
 *
 *   * Neither the name of Qualcomm Atheros nor the names of
 *     its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission.
 *
 *   NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *   GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE
 *   COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 *   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 *   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *--------------------------------------------------------------------*/

/*====================================================================*
 *
 *   psin.c - load prescalers into int6000 and qca7000 parameter file;
 *
 *
 *   Contributor(s):
 *	Charles Maier
 *      Nathaniel Houghton
 *
 *--------------------------------------------------------------------*/

/*====================================================================*
 *   system header files;
 *--------------------------------------------------------------------*/

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*====================================================================*
 *   custom header files;
 *--------------------------------------------------------------------*/

#include "../key/HPAVKey.h"
#include "../pib/pib.h"
#include "../plc/plc.h"
#include "../tools/chars.h"
#include "../tools/endian.h"
#include "../tools/error.h"
#include "../tools/files.h"
#include "../tools/getoptv.h"
#include "../tools/number.h"
#include "../tools/types.h"

/*====================================================================*
 *   custom source files;
 *--------------------------------------------------------------------*/

#ifndef MAKEFILE
#include "../tools/checksum32.c"
#include "../tools/error.c"
#include "../tools/fdchecksum32.c"
#include "../tools/getoptv.c"
#include "../tools/hexdecode.c"
#include "../tools/hexstring.c"
#include "../tools/putoptv.c"
#include "../tools/todigit.c"
#include "../tools/version.c"
#endif

#ifndef MAKEFILE
#include "../pib/pibfile.c"
#include "../pib/pibfile1.c"
#include "../pib/pibfile2.c"
#include "../pib/piblock.c"
#include "../pib/pibscalers.c"
#endif

#ifndef MAKEFILE
#include "../key/keys.c"
#endif

signed ps_pibbfile1(struct _file_ const *file)
{
  struct simple_pib simple_pib;
  if (lseek(file->file, 0, SEEK_SET)) {
    error(1, errno, FILE_CANTHOME, file->name);
  }
  if (read(file->file, &simple_pib, sizeof(simple_pib)) != sizeof(simple_pib)) {
    error(1, errno, FILE_CANTREAD, file->name);
  }
  if (lseek(file->file, 0, SEEK_SET)) {
    error(1, errno, FILE_CANTHOME, file->name);
  }
  if ((simple_pib.RESERVED1) || (simple_pib.RESERVED2)) {
    error(1, errno, PIB_BADCONTENT, file->name);
  }
  if (fdchecksum32(file->file, LE16TOH(simple_pib.PIBLENGTH), 0)) {
    error(1, errno, PIB_BADCHECKSUM, file->name);
  }
  if (lseek(file->file, 0, SEEK_SET)) {
    error(1, errno, FILE_CANTHOME, file->name);
  }
  return (0);
}
signed ps_pibfile2(struct _file_ const *file)
{
  struct nvm_header2 nvm_header;
  uint32_t origin = ~0;
  uint32_t offset = 0;
  unsigned module = 0;
  if (lseek(file->file, 0, SEEK_SET)) {
    error(1, errno, FILE_CANTHOME, file->name);
  }
  do {
    if (read(file->file, &nvm_header, sizeof(nvm_header)) !=
        sizeof(nvm_header)) {
      error(1, errno, NVM_HDR_CANTREAD, file->name, module);
    }
    if (LE16TOH(nvm_header.MajorVersion) != 1) {
      error(1, errno, NVM_HDR_VERSION, file->name, module);
    }
    if (LE16TOH(nvm_header.MinorVersion) != 1) {
      error(1, errno, NVM_HDR_VERSION, file->name, module);
    }
    if (checksum32(&nvm_header, sizeof(nvm_header), 0)) {
      error(1, errno, NVM_HDR_CHECKSUM, file->name, module);
    }
    if (LE32TOH(nvm_header.PrevHeader) != origin) {
      error(1, errno, NVM_HDR_LINK, file->name, module);
    }
    if (LE32TOH(nvm_header.ImageType) == NVM_IMAGE_PIB) {
      if (fdchecksum32(file->file, LE32TOH(nvm_header.ImageLength),
                       nvm_header.ImageChecksum)) {
        error(1, errno, NVM_IMG_CHECKSUM, file->name, module);
      }
      if (lseek(file->file, 0, SEEK_SET)) {
        error(1, errno, FILE_CANTHOME, file->name);
      }
      return (0);
    }
    if (fdchecksum32(file->file, LE32TOH(nvm_header.ImageLength),
                     nvm_header.ImageChecksum)) {
      error(1, errno, NVM_IMG_CHECKSUM, file->name, module);
    }
    origin = offset;
    offset = LE32TOH(nvm_header.NextHeader);
    module++;
  } while (~nvm_header.NextHeader);

  if (lseek(file->file, 0, SEEK_SET)) {
    error(1, errno, FILE_CANTHOME, file->name);
  }
  return (-1);
}

signed ps_pibfile(struct _file_ const *file)
{
  uint32_t version;
  if (read(file->file, &version, sizeof(version)) != sizeof(version)) {
    error(1, errno, FILE_CANTREAD, file->name);
  }
  if (lseek(file->file, 0, SEEK_SET)) {
    error(1, errno, FILE_CANTHOME, file->name);
  }
  if (LE32TOH(version) == 0x60000000) {
    return (-1);
  }
  if (LE32TOH(version) == 0x00010001) {
    return (ps_pibfile2(file));
  }
  return (ps_pibbfile1(file));
}

/*====================================================================*
 *
 *
 *   signed ar7x00_psin (struct _file_ * pib, uint32_t value, uint32_t index);
 *
 *   Places a single 10 bit prescaler into the pib file at index;
 *
 *--------------------------------------------------------------------*/

signed ar7x00_psin(struct _file_ *pib, uint32_t value, uint32_t index)

{
  off_t offset = AMP_PRESCALER_OFFSET + (index * 10 / 8);
  uint8_t bit_offset = (index * 10) % 8;
  uint16_t tmp;
  if (lseek(pib->file, offset, SEEK_SET) != offset) {
    return (-1);
  }
  if (read(pib->file, &tmp, sizeof(tmp)) != sizeof(tmp)) {
    return (-1);
  }
  if (lseek(pib->file, offset, SEEK_SET) != offset) {
    return (-1);
  }
  value &= 0x03FF;
  tmp = LE16TOH(tmp);
  tmp &= ~(0x03FF << bit_offset);
  tmp |= value << bit_offset;
  tmp = HTOLE16(tmp);
  if (write(pib->file, &tmp, sizeof(tmp)) != sizeof(tmp)) {
    return (-1);
  }
  return (0);
}

/*====================================================================*
 *
 *   signed psin (struct _file_ * pib);
 *
 *
 *--------------------------------------------------------------------*/
unsigned char read_c(struct _file_ *prescaler, signed char *c) {
  if (read(prescaler->file, c, 1) == 1)
    return 1;

  return 0;
}

static signed ps_in(struct _file_ *pib, struct _file_ *prescaler) {
  unsigned index = 0;
  unsigned count = 0;
  unsigned limit = pibscalers(pib);
  uint32_t value = 0;
  signed char c;
  if ((limit != INT_CARRIERS) && (limit != AMP_CARRIERS) &&
      (limit != PLC_CARRIERS)) {
    error(1, 0, "Don't understand this PIB's prescaler format");
  }
  if (limit == INT_CARRIERS) {
    if (lseek(pib->file, INT_PRESCALER_OFFSET, SEEK_SET) !=
        INT_PRESCALER_OFFSET) {
      error(1, errno, FILE_CANTSEEK, pib->name);
    }
  } else if (limit == PLC_CARRIERS) {
    if (lseek(pib->file, QCA_PRESCALER_OFFSET, SEEK_SET) !=
        QCA_PRESCALER_OFFSET) {
      error(1, errno, FILE_CANTSEEK, pib->name);
    }
  }

  prescaler->file = open(prescaler->name, O_TEXT | O_RDONLY);
  while (read_c(prescaler, &c)) {
    if (isspace(c)) {
      continue;
    }
    if ((c == '#') || (c == ';')) {
      do {
        read_c(prescaler, &c);
      } while (nobreak(c));
      continue;
    }
    index = 0;
    while (isdigit(c)) {
      index *= 10;
      index += c - '0';
      read_c(prescaler, &c);
    }
    if (index != count) {
      error(1, ECANCELED, "Carrier %d out of order", index);
    }
    if (index >= limit) {
      error(1, EOVERFLOW, "Too many prescalers");
    }
    while (isblank(c)) {
      read_c(prescaler, &c);
    }
    value = 0;
    while (isxdigit(c)) {
      value *= 16;
      value += todigit(c);
      read_c(prescaler, &c);
    }
    if (limit == INT_CARRIERS) {
      value = HTOLE32(value);
      if (write(pib->file, &value, sizeof(value)) != sizeof(value)) {
        error(1, errno, "Can't save %s", pib->name);
      }
    } else if (limit == AMP_CARRIERS) {
      if (value & ~0x03FF) {
        error(1, errno, "Position %d has invalid prescaler value", index);
      }
      if (ar7x00_psin(pib, value, index)) {
        error(1, errno, "Can't update %s", pib->name);
      }
    } else if (limit == PLC_CARRIERS) {
      uint8_t tmp = value & 0xff;
      if (write(pib->file, &tmp, sizeof(tmp)) != sizeof(tmp)) {
        error(1, errno, "Can't save %s", pib->name);
      }
      if ((count - 1) == limit)
        return 0;
    }

    while (nobreak(c)) {
      read_c(prescaler, &c);
    };
    count++;

    if (limit == (count)) {
      break;
    }
  }
  return (0);
}

signed prescaler_in(const char *to_path) {
  struct _file_ pib;
  pib.name = to_path;

  struct _file_ prescaler;
  prescaler.name = "full_power.txt";

  if ((pib.file = open(pib.name, O_BINARY | O_RDWR)) == -1) {
    error(1, errno, "Can't open %s", pib.name);
  } else if (ps_pibfile(&pib)) {
    error(1, errno, "Bad PIB file: %s", pib.name);
  } else if (ps_in(&pib, &prescaler)) {
    error(1, ECANCELED, "%s", pib.name);
  } else if (piblock(&pib)) {
    error(1, ECANCELED, "%s", pib.name);
  }
  close(pib.file);
  return (0);
}
