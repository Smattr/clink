#include <errno.h>
#include "sql.h"
#include <sqlite3.h>

int sql_err_to_errno(int err) {
  switch (err) {

    case SQLITE_OK:
    case SQLITE_ROW:
    case SQLITE_DONE:
      return 0;

    case SQLITE_ABORT: return EAGAIN;
    case SQLITE_AUTH: return EPERM;
    case SQLITE_BUSY: return EBUSY;
    case SQLITE_CANTOPEN: return EPERM;
    case SQLITE_CONSTRAINT: return EINVAL;
    case SQLITE_CORRUPT: return EIO;
    case SQLITE_ERROR: return ENOTRECOVERABLE;
    case SQLITE_FORMAT: return EBADMSG;
    case SQLITE_FULL: return EIO;
    case SQLITE_INTERNAL: return ENOTRECOVERABLE;
    case SQLITE_INTERRUPT: return EAGAIN;
    case SQLITE_IOERR: return EIO;
    case SQLITE_LOCKED: return EBUSY;
    case SQLITE_MISMATCH: return EINVAL;
    case SQLITE_MISUSE: return EINVAL;
    case SQLITE_NOLFS: return E2BIG;
    case SQLITE_NOMEM: return ENOMEM;
    case SQLITE_NOTADB: return EINVAL;
    case SQLITE_NOTFOUND: return ENOENT;
    case SQLITE_PERM: return EPERM;
    case SQLITE_PROTOCOL: return EBUSY;
    case SQLITE_RANGE: return ERANGE;
    case SQLITE_READONLY: return EROFS;
    case SQLITE_SCHEMA: return EBUSY;
    case SQLITE_TOOBIG: return E2BIG;

    default: return ENOTRECOVERABLE;
  }
}
