/*
  Copyright (C) 2000-2007 MySQL AB

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  There are special exceptions to the terms and conditions of the GPL
  as it is applied to this software. View the full text of the exception
  in file LICENSE.exceptions in the top-level directory of this software
  distribution.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "setupgui.h"
#include "stringutil.h"

/*
   Entry point for GUI prompting from SQLDriverConnect().
*/
BOOL Driver_Prompt(HWND hWnd, SQLWCHAR *instr, SQLUSMALLINT completion,
                   SQLWCHAR *outstr, SQLSMALLINT outmax, SQLSMALLINT *outlen)
{
  DataSource *ds= ds_new();
  BOOL rc= FALSE;

  /*
     parse the attr string, dsn lookup will have already been
     done in the driver
  */
  if (instr && *instr)
  {
    if (ds_from_kvpair(ds, instr, (SQLWCHAR)';'))
    {
      rc= FALSE;
      goto exit;
    }
  }

  /* Show the dialog and handle result */
  if (ShowOdbcParamsDialog(ds, hWnd, TRUE) == 1)
  {
    /* serialize to outstr */
    if ((*outlen= ds_to_kvpair(ds, outstr, outmax, (SQLWCHAR)';')) == -1)
    {
      /* truncated, up to caller to see outmax < *outlen */
      *outlen= ds_to_kvpair_len(ds);
      outstr[outmax]= 0;
    }
    rc= TRUE;
  }

exit:
  ds_delete(ds);
  return rc;
}


/*
   Add, edit, or remove a Data Source Name (DSN). This function is
   called by "Data Source Administrator" on Windows, or similar
   application on Unix.
*/
BOOL INSTAPI ConfigDSNW(HWND hWnd, WORD nRequest, LPCWSTR pszDriver,
                        LPCWSTR pszAttributes)
{
  DataSource *ds= ds_new();
  BOOL rc= TRUE;
  Driver *driver= NULL;

  if (pszAttributes && *pszAttributes)
  {
    if (ds_from_kvpair(ds, pszAttributes, (SQLWCHAR)';'))
    {
      SQLPostInstallerError(ODBC_ERROR_INVALID_KEYWORD_VALUE,
                            W_INVALID_ATTR_STR);
      rc= FALSE;
      goto exitConfigDSN;
    }
    if (ds_lookup(ds))
    {
      /* ds_lookup() will already set SQLInstallerError */
      rc= FALSE;
      goto exitConfigDSN;
    }
  }

  switch (nRequest)
  {
  case ODBC_ADD_DSN:
    /*
       Lookup driver filename using W_DRIVER_NAME for a new DSN.
       It's not given by the DM and needed for Test/DB list.
    */
    driver= driver_new();
    memcpy(driver->name, W_DRIVER_NAME,
           (sqlwcharlen(W_DRIVER_NAME) + 1) * sizeof(SQLWCHAR));
    if (driver_lookup(driver))
    {
      rc= FALSE;
      break;
    }
    ds_set_strattr(&ds->driver, driver->lib);
  case ODBC_CONFIG_DSN:
    if (ShowOdbcParamsDialog(ds, hWnd, FALSE) == 1)
    {
      /* save datasource */
      if (ds_add(ds))
        rc= FALSE;
    }
    break;
  case ODBC_REMOVE_DSN:
    if (SQLRemoveDSNFromIni(ds->name) != TRUE)
      rc= FALSE;
    break;
  }

exitConfigDSN:
  ds_delete(ds);
  if (driver)
    driver_delete(driver);
  return rc;
}
