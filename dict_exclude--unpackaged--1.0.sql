/* contrib/dict_int/dict_int--unpackaged--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION dict_int FROM unpackaged" to load this file. \quit

ALTER EXTENSION dict_int ADD function dict_exclude_init(internal);
ALTER EXTENSION dict_int ADD function dict_exclude_lexize(internal,internal,internal,internal);
ALTER EXTENSION dict_int ADD text search template dict_exclude_template;
ALTER EXTENSION dict_int ADD text search dictionary dict_exclude;
