# Doxygen guide #

## General Rules ##
  * Don't use Doxygen tags and HTML tags not listed in this guide except for very special cases;
  * Prefer natural formatting (indents, spaces, blank lines, "-" bulletins, etc.) to tags;
  * Use Javadoc style (`/** ... */`  and @tags);
  * Summary required.  Blank line between summary and details is optional;
  * Use doxygen only in .h files, not .cc files;
  * Summaries and descriptions should be all full sentences, starting with a capital letter and ending with a period;
  * **Don't** indent wrapped lines except in @param and @return;
  * Take advantage of "Automatic link generation" as much as possible (see http://www.stack.nl/~dimitri/doxygen/autolink.html);
    * Use () to indicate functions/methods in descriptions;
  * We use JAVADOC\_AUTOBRIEF, so the first sentence of a description is treated as the brief introduction, no matter whether there is a line break;
  * Don't omit template arguments when (partially) specializing a template, otherwise doxygen can't correctly generate doc for the (partial) specialized template.
  * Warnings reported by doxygen should be eliminated as many as possible.

### @a _parameter-name_ ###
Used in descriptions for methods or functions to indicate the following word is a parameter.

### @c _word_ ###
The following _word_ is display using a typewriter font to indicate the word is from source code.  Use ```
...``` for multiple words or multiple lines or to separate the 's' in plural forms.

### ```
_source-code_``` ###
See @c.

### @deprecated _description_ ###

### @param _parameter-name_ _parameter-description_ ###
  * Use `[out]` to indicate this parameter is an out parameter;
  * Use `[in,out]` to incidate this parameter is both in and out;
  * Don't use `[in]` for normal in parameters;
  * The first character of the parameter description is not capitalized;
  * If longer than 1 line, indent the following lines 4 characters;
  * All or none for a function;
  * Don't align the descriptions for multiple @param's;


### @return _description-of-the-return-value_ ###
  * The first character of the description is not capitalized;
  * If longer than 1 line, indent the following lines 4 characters;

### @see _references_ ###
Creates a link to other identifiers. The paragraph need not be ended with a period.

### @since _text_ ###

## An example ##
```
  /**
   * Resizes the image to specified @a width and @a height via reduced resolution.
   * The above sentence will be treated as brief.  Here are some more details. These
   * sentences are treated as in one paragraph.
   *
   * And more details requiring a new paragraph.
   * @param width no need to comment about a parameter if its name is descriptive
   *     enough. Indent 4 characters if a param line wraps.
   * @param height parameters should be documented all or none. If you document one
   *     parameter, you must document all the others (leave the description blank
   *     if nothing to say), to let doxygen check the consistency of docs.
   * @param[out] an output parameter. @c 0 if ...
   * @return @c true if succeeds.
   */
  bool SetSrcSize(size_t width, size_t height, int *output);
```

## Another example ##
```
  /**
   * Usage of lists:
   *   - Starting with a '-' and indent 2 characters to create a bulletin item;
   *   - Another item;
   *     - Indent more to create sub items;
   *     - Another sub item;
   *       -# Numbered sub item;
   *       -# Another numbered sub item;
   *     - Long lines in lists should be
   *       aligned if wrapped;
   *   - The last one.
   */
```