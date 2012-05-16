_Parsley_ is a header-only C++ library with no dependencies useful to extract data from text
streams.
The framework currently contains types implementing lexers and
parsers which can be composed to generate more complex parsing classes.

I am personally using this library to parse nearly-unstructured data from quantum electronic
structure computations where in many cases there is no concept of file format
of validation so you usually have to crawl through piles of unrelevant text including
citatons, quote of the day and info on the submitted job to extract and properly interpret
the contained data.

It is also possible to use this library to create Parsing Expression Grammar based
parsers or parse simple languages.

Some additions/changes which will likely happen in the near future:
* hierarchical state controller
* actual `Tuple` class, the current version requires all the elements to be of
  the same kind
