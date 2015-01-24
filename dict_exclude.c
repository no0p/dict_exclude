/*-------------------------------------------------------------------------
 *
 * A postgresql text search dictionary for stop words based on regular expressions.
 * Based largely on the excellent dict_int example in postgresql contrib.
 * Offered under same license as Postgresql.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "commands/defrem.h"
#include "tsearch/ts_public.h"
#include "regex/regex.h"
#include "utils/elog.h"
#include "catalog/pg_collation.h"

#define DICT_EXCLUDE_BACKREF_CNT    10

PG_MODULE_MAGIC;

typedef struct
{
	char*			rule_file;
	regex_t   rules;
} DictExclude;


PG_FUNCTION_INFO_V1(dict_exclude_init);
PG_FUNCTION_INFO_V1(dict_exclude_lexize);

Datum
dict_exclude_init(PG_FUNCTION_ARGS)
{
  int compres;
  pg_wchar   *wstr;
  int     wlen;
	List	   *dictoptions = (List *) PG_GETARG_POINTER(0);
	DictExclude    *d;
	ListCell   *l;
  char* default_regexp = "abc";

	d = (DictExclude *) palloc0(sizeof(DictExclude));
	d->rule_file = "dict_exclude.rules";

	foreach(l, dictoptions)
	{
		DefElem    *defel = (DefElem *) lfirst(l);

		if (pg_strcasecmp(defel->defname, "RULEFILE") == 0)
		{
			d->rule_file = defGetString(defel);
		}
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("unrecognized intdict parameter: \"%s\"",
							defel->defname)));
		}
	}

  /*Manually set a single rule for debug, replace with rule list TODO*/
  
  wstr = (pg_wchar *) palloc((strlen(default_regexp) + 1) * sizeof(pg_wchar));
  wlen = pg_mb2wchar_with_len(default_regexp, wstr, strlen(default_regexp));
  
  compres = pg_regcomp(&d->rules, wstr, wlen, REG_ADVANCED, DEFAULT_COLLATION_OID);
  if (compres != REG_OKAY)
  {
    char    errstr[100];

      pg_regerror(compres, &d->rules, errstr, sizeof(errstr));
      ereport(ERROR,
          (errcode(ERRCODE_INVALID_REGULAR_EXPRESSION),
           errmsg("invalid regular expression: %s", errstr)));
  }
  
  pfree(wstr);
      
	PG_RETURN_POINTER(d);
}

Datum
dict_exclude_lexize(PG_FUNCTION_ARGS)
{
  int regexec_result;
  regmatch_t  pmatch[DICT_EXCLUDE_BACKREF_CNT];
  pg_wchar   *data;
  size_t    data_len;
	DictExclude    *d = (DictExclude *) PG_GETARG_POINTER(0);
	char	   *in = (char *) PG_GETARG_POINTER(1);
	char	   *txt = pnstrdup(in, PG_GETARG_INT32(2));
	TSLexeme   *res = palloc0(sizeof(TSLexeme) * 2);

	res[1].lexeme = NULL;
	
	//re = RE_compile_and_cache(d->rules, REG_ADVANCED, PG_GET_COLLATION());
  data = (pg_wchar *) palloc((strlen(txt) + 1) * sizeof(pg_wchar));
  data_len = pg_mb2wchar_with_len(txt, data, strlen(txt));
  
	regexec_result = pg_regexec(&d->rules,
                              data,
                              data_len,
                              0, /* search start position */
                              NULL,   /* no details */
                              DICT_EXCLUDE_BACKREF_CNT,
                              pmatch,
                              0);

  if (regexec_result == REG_NOMATCH)
	{
		res[0].lexeme = txt;
  }
	else
	{
		/* reject by returning void array */
		pfree(txt);
		res[0].lexeme = NULL;
	}

	PG_RETURN_POINTER(res);
}
