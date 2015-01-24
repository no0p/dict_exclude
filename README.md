## Overview

dict_exclude is a postgresql extension which provides a dictionary which can be used to create stop words based on regular expressions.

At the time of writing it appears like a decent solution towards this goal, but it does not use the stop word facility in text search which effectively creates a sorted list of words.

## Installation & Usage Example

```
git clone git@github.com:no0p/dict_exclude.git
make
sudo make install
```

Next create a list of exclusionary rules by adding a file *exclude.rules* in the postgresql text search resource directory.  For example, with Postgresql 9.4 on ubuntu you might add the file */usr/share/postgresql/9.4/tsearch_data/*.

The contents of the file are regular expressions, one per line:

```
abc
def
```

Then create the appropriate text search configuration in psql.

```
create text search configuration ocr_gibberish ( COPY = pg_catalog.english );
alter text search configuration ocr_gibberish 
  alter mapping for asciihword, asciiword
    with dict_exclude, english_stem;
```

Note that dict_exclude must be the first entry in the with clause to work.

Now querying will achieve the desired results:

```
SELECT to_tsvector('ocr_gibberish', 'fat abc cat def abd');
       to_tsvector       
-------------------------
 'abd':5 'cat':3 'fat':1
(1 row)
```
