Third-party components in the lmmagnum distribution
===================================================

Project license
---------------
lmmagnum's own source is MIT-licensed (`LICENSE`). MIT is compatible with every
component below (all MIT) and with the permissive licenses of the other lm* hosts and
plotting tools it is used beside: nothing here is copyleft.

Magnum (https://github.com/mosra/magnum) - MIT, Copyright Vladimir Vondrus and contributors.
Corrade (https://github.com/mosra/corrade) - MIT, Copyright Vladimir Vondrus and contributors.
  Both are compiled from the commits in `deps.lock` and statically linked.
  Their license texts: `third_party/magnum/COPYING`, `third_party/corrade/COPYING`
  (staged next to release binaries by `scripts/stage-release-notices.sh`).

cpp-dsl-sdk (https://github.com/lmctl-ai/cpp-dsl-sdk) - MIT. `cpp/dslvm.hpp` is vendored, pinned by `sdk.lock`.

`cpp/deflate.hpp` and `cpp/atomic-file.*` originate from lmcad (MIT).

stb_image v2.30 (https://github.com/nothings/stb) - public domain (alternatively MIT, Sean Barrett). `cpp/stb_image.h` is vendored unmodified and decodes PNG, JPEG, BMP and TGA textures.

Linux release binaries are fully static against glibc (LGPL-2.1+); macOS binaries
link Apple system libraries dynamically; Windows binaries use the MinGW-w64 static
runtime (GCC runtime library exception applies).
