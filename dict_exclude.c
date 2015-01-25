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
#include "tsearch/ts_public.h"
#include "tsearch/ts_locale.h"

#define DICT_EXCLUDE_BACKREF_CNT    10

PG_MODULE_MAGIC;

typedef struct
{
  int       size;
  regex_t   *rules;
} DictExclude;

int process_rule_file(DictExclude *d);

PG_FUNCTION_INFO_V1(dict_exclude_init);
PG_FUNCTION_INFO_V1(dict_exclude_lexize);

Datum
dict_exclude_init(PG_FUNCTION_ARGS)
{
  DictExclude    *d;

  d = (DictExclude *) palloc0(sizeof(DictExclude));
  
  d->size = 0;
  d->rules = (regex_t *) palloc(sizeof(regex_t));
  
  process_rule_file(d);
      
  PG_RETURN_POINTER(d);
}

Datum
dict_exclude_lexize(PG_FUNCTION_ARGS)
{
  int regexec_result;
  regmatch_t  pmatch[DICT_EXCLUDE_BACKREF_CNT];
  pg_wchar   *data;
  size_t    data_len;
  int i;
  DictExclude    *d = (DictExclude *) PG_GETARG_POINTER(0);
  char     *in = (char *) PG_GETARG_POINTER(1);
  char     *txt = pnstrdup(in, PG_GETARG_INT32(2));
  TSLexeme   *res = palloc0(sizeof(TSLexeme) * 2);

  res[1].lexeme = NULL;
  
  data = (pg_wchar *) palloc((strlen(txt) + 1) * sizeof(pg_wchar));
  data_len = pg_mb2wchar_with_len(txt, data, strlen(txt));
  
  for(i = 0; i < d->size; i++)
  { 
    regexec_result = pg_regexec(&d->rules[i],
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
      break;
    }
  }
  
  pfree(data);
  
  PG_RETURN_POINTER(res);
}



int process_rule_file(DictExclude *d) 
{
  int compres;
  pg_wchar   *wstr;
  int     wlen;
  char     *line;
  tsearch_readline_state trst;
  char     *filename = get_tsearch_config_filename("exclude", "rules");
  
  if (!tsearch_readline_begin(&trst, filename))
      ereport(ERROR,
          (errcode(ERRCODE_CONFIG_FILE_ERROR),
           errmsg("could not open stop-word file \"%s\": %m",
              filename)));
  
  while ((line = tsearch_readline(&trst)) != NULL)
  {
    regex_t*  rule = palloc(sizeof(regex_t));
    char*     pbuf = line;

    /* Trim trailing space */
    while (*pbuf && !t_isspace(pbuf))
      pbuf += pg_mblen(pbuf);
    *pbuf = '\0';
    
    //skip empty lines
    if (*line == '\0')
    {
      pfree(line);
      continue;
    }
    
    /* Generate a regular expression */
    wstr = (pg_wchar *) palloc((strlen(line) + 1) * sizeof(pg_wchar));
    wlen = pg_mb2wchar_with_len(line, wstr, strlen(line));
    
    compres = pg_regcomp(rule, wstr, wlen, REG_ADVANCED, DEFAULT_COLLATION_OID);
    if (compres != REG_OKAY)
    {
      char    errstr[100];
      
      pg_regerror(compres, rule, errstr, sizeof(errstr));
      ereport(ERROR,
             (errcode(ERRCODE_INVALID_REGULAR_EXPRESSION),
              errmsg("invalid regular expression: %s", errstr)));
      
      pfree(rule);
    } else {
      d->rules = (regex_t *) repalloc(d->rules, sizeof(regex_t) * (d->size + 1));
      d->rules[d->size++] = *rule;
    }
    
    pfree(wstr);
  }
  
  return 0;

}
