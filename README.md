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
* recursive `ParserManager`
* actual `Tuple` class, the current version requires all the elements to be of
  the same kind

[Preliminary public docs)](http://candycode.github.com/parsley)

License
=======

_Parsley_ is distributes under the terms of the _New BSD License_

---

Copyright (c) 2010, Ugo Varetto
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL UGO VARETTO BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
