-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION dict_exclude" to load this file. \quit

CREATE FUNCTION dict_exclude_init(internal)
        RETURNS internal
        AS 'MODULE_PATHNAME'
        LANGUAGE C STRICT;

CREATE FUNCTION dict_exclude_lexize(internal, internal, internal, internal)
        RETURNS internal
        AS 'MODULE_PATHNAME'
        LANGUAGE C STRICT;

CREATE TEXT SEARCH TEMPLATE dict_exclude_template (
        LEXIZE = dict_exclude_lexize,
	      INIT   = dict_exclude_init
);

CREATE TEXT SEARCH DICTIONARY dict_exclude (
	TEMPLATE = dict_exclude_template
);

COMMENT ON TEXT SEARCH DICTIONARY dict_exclude IS 'dictionary for regexp stopwords';
